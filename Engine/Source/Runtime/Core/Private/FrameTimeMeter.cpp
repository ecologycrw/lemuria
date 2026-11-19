#include "Lemuria/Core/FrameTimeMeter.hpp"

#include <algorithm>
#include <functional>
#include <span>

namespace lemuria::core {

namespace {

constexpr double kNanosecondsPerMillisecond = 1.0e6;
constexpr std::size_t kFramesPerWorstFrame = 100;

[[nodiscard]] double average_milliseconds(std::span<const std::uint64_t> frames) noexcept {
    double total_nanoseconds = 0.0;
    for (const std::uint64_t frame : frames) {
        total_nanoseconds += static_cast<double>(frame);
    }
    return total_nanoseconds / static_cast<double>(frames.size()) / kNanosecondsPerMillisecond;
}

} // namespace

FrameTimeMeter::FrameTimeMeter(std::size_t warm_up_frames, std::size_t measured_frames)
    : warm_up_frames_left_(warm_up_frames), frames_(measured_frames) {}

void FrameTimeMeter::record(std::uint64_t frame_nanoseconds) noexcept {
    if (warm_up_frames_left_ > 0) {
        --warm_up_frames_left_;
        return;
    }
    if (frame_count_ < frames_.size()) {
        frames_[frame_count_] = frame_nanoseconds;
        ++frame_count_;
    }
}

bool FrameTimeMeter::finished() const noexcept {
    return frame_count_ == frames_.size();
}

FrameTimeSummary FrameTimeMeter::summary() const {
    if (frame_count_ == 0) {
        return {};
    }
    const std::span<const std::uint64_t> measured = std::span(frames_).first(frame_count_);
    std::vector<std::uint64_t> slowest_first(measured.begin(), measured.end());
    std::ranges::sort(slowest_first, std::greater<>{});

    const std::size_t worst_count = (frame_count_ + kFramesPerWorstFrame - 1) / kFramesPerWorstFrame;
    return FrameTimeSummary{average_milliseconds(measured),
                            average_milliseconds(std::span(slowest_first).first(worst_count)), frame_count_};
}

} // namespace lemuria::core
