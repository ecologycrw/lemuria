#include <catch2/catch_test_macros.hpp>

#include "DemoClient/GroundScene.hpp"
#include "DemoClient/ProofScene.hpp"
#include "Lemuria/Platform/Renderer.hpp"
#include "MeasuredMatch.hpp"

using lemuria::frame_allocations::MeasuredMatch;
using lemuria::frame_allocations::Measurement;

TEST_CASE("after warming up, the frame asks for no memory while it draws walking characters",
          "[0000-CA-18]") {
    lemuria::platform::RendererOptions options;
    options.max_ground_tiles = demo_client::kGroundTileBudget;
    options.max_boxes = demo_client::kBoxBudget;
    auto renderer = lemuria::platform::Renderer::offscreen(demo_client::kProofImageWidth,
                                                           demo_client::kProofImageHeight, options);
    REQUIRE(renderer.has_value());
    MeasuredMatch match(&*renderer);

    const Measurement measured = match.measure_after_warming_up();

    CHECK(measured.frames > 60);
    CHECK(measured.allocations == 0);
    CHECK(measured.frames_with_allocations == 0);
    // In the order: click, game, scene, camera and drawing.
    CHECK(measured.by_part == lemuria::frame_allocations::kNoAllocationInAnyPart);
}
