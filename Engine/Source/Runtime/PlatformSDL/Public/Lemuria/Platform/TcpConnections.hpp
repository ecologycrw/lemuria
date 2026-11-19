#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/Result.hpp"

namespace lemuria::platform {

struct TcpTimeouts {
    // How long finding the server and connecting to it can take.
    std::uint32_t connect_milliseconds = 10'000;

    // How long the bytes already sent can take to leave after the game closes a connection.
    std::uint32_t close_milliseconds = 2'000;
};

// TCP connections. The sockets live on a background thread, and the game only touches queues, so no function
// here waits for the network (INV-04). Destroying this object abandons the bytes that have not left yet.
class TcpConnections final : public core::Connections {
public:
    static constexpr std::size_t kMaxSendBytes = std::size_t{64} * 1024;

    // A send is refused while this many bytes of its connection wait to leave.
    static constexpr std::size_t kMaxQueuedSendBytes = std::size_t{1024} * 1024;

    // The network stops reading while this many received bytes wait for `poll`, which slows the server down.
    static constexpr std::size_t kMaxUndeliveredBytes = std::size_t{1024} * 1024;

    explicit TcpConnections(TcpTimeouts timeouts = {});
    ~TcpConnections() override;

    TcpConnections(const TcpConnections&) = delete;
    TcpConnections(TcpConnections&&) = delete;
    TcpConnections& operator=(const TcpConnections&) = delete;
    TcpConnections& operator=(TcpConnections&&) = delete;

    [[nodiscard]] const core::ConnectionTraits& traits() const noexcept override;

private:
    class NetworkThread;

    [[nodiscard]] core::Result<void> start_connecting(core::ConnectionId connection,
                                                      const core::ConnectionAddress& address) override;
    [[nodiscard]] core::Result<void> queue_bytes(core::ConnectionId connection,
                                                 std::span<const std::byte> bytes,
                                                 core::SendOptions options) override;
    void stop_connection(core::ConnectionId connection) noexcept override;
    void collect_events(core::ConnectionEvents& events) override;

    TcpTimeouts timeouts_;

    // Starts with the first connection, so a program that never connects opens no thread.
    std::unique_ptr<NetworkThread> network_;
};

} // namespace lemuria::platform
