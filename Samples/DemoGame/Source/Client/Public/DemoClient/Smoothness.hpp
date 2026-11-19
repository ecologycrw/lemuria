#pragma once

#include <cstddef>
#include <string>

#include "Lemuria/Core/FrameTimeMeter.hpp"

// The smoothness this game aims for (docs/VISUAL.md).
namespace demo_client {

inline constexpr std::size_t kWarmUpFrames = 120;
inline constexpr std::size_t kMeasuredFrames = 600;

// 60 frames per second.
inline constexpr double kFrameBudgetMilliseconds = 16.6;

// A measurement passes when it is complete and both of its numbers fit the budget.
[[nodiscard]] bool is_within_frame_budget(const lemuria::core::FrameTimeSummary& summary) noexcept;

[[nodiscard]] std::string describe_frame_times(const lemuria::core::FrameTimeSummary& summary);

} // namespace demo_client
