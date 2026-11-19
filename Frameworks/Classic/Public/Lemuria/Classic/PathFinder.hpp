#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include <tl/expected.hpp>

#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/TileGrid.hpp"

namespace lemuria::classic {

// When a step may go diagonally. Each game follows the rule of its own server.
enum class DiagonalRule : std::uint8_t {
    Never,

    // Both tiles beside the step must be walkable, so a step never cuts a corner.
    WhenBothSidesAreFree,

    // Only the tile at the end of the step must be walkable.
    Always,
};

struct PathRules {
    DiagonalRule diagonals = DiagonalRule::WhenBothSidesAreFree;

    // The most tiles one search may go through, so a far click cannot hold up a frame (INV-14). The memory
    // kept for searches grows with this number.
    std::size_t max_searched_tiles = 0;
};

// Why there is no path. In every case the character stays where it is.
enum class PathFailure : std::uint8_t {
    OutsideGrid,
    DestinationBlocked,
    Unreachable,
    SearchLimitReached,
};

// Finds the shortest way between two tiles. The same search always gives the same path, on every compiler.
class PathFinder {
public:
    explicit PathFinder(const PathRules& rules) noexcept;

    // Reserves the memory for searches on this grid, so they do not allocate during the frame (INV-13). Call
    // it when a map loads.
    void prepare_for(const TileGrid& grid);

    // The steps from `start` to `destination`, without `start` and ending at `destination`. The list is valid
    // until the next search. The character may stand on a blocked tile, because the server can put it
    // anywhere.
    [[nodiscard]] tl::expected<std::span<const TileCoord>, PathFailure>
    find_path(const TileGrid& grid, TileCoord start, TileCoord destination);

private:
    struct OpenTile {
        int total_cost = 0;
        int remaining_cost = 0;
        TileCoord tile;
    };

    struct Visit {
        int cost = std::numeric_limits<int>::max();
        std::uint8_t arrived_by = 0;
    };

    [[nodiscard]] static bool is_searched_later(const OpenTile& first, const OpenTile& second) noexcept;

    [[nodiscard]] std::optional<PathFailure> search(const TileGrid& grid, TileCoord start,
                                                    TileCoord destination);
    void open_tile(int cost, TileCoord tile, TileCoord destination);
    [[nodiscard]] OpenTile take_next_tile() noexcept;
    void open_neighbors(const TileGrid& grid, const OpenTile& current, TileCoord destination);
    void rebuild_path(const TileGrid& grid, TileCoord start, TileCoord destination);

    PathRules rules_;
    std::vector<Visit> visits_;
    std::vector<OpenTile> open_tiles_;
    std::vector<TileCoord> path_;
};

} // namespace lemuria::classic
