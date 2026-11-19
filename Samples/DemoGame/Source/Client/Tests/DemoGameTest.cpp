#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "DemoClient/DemoAdapter.hpp"
#include "DemoClient/DemoGame.hpp"
#include "DemoClient/GroundScene.hpp"
#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Messages.hpp"
#include "DemoProtocol/Protocol.hpp"
#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/SessionPhase.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/FrameClock.hpp"
#include "Lemuria/Core/SceneDescription.hpp"
#include "ScriptedConnections.hpp"

namespace classic = lemuria::classic;
namespace core = lemuria::core;
namespace protocol = demo_protocol;

using demo_client::connection_status;
using demo_client::DemoAdapter;
using demo_client::DemoGame;
using demo_client::testing::bytes_of;
using demo_client::testing::ScriptedConnections;
using protocol::DemoMap;

namespace {

constexpr double kFrameSeconds = 1.0 / 60.0;
constexpr protocol::Tile kStart{DemoMap::kSpawnCenter, DemoMap::kSpawnCenter};

// One player's game, with a connection written by the test.
class GameFixture {
public:
    GameFixture() : adapter(connections, core::ConnectionAddress{"127.0.0.1", 24'000}) {
        auto created = DemoGame::create(adapter);
        REQUIRE(created.has_value());
        game = std::move(*created);
        REQUIRE(game->start().has_value());
    }

    void frame() {
        game->frame(core::FrameTime{kFrameSeconds, elapsed_, frame_index_});
        elapsed_ += kFrameSeconds;
        ++frame_index_;
    }

    void run(double seconds) {
        for (double passed = 0.0; passed < seconds; passed += kFrameSeconds) {
            frame();
        }
    }

    void enter_map(std::uint32_t map = DemoMap::kId) {
        connections.arrive_connected();
        connections.arrive_bytes(bytes_of(protocol::MapEntered{map, 7, kStart}));
        frame();
        frame();
    }

    [[nodiscard]] bool has_sent(const protocol::ClientMessage& message) const {
        return std::ranges::find(connections.sent, bytes_of(message)) != connections.sent.end();
    }

    ScriptedConnections connections;
    DemoAdapter adapter;
    std::unique_ptr<DemoGame> game;

private:
    double elapsed_ = 0.0;
    std::uint64_t frame_index_ = 0;
};

[[nodiscard]] std::size_t rock_count(const DemoMap& map) {
    std::size_t count = 0;
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            count += map.kind_at(x, z) == protocol::TileKind::Stone ? 1 : 0;
        }
    }
    return count;
}

// The first tile to the right of the start that the client believes walkable, a few tiles away.
[[nodiscard]] std::optional<classic::TileCoord> walkable_tile_near_start() {
    const DemoMap map;
    for (int x = kStart.x + 3; x < DemoMap::kSize; ++x) {
        if (map.looks_walkable(x, kStart.z)) {
            return classic::TileCoord{x, kStart.z};
        }
    }
    return std::nullopt;
}

} // namespace

TEST_CASE("the window title tells every state of the connection", "[0000-CA-14]") {
    using classic::SessionEndReason;
    using classic::SessionPhase;

    CHECK(connection_status(SessionPhase::Connecting, std::nullopt) == "connecting to the test server");
    CHECK(connection_status(SessionPhase::EnteringMap, std::nullopt) == "entering the map");
    CHECK(connection_status(SessionPhase::InWorld, std::nullopt) == "on the map");
    CHECK(connection_status(SessionPhase::Ended, SessionEndReason::CouldNotConnect) ==
          "the test server could not be reached");
    CHECK(connection_status(SessionPhase::Ended, SessionEndReason::ClosedByServer) ==
          "the connection to the test server was lost");
    CHECK(connection_status(SessionPhase::Ended, SessionEndReason::ConnectionLost) ==
          "the connection to the test server was lost");
    CHECK(connection_status(SessionPhase::Ended, SessionEndReason::TimedOut) ==
          "the test server stopped answering");
    CHECK(connection_status(SessionPhase::Ended, SessionEndReason::InvalidData) ==
          "the test server sent data the client could not use");
    CHECK(connection_status(SessionPhase::Ended, SessionEndReason::ClosedByPlayer) == "disconnected");
}

TEST_CASE("the window title gives a clear state for the phases and ends this demonstration does not use",
          "[0000-CA-14]") {
    using classic::SessionEndReason;
    using classic::SessionPhase;

    const SessionPhase between = GENERATE(SessionPhase::Authenticating, SessionPhase::ChoosingCharacter,
                                          SessionPhase::ChangingMap, SessionPhase::Reconnecting);
    CHECK(connection_status(between, std::nullopt) == "entering the map");
    CHECK(connection_status(SessionPhase::Ended, SessionEndReason::TooManyHeldEvents) ==
          "the test server sent data the client could not use");
    CHECK(connection_status(SessionPhase::Ended, std::nullopt) ==
          "the connection to the test server was lost");
}

