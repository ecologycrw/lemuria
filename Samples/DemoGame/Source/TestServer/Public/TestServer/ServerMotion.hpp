#pragma once

#include <optional>

#include "DemoProtocol/Messages.hpp"

namespace test_server {

using demo_protocol::StepList;
using demo_protocol::Tile;

// The greatest distance on either axis, which is how many steps separate two tiles when steps may go
// diagonally.
[[nodiscard]] int steps_between(Tile first, Tile second) noexcept;

// How long a step is, in tiles: 1 for a straight step and the square root of two for a diagonal one, as on
// the client.
[[nodiscard]] double step_length(Tile from, Tile to) noexcept;

// Where a player walks, as the server sees it: the steps of the last accepted walk, one after the other, at a
// fixed speed. The time is always given by the caller (INV-05).
class ServerMotion {
public:
    ServerMotion(Tile start, double time, double tiles_per_second) noexcept;

    // Starts a new walk at this time. Without steps, the player stands at `origin`.
    void start_walk(Tile origin, double time, const StepList& steps) noexcept;

    // The last tile reached at this time.
    [[nodiscard]] Tile tile_at(double time) const noexcept;

    [[nodiscard]] Tile destination() const noexcept;

    [[nodiscard]] bool is_walking_at(double time) const noexcept;

    // When the player reaches a tile of the current walk, or nothing when the tile is not on it.
    [[nodiscard]] std::optional<double> arrival_on_route(Tile tile) const noexcept;

    [[nodiscard]] double tiles_per_second() const noexcept { return tiles_per_second_; }

private:
    Tile origin_;
    double start_time_ = 0.0;
    StepList steps_;
    double tiles_per_second_ = 0.0;
};

} // namespace test_server
