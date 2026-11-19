#include "Lemuria/Classic/TileGrid.hpp"

#include <string>

namespace lemuria::classic {

namespace {

[[nodiscard]] bool is_valid_side(int side) noexcept {
    return side > 0 && side <= TileGrid::kMaxSide;
}

} // namespace

core::Result<TileGrid> TileGrid::create(int width, int depth) {
    if (!is_valid_side(width) || !is_valid_side(depth)) {
        return core::make_error("a tile grid needs sides from 1 to " + std::to_string(kMaxSide) +
                                " tiles, but got " + std::to_string(width) + " by " + std::to_string(depth));
    }
    return TileGrid(width, depth);
}

TileGrid::TileGrid(int width, int depth)
    : width_(width), depth_(depth),
      walkable_(static_cast<std::size_t>(width) * static_cast<std::size_t>(depth), std::uint8_t{1}) {}

bool TileGrid::set_walkable(TileCoord tile, bool walkable) noexcept {
    if (!contains(tile)) {
        return false;
    }
    walkable_[index_of(tile)] = static_cast<std::uint8_t>(walkable);
    return true;
}

} // namespace lemuria::classic
