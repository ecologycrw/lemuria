#pragma once

#include <cstdint>

namespace lemuria::core {

// Repeatable random numbers for visuals: the same seed always gives the same sequence, on every
// compiler and system (INV-05). It uses the PCG32 algorithm. It is not meant for anything secret.
class SeededRandom {
public:
    static constexpr std::uint64_t kDefaultStream = 54;

    explicit SeededRandom(std::uint64_t seed, std::uint64_t stream = kDefaultStream) noexcept;

    [[nodiscard]] std::uint32_t next() noexcept;

    // A number from 0 up to, but not including, `limit`, with every value equally likely. A limit of 0 gives
    // 0.
    [[nodiscard]] std::uint32_t next_below(std::uint32_t limit) noexcept;

    // A number from 0 up to, but not including, 1.
    [[nodiscard]] float next_unit() noexcept;

private:
    std::uint64_t state_ = 0;
    std::uint64_t increment_ = 0;
};

} // namespace lemuria::core
