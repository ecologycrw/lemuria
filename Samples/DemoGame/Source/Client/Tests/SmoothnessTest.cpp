#include <catch2/catch_test_macros.hpp>

#include "DemoClient/Smoothness.hpp"
#include "Lemuria/Core/FrameTimeMeter.hpp"

using demo_client::describe_frame_times;
using demo_client::is_within_frame_budget;
using demo_client::kMeasuredFrames;
using lemuria::core::FrameTimeSummary;

TEST_CASE("the measurement covers 600 frames after a warm-up", "[0000-CA-23]") {
    CHECK(kMeasuredFrames == 600);
    CHECK(demo_client::kWarmUpFrames > 0);
}

TEST_CASE("frames at the limit of 16.6 milliseconds are within the budget", "[0000-CA-23]") {
    CHECK(is_within_frame_budget(FrameTimeSummary{16.6, 16.6, kMeasuredFrames}));
}

TEST_CASE("a slow average is over the budget", "[0000-CA-23]") {
    CHECK_FALSE(is_within_frame_budget(FrameTimeSummary{16.7, 16.6, kMeasuredFrames}));
}

TEST_CASE("slow frames are over the budget even with a good average", "[0000-CA-23]") {
    CHECK_FALSE(is_within_frame_budget(FrameTimeSummary{5.0, 16.7, kMeasuredFrames}));
}

TEST_CASE("a measurement that did not finish is not within the budget", "[0000-CA-23]") {
    CHECK_FALSE(is_within_frame_budget(FrameTimeSummary{5.0, 6.0, kMeasuredFrames - 1}));
}

TEST_CASE("the result of the measurement is described with its numbers", "[0000-CA-23]") {
    CHECK(describe_frame_times(FrameTimeSummary{1.434, 2.1, kMeasuredFrames}) ==
          "Frame time over 600 frames: average 1.43 ms, worst 1% 2.10 ms (budget 16.6 ms): within budget");
    CHECK(describe_frame_times(FrameTimeSummary{20.0, 31.25, kMeasuredFrames}) ==
          "Frame time over 600 frames: average 20.00 ms, worst 1% 31.25 ms (budget 16.6 ms): over budget");
}
