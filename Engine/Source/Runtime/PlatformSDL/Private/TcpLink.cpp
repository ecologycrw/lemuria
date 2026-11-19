#include "TcpLink.hpp"

#include <algorithm>
#include <string_view>

#include <SDL3/SDL.h>

namespace lemuria::platform {

namespace {

constexpr std::uint64_t kNanosecondsPerMillisecond = 1'000'000;

// Bytes are handed to SDL_net one chunk at a time, because its own queue has no limit.
constexpr std::size_t kWriteChunkBytes = std::size_t{64} * 1024;

[[nodiscard]] std::uint64_t deadline_after(std::uint64_t now_nanoseconds,
                                           std::uint32_t milliseconds) noexcept {
    return now_nanoseconds + (milliseconds * kNanosecondsPerMillisecond);
}

// SDL_net tells that the other side closed only through the text of its error.
[[nodiscard]] core::CloseReason reason_of_failed_read() noexcept {
    return std::string_view(SDL_GetError()) == "End of stream" ? core::CloseReason::PeerClosed
                                                               : core::CloseReason::ConnectionLost;
}

} // namespace

TcpLink::TcpLink(core::ConnectionId connection, const core::ConnectionAddress& address, TcpTimeouts timeouts,
                 std::uint64_t now_nanoseconds, TcpMailbox& mailbox)
    : connection_(connection), port_(address.port), close_timeout_milliseconds_(timeouts.close_milliseconds),
      mailbox_(mailbox),
      deadline_nanoseconds_(deadline_after(now_nanoseconds, timeouts.connect_milliseconds)),
      address_(NET_ResolveHostname(address.host.c_str())) {}

void TcpLink::queue_bytes(std::span<const std::byte> bytes) {
    outgoing_.insert(outgoing_.end(), bytes.begin(), bytes.end());
}

void TcpLink::begin_closing(std::uint64_t now_nanoseconds) {
    if (stage_ != Stage::Connected) {
        socket_.reset();
    }
    stage_ = Stage::Closing;
    deadline_nanoseconds_ = deadline_after(now_nanoseconds, close_timeout_milliseconds_);
}

bool TcpLink::advance(std::uint64_t now_nanoseconds, std::span<std::byte> scratch) {
    switch (stage_) {
    case Stage::FindingServer:
        return find_server(now_nanoseconds);
    case Stage::Connecting:
        return finish_connecting(now_nanoseconds);
    case Stage::Connected:
        return exchange_bytes(scratch);
    case Stage::Closing:
        return finish_closing(now_nanoseconds, scratch);
    }
    return false;
}

NET_StreamSocket* TcpLink::socket_to_watch(bool can_receive) const noexcept {
    // A socket that cannot be read would wake the thread again and again.
    const bool worth_watching = stage_ != Stage::Connected || can_receive;
    return worth_watching ? socket_.get() : nullptr;
}

bool TcpLink::find_server(std::uint64_t now_nanoseconds) {
    if (now_nanoseconds >= deadline_nanoseconds_) {
        return report_end(core::CloseReason::TimedOut);
    }
    if (!address_) {
        return report_end(core::CloseReason::CouldNotConnect);
    }
    const NET_Status status = NET_GetAddressStatus(address_.get());
    if (status == NET_WAITING) {
        return true;
    }
    if (status == NET_FAILURE) {
        return report_end(core::CloseReason::CouldNotConnect);
    }
    socket_.reset(NET_CreateClient(address_.get(), port_, 0));
    if (!socket_) {
        return report_end(core::CloseReason::CouldNotConnect);
    }
    stage_ = Stage::Connecting;
    return true;
}

bool TcpLink::finish_connecting(std::uint64_t now_nanoseconds) {
    if (now_nanoseconds >= deadline_nanoseconds_) {
        return report_end(core::CloseReason::TimedOut);
    }
    const NET_Status status = NET_GetConnectionStatus(socket_.get());
    if (status == NET_WAITING) {
        return true;
    }
    if (status == NET_FAILURE) {
        return report_end(core::CloseReason::CouldNotConnect);
    }
    stage_ = Stage::Connected;
    mailbox_.post_connected(connection_);
    return true;
}

bool TcpLink::exchange_bytes(std::span<std::byte> scratch) {
    // The bytes that arrived before the end of a connection are read first, so none of them is lost.
    const std::size_t room = std::min(mailbox_.room_for_received_bytes(), scratch.size());
    if (room > 0) {
        const int count = NET_ReadFromStreamSocket(socket_.get(), scratch.data(), static_cast<int>(room));
        if (count < 0) {
            return report_end(reason_of_failed_read());
        }
        if (count > 0) {
            mailbox_.post_received(connection_, scratch.first(static_cast<std::size_t>(count)));
        }
    }
    return write_next_chunk() || report_end(core::CloseReason::ConnectionLost);
}

bool TcpLink::finish_closing(std::uint64_t now_nanoseconds, std::span<std::byte> scratch) {
    if (!socket_ || now_nanoseconds >= deadline_nanoseconds_) {
        return false;
    }
    // Closing with unread bytes would reset the connection and could destroy what is still on its way.
    if (NET_ReadFromStreamSocket(socket_.get(), scratch.data(), static_cast<int>(scratch.size())) < 0) {
        return false;
    }
    if (!write_next_chunk()) {
        return false;
    }
    return !outgoing_.empty() || NET_GetStreamSocketPendingWrites(socket_.get()) > 0;
}

bool TcpLink::write_next_chunk() {
    const int waiting_in_library = NET_GetStreamSocketPendingWrites(socket_.get());
    if (waiting_in_library < 0) {
        return false;
    }
    if (waiting_in_library > 0 || outgoing_.empty()) {
        return true;
    }
    const std::size_t count = std::min(outgoing_.size(), kWriteChunkBytes);
    if (!NET_WriteToStreamSocket(socket_.get(), outgoing_.data(), static_cast<int>(count))) {
        return false;
    }
    outgoing_.erase(outgoing_.begin(), outgoing_.begin() + static_cast<std::ptrdiff_t>(count));
    mailbox_.release_send_bytes(connection_, count);
    return true;
}

bool TcpLink::report_end(core::CloseReason reason) {
    mailbox_.post_closed(connection_, reason);
    return false;
}

} // namespace lemuria::platform
