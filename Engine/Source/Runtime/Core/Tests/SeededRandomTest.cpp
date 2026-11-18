#include <array>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Core/SeededRandom.hpp"

using lemuria::core::SeededRandom;

TEST_CASE("the sequence of a seed never changes") {
    // Reference values published with the PCG32 algorithm, for seed 42 and stream 54.
    constexpr std::array<std::uint32_t, 6> kExpected{0xa15c02b7U, 0x7b47f409U, 0xba1d3330U,
                                                     0x83d2f293U, 0xbfa4784bU, 0xcbed606eU};
    SeededRandom random(42);

    for (const std::uint32_t expected : kExpected) {
        CHECK(random.next() == expected);
    }
}

TEST_CASE("different seeds give different sequences") {
    SeededRandom first(1);
    SeededRandom second(2);

    CHECK(first.next() != second.next());
}

TEST_CASE("a limited draw stays below the limit and reaches every value") {
    SeededRandom random(7);
    std::array<int, 5> counts{};

    for (int draw = 0; draw < 1000; ++draw) {
        const std::uint32_t value = random.next_below(5);
        REQUIRE(value < 5);
        ++counts.at(value);
    }

    for (const int count : counts) {
        CHECK(count > 150);
        CHECK(count < 250);
    }
}

TEST_CASE("a limit of zero gives zero") {
    SeededRandom random(7);

    CHECK(random.next_below(0) == 0);
}

TEST_CASE("a unit draw stays from zero up to, but not including, one") {
    SeededRandom random(11);

    for (int draw = 0; draw < 1000; ++draw) {
        const float value = random.next_unit();
        REQUIRE(value >= 0.0F);
        REQUIRE(value < 1.0F);
    }
}
