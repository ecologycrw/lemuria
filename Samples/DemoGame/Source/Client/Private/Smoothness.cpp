#include "DemoClient/Smoothness.hpp"

#include <iomanip>
#include <sstream>

namespace demo_client {

bool is_within_frame_budget(const lemuria::core::FrameTimeSummary& summary) noexcept {
    return summary.frame_count == kMeasuredFrames &&
           summary.average_milliseconds <= kFrameBudgetMilliseconds &&
           summary.worst_one_percent_milliseconds <= kFrameBudgetMilliseconds;
}

std::string describe_frame_times(const lemuria::core::FrameTimeSummary& summary) {
    std::ostringstream text;
    text << "Frame time over " << summary.frame_count << " frames: average " << std::fixed
         << std::setprecision(2) << summary.average_milliseconds << " ms, worst 1% "
         << summary.worst_one_percent_milliseconds << " ms (budget " << std::setprecision(1)
         << kFrameBudgetMilliseconds
         << " ms): " << (is_within_frame_budget(summary) ? "within budget" : "over budget");
    return text.str();
}

} // namespace demo_client
