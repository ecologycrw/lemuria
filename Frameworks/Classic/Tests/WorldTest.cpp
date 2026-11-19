#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "AdapterTestSupport.hpp"
#include "Lemuria/Classic/GameSession.hpp"
#include "Lemuria/Classic/World.hpp"

using lemuria::classic::Entity;
using lemuria::classic::EntityAppeared;
using lemuria::classic::EntityId;
using lemuria::classic::EntityLeaveReason;
using lemuria::classic::EntityLeft;
using lemuria::classic::EntityWalked;
using lemuria::classic::GameEvent;
using lemuria::classic::GameSession;
using lemuria::classic::MapEntered;
using lemuria::classic::MapId;
using lemuria::classic::MapLoadTicket;
using lemuria::classic::PositionCorrected;
using lemuria::classic::SessionEnded;
using lemuria::classic::SessionEndReason;
using lemuria::classic::SessionLimits;
using lemuria::classic::SessionListener;
using lemuria::classic::SessionPhase;
using lemuria::classic::SessionPhaseReached;
using lemuria::classic::TileCoord;
using lemuria::classic::TileGrid;
using lemuria::classic::TilesChanged;
using lemuria::classic::WalkSteps;
using lemuria::classic::World;
using lemuria::classic::testing::ScriptedAdapter;
using lemuria::core::FrameTime;

namespace {

constexpr int kMapSide = 10;
constexpr int kLowest = std::numeric_limits<int>::min();
constexpr int kHighest = std::numeric_limits<int>::max();
constexpr EntityId kPlayer{100};
constexpr EntityId kOther{7};
constexpr EntityId kAnother{8};
constexpr TileCoord kOutside{kMapSide, 0};
constexpr MapEntered kFirstMap{MapId{1}, kPlayer, TileCoord{3, 3}};
constexpr MapEntered kSecondMap{MapId{2}, kPlayer, TileCoord{6, 6}};

[[nodiscard]] TileGrid make_grid() {
    auto grid = TileGrid::create(kMapSide, kMapSide);
    REQUIRE(grid.has_value());
    return *std::move(grid);
}

[[nodiscard]] GameEvent appeared(EntityId entity, TileCoord position) {
    return EntityAppeared{entity, position, std::nullopt};
}

[[nodiscard]] GameEvent walked(EntityId entity, TileCoord destination) {
    return EntityWalked{entity, std::nullopt, destination, WalkSteps{}};
}

[[nodiscard]] GameEvent gone(EntityId entity) {
    return EntityLeft{entity, EntityLeaveReason::LeftView};
}

[[nodiscard]] WalkSteps steps_through(TileCoord tile) {
    WalkSteps steps;
    REQUIRE(steps.add(tile));
    return steps;
}

// A world that already has its map ready.
class WorldFixture {
public:
    explicit WorldFixture(std::size_t max_other_entities = 3) : world(max_other_entities) {
        world.enter_map(kFirstMap);
        world.set_grid(make_grid());
    }

    World world;
};

// Connects a session to a world in the way a game does.
class WorldLoader final : public SessionListener {
public:
    explicit WorldLoader(World& world) : world_(world) {}

    void on_map_entered(const MapEntered& entered, MapLoadTicket ticket) override {
        world_.enter_map(entered);
        last_ticket = ticket;
    }

    void on_world_event(const GameEvent& event) override { world_.apply(event); }

    MapLoadTicket last_ticket;

private:
    World& world_;
};

} // namespace

TEST_CASE("world has nothing before the first map", "[0000-CA-12]") {
    const World world(3);

    CHECK(world.entities().empty());
    CHECK(world.own() == nullptr);
    CHECK(world.grid() == nullptr);
}

TEST_CASE("world refuses events that arrive before its map is ready", "[0000-CA-12]") {
    World world(3);

    world.apply(appeared(kOther, {1, 1}));
    world.enter_map(kFirstMap);
    world.apply(appeared(kOther, {1, 1}));

    CHECK(world.find(kOther) == nullptr);
    CHECK(world.refused_event_count() == 2);
}

