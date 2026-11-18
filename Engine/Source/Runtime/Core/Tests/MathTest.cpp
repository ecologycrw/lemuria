#include <cstddef>
#include <numbers>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lemuria/Core/Math.hpp"

using Catch::Approx;
using lemuria::core::Mat4;
using lemuria::core::Ray;
using lemuria::core::Vec2;
using lemuria::core::Vec3;
using lemuria::core::Vec4;

namespace {

constexpr float kMargin = 0.001F;
constexpr Vec2 kScreen{1280.0F, 720.0F};

[[nodiscard]] Mat4 sample_camera() {
    const Mat4 view = lemuria::core::look_at({20.0F, 18.0F, 20.0F}, {32.0F, 0.0F, 32.0F}, {0.0F, 1.0F, 0.0F});
    const Mat4 projection =
        lemuria::core::perspective(std::numbers::pi_v<float> / 6.0F, kScreen.x / kScreen.y, 1.0F, 200.0F);
    return projection * view;
}

} // namespace

TEST_CASE("a vector is scaled to length one and keeps its direction") {
    const Vec3 result = lemuria::core::normalized({3.0F, 0.0F, 4.0F});

    CHECK(result.x == Approx(0.6F).margin(kMargin));
    CHECK(result.y == Approx(0.0F).margin(kMargin));
    CHECK(result.z == Approx(0.8F).margin(kMargin));
}

TEST_CASE("the zero vector stays zero when normalized") {
    const Vec3 result = lemuria::core::normalized({});

    CHECK(result.x == 0.0F);
    CHECK(result.y == 0.0F);
    CHECK(result.z == 0.0F);
}

TEST_CASE("a matrix multiplied by its inverse gives the identity") {
    const Mat4 camera = sample_camera();

    const auto undone = lemuria::core::inverse(camera);

    REQUIRE(undone.has_value());
    const Mat4 product = camera * *undone;
    const Mat4 identity = Mat4::identity();
    for (std::size_t index = 0; index < identity.values.size(); ++index) {
        CHECK(product.values[index] == Approx(identity.values[index]).margin(kMargin));
    }
}

TEST_CASE("a matrix that cannot be undone has no inverse") {
    CHECK_FALSE(lemuria::core::inverse(Mat4{}).has_value());
}

TEST_CASE("the camera places its target straight ahead") {
    const Mat4 view = lemuria::core::look_at({0.0F, 0.0F, 10.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});

    const Vec4 target = view * Vec4{0.0F, 0.0F, 0.0F, 1.0F};

    CHECK(target.x == Approx(0.0F).margin(kMargin));
    CHECK(target.y == Approx(0.0F).margin(kMargin));
    CHECK(target.z == Approx(-10.0F).margin(kMargin));
}

TEST_CASE("depth goes from zero at the near distance to one at the far distance") {
    const Mat4 projection = lemuria::core::perspective(1.0F, 1.0F, 2.0F, 100.0F);

    const Vec4 near_point = projection * Vec4{0.0F, 0.0F, -2.0F, 1.0F};
    const Vec4 far_point = projection * Vec4{0.0F, 0.0F, -100.0F, 1.0F};

    CHECK(near_point.z / near_point.w == Approx(0.0F).margin(kMargin));
    CHECK(far_point.z / far_point.w == Approx(1.0F).margin(kMargin));
}

TEST_CASE("the camera target lands on the center of the screen") {
    const auto position = lemuria::core::project_to_screen(sample_camera(), {32.0F, 0.0F, 32.0F}, kScreen);

    REQUIRE(position.has_value());
    CHECK(position->x == Approx(640.0F).margin(0.01F));
    CHECK(position->y == Approx(360.0F).margin(0.01F));
}

TEST_CASE("a point behind the camera has no screen position") {
    CHECK_FALSE(lemuria::core::project_to_screen(sample_camera(), {0.0F, 40.0F, 0.0F}, kScreen).has_value());
}

TEST_CASE("a screen without area has no positions or rays") {
    const Vec2 empty{0.0F, 720.0F};

    CHECK_FALSE(lemuria::core::project_to_screen(sample_camera(), {32.0F, 0.0F, 32.0F}, empty).has_value());
    CHECK_FALSE(lemuria::core::screen_ray(Mat4::identity(), {1.0F, 1.0F}, empty).has_value());
}

TEST_CASE("a ground point seen on the screen is found again from that screen position") {
    const float x = GENERATE(26.0F, 32.0F, 37.5F);
    const float z = GENERATE(27.25F, 32.0F, 38.0F);
    const Mat4 camera = sample_camera();
    const auto undone = lemuria::core::inverse(camera);
    REQUIRE(undone.has_value());

    const auto position = lemuria::core::project_to_screen(camera, {x, 0.0F, z}, kScreen);
    REQUIRE(position.has_value());
    const auto ray = lemuria::core::screen_ray(*undone, *position, kScreen);
    REQUIRE(ray.has_value());
    const auto ground = lemuria::core::intersect_horizontal_plane(*ray, 0.0F);

    REQUIRE(ground.has_value());
    CHECK(ground->x == Approx(x).margin(0.01F));
    CHECK(ground->z == Approx(z).margin(0.01F));
}

TEST_CASE("a ray parallel to the ground never reaches it") {
    const Ray ray{{0.0F, 5.0F, 0.0F}, {1.0F, 0.0F, 0.0F}};

    CHECK_FALSE(lemuria::core::intersect_horizontal_plane(ray, 0.0F).has_value());
}

TEST_CASE("a ray pointing away from the ground never reaches it") {
    const Ray ray{{0.0F, 5.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};

    CHECK_FALSE(lemuria::core::intersect_horizontal_plane(ray, 0.0F).has_value());
}
