#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lemuria/Classic/World.hpp"
#include "WorldTestSupport.hpp"

using Catch::Approx;
using lemuria::classic::DiagonalRule;
using lemuria::classic::Entity;
using lemuria::classic::EntityAppeared;
using lemuria::classic::EntityId;
using lemuria::classic::EntityWalked;
using lemuria::classic::MapEntered;
using lemuria::classic::MapId;
using lemuria::classic::MovementRules;
using lemuria::classic::OwnWalkConfirmed;
using lemuria::classic::PathFailure;
using lemuria::classic::PositionCorrected;
using lemuria::classic::TileCoord;
using lemuria::classic::TileGrid;
using lemuria::classic::TilesChanged;
using lemuria::classic::WalkRequest;
using lemuria::classic::WalkRequestMode;
using lemuria::classic::WalkSteps;
using lemuria::classic::World;
using lemuria::classic::testing::kMovementRules;
using lemuria::classic::testing::kStepsPerRequest;
using lemuria::classic::testing::make_grid;
using lemuria::classic::testing::make_world;

namespace {

constexpr int kMapSide = 64;
constexpr double kPrecision = 0.001;
constexpr EntityId kPlayer{100};
constexpr EntityId kOther{7};
constexpr TileCoord kStart{2, 2};
constexpr MapEntered kMap{MapId{1}, kPlayer, kStart};

[[nodiscard]] MovementRules destination_only_rules() {
    MovementRules rules = kMovementRules;
    rules.walk_requests = WalkRequestMode::DestinationOnly;
    rules.max_steps_per_request = 0;
    return rules;
}

[[nodiscard]] WalkSteps steps_of(const std::vector<TileCoord>& tiles) {
    WalkSteps steps;
    for (const TileCoord tile : tiles) {
        REQUIRE(steps.add(tile));
    }
    return steps;
}

class WalkFixture {
public:
    explicit WalkFixture(const MovementRules& rules = kMovementRules, TileGrid grid = make_grid(kMapSide))
        : world(make_world(4, rules)) {
        world.enter_map(kMap);
        world.set_grid(std::move(grid));
    }

    [[nodiscard]] const Entity& own() const { return *world.own(); }

    void walk_to(TileCoord destination) {
        REQUIRE(world.walk_own_to(destination).has_value());
        collect_request();
    }

    // Runs whole frames, keeping the requests that the server would receive.
    void run(double seconds, int frames_per_second = 60) {
        const auto frames = static_cast<int>(std::lround(seconds * frames_per_second));
        for (int frame = 0; frame < frames; ++frame) {
            world.update(1.0 / frames_per_second);
            collect_request();
        }
    }

    World world;
    std::vector<WalkRequest> requests;

private:
    void collect_request() {
        if (const auto request = world.take_walk_request()) {
            requests.push_back(*request);
        }
    }
};

[[nodiscard]] TileGrid grid_with_wall(int wall_x, int gap_z) {
    TileGrid grid = make_grid(kMapSide);
    for (int z = 0; z < kMapSide; ++z) {
        if (z != gap_z) {
            REQUIRE(grid.set_walkable({wall_x, z}, false));
        }
    }
    return grid;
}

} // namespace

TEST_CASE("the player does not move to a tile that cannot be walked to", "[0000-CA-03]") {
    WalkFixture fixture(kMovementRules, grid_with_wall(10, -1));
    const auto [destination, failure] = GENERATE(table<TileCoord, PathFailure>({
        {TileCoord{10, 2}, PathFailure::DestinationBlocked},
        {TileCoord{20, 2}, PathFailure::Unreachable},
        {TileCoord{-1, 2}, PathFailure::OutsideGrid},
    }));

    const auto walked = fixture.world.walk_own_to(destination);
    fixture.run(1.0);

    REQUIRE_FALSE(walked.has_value());
    CHECK(walked.error() == failure);
    CHECK(fixture.own().tile() == kStart);
    CHECK_FALSE(fixture.own().destination.has_value());
    CHECK(fixture.requests.empty());
}

TEST_CASE("the player keeps walking after clicking a tile that cannot be walked to", "[0000-CA-03]") {
    WalkFixture fixture(kMovementRules, grid_with_wall(10, -1));
    fixture.walk_to({6, 2});

    CHECK_FALSE(fixture.world.walk_own_to({10, 2}).has_value());
    fixture.run(2.0);

    CHECK(fixture.own().tile() == TileCoord{6, 2});
    CHECK(fixture.requests.size() == 1);
}