TEST_CASE("world places the player when it enters a map", "[0000-CA-12]") {
    const WorldFixture fixture;

    REQUIRE(fixture.world.own() != nullptr);
    CHECK(fixture.world.own()->id == kPlayer);
    CHECK(fixture.world.own()->tile == kFirstMap.own_position);
    CHECK(fixture.world.entities().size() == 1);
    CHECK(fixture.world.map() == kFirstMap.map);
}

TEST_CASE("world adds what comes into view", "[0000-CA-12]") {
    WorldFixture fixture;

    fixture.world.apply(EntityAppeared{kOther, TileCoord{1, 2}, TileCoord{4, 2}});

    REQUIRE(fixture.world.find(kOther) != nullptr);
    CHECK(*fixture.world.find(kOther) == Entity{kOther, TileCoord{1, 2}, TileCoord{4, 2}});
    CHECK(fixture.world.entities().size() == 2);
}

TEST_CASE("world ignores and counts events about an unknown number", "[0000-CA-12]") {
    WorldFixture fixture;
    const GameEvent event =
        GENERATE(walked(kOther, {2, 2}), GameEvent{PositionCorrected{kOther, TileCoord{2, 2}}}, gone(kOther));

    fixture.world.apply(event);

    CHECK(fixture.world.find(kOther) == nullptr);
    CHECK(fixture.world.entities().size() == 1);
    CHECK(fixture.world.unknown_entity_event_count() == 1);
    CHECK(fixture.world.refused_event_count() == 0);
}

TEST_CASE("world replaces what had a number when the number is used again", "[0000-CA-12]") {
    WorldFixture fixture;
    fixture.world.apply(EntityAppeared{kOther, TileCoord{1, 1}, TileCoord{5, 5}});

    fixture.world.apply(appeared(kOther, {8, 8}));

    REQUIRE(fixture.world.find(kOther) != nullptr);
    CHECK(*fixture.world.find(kOther) == Entity{kOther, TileCoord{8, 8}, std::nullopt});
    CHECK(fixture.world.entities().size() == 2);
    CHECK(fixture.world.unknown_entity_event_count() == 0);
}

TEST_CASE("world accepts a number again after it left", "[0000-CA-12]") {
    WorldFixture fixture;
    fixture.world.apply(appeared(kOther, {1, 1}));
    fixture.world.apply(gone(kOther));
    REQUIRE(fixture.world.find(kOther) == nullptr);

    fixture.world.apply(appeared(kOther, {2, 2}));

    REQUIRE(fixture.world.find(kOther) != nullptr);
    CHECK(fixture.world.find(kOther)->tile == TileCoord{2, 2});
}

TEST_CASE("world removes only what left", "[0000-CA-12]") {
    WorldFixture fixture;
    fixture.world.apply(appeared(kOther, {1, 1}));
    fixture.world.apply(appeared(kAnother, {2, 2}));

    fixture.world.apply(gone(kOther));

    CHECK(fixture.world.find(kOther) == nullptr);
    CHECK(fixture.world.find(kAnother) != nullptr);
    CHECK(fixture.world.own() != nullptr);
    CHECK(fixture.world.entities().size() == 2);
}

