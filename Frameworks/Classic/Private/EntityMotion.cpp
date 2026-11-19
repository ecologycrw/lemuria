#include "Lemuria/Classic/EntityMotion.hpp"

#include <cmath>

namespace lemuria::classic {

namespace {

[[nodiscard]] double center_of(int tile_side) noexcept {
    return static_cast<double>(tile_side) + 0.5;
}

} // namespace

EntityMotion::EntityMotion(TileCoord tile) noexcept
    : tile_(tile), x_(center_of(tile.x)), z_(center_of(tile.z)) {}

core::Vec3 EntityMotion::position() const noexcept {
    return {static_cast<float>(x_), 0.0F, static_cast<float>(z_)};
}

void EntityMotion::follow(std::span<const TileCoord> steps) noexcept {
    steps_ = WalkSteps{};
    next_step_ = 0;
    for (const TileCoord step : steps) {
        if (!steps_.add(step)) {
            break;
        }
    }
    if (!heading_) {
        take_next_step();
    }
}

void EntityMotion::place_at(TileCoord tile) noexcept {
    tile_ = tile;
    heading_.reset();
    steps_ = WalkSteps{};
    next_step_ = 0;
}

double EntityMotion::advance(double seconds, const MotionSpeeds& speeds) noexcept {
    double time_left = seconds;
    while (time_left > 0.0) {
        const bool is_stepping = heading_.has_value();
        const TileCoord target = heading_.value_or(tile_);
        const double x_gap = center_of(target.x) - x_;
        const double z_gap = center_of(target.z) - z_;
        const double speed = is_stepping ? speeds.walk_tiles_per_second : speeds.slide_tiles_per_second;
        const double time_needed = std::hypot(x_gap, z_gap) / speed;

        if (time_needed > time_left) {
            const double part = time_left / time_needed;
            x_ += x_gap * part;
            z_ += z_gap * part;
            return 0.0;
        }

        x_ = center_of(target.x);
        z_ = center_of(target.z);
        time_left -= time_needed;
        if (!is_stepping) {
            return time_left;
        }
        tile_ = target;
        take_next_step();
        if (!heading_) {
            return time_left;
        }
    }
    return 0.0;
}

void EntityMotion::take_next_step() noexcept {
    if (next_step_ < steps_.size()) {
        heading_ = steps_.view()[next_step_];
        ++next_step_;
    } else {
        heading_.reset();
    }
}

} // namespace lemuria::classic
