#include <cstddef>
#include <limits>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lemuria/Classic/TileGrid.hpp"

using lemuria::classic::TileCoord;
using lemuria::classic::TileGrid;

namespace {

constexpr int kLowest = std::numeric_limits<int>::min();
constexpr int kHighest = std::numeric_limits<int>::max();

[[nodiscard]] TileGrid make_grid(int width, int depth) {
    auto grid = TileGrid::create(width, depth);
    REQUIRE(grid.has_value());
    return *grid;
}

} // namespace

TEST_CASE("tile grid accepts sizes from one tile up to the largest side") {
    const auto [width, depth] =
        GENERATE(table<int, int>({{1, 1}, {64, 64}, {3, 200}, {TileGrid::kMaxSide, 1}}));

    const TileGrid grid = make_grid(width, depth);

    CHECK(grid.width() == width);
    CHECK(grid.depth() == depth);
    CHECK(grid.tile_count() == static_cast<std::size_t>(width) * static_cast<std::size_t>(depth));
}

TEST_CASE("tile grid accepts the largest map") {
    const TileGrid grid = make_grid(TileGrid::kMaxSide, TileGrid::kMaxSide);

    CHECK(grid.is_walkable({TileGrid::kMaxSide - 1, TileGrid::kMaxSide - 1}));
}

TEST_CASE("tile grid refuses impossible sizes") {
    const auto [width, depth] = GENERATE(table<int, int>({{0, 10},
                                                          {10, 0},
                                                          {-1, 10},
                                                          {10, -1},
                                                          {TileGrid::kMaxSide + 1, 10},
                                                          {10, TileGrid::kMaxSide + 1},
                                                          {kLowest, kLowest},
                                                          {kHighest, kHighest}}));

    const auto grid = TileGrid::create(width, depth);

    REQUIRE_FALSE(grid.has_value());
    CHECK_FALSE(grid.error().message.empty());
}

TEST_CASE("tile grid starts with every tile walkable") {
    const TileGrid grid = make_grid(3, 2);

    for (int z = 0; z < grid.depth(); ++z) {
        for (int x = 0; x < grid.width(); ++x) {
            CHECK(grid.is_walkable({x, z}));
        }
    }
}

TEST_CASE("tile grid treats everything outside the map as blocked") {
    const TileGrid grid = make_grid(3, 2);
    const TileCoord outside =
        GENERATE(TileCoord{-1, 0}, TileCoord{0, -1}, TileCoord{3, 0}, TileCoord{0, 2}, TileCoord{kLowest, 0},
                 TileCoord{0, kLowest}, TileCoord{kHighest, 0}, TileCoord{0, kHighest});

    CHECK_FALSE(grid.contains(outside));
    CHECK_FALSE(grid.is_walkable(outside));
}

TEST_CASE("tile grid knows the tiles on its edges") {
    const TileGrid grid = make_grid(3, 2);
    const TileCoord edge = GENERATE(TileCoord{0, 0}, TileCoord{2, 0}, TileCoord{0, 1}, TileCoord{2, 1});

    CHECK(grid.contains(edge));
}

TEST_CASE("tile grid changes only the tile that was asked for") {
    TileGrid grid = make_grid(3, 2);

    REQUIRE(grid.set_walkable({2, 0}, false));

    for (int z = 0; z < grid.depth(); ++z) {
        for (int x = 0; x < grid.width(); ++x) {
            CHECK(grid.is_walkable({x, z}) == !(x == 2 && z == 0));
        }
    }
}

TEST_CASE("tile grid can free a blocked tile again") {
    TileGrid grid = make_grid(3, 2);
    REQUIRE(grid.set_walkable({1, 1}, false));

    REQUIRE(grid.set_walkable({1, 1}, true));

    CHECK(grid.is_walkable({1, 1}));
}

TEST_CASE("tile grid refuses to change a tile outside the map") {
    TileGrid grid = make_grid(3, 2);
    const TileCoord outside =
        GENERATE(TileCoord{-1, 0}, TileCoord{3, 0}, TileCoord{0, 2}, TileCoord{kHighest, kLowest});

    CHECK_FALSE(grid.set_walkable(outside, false));
    CHECK(grid.is_walkable({0, 0}));
    CHECK(grid.is_walkable({2, 1}));
}