TEST_CASE("the player walks around blocked tiles", "[0000-CA-03]") {
    WalkFixture fixture(kMovementRules, grid_with_wall(6, 9));
    fixture.walk_to({10, 2});

    for (int frame = 0; frame < 600; ++frame) {
        fixture.run(1.0 / 60.0);
        CHECK(fixture.world.grid()->is_walkable(fixture.own().tile()));
    }

    CHECK(fixture.own().tile() == TileCoord{10, 2});
}

TEST_CASE("the player cannot walk before the map is ready", "[0000-CA-03]") {
    World world = make_world(4);
    CHECK(world.walk_own_to({1, 1}).error() == PathFailure::OutsideGrid);

    world.enter_map(kMap);
    CHECK(world.walk_own_to({1, 1}).error() == PathFailure::OutsideGrid);
    world.update(1.0);
    CHECK(world.own()->tile() == kStart);
}

TEST_CASE("the player starts walking without waiting for the server", "[0000-CA-05]") {
    WalkFixture fixture;

    fixture.walk_to({6, 2});

    REQUIRE(fixture.requests.size() == 1);
    CHECK(fixture.requests[0] ==
          WalkRequest{kStart, TileCoord{6, 2}, steps_of({{3, 2}, {4, 2}, {5, 2}, {6, 2}})});
    CHECK(fixture.own().motion.is_walking());

    fixture.run(0.3);
    CHECK(fixture.own().tile() == TileCoord{3, 2});
}

TEST_CASE("the player arrives even when the server never answers", "[0000-CA-05]") {
    WalkFixture fixture;

    fixture.walk_to({22, 2});
    fixture.run(6.0);

    CHECK(fixture.own().tile() == TileCoord{22, 2});
    CHECK_FALSE(fixture.own().motion.is_walking());
    CHECK_FALSE(fixture.own().destination.has_value());
}

TEST_CASE("a long walk is sent in parts that follow each other", "[0000-CA-05]") {
    WalkFixture fixture;

    fixture.walk_to({22, 2});
    fixture.run(6.0);

    REQUIRE(fixture.requests.size() == 3);
    CHECK(fixture.requests[0].origin == kStart);
    CHECK(fixture.requests[0].steps.size() == kStepsPerRequest);
    CHECK(fixture.requests[1].origin == fixture.requests[0].destination);
    CHECK(fixture.requests[1].steps.size() == kStepsPerRequest);
    CHECK(fixture.requests[2].origin == fixture.requests[1].destination);
    CHECK(fixture.requests[2].destination == TileCoord{22, 2});
}

TEST_CASE("a walk request is handed over only once", "[0000-CA-05]") {
    WalkFixture fixture;
    REQUIRE(fixture.world.walk_own_to({6, 2}).has_value());

    CHECK(fixture.world.take_walk_request().has_value());
    CHECK_FALSE(fixture.world.take_walk_request().has_value());
}

TEST_CASE("a server that receives only the destination gets one request for the whole walk", "[0000-CA-05]") {
    WalkFixture fixture(destination_only_rules());

    fixture.walk_to({60, 2});
    fixture.run(16.0);

    REQUIRE(fixture.requests.size() == 1);
    CHECK(fixture.requests[0] == WalkRequest{kStart, TileCoord{60, 2}, WalkSteps{}});
    CHECK(fixture.own().tile() == TileCoord{60, 2});
}

TEST_CASE("a new click during a step starts from the end of that step", "[0000-CA-05]") {
    WalkFixture fixture;
    fixture.walk_to({6, 2});
    fixture.run(0.1);

    fixture.walk_to({3, 5});
    fixture.run(0.2);

    CHECK(fixture.requests.back().origin == TileCoord{3, 2});
    CHECK(fixture.own().tile() == TileCoord{3, 2});

    fixture.run(2.0);
    CHECK(fixture.own().tile() == TileCoord{3, 5});
}

