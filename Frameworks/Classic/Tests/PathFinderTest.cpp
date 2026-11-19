#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lemuria/Classic/PathFinder.hpp"
#include "Lemuria/Classic/TileGrid.hpp"
#include "Lemuria/Core/SeededRandom.hpp"

using lemuria::classic::DiagonalRule;
using lemuria::classic::PathFailure;
using lemuria::classic::PathFinder;
using lemuria::classic::PathRules;
using lemuria::classic::TileCoord;
using lemuria::classic::TileGrid;
using lemuria::core::SeededRandom;
using Steps = std::vector<TileCoord>;

namespace {

constexpr std::size_t kNoSearchLimit = 100'000;
constexpr int kStraightCost = 10;
constexpr int kDiagonalCost = 14;
constexpr int kNoRoute = std::numeric_limits<int>::max();

[[nodiscard]] PathFinder make_finder(DiagonalRule diagonals = DiagonalRule::WhenBothSidesAreFree,
                                     std::size_t max_searched_tiles = kNoSearchLimit) {
    return PathFinder(PathRules{diagonals, max_searched_tiles});
}

// Builds a grid from a drawing: each text is a row of tiles, and '#' marks a blocked tile.
[[nodiscard]] TileGrid draw_grid(std::initializer_list<std::string_view> rows) {
    auto grid = TileGrid::create(static_cast<int>(rows.begin()->size()), static_cast<int>(rows.size()));
    REQUIRE(grid.has_value());
    int z = 0;
    for (const std::string_view row : rows) {
        for (std::size_t x = 0; x < row.size(); ++x) {
            REQUIRE(grid->set_walkable({static_cast<int>(x), z}, row[x] != '#'));
        }
        ++z;
    }
    return *grid;
}

[[nodiscard]] Steps steps_between(PathFinder& finder, const TileGrid& grid, TileCoord start,
                                  TileCoord destination) {
    const auto path = finder.find_path(grid, start, destination);
    REQUIRE(path.has_value());
    return {path->begin(), path->end()};
}

[[nodiscard]] PathFailure failure_between(PathFinder& finder, const TileGrid& grid, TileCoord start,
                                          TileCoord destination) {
    const auto path = finder.find_path(grid, start, destination);
    REQUIRE_FALSE(path.has_value());
    return path.error();
}

// What one step costs under the rule, written apart from the path finder so the tests can judge its answers.
// Nothing when the step is not allowed.
[[nodiscard]] std::optional<int> step_cost(const TileGrid& grid, TileCoord from, TileCoord to,
                                           DiagonalRule rule) {
    const int x_change = to.x - from.x;
    const int z_change = to.z - from.z;
    const bool is_neighbor =
        std::abs(x_change) <= 1 && std::abs(z_change) <= 1 && (x_change != 0 || z_change != 0);
    if (!is_neighbor || !grid.is_walkable(to)) {
        return std::nullopt;
    }
    if (x_change == 0 || z_change == 0) {
        return kStraightCost;
    }
    const bool both_sides_free = grid.is_walkable({to.x, from.z}) && grid.is_walkable({from.x, to.z});
    const bool allowed =
        rule == DiagonalRule::Always || (rule == DiagonalRule::WhenBothSidesAreFree && both_sides_free);
    return allowed ? std::optional<int>(kDiagonalCost) : std::nullopt;
}

// The cost of walking the steps. Nothing when one of the steps is not allowed.
[[nodiscard]] std::optional<int> walk_cost(const TileGrid& grid, TileCoord start, const Steps& steps,
                                           DiagonalRule rule) {
    int total = 0;
    TileCoord from = start;
    for (const TileCoord to : steps) {
        const auto cost = step_cost(grid, from, to, rule);
        if (!cost) {
            return std::nullopt;
        }
        total += *cost;
        from = to;
    }
    return total;
}

[[nodiscard]] std::size_t index_of(const TileGrid& grid, TileCoord tile) {
    return static_cast<std::size_t>(tile.z) * static_cast<std::size_t>(grid.width()) +
           static_cast<std::size_t>(tile.x);
}

// Lowers the cost of the tile by arriving from its neighbors. True when something improved.
[[nodiscard]] bool improve_tile(const TileGrid& grid, TileCoord to, DiagonalRule rule,
                                std::vector<int>& costs) {
    bool improved = false;
    for (int z = to.z - 1; z <= to.z + 1; ++z) {
        for (int x = to.x - 1; x <= to.x + 1; ++x) {
            const TileCoord from{x, z};
            const auto cost = step_cost(grid, from, to, rule);
            if (!grid.contains(from) || !cost || costs[index_of(grid, from)] == kNoRoute) {
                continue;
            }
            const int through_neighbor = costs[index_of(grid, from)] + *cost;
            if (through_neighbor < costs[index_of(grid, to)]) {
                costs[index_of(grid, to)] = through_neighbor;
                improved = true;
            }
        }
    }
    return improved;
}

// The cheapest cost from `start` to every tile, found by the slow and simple way: lowering the costs until
// nothing changes.
[[nodiscard]] std::vector<int> cheapest_costs(const TileGrid& grid, TileCoord start, DiagonalRule rule) {
    std::vector<int> costs(grid.tile_count(), kNoRoute);
    costs[index_of(grid, start)] = 0;
    bool improved = true;
    while (improved) {
        improved = false;
        for (int z = 0; z < grid.depth(); ++z) {
            for (int x = 0; x < grid.width(); ++x) {
                improved = improve_tile(grid, {x, z}, rule, costs) || improved;
            }
        }
    }
    return costs;
}

[[nodiscard]] TileGrid random_grid(SeededRandom& random, int side, std::uint32_t blocked_percent) {
    auto grid = TileGrid::create(side, side);
    REQUIRE(grid.has_value());
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            REQUIRE(grid->set_walkable({x, z}, random.next_below(100) >= blocked_percent));
        }
    }
    return *grid;
}

