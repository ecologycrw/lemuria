#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Protocol.hpp"
#include "TestServer/ServerWorld.hpp"

using demo_protocol::DemoMap;
using demo_protocol::EntityAppeared;
using demo_protocol::EntityLeft;
using demo_protocol::EntityWalked;
using demo_protocol::Hello;
using demo_protocol::LeaveReason;
using demo_protocol::MapEntered;
using demo_protocol::MapLoaded;
using demo_protocol::PositionCorrected;
using demo_protocol::protocol_version;
using demo_protocol::ServerMessage;
using demo_protocol::WalkConfirmed;
using demo_protocol::WalkRequest;
using test_server::ClientId;
using test_server::ServerConfig;
using test_server::ServerGrid;
using test_server::ServerOutbox;
using test_server::ServerPlayer;
using test_server::ServerWorld;
using test_server::StepList;
using test_server::Tile;
using test_server::WalkRefusal;

namespace {

constexpr ClientId kAlice{10};
constexpr ClientId kBob{20};
constexpr ClientId kCarol{30};
constexpr ClientId kDave{40};
constexpr std::uint32_t kMap = 1;
constexpr Tile kSpawn{5, 5};

constexpr ServerConfig kConfig{
    .map_id = kMap,
    .walk_tiles_per_second = 4.0,
    .walks = {.origin_tolerance_steps = 1, .early_tolerance_seconds = 0.5},
    .max_players = 3,
    .spawn_center = kSpawn,
};

[[nodiscard]] StepList steps_of(std::initializer_list<Tile> tiles) {
    StepList steps;
    for (const Tile tile : tiles) {
        REQUIRE(steps.add(tile));
    }
    return steps;
}

[[nodiscard]] ServerGrid open_grid() {
    auto grid = ServerGrid::create(16, 16);
    REQUIRE(grid.has_value());
    return *std::move(grid);
}

class RecordingOutbox final : public ServerOutbox {
public:
    void send(ClientId client, const ServerMessage& message) override { sent.emplace_back(client, message); }

    void disconnect(ClientId client) override { disconnected.push_back(client); }

    [[nodiscard]] std::vector<ServerMessage> to(ClientId client) const {
        std::vector<ServerMessage> messages;
        for (const auto& [receiver, message] : sent) {
            if (receiver == client) {
                messages.push_back(message);
            }
        }
        return messages;
    }

    std::vector<std::pair<ClientId, ServerMessage>> sent;
    std::vector<ClientId> disconnected;
};

[[nodiscard]] ServerWorld make_world(ServerGrid grid, const ServerConfig& config, ServerOutbox& outbox) {
    auto world = ServerWorld::create(std::move(grid), config, outbox);
    REQUIRE(world.has_value());
    return *std::move(world);
}

class WorldFixture {
public:
    explicit WorldFixture(ServerGrid grid = open_grid(), const ServerConfig& config = kConfig)
        : world(make_world(std::move(grid), config, outbox)) {}

    void hello(ClientId client, double now = 0.0) { world.receive(client, Hello{protocol_version()}, now); }

    void load(ClientId client, double now = 0.0) { world.receive(client, MapLoaded{kMap}, now); }

    void join(ClientId client) {
        hello(client);
        load(client);
    }

    void walk(ClientId client, Tile origin, std::initializer_list<Tile> steps, double now) {
        world.receive(client, WalkRequest{origin, steps_of(steps)}, now);
    }

    [[nodiscard]] const ServerPlayer& player(ClientId client) const {
        const ServerPlayer* found = world.find(client);
        REQUIRE(found != nullptr);
        return *found;
    }

    void forget_messages() {
        outbox.sent.clear();
        outbox.disconnected.clear();
    }

    RecordingOutbox outbox;
    ServerWorld world;
};

} // namespace

TEST_CASE("a client that says hello is placed on the map") {
    WorldFixture fixture;

    fixture.hello(kAlice);

    CHECK(fixture.outbox.to(kAlice) == std::vector<ServerMessage>{MapEntered{kMap, 1, kSpawn}});
    CHECK(fixture.world.player_count() == 1);
    CHECK_FALSE(fixture.player(kAlice).is_in_world);
}

