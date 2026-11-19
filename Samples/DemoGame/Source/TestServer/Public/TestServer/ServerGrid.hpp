#pragma once

#include <cstdint>
#include <vector>

#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Messages.hpp"
#include "Lemuria/Core/Result.hpp"

namespace test_server {

using demo_protocol::Tile;

// Which tiles the server lets players walk on. It is the server's own view of the map, which may forbid more
// than the client believes.
class ServerGrid {
public:
    // A grid with every tile walkable. Each side goes from 1 up to the tiles a message can carry.
    [[nodiscard]] static lemuria::core::Result<ServerGrid> create(int width, int depth);

    [[nodiscard]] static ServerGrid from_demo_map(const demo_protocol::DemoMap& map);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int depth() const noexcept { return depth_; }

    [[nodiscard]] bool contains(Tile tile) const noexcept { return tile.x < width_ && tile.z < depth_; }

    // Anything outside the map counts as blocked.
    [[nodiscard]] bool is_walkable(Tile tile) const noexcept;

    // False, with nothing changed, when the tile is outside the map.
    [[nodiscard]] bool set_walkable(Tile tile, bool walkable) noexcept;

private:
    ServerGrid(int width, int depth);

    [[nodiscard]] std::size_t index_of(Tile tile) const noexcept;

    int width_ = 0;
    int depth_ = 0;
    std::vector<std::uint8_t> walkable_;
};

} // namespace test_server
