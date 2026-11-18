#include "Lemuria/Platform/MonotonicClock.hpp"

#include <chrono>

namespace lemuria::platform {

std::uint64_t monotonic_nanoseconds() noexcept {
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
}

} // namespace lemuria::platform