TEST_CASE("a client that speaks another version of the protocol is sent away") {
    WorldFixture fixture;

    fixture.world.receive(kAlice, Hello{static_cast<std::uint16_t>(protocol_version() + 1)}, 0.0);

    CHECK(fixture.outbox.disconnected == std::vector<ClientId>{kAlice});
    CHECK(fixture.outbox.sent.empty());
    CHECK(fixture.world.player_count() == 0);
}

TEST_CASE("players start on free tiles, away from forbidden tiles and from each other") {
    ServerGrid grid = open_grid();
    REQUIRE(grid.set_walkable(kSpawn, false));
    WorldFixture fixture(std::move(grid));

    fixture.hello(kAlice);
    fixture.hello(kBob);

    CHECK(fixture.player(kAlice).motion.tile_at(0.0) == Tile{4, 4});
    CHECK(fixture.player(kBob).motion.tile_at(0.0) == Tile{5, 4});
}

TEST_CASE("a player that walked away frees its starting tile") {
    WorldFixture fixture;
    fixture.join(kAlice);
    fixture.walk(kAlice, kSpawn, {{6, 5}}, 0.0);

    fixture.hello(kBob, 1.0);

    CHECK(fixture.player(kBob).motion.tile_at(1.0) == kSpawn);
}

TEST_CASE("a full server sends new clients away") {
    WorldFixture fixture;
    fixture.hello(kAlice);
    fixture.hello(kBob);
    fixture.hello(kCarol);

    fixture.hello(kDave);

    CHECK(fixture.outbox.disconnected == std::vector<ClientId>{kDave});
    CHECK(fixture.world.player_count() == 3);
}

TEST_CASE("a server with no free tile near the start sends new clients away") {
    auto grid = ServerGrid::create(1, 1);
    REQUIRE(grid.has_value());
    ServerConfig config = kConfig;
    config.spawn_center = Tile{0, 0};
    WorldFixture fixture(*std::move(grid), config);
    fixture.hello(kAlice);

    fixture.hello(kBob);

    CHECK(fixture.outbox.disconnected == std::vector<ClientId>{kBob});
}

TEST_CASE("each player gets its own number, and a number is not given again") {
    WorldFixture fixture;
    fixture.hello(kAlice);
    fixture.hello(kBob);
    fixture.world.remove(kAlice);

    fixture.hello(kCarol);

    CHECK(fixture.player(kBob).entity == 2);
    CHECK(fixture.player(kCarol).entity == 3);
}

TEST_CASE("only players whose map is loaded see each other") {
    WorldFixture fixture;
    fixture.hello(kAlice);
    fixture.hello(kBob);
    fixture.load(kAlice);
    fixture.forget_messages();

    fixture.load(kBob);

    const Tile bob_tile = fixture.player(kBob).motion.tile_at(0.0);
    CHECK(fixture.outbox.to(kBob) == std::vector<ServerMessage>{EntityAppeared{1, kSpawn, std::nullopt}});
    CHECK(fixture.outbox.to(kAlice) == std::vector<ServerMessage>{EntityAppeared{2, bob_tile, std::nullopt}});
}

TEST_CASE("someone who appears in the middle of a walk shows where it goes") {
    WorldFixture fixture;
    fixture.join(kAlice);
    fixture.walk(kAlice, kSpawn, {{6, 5}, {7, 5}, {8, 5}}, 0.0);
    fixture.hello(kBob);
    fixture.forget_messages();

    fixture.load(kBob, 0.3);

    CHECK(fixture.outbox.to(kBob) == std::vector<ServerMessage>{EntityAppeared{1, {6, 5}, Tile{8, 5}}});
}

TEST_CASE("an accepted walk is confirmed to the player and shown to the others") {
    WorldFixture fixture;
    fixture.join(kAlice);
    fixture.join(kBob);
    fixture.forget_messages();

    fixture.walk(kAlice, kSpawn, {{6, 5}, {7, 5}}, 1.0);

    CHECK(fixture.outbox.to(kAlice) == std::vector<ServerMessage>{WalkConfirmed{kSpawn, {7, 5}}});
    CHECK(fixture.outbox.to(kBob) ==
          std::vector<ServerMessage>{EntityWalked{1, kSpawn, steps_of({{6, 5}, {7, 5}})}});
    CHECK(fixture.player(kAlice).motion.tile_at(1.5) == Tile{7, 5});
}

