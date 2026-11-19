#include "Lemuria/Core/SeededRandom.hpp"

namespace lemuria::core {

namespace {

constexpr std::uint64_t kMultiplier = 6364136223846793005ULL;
constexpr int kMantissaBits = 24;
constexpr float kUnitScale = 1.0F / static_cast<float>(1U << kMantissaBits);

} // namespace

SeededRandom::SeededRandom(std::uint64_t seed, std::uint64_t stream) noexcept
    : increment_((stream << 1U) | 1U) {
    static_cast<void>(next());
    state_ += seed;
    static_cast<void>(next());
}

std::uint32_t SeededRandom::next() noexcept {
    const std::uint64_t previous = state_;
    state_ = previous * kMultiplier + increment_;
    const auto shifted = static_cast<std::uint32_t>(((previous >> 18U) ^ previous) >> 27U);
    const auto rotation = static_cast<std::uint32_t>(previous >> 59U);
    return (shifted >> rotation) | (shifted << ((0U - rotation) & 31U));
}

std::uint32_t SeededRandom::next_below(std::uint32_t limit) noexcept {
    if (limit == 0) {
        return 0;
    }
    // Values below the threshold would make the smaller results more likely, so they are drawn again.
    const std::uint32_t threshold = (0U - limit) % limit;
    std::uint32_t value = next();
    while (value < threshold) {
        value = next();
    }
    return value % limit;
}

float SeededRandom::next_unit() noexcept {
    return static_cast<float>(next() >> (32 - kMantissaBits)) * kUnitScale;
}

} // namespace lemuria::core
