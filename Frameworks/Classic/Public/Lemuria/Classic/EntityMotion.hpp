#pragma once

#include <cstddef>
#include <optional>
#include <span>

#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/WalkSteps.hpp"
#include "Lemuria/Core/Math.hpp"

namespace lemuria::classic {

// How fast things move on the ground. Each game chooses its own values.
struct MotionSpeeds {
    double walk_tiles_per_second = 0.0;

    // How fast something glides to the place the server decided.
    double slide_tiles_per_second = 0.0;
};

// How something moves over the tiles. The distance depends only on the time that passed, so the result is the
// same at any frame rate.
class EntityMotion {
public:
    explicit EntityMotion(TileCoord tile) noexcept;

    // The last tile it reached, or the place the server decided.
    [[nodiscard]] TileCoord tile() const noexcept { return tile_; }

    // Where it is seen on the ground.
    [[nodiscard]] core::Vec3 position() const noexcept;

    [[nodiscard]] bool is_walking() const noexcept { return heading_.has_value(); }

    // Where a new route starts: the end of the step being walked, or the current tile.
    [[nodiscard]] TileCoord route_start() const noexcept { return heading_.value_or(tile_); }

    // Replaces the route. The step being walked always finishes first. Steps above `WalkSteps::kCapacity` are
    // left out.
    void follow(std::span<const TileCoord> steps) noexcept;

    // The server decided where it is: the route is cancelled and it glides there, without jumping.
    void place_at(TileCoord tile) noexcept;

    // Moves for the given time and returns the time left over after it stopped.
    [[nodiscard]] double advance(double seconds, const MotionSpeeds& speeds) noexcept;

private:
    void take_next_step() noexcept;

    TileCoord tile_;
    double x_;
    double z_;
    std::optional<TileCoord> heading_;
    WalkSteps steps_;
    std::size_t next_step_ = 0;
};

} // namespace lemuria::classic
