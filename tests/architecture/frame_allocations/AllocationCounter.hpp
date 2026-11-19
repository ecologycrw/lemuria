#pragma once

#include <cstdint>

// Counts the memory the C++ code asks for with `new`, which every container and smart pointer uses (INV-13).
//
// Only the calling thread is counted. The platform layer moves bytes on a thread of its own, which is not the
// frame. Memory that C libraries ask for directly, such as the SDL or the GPU driver, is not seen here.
namespace lemuria::frame_allocations {

void start_counting() noexcept;

// Returns how many times memory was asked for since `start_counting`, on this thread.
[[nodiscard]] std::uint64_t stop_counting() noexcept;

template <typename Work>
[[nodiscard]] std::uint64_t allocations_during(Work&& work) {
    start_counting();
    work();
    return stop_counting();
}

} // namespace lemuria::frame_allocations
