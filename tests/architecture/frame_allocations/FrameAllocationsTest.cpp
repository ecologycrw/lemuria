#include <catch2/catch_test_macros.hpp>

#include "MeasuredMatch.hpp"

using lemuria::frame_allocations::MeasuredMatch;
using lemuria::frame_allocations::Measurement;

TEST_CASE("after warming up, the frame asks for no memory while characters walk and events arrive",
          "[0000-CA-18]") {
    MeasuredMatch match;

    const Measurement measured = match.measure_after_warming_up();

    // Two cycles of walks, arrivals and departures take far more than one second of game time.
    CHECK(measured.frames > 60);
    CHECK(measured.allocations == 0);
    CHECK(measured.frames_with_allocations == 0);
    // In the order: click, game, scene, camera and drawing.
    CHECK(measured.by_part == lemuria::frame_allocations::kNoAllocationInAnyPart);
}
