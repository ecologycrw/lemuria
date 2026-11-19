#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/Result.hpp"

namespace lemuria::platform {

enum class TcpRequestKind : std::uint8_t { Open, Send, Close };

// Something the game asked the network thread to do.
struct TcpRequest {
    TcpRequestKind kind = TcpRequestKind::Open;
    core::ConnectionId connection;

    // Only for `Open`. The other requests carry no text at all, so sending asks for no memory (INV-13).
    std::optional<core::ConnectionAddress> address;

    std::size_t first_byte = 0; // Only for `Send`: where its bytes start in the batch.
    std::size_t byte_count = 0; // Only for `Send`.
};

// The room each request batch has from the start. The game's thread writes its requests into whichever batch
// the mailbox holds, and the two batches take turns, so both must be ready before the first frame: a frame
// sends far less than this, and sending asks for no memory (INV-13). A bigger batch still works, and keeps
// its memory.
inline constexpr std::size_t kReservedRequestsPerBatch = 64;
inline constexpr std::size_t kReservedRequestBytesPerBatch = std::size_t{64} * 1024;

// The batches are traded with `swap`, member by member. Trading them with `std::swap` would move them through
// a temporary batch, and some standard libraries ask for memory to move a container (INV-13).
struct TcpRequestBatch {
    std::vector<TcpRequest> requests;
    std::vector<std::byte> bytes;

    TcpRequestBatch() {
        requests.reserve(kReservedRequestsPerBatch);
        bytes.reserve(kReservedRequestBytesPerBatch);
    }

    void clear() noexcept {
        requests.clear();
        bytes.clear();
    }

    void swap(TcpRequestBatch& other) noexcept {
        requests.swap(other.requests);
        bytes.swap(other.bytes);
    }
};

enum class TcpNoticeKind : std::uint8_t { Connected, Received, Closed };

// Something that happened on the network thread.
struct TcpNotice {
    TcpNoticeKind kind = TcpNoticeKind::Connected;
    core::ConnectionId connection;
    std::size_t first_byte = 0; // Only for `Received`: where its bytes start in the batch.
    std::size_t byte_count = 0; // Only for `Received`.
    core::CloseReason reason = core::CloseReason::ConnectionLost; // Only for `Closed`.
};

struct TcpNoticeBatch {
    std::vector<TcpNotice> notices;
    std::vector<std::byte> bytes;

    void clear() noexcept {
        notices.clear();
        bytes.clear();
    }

    void swap(TcpNoticeBatch& other) noexcept {
        notices.swap(other.notices);
        bytes.swap(other.bytes);
    }
};

// The only place the game's thread and the network thread share. The lock is never held during a network
// call, and each side trades whole batches, so their memory is reused once it has grown (INV-04 and INV-13).
class TcpMailbox {
public:
    // Used by the game's thread.
    [[nodiscard]] core::Result<void> post_open(core::ConnectionId connection,
                                               const core::ConnectionAddress& address);
    [[nodiscard]] core::Result<void> post_send(core::ConnectionId connection,
                                               std::span<const std::byte> bytes);
    void post_close(core::ConnectionId connection);
    void ask_to_stop();

    // Trades an empty batch for the notices that are waiting.
    void take_notices(TcpNoticeBatch& batch);

    // Used by the network thread. Trades an empty batch for the requests that are waiting, and returns false
    // when the thread must stop.
    [[nodiscard]] bool take_requests(TcpRequestBatch& batch);
    void wait_for_requests();
    void wait_for_requests(std::chrono::milliseconds limit);

    [[nodiscard]] std::size_t room_for_received_bytes();
    void post_connected(core::ConnectionId connection);
    void post_received(core::ConnectionId connection, std::span<const std::byte> bytes);
    void post_closed(core::ConnectionId connection, core::CloseReason reason);

    // Tells that these bytes left the queue of the connection.
    void release_send_bytes(core::ConnectionId connection, std::size_t byte_count);

private:
    // The bytes of one open connection that wait to leave. An empty connection number marks a free entry.
    struct SendBacklog {
        core::ConnectionId connection;
        std::size_t bytes = 0;
    };

    [[nodiscard]] SendBacklog* find_backlog(core::ConnectionId connection) noexcept;
    [[nodiscard]] bool has_requests_or_stop() const noexcept;

    std::mutex mutex_;
    std::condition_variable requests_arrived_;
    TcpRequestBatch requests_;
    TcpNoticeBatch notices_;
    std::array<SendBacklog, core::Connections::kMaxOpenConnections> backlogs_{};
    bool stop_requested_ = false;
};

} // namespace lemuria::platform
