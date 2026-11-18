#pragma once

#include <cstdint>
#include <optional>

namespace lemuria::core {

// The time of one frame.
struct FrameTime {
    double delta_seconds = 0.0;
    double total_seconds = 0.0;
    std::uint64_t frame_index = 0;
};

// Turns readings of a clock into frame times. The readings are injected, so tests control the time (INV-05).
class FrameClock {
public:
    static constexpr double kDefaultLongestStepSeconds = 0.25;

    // A frame never advances more than `longest_step_seconds`, so a long pause does not make the world jump.
    explicit FrameClock(double longest_step_seconds = kDefaultLongestStepSeconds) noexcept;

    // Starts a new frame at the given reading. A clock that goes backwards counts as no time passing.
    FrameTime advance(std::uint64_t now_nanoseconds) noexcept;

private:
    double longest_step_seconds_;
    std::optional<std::uint64_t> previous_reading_;
    FrameTime current_;
};

} // namespace lemuria::core
