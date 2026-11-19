#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Core/FrameClock.hpp"

using Catch::Approx;
using lemuria::core::FrameClock;

namespace {

constexpr std::uint64_t kOneSecond = 1'000'000'000;
constexpr std::uint64_t kStart = 5 * kOneSecond;

} // namespace

TEST_CASE("the first frame takes no time") {
    FrameClock clock;

    const auto frame = clock.advance(kStart);

    CHECK(frame.delta_seconds == 0.0);
    CHECK(frame.total_seconds == 0.0);
    CHECK(frame.frame_index == 0);
}

TEST_CASE("each frame measures the time since the previous one") {
    FrameClock clock;
    static_cast<void>(clock.advance(kStart));

    const auto second = clock.advance(kStart + kOneSecond / 10);
    const auto third = clock.advance(kStart + kOneSecond / 4);

    CHECK(second.delta_seconds == Approx(0.1));
    CHECK(third.delta_seconds == Approx(0.15));
    CHECK(third.total_seconds == Approx(0.25));
    CHECK(third.frame_index == 2);
}

TEST_CASE("a long pause advances the world by the longest step only") {
    FrameClock clock(0.25);
    static_cast<void>(clock.advance(kStart));

    const auto frame = clock.advance(kStart + 30 * kOneSecond);

    CHECK(frame.delta_seconds == Approx(0.25));
    CHECK(frame.total_seconds == Approx(0.25));
}

TEST_CASE("a clock that goes backwards counts as no time passing") {
    FrameClock clock;
    static_cast<void>(clock.advance(kStart));

    const auto backwards = clock.advance(kStart - kOneSecond);
    const auto recovered = clock.advance(kStart + kOneSecond / 10);

    CHECK(backwards.delta_seconds == 0.0);
    CHECK(recovered.delta_seconds == Approx(0.1));
}
