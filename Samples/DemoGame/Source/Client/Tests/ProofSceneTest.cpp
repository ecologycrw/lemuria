#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"
#include "DemoClient/ProofScene.hpp"
#include "DemoProtocol/DemoMap.hpp"
#include "Lemuria/Core/SceneDescription.hpp"

using demo_protocol::DemoMap;
using lemuria::core::Color;
using lemuria::core::SceneDescription;
using lemuria::core::ShapeInstance;
using lemuria::core::Vec3;

namespace {

[[nodiscard]] bool same_point(Vec3 first, Vec3 second) {
    return first.x == second.x && first.y == second.y && first.z == second.z;
}

[[nodiscard]] bool same_color(Color first, Color second) {
    return first.r == second.r && first.g == second.g && first.b == second.b && first.a == second.a;
}

[[nodiscard]] bool same_shapes(std::span<const ShapeInstance> first, std::span<const ShapeInstance> second) {
    if (first.size() != second.size()) {
        return false;
    }
    for (std::size_t index = 0; index < first.size(); ++index) {
        if (!same_point(first[index].position, second[index].position) ||
            !same_point(first[index].size, second[index].size) ||
            !same_color(first[index].color, second[index].color)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool same_scene(const SceneDescription& first, const SceneDescription& second) {
    return first.view_projection.values == second.view_projection.values &&
           same_color(first.background, second.background) &&
           same_shapes(first.ground_tiles.shapes(), second.ground_tiles.shapes()) &&
           same_shapes(first.boxes.shapes(), second.boxes.shapes());
}

[[nodiscard]] SceneDescription empty_scene() {
    return {demo_client::kGroundTileBudget, demo_client::kBoxBudget};
}

[[nodiscard]] std::size_t count_characters(const SceneDescription& scene, std::uint32_t rgb) {
    std::size_t characters = 0;
    for (const ShapeInstance& shape : scene.boxes.shapes()) {
        const bool is_character = shape.size.y == demo_client::kCharacterHeight &&
                                  same_color(shape.color, lemuria::core::color_from_rgb(rgb));
        characters += is_character ? 1U : 0U;
    }
    return characters;
}

} // namespace

TEST_CASE("the proof scene is the same every time it is filled", "[0000-CA-19]") {
    const DemoMap first_map;
    const DemoMap second_map;
    SceneDescription first = empty_scene();
    SceneDescription second = empty_scene();

    demo_client::fill_proof_scene(first_map, first);
    demo_client::fill_proof_scene(second_map, second);

    CHECK(same_scene(first, second));
}

TEST_CASE("filling the proof scene replaces what the scene had before", "[0000-CA-19]") {
    const DemoMap map;
    SceneDescription fresh = empty_scene();
    SceneDescription reused = empty_scene();
    demo_client::add_character({1.5F, 0.0F, 1.5F}, 0xFF0000U, reused);

    demo_client::fill_proof_scene(map, fresh);
    demo_client::fill_proof_scene(map, reused);

    CHECK(same_scene(fresh, reused));
}

TEST_CASE("the proof scene fits the drawing budget") {
    const DemoMap map;
    SceneDescription scene = empty_scene();

    demo_client::fill_proof_scene(map, scene);

    CHECK(scene.ground_tiles.dropped_count() == 0);
    CHECK(scene.boxes.dropped_count() == 0);
}

TEST_CASE("the proof scene shows one character of each color") {
    const DemoMap map;
    SceneDescription scene = empty_scene();

    demo_client::fill_proof_scene(map, scene);

    CHECK(count_characters(scene, demo_client::kOwnCharacterRgb) == 1);
    CHECK(count_characters(scene, demo_client::kOtherCharacterRgb) == 1);
}
