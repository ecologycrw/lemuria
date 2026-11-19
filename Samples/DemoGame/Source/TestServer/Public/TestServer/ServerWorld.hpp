#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "DemoProtocol/Messages.hpp"
#include "TestServer/ServerGrid.hpp"
#include "TestServer/ServerMotion.hpp"
#include "TestServer/WalkCheck.hpp"

namespace test_server {

// Identifies a client while it is connected. The program that owns the connections chooses the numbers.
struct ClientId {
    std::uint64_t value = 0;

    friend constexpr bool operator==(ClientId, ClientId) noexcept = default;
};

// Where the rules send what they decide.
class ServerOutbox {
public:
    virtual ~ServerOutbox() = default;

    virtual void send(ClientId client, const demo_protocol::ServerMessage& message) = 0;

    // The client broke the conversation, or there is no room for it, and its connection must end.
    virtual void disconnect(ClientId client) = 0;

protected:
    ServerOutbox() = default;
    ServerOutbox(const ServerOutbox&) = default;
    ServerOutbox(ServerOutbox&&) = default;
    ServerOutbox& operator=(const ServerOutbox&) = default;
    ServerOutbox& operator=(ServerOutbox&&) = default;
};

struct ServerConfig {
    // The number of the map in the messages.
    std::uint32_t map_id = 0;

    double walk_tiles_per_second = 0.0;
    WalkLimits walks;

    // The most players at the same time (INV-14).
    std::size_t max_players = 0;

    // Players start on the free tile closest to this one.
    Tile spawn_center;
};

struct ServerPlayer {
    ClientId client;
    std::uint32_t entity = 0;
    ServerMotion motion;

    // The client said its map is loaded, so it sees the others and the others see it.
    bool is_in_world = false;
};

// The decisions of the test server, without any connection: who is on the map, where everyone walks and which
// walks are refused. The time is always given by the caller (INV-05).
class ServerWorld {
public:
    // A configuration it cannot follow is refused with the reason.
    [[nodiscard]] static lemuria::core::Result<ServerWorld>
    create(ServerGrid grid, const ServerConfig& config, ServerOutbox& outbox);

    // Handles a message that a client sent.
    void receive(ClientId client, const demo_protocol::ClientMessage& message, double now);

    // The connection of a client ended.
    void remove(ClientId client);

    [[nodiscard]] const ServerPlayer* find(ClientId client) const noexcept;
    [[nodiscard]] std::size_t player_count() const noexcept { return players_.size(); }

    [[nodiscard]] std::uint64_t refused_walk_count(WalkRefusal refusal) const noexcept;

    // Messages that arrived at the wrong moment, kept for diagnosis.
    [[nodiscard]] std::uint64_t ignored_message_count() const noexcept { return ignored_messages_; }

private:
    ServerWorld(ServerGrid grid, const ServerConfig& config, ServerOutbox& outbox);

    void greet(ClientId client, const demo_protocol::Hello& hello, double now);
    void enter_world(ServerPlayer& player, const demo_protocol::MapLoaded& loaded, double now);
    void walk(ServerPlayer& player, const demo_protocol::WalkRequest& request, double now);
    void accept_walk(ServerPlayer& player, const demo_protocol::WalkRequest& request, double start);
    void refuse_walk(ServerPlayer& player, WalkRefusal refusal, double now);

    [[nodiscard]] ServerPlayer* find_player(ClientId client) noexcept;
    [[nodiscard]] std::optional<Tile> free_spawn_tile(double now) const noexcept;
    [[nodiscard]] bool is_taken(Tile tile, double now) const noexcept;
    [[nodiscard]] std::uint32_t next_entity() noexcept;
    void send_to_others(const ServerPlayer& player, const demo_protocol::ServerMessage& message);
    [[nodiscard]] demo_protocol::EntityAppeared appearance_of(const ServerPlayer& player,
                                                              double now) const noexcept;

    ServerGrid grid_;
    ServerConfig config_;
    ServerOutbox& outbox_;
    std::vector<ServerPlayer> players_;
    std::array<std::uint64_t, kWalkRefusalCount> refused_walks_{};
    std::uint64_t ignored_messages_ = 0;
    std::uint32_t last_entity_ = 0;
};

} // namespace test_server
