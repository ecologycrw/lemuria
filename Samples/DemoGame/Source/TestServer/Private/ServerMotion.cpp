#include "TestServer/ServerMotion.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

namespace test_server {

int steps_between(Tile first, Tile second) noexcept {
    return std::max(std::abs(first.x - second.x), std::abs(first.z - second.z));
}

double step_length(Tile from, Tile to) noexcept {
    return from.x != to.x && from.z != to.z ? std::numbers::sqrt2 : 1.0;
}

ServerMotion::ServerMotion(Tile start, double time, double tiles_per_second) noexcept
    : origin_(start), start_time_(time), tiles_per_second_(tiles_per_second) {}

void ServerMotion::start_walk(Tile origin, double time, const StepList& steps) noexcept {
    origin_ = origin;
    start_time_ = time;
    steps_ = steps;
}

Tile ServerMotion::tile_at(double time) const noexcept {
    Tile reached = origin_;
    double arrival = start_time_;
    for (const Tile step : steps_.view()) {
        arrival += step_length(reached, step) / tiles_per_second_;
        if (arrival > time) {
            break;
        }
        reached = step;
    }
    return reached;
}

Tile ServerMotion::destination() const noexcept {
    return steps_.empty() ? origin_ : steps_.view().back();
}

bool ServerMotion::is_walking_at(double time) const noexcept {
    return tile_at(time) != destination();
}

std::optional<double> ServerMotion::arrival_ahead(Tile tile, double time) const noexcept {
    Tile previous = origin_;
    double arrival = start_time_;
    std::optional<double> found = tile == origin_ ? std::optional<double>(arrival) : std::nullopt;
    for (const Tile step : steps_.view()) {
        arrival += step_length(previous, step) / tiles_per_second_;
        if (arrival <= time) {
            // The player already reached this step, so the tiles before it were left behind.
            found.reset();
        }
        if (!found && step == tile) {
            found = arrival;
        }
        previous = step;
    }
    return found;
}

} // namespace test_server
