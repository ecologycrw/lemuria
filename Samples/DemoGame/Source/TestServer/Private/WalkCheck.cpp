#include "TestServer/WalkCheck.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <optional>

namespace test_server {

namespace {

// The shortest walk between two tiles on an open map: diagonal steps first, then straight ones.
[[nodiscard]] double open_distance(Tile from, Tile to) noexcept {
    const int x_distance = std::abs(from.x - to.x);
    const int z_distance = std::abs(from.z - to.z);
    const int diagonal_steps = std::min(x_distance, z_distance);
    const int straight_steps = std::max(x_distance, z_distance) - diagonal_steps;
    return (diagonal_steps * std::numbers::sqrt2) + straight_steps;
}

// The earliest time the player can be at the start of the walk: along its route when the start is still ahead
// on it, or straight from where it is when it is not. A tile already left behind is measured like any other
// tile off the route, so a player can never go back along its walk for free.
[[nodiscard]] tl::expected<double, WalkRefusal>
earliest_start(Tile origin, const ServerMotion& motion, double now, const WalkLimits& limits) noexcept {
    if (const std::optional<double> arrival = motion.arrival_ahead(origin, now)) {
        return *arrival;
    }
    const Tile current = motion.tile_at(now);
    if (steps_between(origin, current) > limits.origin_tolerance_steps) {
        return tl::unexpected(WalkRefusal::OriginTooFar);
    }
    return now + (open_distance(current, origin) / motion.tiles_per_second());
}

[[nodiscard]] bool cuts_corner(Tile from, Tile to, const ServerGrid& grid) noexcept {
    if (from.x == to.x || from.z == to.z) {
        return false;
    }
    return !grid.is_walkable(Tile{to.x, from.z}) || !grid.is_walkable(Tile{from.x, to.z});
}

[[nodiscard]] std::optional<WalkRefusal> check_steps(const demo_protocol::WalkRequest& request,
                                                     const ServerGrid& grid) noexcept {
    Tile previous = request.origin;
    for (const Tile step : request.steps.view()) {
        if (steps_between(previous, step) != 1) {
            return WalkRefusal::NotNeighbor;
        }
        if (!grid.is_walkable(step)) {
            return WalkRefusal::BlockedStep;
        }
        if (cuts_corner(previous, step, grid)) {
            return WalkRefusal::CutsCorner;
        }
        previous = step;
    }
    return std::nullopt;
}

} // namespace

tl::expected<double, WalkRefusal> check_walk(const demo_protocol::WalkRequest& request,
                                             const ServerMotion& motion, double now, const ServerGrid& grid,
                                             const WalkLimits& limits) noexcept {
    const auto earliest = earliest_start(request.origin, motion, now, limits);
    if (!earliest) {
        return earliest;
    }
    if (*earliest > now + limits.early_tolerance_seconds) {
        return tl::unexpected(WalkRefusal::TooFast);
    }
    if (const auto refusal = check_steps(request, grid)) {
        return tl::unexpected(*refusal);
    }
    return std::max(now, *earliest);
}

} // namespace test_server
