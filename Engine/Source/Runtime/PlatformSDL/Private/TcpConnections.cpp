#include "Lemuria/Platform/TcpConnections.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include "Lemuria/Platform/MonotonicClock.hpp"
#include "NetworkLibrary.hpp"
#include "TcpLink.hpp"
#include "TcpMailbox.hpp"

namespace lemuria::platform {

namespace {

constexpr core::ConnectionTraits kTcpTraits{core::kTcpConnectionKind, false, false,
                                            TcpConnections::kMaxSendBytes};

constexpr std::size_t kReadChunkBytes = std::size_t{16} * 1024;

// How long the network thread sleeps while connections are open, which also limits how late a send can leave.
constexpr int kWaitMilliseconds = 1;

} // namespace

// Owns the sockets. Only `mailbox` and `deliver_notices` are used from the game's thread.
class TcpConnections::NetworkThread {
public:
    explicit NetworkThread(TcpTimeouts timeouts) : timeouts_(timeouts) {}

    ~NetworkThread() {
        mailbox_.ask_to_stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    NetworkThread(const NetworkThread&) = delete;
    NetworkThread(NetworkThread&&) = delete;
    NetworkThread& operator=(const NetworkThread&) = delete;
    NetworkThread& operator=(NetworkThread&&) = delete;

    [[nodiscard]] core::Result<void> start() {
        if (!library_.ready()) {
            return core::make_error(std::string("could not start the network: ") + SDL_GetError());
        }
        try {
            thread_ = std::thread([this] { run(); });
        } catch (const std::system_error& error) {
            return core::make_error(std::string("could not start the network thread: ") + error.what());
        }
        return {};
    }

    [[nodiscard]] TcpMailbox& mailbox() noexcept { return mailbox_; }

    void deliver_notices(core::ConnectionEvents& events) {
        if (delivering_) {
            return; // A `poll` made from inside an event must not replace the notices being read.
        }
        delivering_ = true;
        mailbox_.take_notices(delivered_);
        for (const TcpNotice& notice : delivered_.notices) {
            deliver(notice, events);
        }
        delivered_.clear();
        delivering_ = false;
    }

private:
    void deliver(const TcpNotice& notice, core::ConnectionEvents& events) const {
        switch (notice.kind) {
        case TcpNoticeKind::Connected:
            events.on_connected(notice.connection);
            break;
        case TcpNoticeKind::Received:
            events.on_received(core::ReceivedBytes{
                notice.connection,
                std::span<const std::byte>(delivered_.bytes).subspan(notice.first_byte, notice.byte_count),
                false});
            break;
        case TcpNoticeKind::Closed:
            // SDL_net reports failures only as text, so there is no original code to keep.
            events.on_closed(notice.connection, core::ConnectionClosed{notice.reason, 0});
            break;
        }
    }

    void run() {
        while (mailbox_.take_requests(requests_)) {
            apply_requests();
            advance_links();
            wait_for_work();
        }
    }

    [[nodiscard]] TcpLink* find_link(core::ConnectionId connection) noexcept {
        const auto found = std::ranges::find_if(
            links_, [connection](const auto& link) { return link->connection() == connection; });
        return found == links_.end() ? nullptr : found->get();
    }

    void apply_requests() {
        const std::uint64_t now = monotonic_nanoseconds();
        for (const TcpRequest& request : requests_.requests) {
            if (request.kind == TcpRequestKind::Open) {
                links_.push_back(
                    std::make_unique<TcpLink>(request.connection, request.address, timeouts_, now, mailbox_));
                continue;
            }
            // The connection may have ended while the request was on its way.
            TcpLink* link = find_link(request.connection);
            if (link == nullptr) {
                continue;
            }
            if (request.kind == TcpRequestKind::Send) {
                link->queue_bytes(std::span<const std::byte>(requests_.bytes)
                                      .subspan(request.first_byte, request.byte_count));
            } else {
                link->begin_closing(now);
            }
        }
        requests_.clear();
    }

    void advance_links() {
        const std::uint64_t now = monotonic_nanoseconds();
        std::erase_if(links_, [&](const auto& link) { return !link->advance(now, scratch_); });
    }

    void wait_for_work() {
        if (links_.empty()) {
            mailbox_.wait_for_requests();
            return;
        }
        const bool can_receive = mailbox_.room_for_received_bytes() > 0;
        watched_sockets_.clear();
        for (const auto& link : links_) {
            if (NET_StreamSocket* socket = link->socket_to_watch(can_receive); socket != nullptr) {
                watched_sockets_.push_back(socket);
            }
        }
        const int count = static_cast<int>(watched_sockets_.size());
        if (count == 0 ||
            NET_WaitUntilInputAvailable(watched_sockets_.data(), count, kWaitMilliseconds) < 0) {
            mailbox_.wait_for_requests(std::chrono::milliseconds(kWaitMilliseconds));
        }
    }

    NetworkLibrary library_;
    TcpTimeouts timeouts_;
    TcpMailbox mailbox_;
    std::thread thread_;

    // Used only by the network thread.
    TcpRequestBatch requests_;
    std::vector<std::unique_ptr<TcpLink>> links_;
    std::vector<void*> watched_sockets_;
    std::array<std::byte, kReadChunkBytes> scratch_{};

    // Used only by the game's thread.
    TcpNoticeBatch delivered_;
    bool delivering_ = false;
};

TcpConnections::TcpConnections(TcpTimeouts timeouts) : timeouts_(timeouts) {}

TcpConnections::~TcpConnections() = default;

const core::ConnectionTraits& TcpConnections::traits() const noexcept {
    return kTcpTraits;
}

core::Result<void> TcpConnections::start_connecting(core::ConnectionId connection,
                                                    const core::ConnectionAddress& address) {
    if (!network_) {
        auto network = std::make_unique<NetworkThread>(timeouts_);
        auto started = network->start();
        if (!started) {
            return started;
        }
        network_ = std::move(network);
    }
    return network_->mailbox().post_open(connection, address);
}

core::Result<void> TcpConnections::queue_bytes(core::ConnectionId connection,
                                               std::span<const std::byte> bytes,
                                               core::SendOptions /*options*/) {
    return network_->mailbox().post_send(connection, bytes);
}

void TcpConnections::stop_connection(core::ConnectionId connection) noexcept {
    network_->mailbox().post_close(connection);
}

void TcpConnections::collect_events(core::ConnectionEvents& events) {
    if (network_) {
        network_->deliver_notices(events);
    }
}

} // namespace lemuria::platform
