#pragma once

#include "Lemuria/Core/Math.hpp"

namespace lemuria::classic {

// A tile of the map. Tile (x, z) covers the ground from x to x + 1 and from z to z + 1.
struct TileCoord {
    int x = 0;
    int z = 0;

    friend bool operator==(const TileCoord&, const TileCoord&) = default;
};

// The middle of a tile, on the ground.
[[nodiscard]] inline core::Vec3 tile_center(TileCoord tile) noexcept {
    return {static_cast<float>(tile.x) + 0.5F, 0.0F, static_cast<float>(tile.z) + 0.5F};
}

} // namespace lemuria::classic
