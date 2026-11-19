#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

#include "Lemuria/Classic/TileCoord.hpp"

namespace lemuria::classic {

// The steps of one walk, kept inside the object so events and requests never allocate memory (INV-13).
class WalkSteps {
public:
    static constexpr std::size_t kCapacity = 32;

    // False, with nothing changed, when the list is full.
    [[nodiscard]] constexpr bool add(TileCoord step) noexcept {
        if (count_ == kCapacity) {
            return false;
        }
        steps_[count_] = step;
        ++count_;
        return true;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return count_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return count_ == 0; }

    [[nodiscard]] std::span<const TileCoord> view() const noexcept { return {steps_.data(), count_}; }

    friend bool operator==(const WalkSteps& first, const WalkSteps& second) noexcept {
        return std::ranges::equal(first.view(), second.view());
    }

private:
    std::array<TileCoord, kCapacity> steps_{};
    std::size_t count_ = 0;
};

} // namespace lemuria::classic