TEST_CASE("a click on the step being walked stops the player there", "[0000-CA-05]") {
    WalkFixture fixture;
    fixture.walk_to({6, 2});
    fixture.run(0.1);

    fixture.walk_to({3, 2});
    fixture.run(2.0);

    CHECK(fixture.requests.back() == WalkRequest{TileCoord{3, 2}, TileCoord{3, 2}, WalkSteps{}});
    CHECK(fixture.own().tile() == TileCoord{3, 2});
    CHECK_FALSE(fixture.own().destination.has_value());
}

TEST_CASE("a click on the tile of the stopped player does nothing", "[0000-CA-05]") {
    WalkFixture fixture;

    fixture.walk_to(kStart);
    fixture.run(1.0);

    CHECK(fixture.requests.empty());
    CHECK(fixture.own().tile() == kStart);
}

TEST_CASE("a long walk reaches the same point at any frame rate", "[0000-CA-04]") {
    MovementRules rules = kMovementRules;
    rules.speeds.walk_tiles_per_second = 4.1;
    const int frames_per_second = GENERATE(30, 60, 144);
    WalkFixture fixture(rules);

    fixture.walk_to({40, 2});
    fixture.run(2.5, frames_per_second);

    CHECK(fixture.own().motion.position().x == Approx(kStart.x + 0.5 + 4.1 * 2.5).margin(kPrecision));
    CHECK(fixture.own().motion.position().z == Approx(kStart.z + 0.5).margin(kPrecision));
}

TEST_CASE("someone else reaches the same point at any frame rate", "[0000-CA-04]") {
    MovementRules rules = kMovementRules;
    rules.speeds.walk_tiles_per_second = 4.1;
    const int frames_per_second = GENERATE(30, 60, 144);
    WalkFixture fixture(rules);
    fixture.world.apply(EntityAppeared{kOther, TileCoord{0, 20}, std::nullopt});

    fixture.world.apply(EntityWalked{kOther, std::nullopt, TileCoord{60, 20}, WalkSteps{}});
    fixture.run(10.0, frames_per_second);

    CHECK(fixture.world.find(kOther)->motion.position().x == Approx(0.5 + 41.0).margin(kPrecision));
}

TEST_CASE("a correction replaces the position and cancels the walk", "[0000-CA-06]") {
    WalkFixture fixture;
    fixture.walk_to({22, 2});
    fixture.run(0.6);
    const auto before = fixture.own().motion.position();

    fixture.world.apply(PositionCorrected{kPlayer, TileCoord{5, 8}});

    CHECK(fixture.own().tile() == TileCoord{5, 8});
    CHECK_FALSE(fixture.own().motion.is_walking());
    CHECK_FALSE(fixture.own().destination.has_value());
    CHECK(fixture.own().motion.position().x == before.x);
    CHECK(fixture.own().motion.position().z == before.z);
}

TEST_CASE("the player glides to the corrected position and stays there", "[0000-CA-06]") {
    WalkFixture fixture;
    fixture.walk_to({22, 2});
    fixture.run(0.6);
    const std::size_t requests_before = fixture.requests.size();

    fixture.world.apply(PositionCorrected{kPlayer, TileCoord{5, 8}});
    fixture.run(5.0);

    CHECK(fixture.own().tile() == TileCoord{5, 8});
    CHECK(fixture.own().motion.position().x == Approx(5.5));
    CHECK(fixture.own().motion.position().z == Approx(8.5));
    CHECK(fixture.requests.size() == requests_before);
}

TEST_CASE("a correction drops the request that was still waiting to be sent", "[0000-CA-06]") {
    WalkFixture fixture;
    REQUIRE(fixture.world.walk_own_to({6, 2}).has_value());

    fixture.world.apply(PositionCorrected{kPlayer, TileCoord{5, 8}});

    CHECK_FALSE(fixture.world.take_walk_request().has_value());
}

TEST_CASE("an old answer does not undo a correction", "[0000-CA-06]") {
    WalkFixture fixture;
    fixture.walk_to({6, 2});
    fixture.run(0.3);
    fixture.world.apply(PositionCorrected{kPlayer, TileCoord{2, 8}});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{6, 2}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{2, 8});
    CHECK_FALSE(fixture.own().destination.has_value());
    CHECK(fixture.world.stale_confirmation_count() == 1);
}

