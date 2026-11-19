#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace lemuria::core {

// How smooth a sequence of frames was.
struct FrameTimeSummary {
    double average_milliseconds = 0.0;

    // The average of the slowest 1% of the frames, which an overall average would hide.
    double worst_one_percent_milliseconds = 0.0;

    std::size_t frame_count = 0;
};

// Measures the time of a fixed number of frames. Its memory is reserved up front, so recording a frame never
// allocates (INV-13).
class FrameTimeMeter {
public:
    // The first `warm_up_frames` are left out, because caches and drivers are still settling.
    FrameTimeMeter(std::size_t warm_up_frames, std::size_t measured_frames);

    // Frames that arrive after the measurement has finished are ignored.
    void record(std::uint64_t frame_nanoseconds) noexcept;

    [[nodiscard]] bool finished() const noexcept;

    // Works on a copy of the measured frames, so it belongs outside the frame.
    [[nodiscard]] FrameTimeSummary summary() const;

private:
    std::size_t warm_up_frames_left_;
    std::vector<std::uint64_t> frames_;
    std::size_t frame_count_ = 0;
};

} // namespace lemuria::core
