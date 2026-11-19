#include <cstddef>
#include <map>

#include <catch2/catch_test_macros.hpp>

#include "DemoProtocol/DemoMap.hpp"

using demo_protocol::DemoMap;
using demo_protocol::TileKind;

namespace {

[[nodiscard]] std::map<TileKind, std::size_t> count_tiles(const DemoMap& map) {
    std::map<TileKind, std::size_t> counts;
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            ++counts[map.kind_at(x, z)];
        }
    }
    return counts;
}

[[nodiscard]] bool same_tiles(const DemoMap& first, const DemoMap& second) {
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            if (first.kind_at(x, z) != second.kind_at(x, z)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

TEST_CASE("the same seed always generates the same map") {
    CHECK(same_tiles(DemoMap(7), DemoMap(7)));
}

TEST_CASE("different seeds generate different maps") {
    CHECK_FALSE(same_tiles(DemoMap(7), DemoMap(8)));
}

TEST_CASE("the map is mostly dark ground, with a few rocks") {
    auto counts = count_tiles(DemoMap{});
    const std::size_t total = static_cast<std::size_t>(DemoMap::kSize) * DemoMap::kSize;

    CHECK(counts[TileKind::DeepShadow] * 100 / total >= 28);
    CHECK(counts[TileKind::Earth] * 100 / total >= 40);
    CHECK(counts[TileKind::Moss] * 100 / total >= 14);
    CHECK(counts[TileKind::Stone] > 0);
    CHECK(counts[TileKind::Stone] * 100 / total <= 3);
    CHECK(counts[TileKind::Trail] > 0);
}

TEST_CASE("rocks block the client and the server") {
    const DemoMap map;
    bool found_stone = false;
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            if (map.kind_at(x, z) == TileKind::Stone) {
                found_stone = true;
                CHECK_FALSE(map.looks_walkable(x, z));
                CHECK_FALSE(map.is_walkable_on_server(x, z));
            }
        }
    }
    CHECK(found_stone);
}

TEST_CASE("the trail strip looks free to the client and is forbidden by the server") {
    const DemoMap map;

    REQUIRE(map.kind_at(32, 27) == TileKind::Trail);
    CHECK(map.looks_walkable(32, 27));
    CHECK_FALSE(map.is_walkable_on_server(32, 27));
}

TEST_CASE("the starting area is free of rocks") {
    const DemoMap map;

    for (int z = 29; z <= 36; ++z) {
        for (int x = 28; x <= 36; ++x) {
            CHECK(map.is_walkable_on_server(x, z));
        }
    }
}

TEST_CASE("everything outside the map is blocked") {
    const DemoMap map;

    CHECK_FALSE(map.contains(-1, 0));
    CHECK_FALSE(map.contains(0, DemoMap::kSize));
    CHECK(map.kind_at(-1, 0) == TileKind::Stone);
    CHECK_FALSE(map.looks_walkable(DemoMap::kSize, 0));
    CHECK_FALSE(map.is_walkable_on_server(0, -1));
}
