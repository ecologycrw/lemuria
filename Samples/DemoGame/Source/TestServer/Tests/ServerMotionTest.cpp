#include <initializer_list>
#include <numbers>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "TestServer/ServerMotion.hpp"

using Catch::Approx;
using test_server::ServerMotion;
using test_server::step_length;
using test_server::StepList;
using test_server::steps_between;
using test_server::Tile;

namespace {

constexpr double kSpeed = 4.0;
constexpr double kStart = 10.0;

[[nodiscard]] StepList steps_of(std::initializer_list<Tile> tiles) {
    StepList steps;
    for (const Tile tile : tiles) {
        REQUIRE(steps.add(tile));
    }
    return steps;
}

[[nodiscard]] ServerMotion walking(std::initializer_list<Tile> tiles) {
    ServerMotion motion({2, 2}, 0.0, kSpeed);
    motion.start_walk({2, 2}, kStart, steps_of(tiles));
    return motion;
}

} // namespace

TEST_CASE("steps between tiles count diagonal steps as one") {
    CHECK(steps_between({2, 2}, {2, 2}) == 0);
    CHECK(steps_between({2, 2}, {3, 3}) == 1);
    CHECK(steps_between({2, 2}, {5, 3}) == 3);
    CHECK(steps_between({5, 3}, {2, 2}) == 3);
    CHECK(steps_between({2, 7}, {3, 2}) == 5);
}

TEST_CASE("a diagonal step is longer than a straight one, as on the client") {
    CHECK(step_length({2, 2}, {3, 2}) == 1.0);
    CHECK(step_length({2, 2}, {2, 1}) == 1.0);
    CHECK(step_length({2, 2}, {3, 3}) == std::numbers::sqrt2);
    CHECK(step_length({2, 2}, {1, 3}) == std::numbers::sqrt2);
}

TEST_CASE("a player reaches each tile of its walk at the walking speed") {
    const ServerMotion motion = walking({{3, 2}, {4, 2}});

    CHECK(motion.tile_at(kStart - 1.0) == Tile{2, 2});
    CHECK(motion.tile_at(kStart) == Tile{2, 2});
    CHECK(motion.tile_at(kStart + 0.24) == Tile{2, 2});
    CHECK(motion.tile_at(kStart + 0.25) == Tile{3, 2});
    CHECK(motion.tile_at(kStart + 0.5) == Tile{4, 2});
    CHECK(motion.tile_at(kStart + 100.0) == Tile{4, 2});
}

TEST_CASE("a diagonal step takes longer than a straight one") {
    const ServerMotion motion = walking({{3, 3}});

    CHECK(motion.tile_at(kStart + 0.3) == Tile{2, 2});
    CHECK(motion.tile_at(kStart + 0.36) == Tile{3, 3});
}

TEST_CASE("a player is walking until it reaches the end of its walk") {
    const ServerMotion motion = walking({{3, 2}, {4, 2}});

    CHECK(motion.destination() == Tile{4, 2});
    CHECK(motion.is_walking_at(kStart + 0.3));
    CHECK_FALSE(motion.is_walking_at(kStart + 0.5));
}

TEST_CASE("the server knows when a player reaches each tile of its route") {
    const ServerMotion motion = walking({{3, 2}, {4, 3}});

    CHECK(motion.arrival_on_route({2, 2}) == Approx(kStart));
    CHECK(motion.arrival_on_route({3, 2}) == Approx(kStart + 0.25));
    CHECK(motion.arrival_on_route({4, 3}) == Approx(kStart + 0.25 + (std::numbers::sqrt2 / kSpeed)));
    CHECK_FALSE(motion.arrival_on_route({9, 9}).has_value());
}

TEST_CASE("a walk without steps leaves the player standing") {
    ServerMotion motion = walking({{3, 2}, {4, 2}});

    motion.start_walk({3, 2}, kStart + 0.3, {});

    CHECK(motion.tile_at(kStart + 5.0) == Tile{3, 2});
    CHECK(motion.destination() == Tile{3, 2});
    CHECK_FALSE(motion.is_walking_at(kStart + 0.3));
    CHECK_FALSE(motion.arrival_on_route({4, 2}).has_value());
}

TEST_CASE("a new walk replaces the previous one") {
    ServerMotion motion = walking({{3, 2}, {4, 2}});

    motion.start_walk({3, 2}, kStart + 1.0, steps_of({{3, 3}}));

    CHECK(motion.tile_at(kStart + 1.0) == Tile{3, 2});
    CHECK(motion.tile_at(kStart + 1.25) == Tile{3, 3});
    CHECK_FALSE(motion.arrival_on_route({4, 2}).has_value());
}
