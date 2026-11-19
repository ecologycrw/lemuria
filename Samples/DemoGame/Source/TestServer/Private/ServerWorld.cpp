#include "TestServer/ServerWorld.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <variant>

#include "DemoProtocol/Protocol.hpp"

namespace test_server {

namespace {

using demo_protocol::ClientMessage;
using demo_protocol::EntityAppeared;
using demo_protocol::EntityLeft;
using demo_protocol::EntityWalked;
using demo_protocol::Hello;
using demo_protocol::LeaveReason;
using demo_protocol::MapEntered;
using demo_protocol::MapLoaded;
using demo_protocol::PositionCorrected;
using demo_protocol::WalkConfirmed;
using demo_protocol::WalkRequest;

// How far from the center the server looks for a free tile to place a new player.
constexpr int kSpawnSearchSteps = 8;

[[nodiscard]] lemuria::core::Result<void> check_config(const ServerGrid& grid, const ServerConfig& config) {
    if (!std::isfinite(config.walk_tiles_per_second) || config.walk_tiles_per_second <= 0.0) {
        return lemuria::core::make_error("the server needs a walking speed above zero");
    }
    if (config.walks.origin_tolerance_steps < 0 || !std::isfinite(config.walks.early_tolerance_seconds) ||
        config.walks.early_tolerance_seconds < 0.0) {
        return lemuria::core::make_error("the tolerances of the walks cannot be negative");
    }
    if (config.max_players == 0) {
        return lemuria::core::make_error("the server needs room for at least one player");
    }
    if (!grid.contains(config.spawn_center)) {
        return lemuria::core::make_error("the players must start inside the map");
    }
    return {};
}

} // namespace

lemuria::core::Result<ServerWorld> ServerWorld::create(ServerGrid grid, const ServerConfig& config,
                                                       ServerOutbox& outbox) {
    if (auto valid = check_config(grid, config); !valid) {
        return tl::unexpected(std::move(valid.error()));
    }
    return ServerWorld(std::move(grid), config, outbox);
}

ServerWorld::ServerWorld(ServerGrid grid, const ServerConfig& config, ServerOutbox& outbox)
    : grid_(std::move(grid)), config_(config), outbox_(outbox) {
    players_.reserve(config.max_players);
}

void ServerWorld::receive(ClientId client, const ClientMessage& message, double now) {
    if (const auto* hello = std::get_if<Hello>(&message)) {
        greet(client, *hello, now);
        return;
    }
    ServerPlayer* player = find_player(client);
    if (player == nullptr) {
        ++ignored_messages_;
    } else if (const auto* loaded = std::get_if<MapLoaded>(&message)) {
        enter_world(*player, *loaded, now);
    } else if (const auto* request = std::get_if<WalkRequest>(&message)) {
        walk(*player, *request, now);
    }
}

void ServerWorld::remove(ClientId client) {
    const auto found = std::ranges::find(players_, client, &ServerPlayer::client);
    if (found == players_.end()) {
        return;
    }
    const ServerPlayer player = *found;
    players_.erase(found);
    if (player.is_in_world) {
        send_to_others(player, EntityLeft{player.entity, LeaveReason::Disconnected});
    }
}

const ServerPlayer* ServerWorld::find(ClientId client) const noexcept {
    const auto found = std::ranges::find(players_, client, &ServerPlayer::client);
    return found == players_.end() ? nullptr : &*found;
}

std::uint64_t ServerWorld::refused_walk_count(WalkRefusal refusal) const noexcept {
    return refused_walks_[static_cast<std::size_t>(refusal)];
}

// A client that speaks another version of the protocol, or that finds no room, is sent away.
void ServerWorld::greet(ClientId client, const Hello& hello, double now) {
    if (find_player(client) != nullptr) {
        ++ignored_messages_;
        return;
    }
    if (hello.protocol_version != demo_protocol::protocol_version() ||
        players_.size() >= config_.max_players) {
        outbox_.disconnect(client);
        return;
    }
    const std::optional<Tile> spawn = free_spawn_tile(now);
    if (!spawn) {
        outbox_.disconnect(client);
        return;
    }
    const ServerPlayer& player = players_.emplace_back(
        ServerPlayer{client, next_entity(), ServerMotion(*spawn, now, config_.walk_tiles_per_second), false});
    outbox_.send(client, MapEntered{config_.map_id, player.entity, *spawn});
}

// Only players whose map is loaded see each other.
void ServerWorld::enter_world(ServerPlayer& player, const MapLoaded& loaded, double now) {
    if (player.is_in_world || loaded.map != config_.map_id) {
        ++ignored_messages_;
        return;
    }
    player.is_in_world = true;
    for (const ServerPlayer& other : players_) {
        if (other.client != player.client && other.is_in_world) {
            outbox_.send(player.client, appearance_of(other, now));
        }
    }
    send_to_others(player, appearance_of(player, now));
}

void ServerWorld::walk(ServerPlayer& player, const WalkRequest& request, double now) {
    if (!player.is_in_world) {
        ++ignored_messages_;
        return;
    }
    const auto start = check_walk(request, player.motion, now, grid_, config_.walks);
    if (start) {
        accept_walk(player, request, *start);
    } else {
        refuse_walk(player, start.error(), now);
    }
}

// A walk without steps means the player stops, which the others see as a correction to where it stopped.
void ServerWorld::accept_walk(ServerPlayer& player, const WalkRequest& request, double start) {
    player.motion.start_walk(request.origin, start, request.steps);
    outbox_.send(player.client, WalkConfirmed{request.origin, player.motion.destination()});
    if (request.steps.empty()) {
        send_to_others(player, PositionCorrected{player.entity, request.origin});
    } else {
        send_to_others(player, EntityWalked{player.entity, request.origin, request.steps});
    }
}

// The player stops where the server believes it is, and everyone is told, the player included.
void ServerWorld::refuse_walk(ServerPlayer& player, WalkRefusal refusal, double now) {
    ++refused_walks_[static_cast<std::size_t>(refusal)];
    const Tile tile = player.motion.tile_at(now);
    player.motion.start_walk(tile, now, {});
    const PositionCorrected correction{player.entity, tile};
    outbox_.send(player.client, correction);
    send_to_others(player, correction);
}

ServerPlayer* ServerWorld::find_player(ClientId client) noexcept {
    const auto found = std::ranges::find(players_, client, &ServerPlayer::client);
    return found == players_.end() ? nullptr : &*found;
}

// The closest free tile to the center, looking around it in rings that grow one step at a time.
std::optional<Tile> ServerWorld::free_spawn_tile(double now) const noexcept {
    const Tile center = config_.spawn_center;
    for (int distance = 0; distance <= kSpawnSearchSteps; ++distance) {
        for (int z = center.z - distance; z <= center.z + distance; ++z) {
            for (int x = center.x - distance; x <= center.x + distance; ++x) {
                const std::optional<Tile> tile = demo_protocol::tile_from(x, z);
                if (tile && steps_between(*tile, center) == distance && grid_.is_walkable(*tile) &&
                    !is_taken(*tile, now)) {
                    return tile;
                }
            }
        }
    }
    return std::nullopt;
}

bool ServerWorld::is_taken(Tile tile, double now) const noexcept {
    return std::ranges::any_of(
        players_, [tile, now](const ServerPlayer& player) { return player.motion.tile_at(now) == tile; });
}

// Numbers are never 0, as the messages require.
std::uint32_t ServerWorld::next_entity() noexcept {
    ++last_entity_;
    if (last_entity_ == 0) {
        ++last_entity_;
    }
    return last_entity_;
}

void ServerWorld::send_to_others(const ServerPlayer& player, const demo_protocol::ServerMessage& message) {
    for (const ServerPlayer& other : players_) {
        if (other.client != player.client && other.is_in_world) {
            outbox_.send(other.client, message);
        }
    }
}

EntityAppeared ServerWorld::appearance_of(const ServerPlayer& player, double now) const noexcept {
    const Tile tile = player.motion.tile_at(now);
    std::optional<Tile> walking_to;
    if (player.motion.is_walking_at(now)) {
        walking_to = player.motion.destination();
    }
    return EntityAppeared{player.entity, tile, walking_to};
}

} // namespace test_server
