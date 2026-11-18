#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lemuria/Classic/ThreeQuarterCamera.hpp"

using Catch::Approx;
using lemuria::classic::CameraSettings;
using lemuria::classic::ThreeQuarterCamera;
using lemuria::classic::TileCoord;
using lemuria::core::Vec2;
using lemuria::core::Vec3;

namespace {

constexpr Vec2 kScreen{1280.0F, 720.0F};
constexpr Vec3 kFocus{32.0F, 0.0F, 32.0F};
constexpr CameraSettings kSettings{50.0F, 45.0F, 24.0F, 30.0F, 1.0F, 200.0F};

[[nodiscard]] ThreeQuarterCamera focused_camera(const CameraSettings& settings = kSettings) {
    ThreeQuarterCamera camera(settings);
    camera.focus_on(kFocus, kScreen);
    return camera;
}

[[nodiscard]] Vec2 on_screen(const ThreeQuarterCamera& camera, Vec3 point) {
    const auto position = lemuria::core::project_to_screen(camera.view_projection(), point, kScreen);
    REQUIRE(position.has_value());
    return *position;
}

[[nodiscard]] float turn(Vec2 a, Vec2 b, Vec2 point) {
    return (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
}

// True when the point is inside the outline drawn by the four corners, in order.
[[nodiscard]] bool is_inside(const std::array<Vec2, 4>& outline, Vec2 point) {
    bool any_positive = false;
    bool any_negative = false;
    for (std::size_t index = 0; index < outline.size(); ++index) {
        const float side = turn(outline[index], outline[(index + 1) % outline.size()], point);
        any_positive = any_positive || side > 0.0F;
        any_negative = any_negative || side < 0.0F;
    }
    return !(any_positive && any_negative);
}

[[nodiscard]] std::array<Vec2, 4> tile_outline(const ThreeQuarterCamera& camera, TileCoord tile) {
    const auto x = static_cast<float>(tile.x);
    const auto z = static_cast<float>(tile.z);
    return {on_screen(camera, {x, 0.0F, z}), on_screen(camera, {x + 1.0F, 0.0F, z}),
            on_screen(camera, {x + 1.0F, 0.0F, z + 1.0F}), on_screen(camera, {x, 0.0F, z + 1.0F})};
}

} // namespace

TEST_CASE("the camera looks down at the chosen angle") {
    const float pitch = GENERATE(35.0F, 50.0F, 65.0F);
    CameraSettings settings = kSettings;
    settings.pitch_degrees = pitch;

    const ThreeQuarterCamera camera = focused_camera(settings);

    const Vec3 to_focus = lemuria::core::normalized(kFocus - camera.eye());
    const float degrees = std::asin(-to_focus.y) * 180.0F / std::numbers::pi_v<float>;
    CHECK(degrees == Approx(pitch).margin(0.01F));
}

TEST_CASE("the camera stays at the chosen distance from what it looks at") {
    const ThreeQuarterCamera camera = focused_camera();

    CHECK(lemuria::core::length(camera.eye() - kFocus) == Approx(kSettings.distance).margin(0.001F));
}

TEST_CASE("the point the camera follows is at the center of the screen") {
    const Vec2 center = on_screen(focused_camera(), kFocus);

    CHECK(center.x == Approx(kScreen.x / 2.0F).margin(0.01F));
    CHECK(center.y == Approx(kScreen.y / 2.0F).margin(0.01F));
}

TEST_CASE("a click on a tile finds that tile", "[0000-CA-02]") {
    const int x = GENERATE(26, 30, 32, 33, 36);
    const int z = GENERATE(27, 31, 32, 35);
    const ThreeQuarterCamera camera = focused_camera();
    const TileCoord tile{x, z};

    const auto found = camera.tile_under(on_screen(camera, lemuria::classic::tile_center(tile)));

    REQUIRE(found.has_value());
    CHECK(*found == tile);
}

TEST_CASE("a click near the edge of a tile still finds that tile", "[0000-CA-02]") {
    const float offset_x = GENERATE(0.02F, 0.98F);
    const float offset_z = GENERATE(0.02F, 0.98F);
    const ThreeQuarterCamera camera = focused_camera();

    const auto found = camera.tile_under(on_screen(camera, {30.0F + offset_x, 0.0F, 34.0F + offset_z}));

    REQUIRE(found.has_value());
    CHECK(*found == TileCoord{30, 34});
}

TEST_CASE("a click on the border of the window finds the tile shown there", "[0000-CA-02]") {
    const Vec2 position = GENERATE(Vec2{0.0F, 0.0F}, Vec2{kScreen.x - 1.0F, 0.0F},
                                   Vec2{0.0F, kScreen.y - 1.0F}, Vec2{kScreen.x - 1.0F, kScreen.y - 1.0F},
                                   Vec2{kScreen.x / 2.0F, 0.0F}, Vec2{kScreen.x / 2.0F, kScreen.y - 1.0F},
                                   Vec2{0.0F, kScreen.y / 2.0F}, Vec2{kScreen.x - 1.0F, kScreen.y / 2.0F});
    const ThreeQuarterCamera camera = focused_camera();

    const auto found = camera.tile_under(position);

    REQUIRE(found.has_value());
    CHECK(is_inside(tile_outline(camera, *found), position));
}

TEST_CASE("the picked tile follows the camera") {
    ThreeQuarterCamera camera(kSettings);
    camera.focus_on({10.5F, 0.0F, 20.5F}, kScreen);

    const auto found = camera.tile_under({kScreen.x / 2.0F, kScreen.y / 2.0F});

    REQUIRE(found.has_value());
    CHECK(*found == TileCoord{10, 20});
}

TEST_CASE("a position that shows the sky is not a tile") {
    CameraSettings settings = kSettings;
    settings.pitch_degrees = 5.0F;
    settings.vertical_field_of_view_degrees = 60.0F;

    CHECK_FALSE(focused_camera(settings).tile_under({kScreen.x / 2.0F, 0.0F}).has_value());
}

TEST_CASE("a screen without area shows no tiles") {
    ThreeQuarterCamera camera(kSettings);
    camera.focus_on(kFocus, {0.0F, 0.0F});

    CHECK_FALSE(camera.tile_under({0.0F, 0.0F}).has_value());
}

TEST_CASE("a camera that was never pointed anywhere shows no tiles") {
    const ThreeQuarterCamera camera(kSettings);

    CHECK_FALSE(camera.tile_under({10.0F, 10.0F}).has_value());
}
