#include "TcpMailbox.hpp"

#include <algorithm>
#include <utility>

#include "Lemuria/Platform/TcpConnections.hpp"

namespace lemuria::platform {

TcpMailbox::SendBacklog* TcpMailbox::find_backlog(core::ConnectionId connection) noexcept {
    const auto found = std::ranges::find(backlogs_, connection, &SendBacklog::connection);
    return found == backlogs_.end() ? nullptr : &*found;
}

bool TcpMailbox::has_requests_or_stop() const noexcept {
    return stop_requested_ || !requests_.requests.empty();
}

core::Result<void> TcpMailbox::post_open(core::ConnectionId connection,
                                         const core::ConnectionAddress& address) {
    {
        const std::scoped_lock lock(mutex_);
        SendBacklog* backlog = find_backlog(core::ConnectionId{});
        if (backlog == nullptr) {
            return core::make_error("too many open connections");
        }
        *backlog = SendBacklog{connection, 0};
        requests_.requests.push_back(TcpRequest{TcpRequestKind::Open, connection, address, 0, 0});
    }
    requests_arrived_.notify_one();
    return {};
}

core::Result<void> TcpMailbox::post_send(core::ConnectionId connection, std::span<const std::byte> bytes) {
    {
        const std::scoped_lock lock(mutex_);
        SendBacklog* backlog = find_backlog(connection);
        if (backlog == nullptr) {
            return core::make_error("the connection is closed");
        }
        if (backlog->bytes + bytes.size() > TcpConnections::kMaxQueuedSendBytes) {
            return core::make_error("the send queue of the connection is full");
        }
        backlog->bytes += bytes.size();
        const std::size_t first_byte = requests_.bytes.size();
        requests_.bytes.insert(requests_.bytes.end(), bytes.begin(), bytes.end());
        requests_.requests.push_back(
            TcpRequest{TcpRequestKind::Send, connection, {}, first_byte, bytes.size()});
    }
    requests_arrived_.notify_one();
    return {};
}

void TcpMailbox::post_close(core::ConnectionId connection) {
    {
        const std::scoped_lock lock(mutex_);
        if (SendBacklog* backlog = find_backlog(connection); backlog != nullptr) {
            *backlog = SendBacklog{};
        }
        requests_.requests.push_back(TcpRequest{TcpRequestKind::Close, connection, {}, 0, 0});
    }
    requests_arrived_.notify_one();
}

void TcpMailbox::ask_to_stop() {
    {
        const std::scoped_lock lock(mutex_);
        stop_requested_ = true;
    }
    requests_arrived_.notify_one();
}

void TcpMailbox::take_notices(TcpNoticeBatch& batch) {
    const std::scoped_lock lock(mutex_);
    std::swap(batch, notices_);
    for (const TcpNotice& notice : batch.notices) {
        if (notice.kind != TcpNoticeKind::Closed) {
            continue;
        }
        if (SendBacklog* backlog = find_backlog(notice.connection); backlog != nullptr) {
            *backlog = SendBacklog{};
        }
    }
}

bool TcpMailbox::take_requests(TcpRequestBatch& batch) {
    const std::scoped_lock lock(mutex_);
    std::swap(batch, requests_);
    return !stop_requested_;
}

void TcpMailbox::wait_for_requests() {
    std::unique_lock lock(mutex_);
    requests_arrived_.wait(lock, [this] { return has_requests_or_stop(); });
}

void TcpMailbox::wait_for_requests(std::chrono::milliseconds limit) {
    std::unique_lock lock(mutex_);
    requests_arrived_.wait_for(lock, limit, [this] { return has_requests_or_stop(); });
}

std::size_t TcpMailbox::room_for_received_bytes() {
    const std::scoped_lock lock(mutex_);
    return TcpConnections::kMaxUndeliveredBytes -
           std::min(notices_.bytes.size(), TcpConnections::kMaxUndeliveredBytes);
}

void TcpMailbox::post_connected(core::ConnectionId connection) {
    const std::scoped_lock lock(mutex_);
    notices_.notices.push_back(
        TcpNotice{TcpNoticeKind::Connected, connection, 0, 0, core::CloseReason::ConnectionLost});
}

void TcpMailbox::post_received(core::ConnectionId connection, std::span<const std::byte> bytes) {
    const std::scoped_lock lock(mutex_);
    const std::size_t first_byte = notices_.bytes.size();
    notices_.bytes.insert(notices_.bytes.end(), bytes.begin(), bytes.end());
    notices_.notices.push_back(TcpNotice{TcpNoticeKind::Received, connection, first_byte, bytes.size(),
                                         core::CloseReason::ConnectionLost});
}

void TcpMailbox::post_closed(core::ConnectionId connection, core::CloseReason reason) {
    const std::scoped_lock lock(mutex_);
    notices_.notices.push_back(TcpNotice{TcpNoticeKind::Closed, connection, 0, 0, reason});
}

void TcpMailbox::release_send_bytes(core::ConnectionId connection, std::size_t byte_count) {
    const std::scoped_lock lock(mutex_);
    if (SendBacklog* backlog = find_backlog(connection); backlog != nullptr) {
        backlog->bytes -= std::min(backlog->bytes, byte_count);
    }
}

} // namespace lemuria::platform
