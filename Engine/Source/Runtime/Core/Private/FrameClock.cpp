#include "Lemuria/Core/FrameClock.hpp"

#include <algorithm>

namespace lemuria::core {

namespace {

constexpr double kNanosecondsPerSecond = 1.0e9;

} // namespace

FrameClock::FrameClock(double longest_step_seconds) noexcept : longest_step_seconds_(longest_step_seconds) {}

FrameTime FrameClock::advance(std::uint64_t now_nanoseconds) noexcept {
    if (!previous_reading_) {
        previous_reading_ = now_nanoseconds;
        return current_;
    }

    const std::uint64_t previous = *previous_reading_;
    const double elapsed_seconds =
        now_nanoseconds > previous ? static_cast<double>(now_nanoseconds - previous) / kNanosecondsPerSecond
                                   : 0.0;
    previous_reading_ = std::max(now_nanoseconds, previous);

    current_.delta_seconds = std::min(elapsed_seconds, longest_step_seconds_);
    current_.total_seconds += current_.delta_seconds;
    ++current_.frame_index;
    return current_;
}

} // namespace lemuria::core