TEST_CASE("a stop is confirmed to the player and shown to the others as a correction") {
    WorldFixture fixture;
    fixture.join(kAlice);
    fixture.join(kBob);
    fixture.walk(kAlice, kSpawn, {{6, 5}, {7, 5}, {8, 5}}, 0.0);
    fixture.forget_messages();

    fixture.walk(kAlice, {6, 5}, {}, 0.1);

    CHECK(fixture.outbox.to(kAlice) == std::vector<ServerMessage>{WalkConfirmed{{6, 5}, {6, 5}}});
    CHECK(fixture.outbox.to(kBob) == std::vector<ServerMessage>{PositionCorrected{1, {6, 5}}});
    CHECK(fixture.player(kAlice).motion.tile_at(5.0) == Tile{6, 5});
}

TEST_CASE("a refused walk keeps the player where it is, and everyone is told", "[0000-CA-08]") {
    ServerGrid grid = open_grid();
    REQUIRE(grid.set_walkable({7, 5}, false));
    WorldFixture fixture(std::move(grid));
    fixture.join(kAlice);
    fixture.join(kBob);
    fixture.forget_messages();

    fixture.walk(kAlice, kSpawn, {{6, 5}, {7, 5}}, 1.0);

    const std::vector<ServerMessage> correction{PositionCorrected{1, kSpawn}};
    CHECK(fixture.outbox.to(kAlice) == correction);
    CHECK(fixture.outbox.to(kBob) == correction);
    CHECK(fixture.world.refused_walk_count(WalkRefusal::BlockedStep) == 1);
    CHECK(fixture.player(kAlice).motion.tile_at(5.0) == kSpawn);
}

TEST_CASE("a walk refused in the middle of another stops the player at the tile it reached", "[0000-CA-08]") {
    WorldFixture fixture;
    fixture.join(kAlice);
    fixture.walk(kAlice, kSpawn, {{6, 5}, {7, 5}, {8, 5}, {9, 5}}, 0.0);
    fixture.forget_messages();

    fixture.walk(kAlice, {9, 5}, {{10, 5}}, 0.4);

    CHECK(fixture.outbox.to(kAlice) == std::vector<ServerMessage>{PositionCorrected{1, {6, 5}}});
    CHECK(fixture.world.refused_walk_count(WalkRefusal::TooFast) == 1);
    CHECK_FALSE(fixture.player(kAlice).motion.is_walking_at(0.4));
}

TEST_CASE("every kind of refusal is counted apart", "[0000-CA-08]") {
    ServerGrid grid = open_grid();
    REQUIRE(grid.set_walkable({6, 6}, false));
    WorldFixture fixture(std::move(grid));
    fixture.join(kAlice);

    fixture.walk(kAlice, {9, 9}, {}, 1.0);
    fixture.walk(kAlice, kSpawn, {{7, 5}}, 2.0);
    fixture.walk(kAlice, kSpawn, {{6, 6}}, 3.0);
    fixture.walk(kAlice, kSpawn, {{6, 5}, {7, 6}, {6, 7}}, 4.0);

    CHECK(fixture.world.refused_walk_count(WalkRefusal::OriginTooFar) == 1);
    CHECK(fixture.world.refused_walk_count(WalkRefusal::NotNeighbor) == 1);
    CHECK(fixture.world.refused_walk_count(WalkRefusal::BlockedStep) == 1);
    CHECK(fixture.world.refused_walk_count(WalkRefusal::CutsCorner) == 1);
    CHECK(fixture.world.refused_walk_count(WalkRefusal::TooFast) == 0);
}

TEST_CASE("the server forbids the trail strip that the client believes is free", "[0000-CA-08]") {
    const DemoMap map;
    std::optional<Tile> trail;
    std::optional<Tile> beside_trail;
    for (int z = 1; z < DemoMap::kSize && !trail; ++z) {
        for (int x = 0; x < DemoMap::kSize && !trail; ++x) {
            if (map.looks_walkable(x, z) && !map.is_walkable_on_server(x, z) &&
                map.is_walkable_on_server(x, z - 1)) {
                trail = Tile{static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(z)};
                beside_trail = Tile{static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(z - 1)};
            }
        }
    }
    REQUIRE(trail.has_value());
    ServerConfig config = kConfig;
    config.spawn_center = *beside_trail;
    WorldFixture fixture(ServerGrid::from_demo_map(map), config);
    fixture.join(kAlice);
    REQUIRE(fixture.player(kAlice).motion.tile_at(0.0) == *beside_trail);
    fixture.forget_messages();

    fixture.walk(kAlice, *beside_trail, {*trail}, 1.0);

    CHECK(fixture.outbox.to(kAlice) == std::vector<ServerMessage>{PositionCorrected{1, *beside_trail}});
    CHECK(fixture.world.refused_walk_count(WalkRefusal::BlockedStep) == 1);
}