TEST_CASE("an old answer does not change a newer walk", "[0000-CA-06]") {
    WalkFixture fixture;
    fixture.walk_to({6, 2});
    fixture.run(2.0);
    fixture.walk_to({6, 8});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{4, 2}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{6, 8});
    CHECK(fixture.world.stale_confirmation_count() == 1);
}

TEST_CASE("an old answer from the same origin does not change a newer walk", "[0000-CA-06]") {
    WalkFixture fixture;
    fixture.walk_to({6, 2});
    fixture.world.apply(PositionCorrected{kPlayer, kStart});
    fixture.walk_to({2, 8});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{6, 2}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{2, 8});
    CHECK(fixture.world.stale_confirmation_count() == 1);
}

TEST_CASE("an answer from another origin is ignored even when its tile is on the route", "[0000-CA-06]") {
    WalkFixture fixture;
    fixture.walk_to({6, 2});
    fixture.run(2.0);
    fixture.walk_to({6, 8});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{6, 4}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{6, 8});
    CHECK(fixture.world.stale_confirmation_count() == 1);
}

TEST_CASE("an answer without any walk changes nothing", "[0000-CA-06]") {
    WalkFixture fixture;

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{6, 2}});
    fixture.run(1.0);

    CHECK(fixture.own().tile() == kStart);
    CHECK(fixture.world.stale_confirmation_count() == 1);
}

TEST_CASE("the same answer as requested changes nothing", "[0000-CA-07]") {
    WalkFixture fixture;
    fixture.walk_to({6, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{6, 2}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{6, 2});
    CHECK(fixture.world.stale_confirmation_count() == 0);
    CHECK(fixture.world.refused_event_count() == 0);
}

TEST_CASE("confirming one part of a long walk does not stop the walk", "[0000-CA-07]") {
    WalkFixture fixture;
    fixture.walk_to({22, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{10, 2}});
    fixture.run(6.0);

    CHECK(fixture.own().tile() == TileCoord{22, 2});
    CHECK(fixture.world.stale_confirmation_count() == 0);
}

TEST_CASE("the player adopts a shorter destination without treating it as an error", "[0000-CA-07]") {
    WalkFixture fixture;
    fixture.walk_to({8, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{5, 2}});

    CHECK(fixture.own().destination == TileCoord{5, 2});
    fixture.run(3.0);
    CHECK(fixture.own().tile() == TileCoord{5, 2});
    CHECK_FALSE(fixture.own().destination.has_value());
    CHECK(fixture.world.stale_confirmation_count() == 0);
    CHECK(fixture.world.refused_event_count() == 0);
    CHECK(fixture.requests.size() == 1);
}

TEST_CASE("a shorter destination also cancels the next parts of a long walk", "[0000-CA-07]") {
    WalkFixture fixture;
    fixture.walk_to({22, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{5, 2}});
    fixture.run(6.0);

    CHECK(fixture.own().tile() == TileCoord{5, 2});
    CHECK(fixture.requests.size() == 1);
}

TEST_CASE("the player turns back when it already passed the shorter destination", "[0000-CA-07]") {
    WalkFixture fixture;
    fixture.walk_to({8, 2});
    fixture.run(1.1);
    REQUIRE(fixture.own().tile() == TileCoord{6, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{4, 2}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{4, 2});
    CHECK_FALSE(fixture.own().motion.is_walking());
    CHECK(fixture.requests.size() == 1);
}

TEST_CASE("the player returns to the origin when the server accepts no step", "[0000-CA-07]") {
    WalkFixture fixture;
    fixture.walk_to({8, 2});
    fixture.run(0.6);

    fixture.world.apply(OwnWalkConfirmed{kStart, kStart});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == kStart);
    CHECK(fixture.world.stale_confirmation_count() == 0);
}

TEST_CASE("a repeated shorter answer changes nothing more", "[0000-CA-07]") {
    WalkFixture fixture;
    fixture.walk_to({8, 2});
    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{5, 2}});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{5, 2}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{5, 2});
}

