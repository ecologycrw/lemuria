#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Platform/TcpConnections.hpp"
#include "NetworkLibrary.hpp"
#include "TcpMailbox.hpp"

namespace lemuria::platform {

// One TCP connection, from finding the server to the last byte. It lives on the network thread.
class TcpLink {
public:
    TcpLink(core::ConnectionId connection, const core::ConnectionAddress& address, TcpTimeouts timeouts,
            std::uint64_t now_nanoseconds, TcpMailbox& mailbox);

    [[nodiscard]] core::ConnectionId connection() const noexcept { return connection_; }

    void queue_bytes(std::span<const std::byte> bytes);

    // The game no longer wants this connection: only the bytes it already sent still matter.
    void begin_closing(std::uint64_t now_nanoseconds);

    // Moves the connection forward without waiting, and returns false once it has ended.
    [[nodiscard]] bool advance(std::uint64_t now_nanoseconds, std::span<std::byte> scratch);

    // The socket whose activity is worth waking up for, if there is one.
    [[nodiscard]] NET_StreamSocket* socket_to_watch(bool can_receive) const noexcept;

private:
    enum class Stage : std::uint8_t { FindingServer, Connecting, Connected, Closing };

    [[nodiscard]] bool find_server(std::uint64_t now_nanoseconds);
    [[nodiscard]] bool finish_connecting(std::uint64_t now_nanoseconds);
    [[nodiscard]] bool exchange_bytes(std::span<std::byte> scratch);
    [[nodiscard]] bool finish_closing(std::uint64_t now_nanoseconds, std::span<std::byte> scratch);

    // Returns false when the connection has failed.
    [[nodiscard]] bool write_next_chunk();

    // Tells the game that the connection ended, and returns false to end the link.
    [[nodiscard]] bool report_end(core::CloseReason reason);

    core::ConnectionId connection_;
    std::uint16_t port_;
    std::uint32_t close_timeout_milliseconds_;
    TcpMailbox& mailbox_;
    Stage stage_ = Stage::FindingServer;
    std::uint64_t deadline_nanoseconds_;
    NetAddressHandle address_;
    StreamSocketHandle socket_;
    std::vector<std::byte> outgoing_;
};

} // namespace lemuria::platform
