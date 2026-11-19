#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "Lemuria/Core/Result.hpp"
#include "Lemuria/Platform/TcpListener.hpp"
#include "TestServer/ServerGrid.hpp"
#include "TestServer/ServerWorld.hpp"

namespace test_server {

struct HostLimits {
    // The most open connections, counting the ones that have not said hello yet (INV-14). A connection beyond
    // this number is closed as soon as it arrives.
    std::size_t max_connections = 0;
};

// The rules of the demonstration: its map, its speed, and how much the server forgives the delay of a
// connection on this computer.
[[nodiscard]] ServerConfig demo_server_config() noexcept;
[[nodiscard]] HostLimits demo_host_limits() noexcept;

// Connects the rules of the test server to real connections: it accepts the clients, cuts their bytes into
// messages, hands them to the `ServerWorld` and sends back what it decides. Nothing here waits for the
// network.
class ServerHost {
public:
    // A configuration the rules cannot follow is refused with the reason.
    [[nodiscard]] static lemuria::core::Result<ServerHost> create(lemuria::platform::TcpListener listener,
                                                                  ServerGrid grid, const ServerConfig& config,
                                                                  const HostLimits& limits);

    ServerHost(ServerHost&& other) noexcept;
    ServerHost(const ServerHost&) = delete;
    ServerHost& operator=(const ServerHost&) = delete;
    ServerHost& operator=(ServerHost&&) = delete;
    ~ServerHost();

    // One round: accepts the connections that arrived, handles the messages that arrived and ends the
    // connections that must end. The time is given by the caller (INV-05). It fails only when the server can
    // no longer accept connections.
    [[nodiscard]] lemuria::core::Result<void> serve(double now);

    [[nodiscard]] std::uint16_t port() const noexcept { return listener_.port(); }
    [[nodiscard]] std::size_t connection_count() const noexcept;
    [[nodiscard]] const ServerWorld& world() const noexcept { return world_; }

    // Connections closed because there was no room for them.
    [[nodiscard]] std::uint64_t refused_connection_count() const noexcept { return refused_connections_; }

    // Connections closed because their bytes broke the format of the frames.
    [[nodiscard]] std::uint64_t broken_connection_count() const noexcept;

    // Messages left out because only they were wrong. The connection goes on.
    [[nodiscard]] std::uint64_t invalid_message_count() const noexcept;

private:
    struct Client;
    class Clients;
    class ClientReader;

    ServerHost(lemuria::platform::TcpListener listener, std::unique_ptr<Clients> clients,
               ServerWorld world) noexcept;

    [[nodiscard]] lemuria::core::Result<void> accept_connections();
    void read_messages(double now);
    void read_from(Client& client, double now);
    void end_connections();

    lemuria::platform::TcpListener listener_;

    // Kept apart, so the reference the rules hold stays valid when the host moves.
    std::unique_ptr<Clients> clients_;
    ServerWorld world_;
    std::uint64_t refused_connections_ = 0;
};

} // namespace test_server
