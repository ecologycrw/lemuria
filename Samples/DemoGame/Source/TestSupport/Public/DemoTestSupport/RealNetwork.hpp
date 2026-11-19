#pragma once

#include <cstdint>

// What the tests of the demonstration share when they use real connections on this computer.
namespace demo_test_support {

// The same range the tests of the platform layer use. SDL_net cannot tell which port the system chose, so
// each test looks for a free port in it.
inline constexpr std::uint16_t kFirstTestPort = 24'100;
inline constexpr std::uint16_t kTestPortCount = 400;

// Real connections take real time, so the tests wait for a condition, always with this limit.
inline constexpr double kWaitLimitSeconds = 20.0;

// Seconds on the real clock, counted from an arbitrary moment. Only for waiting: the rules and the games
// always receive their time from the test (INV-05).
[[nodiscard]] double real_seconds() noexcept;

// Gives the connections a moment of real time before the next round.
void pause_briefly();

// Plays `round` again and again until `condition` holds. Returns false when the limit passes first.
template <typename Condition, typename Round>
[[nodiscard]] bool wait_until(Condition condition, Round round) {
    const double give_up_at = real_seconds() + kWaitLimitSeconds;
    while (!condition()) {
        if (real_seconds() > give_up_at) {
            return false;
        }
        round();
        pause_briefly();
    }
    return true;
}

} // namespace demo_test_support
