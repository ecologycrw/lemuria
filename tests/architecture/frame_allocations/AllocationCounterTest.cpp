#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "AllocationCounter.hpp"

using lemuria::frame_allocations::allocations_during;

namespace {

// Fills its whole alignment, so the compiler has nothing to pad. Asking for it uses the aligned `new`.
struct alignas(64) WideAligned {
    std::array<std::uint8_t, 64> bytes{};
};

} // namespace

TEST_CASE("the counter sees every way the C++ code asks for memory") {
    CHECK(allocations_during([] { auto number = std::make_unique<int>(1); }) == 1);
    CHECK(allocations_during([] { auto numbers = std::make_unique<int[]>(4); }) == 1);
    CHECK(allocations_during([] { auto aligned = std::make_unique<WideAligned>(); }) == 1);
    CHECK(allocations_during([] { auto shared = std::make_shared<int>(1); }) == 1);

    // In the development build of MSVC, every container also asks for a small block that checks its
    // iterators, so a container may count more than once.
    CHECK(allocations_during([] { std::vector<int> numbers(10); }) >= 1);
    CHECK(allocations_during([] { const std::string text(100, 'a'); }) >= 1);
}

TEST_CASE("the counter sees nothing when the work asks for no memory") {
    std::vector<int> numbers;
    numbers.reserve(16);

    CHECK(allocations_during([&] {
              for (int index = 0; index < 16; ++index) {
                  numbers.push_back(index);
              }
              numbers.clear();
          }) == 0);
}

TEST_CASE("the counter counts only the thread that measures") {
    std::atomic<int> step{0};
    std::thread other([&] {
        while (step.load() != 1) {
            std::this_thread::yield();
        }
        const std::vector<int> numbers(100);
        step.store(2);
    });

    const std::uint64_t counted = allocations_during([&] {
        step.store(1);
        while (step.load() != 2) {
            std::this_thread::yield();
        }
    });
    other.join();

    CHECK(counted == 0);
}

TEST_CASE("the counter starts from zero each time") {
    CHECK(allocations_during([] { auto number = std::make_unique<int>(1); }) == 1);
    CHECK(allocations_during([] {}) == 0);
}