// Compares the answer of the path finder for one destination with the cheapest cost known for it.
void check_answer(PathFinder& finder, const TileGrid& grid, TileCoord start, TileCoord destination,
                  int cheapest, DiagonalRule rule) {
    const auto path = finder.find_path(grid, start, destination);
    if (!grid.is_walkable(destination)) {
        REQUIRE_FALSE(path.has_value());
        CHECK(path.error() == PathFailure::DestinationBlocked);
    } else if (cheapest == kNoRoute) {
        REQUIRE_FALSE(path.has_value());
        CHECK(path.error() == PathFailure::Unreachable);
    } else {
        REQUIRE(path.has_value());
        const Steps steps(path->begin(), path->end());
        CHECK(walk_cost(grid, start, steps, rule) == cheapest);
        CHECK((steps.empty() ? start : steps.back()) == destination);
    }
}

} // namespace

TEST_CASE("path finder lists every step up to the destination", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({"....."});
    PathFinder finder = make_finder();

    CHECK(steps_between(finder, grid, {0, 0}, {3, 0}) == Steps{{1, 0}, {2, 0}, {3, 0}});
    CHECK(steps_between(finder, grid, {4, 0}, {2, 0}) == Steps{{3, 0}, {2, 0}});
}

TEST_CASE("path finder has no steps when the character is already there", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({"..."});
    PathFinder finder = make_finder(DiagonalRule::WhenBothSidesAreFree, 0);

    CHECK(steps_between(finder, grid, {1, 0}, {1, 0}).empty());
}

TEST_CASE("path finder goes around blocked tiles", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        ".....",
        ".###.",
        ".....",
    });
    PathFinder finder = make_finder();

    const Steps steps = steps_between(finder, grid, {2, 0}, {2, 2});

    CHECK(steps.size() == 6);
    CHECK(steps.back() == TileCoord{2, 2});
    CHECK(walk_cost(grid, {2, 0}, steps, DiagonalRule::WhenBothSidesAreFree) == 6 * kStraightCost);
}

TEST_CASE("path finder always gives the same answer when several paths cost the same", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        "....",
        "....",
    });
    PathFinder finder = make_finder();

    CHECK(steps_between(finder, grid, {0, 0}, {3, 1}) == Steps{{1, 1}, {2, 1}, {3, 1}});
}

