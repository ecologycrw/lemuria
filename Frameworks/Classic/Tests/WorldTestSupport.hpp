#pragma once

#include <cstddef>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Classic/World.hpp"

namespace lemuria::classic::testing {

inline constexpr double kWalkSpeed = 4.0;
inline constexpr double kSlideSpeed = 8.0;
inline constexpr std::size_t kStepsPerRequest = 8;

inline constexpr MovementRules kMovementRules{
    .paths = {DiagonalRule::WhenBothSidesAreFree, 4096},
    .speeds = {kWalkSpeed, kSlideSpeed},
    .walk_requests = WalkRequestMode::Steps,
    .max_steps_per_request = kStepsPerRequest,
};

[[nodiscard]] inline World make_world(std::size_t max_other_entities,
                                      const MovementRules& rules = kMovementRules) {
    auto world = World::create(max_other_entities, rules);
    REQUIRE(world.has_value());
    return *std::move(world);
}

[[nodiscard]] inline TileGrid make_grid(int side) {
    auto grid = TileGrid::create(side, side);
    REQUIRE(grid.has_value());
    return *std::move(grid);
}

} // namespace lemuria::classic::testing
