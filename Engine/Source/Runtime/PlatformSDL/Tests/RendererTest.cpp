#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numbers>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Core/SceneDescription.hpp"
#include "Lemuria/Platform/Renderer.hpp"

using lemuria::core::Color;
using lemuria::core::color_from_rgb;
using lemuria::core::SceneDescription;
using lemuria::core::ShapeInstance;
using lemuria::platform::Image;
using lemuria::platform::Renderer;
using lemuria::platform::RendererOptions;

namespace {

constexpr std::uint32_t kImageSize = 64;
constexpr std::uint32_t kBackground = 0x102030U;
constexpr std::uint32_t kGreen = 0x20C040U;
constexpr std::uint32_t kRed = 0xC83214U;

[[nodiscard]] SceneDescription top_down_scene() {
    SceneDescription scene(16, 16);
    const auto view = lemuria::core::look_at({0.0F, 10.0F, 0.0F}, {}, {0.0F, 0.0F, -1.0F});
    const auto projection = lemuria::core::perspective(std::numbers::pi_v<float> / 3.0F, 1.0F, 1.0F, 50.0F);
    scene.view_projection = projection * view;
    scene.background = color_from_rgb(kBackground);
    return scene;
}

[[nodiscard]] ShapeInstance flat_tile(float z, float height, float size, std::uint32_t rgb) {
    return ShapeInstance{{0.0F, height, z}, {size, 0.0F, size}, color_from_rgb(rgb)};
}

[[nodiscard]] Image draw_and_read(const SceneDescription& scene) {
    auto renderer = Renderer::offscreen(kImageSize, kImageSize, RendererOptions{});
    INFO((renderer ? "" : renderer.error().message));
    REQUIRE(renderer.has_value());

    const auto drawn = renderer->draw(scene);
    INFO((drawn ? "" : drawn.error().message));
    REQUIRE(drawn.has_value());

    auto image = renderer->read_image();
    INFO((image ? "" : image.error().message));
    REQUIRE(image.has_value());
    return *image;
}

[[nodiscard]] bool pixel_is(const Image& image, std::uint32_t x, std::uint32_t y, std::uint32_t rgb) {
    const std::size_t offset = (std::size_t{y} * image.width + x) * 4;
    const std::array<int, 3> expected{static_cast<int>((rgb >> 16U) & 0xFFU),
                                      static_cast<int>((rgb >> 8U) & 0xFFU), static_cast<int>(rgb & 0xFFU)};
    for (std::size_t channel = 0; channel < expected.size(); ++channel) {
        if (std::abs(static_cast<int>(image.pixels[offset + channel]) - expected[channel]) > 1) {
            return false;
        }
    }
    return image.pixels[offset + 3] == 255;
}

} // namespace

TEST_CASE("an empty scene fills the image with the background color") {
    const Image image = draw_and_read(top_down_scene());

    REQUIRE(image.width == kImageSize);
    REQUIRE(image.height == kImageSize);
    CHECK(pixel_is(image, 0, 0, kBackground));
    CHECK(pixel_is(image, kImageSize / 2, kImageSize / 2, kBackground));
    CHECK(pixel_is(image, kImageSize - 1, kImageSize - 1, kBackground));
}

TEST_CASE("a ground tile is drawn with its color and only where it is") {
    SceneDescription scene = top_down_scene();
    static_cast<void>(scene.ground_tiles.add(flat_tile(0.0F, 0.0F, 4.0F, kGreen)));

    const Image image = draw_and_read(scene);

    CHECK(pixel_is(image, kImageSize / 2, kImageSize / 2, kGreen));
    CHECK(pixel_is(image, 2, 2, kBackground));
}

TEST_CASE("the top of the image shows what is farther along the negative Z axis") {
    SceneDescription scene = top_down_scene();
    static_cast<void>(scene.ground_tiles.add(flat_tile(-3.0F, 0.0F, 3.0F, kGreen)));

    const Image image = draw_and_read(scene);

    CHECK(pixel_is(image, kImageSize / 2, 15, kGreen));
    CHECK(pixel_is(image, kImageSize / 2, 49, kBackground));
}

TEST_CASE("the top of a box is drawn with the full color of the box") {
    SceneDescription scene = top_down_scene();
    static_cast<void>(scene.ground_tiles.add(flat_tile(0.0F, 0.0F, 8.0F, kGreen)));
    static_cast<void>(scene.boxes.add(ShapeInstance{{}, {2.0F, 2.0F, 2.0F}, color_from_rgb(kRed)}));

    const Image image = draw_and_read(scene);

    CHECK(pixel_is(image, kImageSize / 2, kImageSize / 2, kRed));
}

TEST_CASE("a shape drawn later stays hidden behind a closer one") {
    SceneDescription scene = top_down_scene();
    static_cast<void>(scene.ground_tiles.add(flat_tile(0.0F, 1.0F, 4.0F, kGreen)));
    static_cast<void>(scene.boxes.add(ShapeInstance{{}, {2.0F, 0.5F, 2.0F}, color_from_rgb(kRed)}));

    const Image image = draw_and_read(scene);

    CHECK(pixel_is(image, kImageSize / 2, kImageSize / 2, kGreen));
}

TEST_CASE("drawing the same scene twice gives the same image") {
    SceneDescription scene = top_down_scene();
    static_cast<void>(scene.ground_tiles.add(flat_tile(0.0F, 0.0F, 4.0F, kGreen)));

    const Image first = draw_and_read(scene);
    const Image second = draw_and_read(scene);

    CHECK(first.pixels == second.pixels);
}

TEST_CASE("an image in memory always has a frame to draw on") {
    auto renderer = Renderer::offscreen(kImageSize, kImageSize, RendererOptions{});
    REQUIRE(renderer.has_value());

    const auto drawn = renderer->draw(top_down_scene());

    REQUIRE(drawn.has_value());
    CHECK(renderer->skipped_frame_count() == 0);
}

TEST_CASE("an image to draw on needs an area") {
    const auto renderer = Renderer::offscreen(0, 64, RendererOptions{});

    REQUIRE_FALSE(renderer.has_value());
    CHECK_FALSE(renderer.error().message.empty());
}