TEST_CASE("path finder does not move to a blocked tile", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        "...",
        ".#.",
        "...",
    });
    PathFinder finder = make_finder();

    CHECK(failure_between(finder, grid, {0, 0}, {1, 1}) == PathFailure::DestinationBlocked);
}

TEST_CASE("path finder does not move to a tile that cannot be reached", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        "..#.",
        "..#.",
        "..#.",
    });
    PathFinder finder = make_finder();

    CHECK(failure_between(finder, grid, {0, 0}, {3, 1}) == PathFailure::Unreachable);
}

TEST_CASE("path finder refuses tiles outside the map", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        "...",
        "...",
    });
    PathFinder finder = make_finder();
    const TileCoord outside = GENERATE(TileCoord{-1, 0}, TileCoord{0, -1}, TileCoord{3, 0}, TileCoord{0, 2},
                                       TileCoord{std::numeric_limits<int>::min(), 0},
                                       TileCoord{0, std::numeric_limits<int>::max()});

    CHECK(failure_between(finder, grid, {0, 0}, outside) == PathFailure::OutsideGrid);
    CHECK(failure_between(finder, grid, outside, {0, 0}) == PathFailure::OutsideGrid);
}

TEST_CASE("path finder lets a character leave a tile that became blocked", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({"#.."});
    PathFinder finder = make_finder();

    CHECK(steps_between(finder, grid, {0, 0}, {2, 0}) == Steps{{1, 0}, {2, 0}});
}

TEST_CASE("path finder follows the diagonal rule between two blocked corners", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        ".#",
        "#.",
    });

    PathFinder always = make_finder(DiagonalRule::Always);
    CHECK(steps_between(always, grid, {0, 0}, {1, 1}) == Steps{{1, 1}});

    PathFinder both_sides = make_finder(DiagonalRule::WhenBothSidesAreFree);
    CHECK(failure_between(both_sides, grid, {0, 0}, {1, 1}) == PathFailure::Unreachable);

    PathFinder never = make_finder(DiagonalRule::Never);
    CHECK(failure_between(never, grid, {0, 0}, {1, 1}) == PathFailure::Unreachable);
}

TEST_CASE("path finder follows the diagonal rule beside one blocked corner", "[0000-CA-03]") {
    const TileGrid blocked_along_x = draw_grid({
        ".#",
        "..",
    });
    const TileGrid blocked_along_z = draw_grid({
        "..",
        "#.",
    });

    PathFinder always = make_finder(DiagonalRule::Always);
    CHECK(steps_between(always, blocked_along_x, {0, 0}, {1, 1}) == Steps{{1, 1}});
    CHECK(steps_between(always, blocked_along_z, {0, 0}, {1, 1}) == Steps{{1, 1}});

    PathFinder both_sides = make_finder(DiagonalRule::WhenBothSidesAreFree);
    CHECK(steps_between(both_sides, blocked_along_x, {0, 0}, {1, 1}) == Steps{{0, 1}, {1, 1}});
    CHECK(steps_between(both_sides, blocked_along_z, {0, 0}, {1, 1}) == Steps{{1, 0}, {1, 1}});
}

TEST_CASE("path finder takes the diagonal only when the rule allows it", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        "...",
        "...",
        "...",
    });

    PathFinder both_sides = make_finder(DiagonalRule::WhenBothSidesAreFree);
    CHECK(steps_between(both_sides, grid, {0, 0}, {2, 2}) == Steps{{1, 1}, {2, 2}});

    PathFinder never = make_finder(DiagonalRule::Never);
    const Steps steps = steps_between(never, grid, {0, 0}, {2, 2});
    CHECK(steps.size() == 4);
    CHECK(walk_cost(grid, {0, 0}, steps, DiagonalRule::Never) == 4 * kStraightCost);
}