TEST_CASE("a server that finds the path may confirm a tile off the planned route", "[0000-CA-07]") {
    WalkFixture fixture(destination_only_rules());
    fixture.walk_to({8, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{5, 4}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{5, 4});
}

TEST_CASE("the player goes straight to a confirmed tile it cannot find a way to", "[0000-CA-07]") {
    TileGrid grid = grid_with_wall(10, -1);
    WalkFixture fixture(destination_only_rules(), std::move(grid));
    fixture.walk_to({8, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{20, 2}});
    fixture.run(5.0);

    CHECK(fixture.own().tile() == TileCoord{20, 2});
    CHECK_FALSE(fixture.own().destination.has_value());
}

TEST_CASE("an answer with a tile outside the map is refused", "[0000-CA-07]") {
    WalkFixture fixture(destination_only_rules());
    fixture.walk_to({8, 2});

    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{kMapSide, 2}});
    fixture.run(3.0);

    CHECK(fixture.own().tile() == TileCoord{8, 2});
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("the player stops at the end of a part when the way closes during the walk", "[0000-CA-05]") {
    WalkFixture fixture;
    fixture.walk_to({22, 2});

    fixture.world.apply(TilesChanged{TileCoord{15, 0}, TileCoord{15, kMapSide - 1}, false});
    fixture.run(6.0);

    CHECK(fixture.own().tile() == TileCoord{10, 2});
    CHECK_FALSE(fixture.own().motion.is_walking());
    CHECK_FALSE(fixture.own().destination.has_value());
}

TEST_CASE("changing map forgets the walk of the previous map", "[0000-CA-06]") {
    WalkFixture fixture;
    REQUIRE(fixture.world.walk_own_to({6, 2}).has_value());

    fixture.world.enter_map(kMap);
    fixture.world.set_grid(make_grid(kMapSide));
    fixture.world.apply(OwnWalkConfirmed{kStart, TileCoord{4, 2}});

    CHECK_FALSE(fixture.world.take_walk_request().has_value());
    CHECK_FALSE(fixture.own().motion.is_walking());
    CHECK(fixture.world.stale_confirmation_count() == 1);
}

TEST_CASE("appearing again forgets the walk of the player", "[0000-CA-06]") {
    WalkFixture fixture;
    REQUIRE(fixture.world.walk_own_to({6, 2}).has_value());

    fixture.world.apply(EntityAppeared{kPlayer, TileCoord{9, 9}, TileCoord{12, 9}});
    fixture.run(2.0);

    CHECK(fixture.own().tile() == TileCoord{9, 9});
    CHECK_FALSE(fixture.world.take_walk_request().has_value());
}

TEST_CASE("someone else finds the way when the server sends only the destination", "[0000-CA-15]") {
    WalkFixture fixture(kMovementRules, grid_with_wall(6, 9));
    fixture.world.apply(EntityAppeared{kOther, TileCoord{2, 4}, std::nullopt});

    fixture.world.apply(EntityWalked{kOther, std::nullopt, TileCoord{10, 4}, WalkSteps{}});

    for (int frame = 0; frame < 600; ++frame) {
        fixture.run(1.0 / 60.0);
        CHECK(fixture.world.grid()->is_walkable(fixture.world.find(kOther)->tile()));
    }
    CHECK(fixture.world.find(kOther)->tile() == TileCoord{10, 4});
    CHECK_FALSE(fixture.world.find(kOther)->destination.has_value());
}

TEST_CASE("someone else follows the steps the server sends", "[0000-CA-15]") {
    WalkFixture fixture;
    fixture.world.apply(EntityAppeared{kOther, TileCoord{10, 10}, std::nullopt});

    fixture.world.apply(
        EntityWalked{kOther, TileCoord{10, 10}, TileCoord{11, 12}, steps_of({{10, 11}, {10, 12}, {11, 12}})});
    fixture.run(0.6);

    CHECK(fixture.world.find(kOther)->tile() == TileCoord{10, 12});
    fixture.run(1.0);
    CHECK(fixture.world.find(kOther)->tile() == TileCoord{11, 12});
}

TEST_CASE("someone else continues past the steps of one event", "[0000-CA-15]") {
    WalkFixture fixture;
    fixture.world.apply(EntityAppeared{kOther, TileCoord{10, 10}, std::nullopt});

    fixture.world.apply(EntityWalked{kOther, std::nullopt, TileCoord{15, 10}, steps_of({{11, 10}})});
    fixture.run(3.0);

    CHECK(fixture.world.find(kOther)->tile() == TileCoord{15, 10});
}

