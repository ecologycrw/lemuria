#include <algorithm>
#include <initializer_list>
#include <numbers>
#include <utility>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "TestServer/WalkCheck.hpp"

using Catch::Approx;
using demo_protocol::WalkRequest;
using test_server::check_walk;
using test_server::ServerGrid;
using test_server::ServerMotion;
using test_server::StepList;
using test_server::Tile;
using test_server::WalkLimits;
using test_server::WalkRefusal;

namespace {

constexpr double kSpeed = 4.0;
constexpr WalkLimits kLimits{.origin_tolerance_steps = 1, .early_tolerance_seconds = 0.5};
constexpr Tile kStart{2, 2};

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

// A player standing at the start since the beginning.
[[nodiscard]] ServerMotion standing() {
    return ServerMotion(kStart, 0.0, kSpeed);
}

// A player that started at time 0 to walk four straight steps, which it finishes at time 1.
[[nodiscard]] ServerMotion walking_four_steps() {
    ServerMotion motion = standing();
    motion.start_walk(kStart, 0.0, steps_of({{3, 2}, {4, 2}, {5, 2}, {6, 2}}));
    return motion;
}

[[nodiscard]] auto check(const WalkRequest& request, const ServerMotion& motion, double now,
                         const ServerGrid& grid = open_grid(), const WalkLimits& limits = kLimits) {
    return check_walk(request, motion, now, grid, limits);
}

void require_refused(const tl::expected<double, WalkRefusal>& checked, WalkRefusal refusal) {
    REQUIRE_FALSE(checked.has_value());
    CHECK(checked.error() == refusal);
}

} // namespace

TEST_CASE("a walk over free tiles that touch each other starts at once") {
    const auto checked = check(WalkRequest{kStart, steps_of({{3, 2}, {4, 3}, {4, 4}})}, standing(), 5.0);

    REQUIRE(checked.has_value());
    CHECK(*checked == 5.0);
}

TEST_CASE("a player can ask to stop where it stands") {
    const auto checked = check(WalkRequest{kStart, {}}, standing(), 5.0);

    REQUIRE(checked.has_value());
    CHECK(*checked == 5.0);
}

TEST_CASE("the next part of a walk is accepted when it arrives in time") {
    const double now = GENERATE(0.5, 1.0, 3.0);
    const auto checked = check(WalkRequest{{6, 2}, steps_of({{7, 2}})}, walking_four_steps(), now);

    REQUIRE(checked.has_value());
    CHECK(*checked == std::max(now, 1.0));
}

TEST_CASE("a new walk can start from the tile the player is stepping to") {
    const auto checked = check(WalkRequest{{3, 2}, steps_of({{3, 3}})}, walking_four_steps(), 0.1);

    REQUIRE(checked.has_value());
    CHECK(*checked == 0.25);
}

TEST_CASE("a walk may start a step away from where the player stands, paying for that step") {
    const auto checked = check(WalkRequest{{3, 3}, steps_of({{4, 3}})}, standing(), 5.0);

    REQUIRE(checked.has_value());
    CHECK(*checked == Approx(5.0 + (std::numbers::sqrt2 / kSpeed)));
}

TEST_CASE("a step onto a tile the server forbids is refused", "[0000-CA-08]") {
    ServerGrid grid = open_grid();
    REQUIRE(grid.set_walkable({4, 2}, false));

    const auto checked =
        check(WalkRequest{kStart, steps_of({{3, 2}, {4, 2}, {5, 2}})}, standing(), 5.0, grid);

    require_refused(checked, WalkRefusal::BlockedStep);
}

TEST_CASE("a step outside the map is refused", "[0000-CA-08]") {
    const ServerMotion motion({15, 2}, 0.0, kSpeed);

    const auto checked = check(WalkRequest{{15, 2}, steps_of({{16, 2}})}, motion, 5.0);

    require_refused(checked, WalkRefusal::BlockedStep);
}

