#include <cstddef>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"
#include "DemoProtocol/DemoMap.hpp"

using Catch::Approx;
using demo_protocol::DemoMap;
using demo_protocol::TileKind;
using lemuria::core::SceneDescription;

namespace {

[[nodiscard]] std::size_t count_stones(const DemoMap& map) {
    std::size_t stones = 0;
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            stones += map.kind_at(x, z) == TileKind::Stone ? 1U : 0U;
        }
    }
    return stones;
}

} // namespace

TEST_CASE("every tile of the map is added to the scene") {
    const DemoMap map;
    SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);

    demo_client::add_ground(map, scene);

    CHECK(scene.ground_tiles.shapes().size() == demo_client::kGroundTileBudget);
    CHECK(scene.ground_tiles.dropped_count() == 0);
}

TEST_CASE("each rock gets a raised block within the budget") {
    const DemoMap map;
    SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);

    demo_client::add_ground(map, scene);

    CHECK(scene.boxes.shapes().size() == count_stones(map));
    CHECK(scene.boxes.dropped_count() == 0);
}

TEST_CASE("a tile keeps the color of its kind of ground, with a small variation") {
    const DemoMap map;
    SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);

    demo_client::add_ground(map, scene);

    const auto& tile = scene.ground_tiles.shapes()[0];
    const auto expected = demo_client::tile_color(map.kind_at(0, 0));
    CHECK(tile.color.r == Approx(expected.r).margin(expected.r * 0.09F));
    CHECK(tile.color.g == Approx(expected.g).margin(expected.g * 0.09F));
    CHECK(tile.color.b == Approx(expected.b).margin(expected.b * 0.09F));
}

TEST_CASE("the same map always gives the same scene") {
    const DemoMap map;
    SceneDescription first(demo_client::kGroundTileBudget, demo_client::kBoxBudget);
    SceneDescription second(demo_client::kGroundTileBudget, demo_client::kBoxBudget);

    demo_client::add_ground(map, first);
    demo_client::add_ground(map, second);

    for (std::size_t index = 0; index < first.ground_tiles.shapes().size(); ++index) {
        REQUIRE(first.ground_tiles.shapes()[index].color.r == second.ground_tiles.shapes()[index].color.r);
    }
}

TEST_CASE("a character stands on the given position with the given color") {
    SceneDescription scene(1, 1);

    demo_client::add_character({3.5F, 0.0F, 4.5F}, 0xFF0000U, scene);

    REQUIRE(scene.boxes.shapes().size() == 1);
    const auto& character = scene.boxes.shapes()[0];
    CHECK(character.position.x == 3.5F);
    CHECK(character.position.z == 4.5F);
    CHECK(character.size.y == demo_client::kCharacterHeight);
    CHECK(character.color.r == Approx(1.0F));
    CHECK(character.color.g == Approx(0.0F));
}
