#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Core/Result.hpp"

namespace lemuria::classic {

// Which tiles of a map can be walked on, as far as the client knows. The server still has the last word
// (INV-01), and the grid does not know where its values came from.
class TileGrid {
public:
    static constexpr int kMaxSide = 1024;

    // A grid with every tile walkable.
    [[nodiscard]] static core::Result<TileGrid> create(int width, int depth);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int depth() const noexcept { return depth_; }
    [[nodiscard]] std::size_t tile_count() const noexcept { return walkable_.size(); }

    [[nodiscard]] bool contains(TileCoord tile) const noexcept {
        return tile.x >= 0 && tile.x < width_ && tile.z >= 0 && tile.z < depth_;
    }

    // Where a tile of this grid is kept, from 0 up to, but not including, `tile_count`.
    [[nodiscard]] std::size_t index_of(TileCoord tile) const noexcept {
        return static_cast<std::size_t>(tile.z) * static_cast<std::size_t>(width_) +
               static_cast<std::size_t>(tile.x);
    }

    // Anything outside the map counts as blocked.
    [[nodiscard]] bool is_walkable(TileCoord tile) const noexcept {
        return contains(tile) && walkable_[index_of(tile)] != 0;
    }

    // False, with nothing changed, when the tile is outside the map.
    [[nodiscard]] bool set_walkable(TileCoord tile, bool walkable) noexcept;

private:
    TileGrid(int width, int depth);

    int width_ = 0;
    int depth_ = 0;
    std::vector<std::uint8_t> walkable_;
};

} // namespace lemuria::classic
