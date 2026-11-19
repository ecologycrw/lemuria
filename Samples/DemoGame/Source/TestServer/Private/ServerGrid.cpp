#include "TestServer/ServerGrid.hpp"

#include <cstddef>
#include <string>

namespace test_server {

namespace {

constexpr int kMaxSide = demo_protocol::kMaxTileCoordinate + 1;

} // namespace

lemuria::core::Result<ServerGrid> ServerGrid::create(int width, int depth) {
    if (width < 1 || depth < 1 || width > kMaxSide || depth > kMaxSide) {
        return lemuria::core::make_error("a server grid needs sides from 1 to " + std::to_string(kMaxSide) +
                                         " tiles");
    }
    return ServerGrid(width, depth);
}

ServerGrid ServerGrid::from_demo_map(const demo_protocol::DemoMap& map) {
    ServerGrid grid(demo_protocol::DemoMap::kSize, demo_protocol::DemoMap::kSize);
    for (int z = 0; z < grid.depth_; ++z) {
        for (int x = 0; x < grid.width_; ++x) {
            const Tile tile{static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(z)};
            grid.walkable_[grid.index_of(tile)] =
                map.is_walkable_on_server(x, z) ? std::uint8_t{1} : std::uint8_t{0};
        }
    }
    return grid;
}

ServerGrid::ServerGrid(int width, int depth)
    : width_(width), depth_(depth),
      walkable_(static_cast<std::size_t>(width) * static_cast<std::size_t>(depth), std::uint8_t{1}) {}

bool ServerGrid::is_walkable(Tile tile) const noexcept {
    return contains(tile) && walkable_[index_of(tile)] != 0;
}

bool ServerGrid::set_walkable(Tile tile, bool walkable) noexcept {
    if (!contains(tile)) {
        return false;
    }
    walkable_[index_of(tile)] = walkable ? std::uint8_t{1} : std::uint8_t{0};
    return true;
}

std::size_t ServerGrid::index_of(Tile tile) const noexcept {
    return static_cast<std::size_t>(tile.z) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(tile.x);
}

} // namespace test_server