TEST_CASE("someone who appears walking keeps walking", "[0000-CA-15]") {
    WalkFixture fixture;

    fixture.world.apply(EntityAppeared{kOther, TileCoord{10, 10}, TileCoord{14, 10}});
    fixture.run(3.0);

    CHECK(fixture.world.find(kOther)->tile() == TileCoord{14, 10});
    CHECK_FALSE(fixture.world.find(kOther)->destination.has_value());
}

TEST_CASE("the walk of someone else never produces a request", "[0000-CA-15]") {
    WalkFixture fixture;
    fixture.world.apply(EntityAppeared{kOther, TileCoord{0, 20}, std::nullopt});

    fixture.world.apply(EntityWalked{kOther, std::nullopt, TileCoord{60, 20}, WalkSteps{}});
    fixture.run(16.0);

    CHECK(fixture.requests.empty());
}

TEST_CASE("someone else goes straight to a destination the client cannot find a way to", "[0000-CA-15]") {
    WalkFixture fixture(kMovementRules, grid_with_wall(10, -1));
    fixture.world.apply(EntityAppeared{kOther, TileCoord{5, 5}, std::nullopt});

    fixture.world.apply(EntityWalked{kOther, std::nullopt, TileCoord{20, 5}, WalkSteps{}});

    CHECK(fixture.world.find(kOther)->tile() == TileCoord{20, 5});
    CHECK_FALSE(fixture.world.find(kOther)->destination.has_value());
}

TEST_CASE("steps that do not connect are refused", "[0000-CA-15]") {
    WalkFixture fixture;
    fixture.world.apply(EntityAppeared{kOther, TileCoord{10, 10}, std::nullopt});
    const WalkSteps steps = GENERATE(steps_of({{12, 10}}), steps_of({{10, 12}}), steps_of({{10, 10}}),
                                     steps_of({{11, 10}, {13, 10}}));

    fixture.world.apply(EntityWalked{kOther, TileCoord{10, 10}, TileCoord{13, 10}, steps});
    fixture.run(2.0);

    CHECK(fixture.world.find(kOther)->tile() == TileCoord{10, 10});
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("the server cannot walk the player with the event of someone else", "[0000-CA-15]") {
    WalkFixture fixture;

    fixture.world.apply(EntityWalked{kPlayer, std::nullopt, TileCoord{6, 2}, WalkSteps{}});
    fixture.run(2.0);

    CHECK(fixture.own().tile() == kStart);
    CHECK(fixture.world.refused_event_count() == 1);
}

TEST_CASE("world refuses movement rules it cannot follow") {
    MovementRules rules = kMovementRules;
    SECTION("no walk speed") {
        rules.speeds.walk_tiles_per_second = 0.0;
    }
    SECTION("negative walk speed") {
        rules.speeds.walk_tiles_per_second = -1.0;
    }
    SECTION("walk speed that is not a number") {
        rules.speeds.walk_tiles_per_second = std::numeric_limits<double>::quiet_NaN();
    }
    SECTION("no slide speed") {
        rules.speeds.slide_tiles_per_second = 0.0;
    }
    SECTION("infinite slide speed") {
        rules.speeds.slide_tiles_per_second = std::numeric_limits<double>::infinity();
    }
    SECTION("no search limit") {
        rules.paths.max_searched_tiles = 0;
    }
    SECTION("no steps per request") {
        rules.max_steps_per_request = 0;
    }
    SECTION("too many steps per request") {
        rules.max_steps_per_request = WalkSteps::kCapacity + 1;
    }

    CHECK_FALSE(World::create(1, rules).has_value());
}

TEST_CASE("world accepts every step limit it can carry") {
    MovementRules rules = kMovementRules;
    rules.max_steps_per_request = GENERATE(std::size_t{1}, WalkSteps::kCapacity);

    CHECK(World::create(1, rules).has_value());
}

TEST_CASE("world needs no step limit when requests carry only the destination") {
    CHECK(World::create(1, destination_only_rules()).has_value());
}

TEST_CASE("the player can walk diagonally when the rules allow it", "[0000-CA-04]") {
    MovementRules rules = kMovementRules;
    rules.paths.diagonals = DiagonalRule::Always;
    WalkFixture fixture(rules);

    fixture.walk_to({5, 5});

    CHECK(fixture.requests[0].steps.size() == 3);
}