TEST_CASE("steps that do not touch the tile before them are refused", "[0000-CA-08]") {
    const std::vector<Tile> steps =
        GENERATE(std::vector<Tile>{{4, 2}}, std::vector<Tile>{{3, 2}, {5, 2}}, std::vector<Tile>{{2, 2}},
                 std::vector<Tile>{{3, 2}, {3, 2}}, std::vector<Tile>{{3, 2}, {4, 4}});
    StepList list;
    for (const Tile step : steps) {
        REQUIRE(list.add(step));
    }

    const auto checked = check(WalkRequest{kStart, list}, standing(), 5.0);

    require_refused(checked, WalkRefusal::NotNeighbor);
}

TEST_CASE("a diagonal step beside a forbidden tile is refused", "[0000-CA-08]") {
    const Tile beside = GENERATE(Tile{3, 2}, Tile{2, 3});
    ServerGrid grid = open_grid();
    REQUIRE(grid.set_walkable(beside, false));

    const auto checked = check(WalkRequest{kStart, steps_of({{3, 3}})}, standing(), 5.0, grid);

    require_refused(checked, WalkRefusal::CutsCorner);
}

TEST_CASE("a walk that starts too far from the player is refused", "[0000-CA-08]") {
    const Tile origin = GENERATE(Tile{4, 2}, Tile{4, 4}, Tile{0, 0}, Tile{2, 9});

    const auto checked = check(WalkRequest{origin, {}}, standing(), 5.0);

    require_refused(checked, WalkRefusal::OriginTooFar);
}

TEST_CASE("a walk off the route is measured from where the player is now", "[0000-CA-08]") {
    const ServerMotion motion = walking_four_steps();

    CHECK(check(WalkRequest{{5, 3}, {}}, motion, 0.75).has_value());
    require_refused(check(WalkRequest{{5, 3}, {}}, motion, 0.0), WalkRefusal::OriginTooFar);
}

TEST_CASE("a walk that starts on a tile the player already passed is measured from where it is now",
          "[0000-CA-08]") {
    const ServerMotion motion = walking_four_steps();

    SECTION("more than a step behind is too far, even long after the walk") {
        require_refused(check(WalkRequest{kStart, steps_of({{1, 2}})}, motion, 10.0),
                        WalkRefusal::OriginTooFar);
        require_refused(check(WalkRequest{{4, 2}, {}}, motion, 10.0), WalkRefusal::OriginTooFar);
        require_refused(check(WalkRequest{kStart, {}}, motion, 0.6), WalkRefusal::OriginTooFar);
    }
    SECTION("one step behind is accepted, paying for the step back") {
        const auto checked = check(WalkRequest{{5, 2}, steps_of({{5, 3}})}, motion, 10.0);

        REQUIRE(checked.has_value());
        CHECK(*checked == Approx(10.25));
    }
}

TEST_CASE("the next part sent before the player could finish the previous one is too fast", "[0000-CA-08]") {
    const ServerMotion motion = walking_four_steps();

    require_refused(check(WalkRequest{{6, 2}, steps_of({{7, 2}})}, motion, 0.25), WalkRefusal::TooFast);
    require_refused(check(WalkRequest{{6, 2}, steps_of({{7, 2}})}, motion, 0.49), WalkRefusal::TooFast);
}

TEST_CASE("a start the player could reach only by walking too fast is refused, even near it",
          "[0000-CA-08]") {
    constexpr WalkLimits kWideOrigin{.origin_tolerance_steps = 3, .early_tolerance_seconds = 0.5};

    const auto near = check(WalkRequest{{4, 2}, {}}, standing(), 5.0, open_grid(), kWideOrigin);
    const auto far = check(WalkRequest{{5, 2}, {}}, standing(), 5.0, open_grid(), kWideOrigin);

    REQUIRE(near.has_value());
    CHECK(*near == 5.5);
    require_refused(far, WalkRefusal::TooFast);
}

TEST_CASE("the origin is checked before the steps") {
    ServerGrid grid = open_grid();
    REQUIRE(grid.set_walkable({6, 2}, false));

    const auto checked = check(WalkRequest{{5, 2}, steps_of({{6, 2}})}, standing(), 5.0, grid);

    require_refused(checked, WalkRefusal::OriginTooFar);
}
