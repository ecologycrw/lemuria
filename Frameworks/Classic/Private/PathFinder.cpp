#include "Lemuria/Classic/PathFinder.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <tuple>

namespace lemuria::classic {

namespace {

// Whole numbers keep the result the same on every compiler. A diagonal step is about 1.4 times a straight
// one.
constexpr int kStraightCost = 10;
constexpr int kDiagonalCost = 14;

struct StepDirection {
    int x = 0;
    int z = 0;
};

constexpr std::array<StepDirection, 8> kStepDirections{{
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1},
    {1, 1},
    {1, -1},
    {-1, 1},
    {-1, -1},
}};

[[nodiscard]] bool is_diagonal(StepDirection direction) noexcept {
    return direction.x != 0 && direction.z != 0;
}

[[nodiscard]] TileCoord after_step(TileCoord tile, StepDirection direction) noexcept {
    return {tile.x + direction.x, tile.z + direction.z};
}

[[nodiscard]] TileCoord before_step(TileCoord tile, StepDirection direction) noexcept {
    return {tile.x - direction.x, tile.z - direction.z};
}

[[nodiscard]] bool can_step(const TileGrid& grid, TileCoord from, StepDirection direction,
                            DiagonalRule rule) noexcept {
    if (!grid.is_walkable(after_step(from, direction))) {
        return false;
    }
    if (!is_diagonal(direction) || rule == DiagonalRule::Always) {
        return true;
    }
    return rule == DiagonalRule::WhenBothSidesAreFree && grid.is_walkable({from.x + direction.x, from.z}) &&
           grid.is_walkable({from.x, from.z + direction.z});
}

// The cost of the way between two tiles with nothing in the way. It never exceeds the real cost, which is
// what makes the path found the shortest one.
[[nodiscard]] int free_way_cost(TileCoord from, TileCoord to, DiagonalRule rule) noexcept {
    const int x_distance = std::abs(from.x - to.x);
    const int z_distance = std::abs(from.z - to.z);
    if (rule == DiagonalRule::Never) {
        return (x_distance + z_distance) * kStraightCost;
    }
    const auto [shorter, longer] = std::minmax(x_distance, z_distance);
    return shorter * kDiagonalCost + (longer - shorter) * kStraightCost;
}

} // namespace

PathFinder::PathFinder(const PathRules& rules) noexcept : rules_(rules) {}

void PathFinder::prepare_for(const TileGrid& grid) {
    // Each searched tile opens at most one tile per direction, and the first tile is opened by the search
    // itself.
    const std::size_t searched_tiles = std::min(rules_.max_searched_tiles, grid.tile_count());
    visits_.resize(grid.tile_count());
    open_tiles_.reserve(searched_tiles * kStepDirections.size() + 1);
    path_.reserve(searched_tiles + 1);
}

tl::expected<std::span<const TileCoord>, PathFailure>
PathFinder::find_path(const TileGrid& grid, TileCoord start, TileCoord destination) {
    path_.clear();
    if (!grid.contains(start) || !grid.contains(destination)) {
        return tl::unexpected(PathFailure::OutsideGrid);
    }
    if (!grid.is_walkable(destination)) {
        return tl::unexpected(PathFailure::DestinationBlocked);
    }
    if (start == destination) {
        return std::span<const TileCoord>(path_);
    }

    prepare_for(grid);
    if (const auto failure = search(grid, start, destination)) {
        return tl::unexpected(*failure);
    }
    rebuild_path(grid, start, destination);
    return std::span<const TileCoord>(path_);
}

// Every field takes part in the order, so the result does not depend on how the compiler's library sorts
// ties.
bool PathFinder::is_searched_later(const OpenTile& first, const OpenTile& second) noexcept {
    return std::tie(first.total_cost, first.remaining_cost, first.tile.z, first.tile.x) >
           std::tie(second.total_cost, second.remaining_cost, second.tile.z, second.tile.x);
}

std::optional<PathFailure> PathFinder::search(const TileGrid& grid, TileCoord start, TileCoord destination) {
    std::fill(visits_.begin(), visits_.end(), Visit{});
    open_tiles_.clear();
    visits_[grid.index_of(start)].cost = 0;
    open_tile(0, start, destination);

    std::size_t searched_tiles = 0;
    while (!open_tiles_.empty()) {
        const OpenTile current = take_next_tile();
        const bool found_cheaper_way =
            current.total_cost - current.remaining_cost > visits_[grid.index_of(current.tile)].cost;
        if (found_cheaper_way) {
            continue;
        }
        if (current.tile == destination) {
            return std::nullopt;
        }
        if (searched_tiles == rules_.max_searched_tiles) {
            return PathFailure::SearchLimitReached;
        }
        ++searched_tiles;
        open_neighbors(grid, current, destination);
    }
    return PathFailure::Unreachable;
}

void PathFinder::open_tile(int cost, TileCoord tile, TileCoord destination) {
    const int remaining_cost = free_way_cost(tile, destination, rules_.diagonals);
    open_tiles_.push_back({cost + remaining_cost, remaining_cost, tile});
    std::push_heap(open_tiles_.begin(), open_tiles_.end(), is_searched_later);
}

PathFinder::OpenTile PathFinder::take_next_tile() noexcept {
    std::pop_heap(open_tiles_.begin(), open_tiles_.end(), is_searched_later);
    const OpenTile next = open_tiles_.back();
    open_tiles_.pop_back();
    return next;
}

void PathFinder::open_neighbors(const TileGrid& grid, const OpenTile& current, TileCoord destination) {
    const int cost_so_far = current.total_cost - current.remaining_cost;
    for (std::size_t index = 0; index < kStepDirections.size(); ++index) {
        const StepDirection direction = kStepDirections[index];
        if (!can_step(grid, current.tile, direction, rules_.diagonals)) {
            continue;
        }
        const TileCoord neighbor = after_step(current.tile, direction);
        const int cost = cost_so_far + (is_diagonal(direction) ? kDiagonalCost : kStraightCost);
        Visit& visit = visits_[grid.index_of(neighbor)];
        if (cost < visit.cost) {
            visit = {cost, static_cast<std::uint8_t>(index)};
            open_tile(cost, neighbor, destination);
        }
    }
}

void PathFinder::rebuild_path(const TileGrid& grid, TileCoord start, TileCoord destination) {
    for (TileCoord tile = destination; tile != start;
         tile = before_step(tile, kStepDirections[visits_[grid.index_of(tile)].arrived_by])) {
        path_.push_back(tile);
    }
    std::reverse(path_.begin(), path_.end());
}

} // namespace lemuria::classic