TEST_CASE("the server sees the demo map as the server rules say") {
    const DemoMap map;
    const ServerGrid grid = ServerGrid::from_demo_map(map);
    int forbidden_only_on_server = 0;

    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            const Tile tile{static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(z)};
            CHECK(grid.is_walkable(tile) == map.is_walkable_on_server(x, z));
            if (map.looks_walkable(x, z) && !grid.is_walkable(tile)) {
                ++forbidden_only_on_server;
            }
        }
    }

    CHECK(grid.width() == DemoMap::kSize);
    CHECK(grid.depth() == DemoMap::kSize);
    CHECK(forbidden_only_on_server > 0);
}

TEST_CASE("messages that arrive at the wrong moment are ignored") {
    WorldFixture fixture;
    fixture.walk(kAlice, kSpawn, {}, 0.0);
    fixture.hello(kAlice);
    fixture.walk(kAlice, kSpawn, {}, 0.0);
    fixture.world.receive(kAlice, MapLoaded{kMap + 1}, 0.0);
    CHECK_FALSE(fixture.player(kAlice).is_in_world);
    fixture.load(kAlice);
    CHECK(fixture.player(kAlice).is_in_world);
    fixture.load(kAlice);
    fixture.hello(kAlice);

    CHECK(fixture.world.ignored_message_count() == 5);
    CHECK(fixture.outbox.to(kAlice) == std::vector<ServerMessage>{MapEntered{kMap, 1, kSpawn}});
    CHECK(fixture.world.player_count() == 1);
}

TEST_CASE("a player that leaves disappears for the others") {
    WorldFixture fixture;
    fixture.join(kAlice);
    fixture.join(kBob);
    fixture.forget_messages();

    fixture.world.remove(kAlice);
    fixture.world.remove(kCarol);

    CHECK(fixture.outbox.to(kBob) == std::vector<ServerMessage>{EntityLeft{1, LeaveReason::Disconnected}});
    CHECK(fixture.outbox.to(kAlice).empty());
    CHECK(fixture.world.player_count() == 1);
}

TEST_CASE("a player that leaves before its map loads is not announced") {
    WorldFixture fixture;
    fixture.join(kAlice);
    fixture.hello(kBob);
    fixture.forget_messages();

    fixture.world.remove(kBob);

    CHECK(fixture.outbox.sent.empty());
}

TEST_CASE("the server refuses a configuration it cannot follow") {
    ServerConfig config = kConfig;
    SECTION("no speed") {
        config.walk_tiles_per_second = 0.0;
    }
    SECTION("a speed that is not a number") {
        config.walk_tiles_per_second = std::numeric_limits<double>::quiet_NaN();
    }
    SECTION("a negative origin tolerance") {
        config.walks.origin_tolerance_steps = -1;
    }
    SECTION("a negative early tolerance") {
        config.walks.early_tolerance_seconds = -0.1;
    }
    SECTION("an endless early tolerance") {
        config.walks.early_tolerance_seconds = std::numeric_limits<double>::infinity();
    }
    SECTION("no room for players") {
        config.max_players = 0;
    }
    SECTION("a start outside the map") {
        config.spawn_center = Tile{16, 0};
    }
    RecordingOutbox outbox;

    const auto world = ServerWorld::create(open_grid(), config, outbox);

    REQUIRE_FALSE(world.has_value());
    CHECK_FALSE(world.error().message.empty());
}

TEST_CASE("a server grid refuses sides it cannot hold") {
    const auto [width, depth] = GENERATE(table<int, int>({{0, 4}, {4, 0}, {-1, 4}, {1025, 4}, {4, 1025}}));

    CHECK_FALSE(ServerGrid::create(width, depth).has_value());
    CHECK(ServerGrid::create(1024, 1).has_value());
}
