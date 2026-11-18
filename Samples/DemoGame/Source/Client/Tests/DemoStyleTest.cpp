#include <cmath>
#include <numbers>

#include <catch2/catch_test_macros.hpp>

#include "DemoClient/DemoStyle.hpp"
#include "Lemuria/Classic/ThreeQuarterCamera.hpp"

using lemuria::classic::ThreeQuarterCamera;
using lemuria::core::Vec2;
using lemuria::core::Vec3;

namespace {

constexpr Vec2 kScreen{1280.0F, 720.0F};
constexpr Vec3 kFocus{32.5F, 0.0F, 32.5F};

} // namespace

TEST_CASE("the demonstration camera looks down at about 50 degrees", "[0000-CA-01]") {
    ThreeQuarterCamera camera(demo_client::kCameraSettings);
    camera.focus_on(kFocus, kScreen);

    const Vec3 direction = lemuria::core::normalized(kFocus - camera.eye());
    const float degrees = std::asin(-direction.y) * 180.0F / std::numbers::pi_v<float>;

    CHECK(degrees >= 49.0F);
    CHECK(degrees <= 51.0F);
}

TEST_CASE("a character takes 8 to 10 percent of the screen height", "[0000-CA-01]") {
    ThreeQuarterCamera camera(demo_client::kCameraSettings);
    camera.focus_on(kFocus, kScreen);
    const Vec3 head{kFocus.x, demo_client::kCharacterHeight, kFocus.z};

    const auto feet_on_screen = lemuria::core::project_to_screen(camera.view_projection(), kFocus, kScreen);
    const auto head_on_screen = lemuria::core::project_to_screen(camera.view_projection(), head, kScreen);

    REQUIRE(feet_on_screen.has_value());
    REQUIRE(head_on_screen.has_value());
    const float share = std::abs(feet_on_screen->y - head_on_screen->y) / kScreen.y;
    CHECK(share >= 0.08F);
    CHECK(share <= 0.10F);
}
