#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Core/SceneDescription.hpp"

using Catch::Approx;
using lemuria::core::SceneDescription;
using lemuria::core::ShapeInstance;
using lemuria::core::ShapeList;

namespace {

[[nodiscard]] ShapeInstance shape_at(float x) {
    return ShapeInstance{{x, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}, {}};
}

} // namespace

TEST_CASE("a color is read from its red, green and blue values") {
    const auto color = lemuria::core::color_from_rgb(0xFF8000U);

    CHECK(color.r == Approx(1.0F));
    CHECK(color.g == Approx(128.0F / 255.0F));
    CHECK(color.b == Approx(0.0F));
    CHECK(color.a == Approx(1.0F));
}

TEST_CASE("shapes are kept in the order they were added") {
    ShapeList list(3);

    CHECK(list.add(shape_at(1.0F)));
    CHECK(list.add(shape_at(2.0F)));

    REQUIRE(list.shapes().size() == 2);
    CHECK(list.shapes()[0].position.x == 1.0F);
    CHECK(list.shapes()[1].position.x == 2.0F);
}

TEST_CASE("shapes beyond the budget are left out and counted") {
    ShapeList list(1);
    static_cast<void>(list.add(shape_at(1.0F)));

    const bool added = list.add(shape_at(2.0F));

    CHECK_FALSE(added);
    REQUIRE(list.shapes().size() == 1);
    CHECK(list.shapes()[0].position.x == 1.0F);
    CHECK(list.dropped_count() == 1);
}

TEST_CASE("clearing a list empties it and keeps its budget") {
    ShapeList list(1);
    static_cast<void>(list.add(shape_at(1.0F)));
    static_cast<void>(list.add(shape_at(2.0F)));

    list.clear();

    CHECK(list.shapes().empty());
    CHECK(list.dropped_count() == 0);
    CHECK(list.capacity() == 1);
    CHECK(list.add(shape_at(3.0F)));
}

TEST_CASE("clearing a scene empties both shape lists") {
    SceneDescription scene(2, 2);
    static_cast<void>(scene.ground_tiles.add(shape_at(1.0F)));
    static_cast<void>(scene.boxes.add(shape_at(2.0F)));

    scene.clear();

    CHECK(scene.ground_tiles.shapes().empty());
    CHECK(scene.boxes.shapes().empty());
}
