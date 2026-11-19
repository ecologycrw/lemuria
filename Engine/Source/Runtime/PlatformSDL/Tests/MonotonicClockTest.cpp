#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Platform/MonotonicClock.hpp"

TEST_CASE("the real clock never goes backwards") {
    std::uint64_t previous = lemuria::platform::monotonic_nanoseconds();

    for (int reading = 0; reading < 1000; ++reading) {
        const std::uint64_t now = lemuria::platform::monotonic_nanoseconds();
        REQUIRE(now >= previous);
        previous = now;
    }
}