TEST_CASE("path finder stops at the search limit", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({"......"});

    PathFinder enough = make_finder(DiagonalRule::WhenBothSidesAreFree, 5);
    CHECK(steps_between(enough, grid, {0, 0}, {5, 0}).size() == 5);

    PathFinder too_few = make_finder(DiagonalRule::WhenBothSidesAreFree, 4);
    CHECK(failure_between(too_few, grid, {0, 0}, {5, 0}) == PathFailure::SearchLimitReached);

    PathFinder none = make_finder(DiagonalRule::WhenBothSidesAreFree, 0);
    CHECK(failure_between(none, grid, {0, 0}, {1, 0}) == PathFailure::SearchLimitReached);
}

TEST_CASE("path finder accepts a search limit larger than any map", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({"..."});
    PathFinder finder = make_finder(DiagonalRule::Always, std::numeric_limits<std::size_t>::max());

    CHECK(steps_between(finder, grid, {0, 0}, {2, 0}).size() == 2);
}

TEST_CASE("path finder is not affected by the searches before", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({
        ".....",
        ".###.",
        ".....",
    });
    PathFinder reused = make_finder();
    PathFinder fresh = make_finder();

    REQUIRE(failure_between(reused, grid, {0, 0}, {2, 1}) == PathFailure::DestinationBlocked);
    REQUIRE(steps_between(reused, grid, {4, 2}, {0, 0}).size() == 6);

    CHECK(steps_between(reused, grid, {2, 0}, {2, 2}) == steps_between(fresh, grid, {2, 0}, {2, 2}));
}

TEST_CASE("path finder sees tiles that changed since the last search", "[0000-CA-03]") {
    TileGrid grid = draw_grid({"...", "..."});
    PathFinder finder = make_finder();
    REQUIRE(steps_between(finder, grid, {0, 0}, {2, 0}) == Steps{{1, 0}, {2, 0}});

    REQUIRE(grid.set_walkable({1, 0}, false));
    REQUIRE(grid.set_walkable({1, 1}, false));

    CHECK(failure_between(finder, grid, {0, 0}, {2, 0}) == PathFailure::Unreachable);
}

TEST_CASE("path finder works on maps of different sizes, one after the other", "[0000-CA-03]") {
    const TileGrid small = draw_grid({"..."});
    const TileGrid large = draw_grid({
        ".......",
        ".......",
        ".......",
        ".......",
    });
    PathFinder finder = make_finder();

    CHECK(steps_between(finder, small, {0, 0}, {2, 0}).size() == 2);
    CHECK(steps_between(finder, large, {0, 0}, {6, 3}).size() == 6);
    CHECK(steps_between(finder, small, {2, 0}, {0, 0}).size() == 2);
}

TEST_CASE("path finder can prepare its memory before the first search", "[0000-CA-03]") {
    const TileGrid grid = draw_grid({"..."});
    PathFinder finder = make_finder();

    finder.prepare_for(grid);

    CHECK(steps_between(finder, grid, {0, 0}, {2, 0}).size() == 2);
}

TEST_CASE("path finder always finds the cheapest route on random maps", "[0000-CA-03]") {
    constexpr int kSide = 8;
    constexpr int kMaps = 40;
    const DiagonalRule rule =
        GENERATE(DiagonalRule::Never, DiagonalRule::WhenBothSidesAreFree, DiagonalRule::Always);
    const std::uint32_t blocked_percent = GENERATE(15U, 35U);
    SeededRandom random(20261118);

    for (int map = 0; map < kMaps; ++map) {
        const TileGrid grid = random_grid(random, kSide, blocked_percent);
        const TileCoord start{static_cast<int>(random.next_below(kSide)),
                              static_cast<int>(random.next_below(kSide))};
        const std::vector<int> costs = cheapest_costs(grid, start, rule);

        // A limit equal to the tiles that can be reached is always enough, because no tile is searched twice.
        const auto reachable_tiles = static_cast<std::size_t>(
            std::count_if(costs.begin(), costs.end(), [](int cost) { return cost != kNoRoute; }));
        PathFinder finder = make_finder(rule, reachable_tiles);

        for (int z = 0; z < kSide; ++z) {
            for (int x = 0; x < kSide; ++x) {
                check_answer(finder, grid, start, {x, z}, costs[index_of(grid, {x, z})], rule);
            }
        }
    }
}