TEST_CASE("world never removes the player", "[0000-CA-12]") {
    WorldFixture fixture;

    fixture.world.apply(gone(kPlayer));

    CHECK(fixture.world.own() != nullptr);
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("world keeps room for the player even without room for anything else", "[0000-CA-12]") {
    WorldFixture fixture(0);

    fixture.world.apply(appeared(kOther, {1, 1}));

    CHECK(fixture.world.own() != nullptr);
    CHECK(fixture.world.find(kOther) == nullptr);
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("world refuses new arrivals when it is full", "[0000-CA-12]") {
    WorldFixture fixture(2);
    fixture.world.apply(appeared(EntityId{1}, {1, 1}));
    fixture.world.apply(appeared(EntityId{2}, {2, 2}));

    fixture.world.apply(appeared(EntityId{3}, {3, 3}));

    CHECK(fixture.world.entities().size() == 3);
    CHECK(fixture.world.find(EntityId{3}) == nullptr);
    CHECK(fixture.world.refused_event_count() == 1);

    SECTION("a number in use can still be replaced") {
        fixture.world.apply(appeared(EntityId{2}, {4, 4}));
        CHECK(fixture.world.find(EntityId{2})->tile == TileCoord{4, 4});
        CHECK(fixture.world.refused_event_count() == 1);
    }
    SECTION("there is room again after someone leaves") {
        fixture.world.apply(gone(EntityId{1}));
        fixture.world.apply(appeared(EntityId{3}, {3, 3}));
        CHECK(fixture.world.find(EntityId{3}) != nullptr);
    }
}

TEST_CASE("world records where someone is walking to", "[0000-CA-12]") {
    WorldFixture fixture;
    fixture.world.apply(appeared(kOther, {1, 1}));

    fixture.world.apply(walked(kOther, {5, 1}));

    CHECK(*fixture.world.find(kOther) == Entity{kOther, TileCoord{1, 1}, TileCoord{5, 1}});
}

TEST_CASE("world follows the origin of a walk when the server sends it", "[0000-CA-12]") {
    WorldFixture fixture;
    fixture.world.apply(appeared(kOther, {1, 1}));

    fixture.world.apply(EntityWalked{kOther, TileCoord{2, 1}, TileCoord{5, 1}, steps_through({3, 1})});

    CHECK(*fixture.world.find(kOther) == Entity{kOther, TileCoord{2, 1}, TileCoord{5, 1}});
}

TEST_CASE("world moves what the server corrects and cancels its walk", "[0000-CA-12]") {
    WorldFixture fixture;
    fixture.world.apply(EntityAppeared{kOther, TileCoord{1, 1}, TileCoord{5, 5}});

    fixture.world.apply(PositionCorrected{kOther, TileCoord{9, 0}});

    CHECK(*fixture.world.find(kOther) == Entity{kOther, TileCoord{9, 0}, std::nullopt});
}

TEST_CASE("world refuses events with tiles outside the map", "[0000-CA-12]") {
    WorldFixture fixture;
    const EntityId known{50};
    fixture.world.apply(EntityAppeared{known, TileCoord{1, 1}, TileCoord{2, 2}});
    const Entity before = *fixture.world.find(known);
    const GameEvent event = GENERATE_COPY(
        GameEvent{EntityAppeared{known, kOutside, std::nullopt}},
        GameEvent{EntityAppeared{known, TileCoord{3, 3}, kOutside}},
        GameEvent{EntityWalked{known, std::nullopt, kOutside, WalkSteps{}}},
        GameEvent{EntityWalked{known, kOutside, TileCoord{3, 3}, WalkSteps{}}},
        GameEvent{EntityWalked{known, std::nullopt, TileCoord{3, 3}, steps_through(kOutside)}},
        GameEvent{PositionCorrected{known, kOutside}}, GameEvent{PositionCorrected{known, TileCoord{-1, 0}}},
        GameEvent{PositionCorrected{known, TileCoord{0, kMapSide}}});

    fixture.world.apply(event);

    CHECK(*fixture.world.find(known) == before);
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("world refuses something that appears outside the map", "[0000-CA-12]") {
    WorldFixture fixture;

    fixture.world.apply(appeared(kOther, kOutside));

    CHECK(fixture.world.find(kOther) == nullptr);
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("world refuses events that belong to the session") {
    WorldFixture fixture;
    const GameEvent event =
        GENERATE(GameEvent{SessionPhaseReached{SessionPhase::InWorld}},
                 GameEvent{SessionEnded{SessionEndReason::TimedOut}}, GameEvent{kSecondMap});

    fixture.world.apply(event);

    CHECK(fixture.world.map() == kFirstMap.map);
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("world changes an area of the grid during the game", "[0000-CA-12]") {
    WorldFixture fixture;
    const auto [first, last] = GENERATE(table<TileCoord, TileCoord>({
        {TileCoord{2, 3}, TileCoord{4, 5}},
        {TileCoord{4, 5}, TileCoord{2, 3}},
        {TileCoord{4, 3}, TileCoord{2, 5}},
    }));

    fixture.world.apply(TilesChanged{first, last, false});

    const TileGrid& grid = *fixture.world.grid();
    for (int z = 0; z < kMapSide; ++z) {
        for (int x = 0; x < kMapSide; ++x) {
            const bool is_inside_area = x >= 2 && x <= 4 && z >= 3 && z <= 5;
            CHECK(grid.is_walkable({x, z}) == !is_inside_area);
        }
    }
}

TEST_CASE("world can free an area of the grid again", "[0000-CA-12]") {
    WorldFixture fixture;
    fixture.world.apply(TilesChanged{TileCoord{1, 1}, TileCoord{2, 2}, false});

    fixture.world.apply(TilesChanged{TileCoord{1, 1}, TileCoord{1, 2}, true});

    CHECK(fixture.world.grid()->is_walkable({1, 1}));
    CHECK(fixture.world.grid()->is_walkable({1, 2}));
    CHECK_FALSE(fixture.world.grid()->is_walkable({2, 1}));
}

TEST_CASE("world keeps only the part of an area that is inside the map", "[0000-CA-12]") {
    WorldFixture fixture;

    fixture.world.apply(TilesChanged{TileCoord{kLowest, 8}, TileCoord{kHighest, kHighest}, false});

    CHECK_FALSE(fixture.world.grid()->is_walkable({0, 8}));
    CHECK_FALSE(fixture.world.grid()->is_walkable({kMapSide - 1, kMapSide - 1}));
    CHECK(fixture.world.grid()->is_walkable({0, 7}));
    CHECK(fixture.world.refused_event_count() == 0);
}

TEST_CASE("world changes the first tiles of an area that starts before the map", "[0000-CA-12]") {
    WorldFixture fixture;

    fixture.world.apply(TilesChanged{TileCoord{4, kLowest}, TileCoord{4, 1}, false});

    CHECK_FALSE(fixture.world.grid()->is_walkable({4, 0}));
    CHECK_FALSE(fixture.world.grid()->is_walkable({4, 1}));
    CHECK(fixture.world.grid()->is_walkable({4, 2}));
}

TEST_CASE("world refuses an area that is completely outside the map", "[0000-CA-12]") {
    WorldFixture fixture;
    const auto [first, last] = GENERATE(table<TileCoord, TileCoord>({
        {TileCoord{kMapSide, 0}, TileCoord{kHighest, 5}},
        {TileCoord{0, kMapSide}, TileCoord{5, kHighest}},
        {TileCoord{kLowest, 0}, TileCoord{-1, 5}},
        {TileCoord{0, kLowest}, TileCoord{5, -1}},
    }));

    fixture.world.apply(TilesChanged{first, last, false});

    CHECK(fixture.world.refused_event_count() == 1);
    CHECK(fixture.world.grid()->is_walkable({0, 0}));
}

TEST_CASE("world removes everyone when the map changes", "[0000-CA-13]") {
    WorldFixture fixture;
    fixture.world.apply(appeared(kOther, {1, 1}));
    fixture.world.apply(appeared(kAnother, {2, 2}));

    fixture.world.enter_map(kSecondMap);

    REQUIRE(fixture.world.entities().size() == 1);
    CHECK(fixture.world.own()->tile == kSecondMap.own_position);
    CHECK(fixture.world.map() == kSecondMap.map);
    CHECK(fixture.world.grid() == nullptr);
}

TEST_CASE("nothing of the previous map survives a change of map during a session", "[0000-CA-13]") {
    constexpr SessionLimits kLimits{5.0, 5.0, 5.0, 5.0, 8, 8};
    ScriptedAdapter adapter;
    World world(8);
    WorldLoader loader(world);
    GameSession session(adapter, kLimits, loader);
    REQUIRE(session.start().has_value());

    adapter.arrive(kFirstMap);
    session.update(FrameTime{});
    world.set_grid(make_grid());
    session.finish_map_load(loader.last_ticket);
    adapter.arrive(appeared(kOther, {1, 1}));
    session.update(FrameTime{});
    REQUIRE(world.find(kOther) != nullptr);

    adapter.arrive(kSecondMap);
    adapter.arrive(appeared(kAnother, {2, 2}));
    session.update(FrameTime{});
    CHECK(world.entities().size() == 1);

    world.set_grid(make_grid());
    session.finish_map_load(loader.last_ticket);
    session.update(FrameTime{});

    CHECK(world.find(kOther) == nullptr);
    CHECK(world.find(kAnother) != nullptr);
    CHECK(world.entities().size() == 2);
    CHECK(world.refused_event_count() == 0);
}
