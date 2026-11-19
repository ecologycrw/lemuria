#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

#include "Lemuria/Platform/MonotonicClock.hpp"

// Real connections take real time, so these tests wait for a condition, always with a limit.
namespace lemuria::platform::test {

// SDL_net cannot tell which port the system chose, so each test looks for a free one in this range.
inline constexpr std::uint16_t kFirstTestPort = 24'100;
inline constexpr std::uint16_t kTestPortCount = 400;

// No test ever listens on this port, even when several test programs run at the same time.
inline constexpr std::uint16_t kPortWithoutServer = kFirstTestPort - 1;

inline constexpr std::uint64_t kWaitLimitNanoseconds = 20'000'000'000;

template <typename Condition>
[[nodiscard]] bool wait_until(Condition condition) {
    const std::uint64_t give_up_at = monotonic_nanoseconds() + kWaitLimitNanoseconds;
    while (!condition()) {
        if (monotonic_nanoseconds() > give_up_at) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

// A sequence that changes from byte to byte, so lost or reordered bytes are noticed.
[[nodiscard]] inline std::byte pattern_byte(std::size_t position) noexcept {
    return static_cast<std::byte>(((position * 31U) + (position >> 8U)) & 0xFFU);
}

[[nodiscard]] inline std::vector<std::byte> pattern_bytes(std::size_t first_position, std::size_t count) {
    std::vector<std::byte> bytes(count);
    for (std::size_t index = 0; index < count; ++index) {
        bytes[index] = pattern_byte(first_position + index);
    }
    return bytes;
}

} // namespace lemuria::platform::test
