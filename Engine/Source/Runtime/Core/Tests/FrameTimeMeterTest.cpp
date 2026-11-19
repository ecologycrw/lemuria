#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Core/FrameTimeMeter.hpp"

using Catch::Approx;
using lemuria::core::FrameTimeMeter;

namespace {

constexpr std::uint64_t kOneMillisecond = 1'000'000;

void record_frames(FrameTimeMeter& meter, std::size_t count, std::uint64_t milliseconds) {
    for (std::size_t frame = 0; frame < count; ++frame) {
        meter.record(milliseconds * kOneMillisecond);
    }
}

} // namespace

TEST_CASE("the meter reports the average and the worst one percent of 600 frames", "[0000-CA-23]") {
    FrameTimeMeter meter(0, 600);
    record_frames(meter, 594, 10);
    record_frames(meter, 6, 40);

    const auto summary = meter.summary();

    CHECK(summary.frame_count == 600);
    CHECK(summary.average_milliseconds == Approx(10.3));
    CHECK(summary.worst_one_percent_milliseconds == Approx(40.0));
}

TEST_CASE("the worst one percent is the average of the slowest frames", "[0000-CA-23]") {
    FrameTimeMeter meter(0, 600);
    record_frames(meter, 594, 10);
    for (const std::uint64_t milliseconds : {20U, 30U, 40U, 50U, 60U, 70U}) {
        meter.record(milliseconds * kOneMillisecond);
    }

    CHECK(meter.summary().worst_one_percent_milliseconds == Approx(45.0));
}

TEST_CASE("the slowest frames count wherever they happen", "[0000-CA-23]") {
    FrameTimeMeter meter(0, 200);
    record_frames(meter, 1, 90);
    record_frames(meter, 99, 10);
    record_frames(meter, 1, 70);
    record_frames(meter, 99, 10);

    CHECK(meter.summary().worst_one_percent_milliseconds == Approx(80.0));
}

TEST_CASE("the frames of the warm-up are not measured", "[0000-CA-23]") {
    FrameTimeMeter meter(3, 2);
    record_frames(meter, 3, 500);
    record_frames(meter, 2, 10);

    const auto summary = meter.summary();

    CHECK(summary.frame_count == 2);
    CHECK(summary.average_milliseconds == Approx(10.0));
    CHECK(summary.worst_one_percent_milliseconds == Approx(10.0));
}

TEST_CASE("the frames after the measurement are ignored") {
    FrameTimeMeter meter(0, 2);
    record_frames(meter, 2, 10);
    record_frames(meter, 5, 500);

    const auto summary = meter.summary();

    CHECK(summary.frame_count == 2);
    CHECK(summary.average_milliseconds == Approx(10.0));
}

TEST_CASE("the measurement finishes after the warm-up and all the measured frames") {
    FrameTimeMeter meter(2, 2);

    CHECK_FALSE(meter.finished());
    record_frames(meter, 3, 10);
    CHECK_FALSE(meter.finished());
    record_frames(meter, 1, 10);
    CHECK(meter.finished());
}

TEST_CASE("a short measurement still has a slowest frame") {
    FrameTimeMeter meter(0, 10);
    record_frames(meter, 9, 10);
    record_frames(meter, 1, 30);

    CHECK(meter.summary().worst_one_percent_milliseconds == Approx(30.0));
}

TEST_CASE("the worst one percent rounds up to whole frames") {
    FrameTimeMeter meter(0, 101);
    record_frames(meter, 99, 10);
    record_frames(meter, 1, 20);
    record_frames(meter, 1, 40);

    CHECK(meter.summary().worst_one_percent_milliseconds == Approx(30.0));
}

TEST_CASE("a measurement that has not started reports nothing") {
    const FrameTimeMeter meter(5, 10);

    const auto summary = meter.summary();

    CHECK(summary.frame_count == 0);
    CHECK(summary.average_milliseconds == 0.0);
    CHECK(summary.worst_one_percent_milliseconds == 0.0);
}

TEST_CASE("a measurement that stopped early reports only the frames it has") {
    FrameTimeMeter meter(0, 10);
    record_frames(meter, 2, 10);

    const auto summary = meter.summary();

    CHECK(summary.frame_count == 2);
    CHECK(summary.average_milliseconds == Approx(10.0));
    CHECK_FALSE(meter.finished());
}

TEST_CASE("reading the summary does not disturb the measurement") {
    FrameTimeMeter meter(0, 3);
    meter.record(30 * kOneMillisecond);
    meter.record(10 * kOneMillisecond);

    const auto before = meter.summary();
    meter.record(20 * kOneMillisecond);
    const auto after = meter.summary();

    CHECK(before.frame_count == 2);
    CHECK(after.frame_count == 3);
    CHECK(after.average_milliseconds == Approx(20.0));
    CHECK(after.worst_one_percent_milliseconds == Approx(30.0));
}