TEST_CASE("before the player arrives, the camera looks at the middle of the map") {
    GameFixture fixture;
    const auto check_focus_on = [&](classic::TileCoord tile) {
        const core::Vec3 focus = fixture.game->focus();
        const core::Vec3 expected = classic::tile_center(tile);
        CHECK(focus.x == expected.x);
        CHECK(focus.z == expected.z);
    };

    check_focus_on({DemoMap::kSpawnCenter, DemoMap::kSpawnCenter});

    const protocol::Tile arrival{kStart.x + 5, kStart.z};
    fixture.connections.arrive_connected();
    fixture.connections.arrive_bytes(bytes_of(protocol::MapEntered{DemoMap::kId, 7, arrival}));
    fixture.frame();
    fixture.frame();

    check_focus_on({arrival.x, arrival.z});
}

TEST_CASE("the client believes only stone blocks the way, even where the server forbids more") {
    const DemoMap map;
    const auto grid = demo_client::client_grid(map);
    REQUIRE(grid.has_value());

    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            CHECK(grid->is_walkable({x, z}) == map.looks_walkable(x, z));
        }
    }
}

TEST_CASE("the demo game enters the map, says the map is loaded and walks where the player clicks") {
    GameFixture fixture;
    CHECK(fixture.game->status() == "connecting to the test server");

    fixture.enter_map();

    CHECK(fixture.game->status() == "on the map");
    CHECK(fixture.has_sent(protocol::Hello{protocol::protocol_version()}));
    CHECK(fixture.has_sent(protocol::MapLoaded{DemoMap::kId}));
    REQUIRE(fixture.game->world().own() != nullptr);
    CHECK(fixture.game->world().own()->tile() == classic::TileCoord{kStart.x, kStart.z});

    const std::optional<classic::TileCoord> destination = walkable_tile_near_start();
    REQUIRE(destination.has_value());
    REQUIRE(fixture.game->click(*destination));
    fixture.frame();
    // The third byte of a frame is the type of the message, and 2 is a walk request.
    REQUIRE(fixture.connections.sent.size() == 3);
    CHECK(fixture.connections.sent.back().at(2) == std::byte{2});

    fixture.run(10.0);

    CHECK(fixture.game->world().own()->tile() == *destination);
}

TEST_CASE("the demo game draws the ground, the rocks and everyone on the map, and follows the player") {
    GameFixture fixture;
    fixture.enter_map();
    fixture.connections.arrive_bytes(bytes_of(
        protocol::EntityAppeared{8, {static_cast<std::uint16_t>(kStart.x + 2), kStart.z}, std::nullopt}));
    fixture.frame();
    core::SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);

    fixture.game->describe(scene);

    CHECK(scene.ground_tiles.shapes().size() == demo_client::kGroundTileBudget);
    CHECK(scene.boxes.shapes().size() == rock_count(DemoMap{}) + 2);
    CHECK(scene.boxes.dropped_count() == 0);
    const core::Vec3 focus = fixture.game->focus();
    const core::Vec3 own = classic::tile_center({kStart.x, kStart.z});
    CHECK(focus.x == own.x);
    CHECK(focus.z == own.z);
}

TEST_CASE("a server that does not exist shows in the title, and the game goes on without it",
          "[0000-CA-14]") {
    GameFixture fixture;

    fixture.connections.arrive_closed(core::CloseReason::CouldNotConnect);
    fixture.run(1.0);

    CHECK(fixture.game->session().phase() == classic::SessionPhase::Ended);
    CHECK(fixture.game->status() == "the test server could not be reached");
    CHECK_FALSE(fixture.game->click({kStart.x + 1, kStart.z}));
}

TEST_CASE("a server that falls during the game shows in the title, and the player can no longer walk",
          "[0000-CA-14]") {
    GameFixture fixture;
    fixture.enter_map();

    fixture.connections.arrive_closed(core::CloseReason::PeerClosed);
    fixture.frame();

    CHECK(fixture.game->status() == "the connection to the test server was lost");
    const std::optional<classic::TileCoord> destination = walkable_tile_near_start();
    REQUIRE(destination.has_value());
    CHECK_FALSE(fixture.game->click(*destination));
    fixture.run(1.0);
    CHECK(fixture.game->world().own()->tile() == classic::TileCoord{kStart.x, kStart.z});
}

TEST_CASE("the demo game leaves a map it does not know") {
    GameFixture fixture;

    fixture.enter_map(DemoMap::kId + 1);
    fixture.frame();

    CHECK(fixture.game->session().phase() == classic::SessionPhase::Ended);
    CHECK_FALSE(fixture.has_sent(protocol::MapLoaded{DemoMap::kId + 1}));
    CHECK(fixture.game->status() == "the test server sent a map the client does not know");
}

TEST_CASE("the demo game leaves a map where the server placed the player outside it", "[0000-CA-11]") {
    GameFixture fixture;
    fixture.connections.arrive_connected();
    fixture.connections.arrive_bytes(bytes_of(protocol::MapEntered{DemoMap::kId, 7, {DemoMap::kSize, 3}}));

    fixture.frame();
    fixture.frame();

    CHECK(fixture.game->session().phase() == classic::SessionPhase::Ended);
    CHECK_FALSE(fixture.has_sent(protocol::MapLoaded{DemoMap::kId}));
    CHECK(fixture.game->status() == "the test server placed the player outside the map");
}
