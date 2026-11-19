// Replaces the functions behind `new` and `delete` for the whole test program, so every request for memory
// goes through here. This is the only folder where the repository allows it (tools/checks, cpp-rules).
#include "AllocationCounter.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace {

thread_local bool is_counting = false;
thread_local std::uint64_t counted = 0;

void count_one() noexcept {
    if (is_counting) {
        ++counted;
    }
}

[[nodiscard]] void* allocate(std::size_t size) noexcept {
    count_one();
    return std::malloc(size == 0 ? 1 : size);
}

[[nodiscard]] void* allocate_aligned(std::size_t size, std::align_val_t alignment) noexcept {
    count_one();
    const auto bytes = static_cast<std::size_t>(alignment);
#if defined(_MSC_VER)
    return _aligned_malloc(size == 0 ? 1 : size, bytes);
#else
    // The size must be a multiple of the alignment.
    const std::size_t rounded = size == 0 ? bytes : ((size + bytes - 1) / bytes) * bytes;
    return std::aligned_alloc(bytes, rounded);
#endif
}

void release(void* memory) noexcept {
    std::free(memory);
}

void release_aligned(void* memory) noexcept {
#if defined(_MSC_VER)
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}

[[nodiscard]] void* allocate_or_throw(std::size_t size) {
    void* memory = allocate(size);
    if (memory == nullptr) {
        throw std::bad_alloc();
    }
    return memory;
}

[[nodiscard]] void* allocate_aligned_or_throw(std::size_t size, std::align_val_t alignment) {
    void* memory = allocate_aligned(size, alignment);
    if (memory == nullptr) {
        throw std::bad_alloc();
    }
    return memory;
}

} // namespace

namespace lemuria::frame_allocations {

void start_counting() noexcept {
    counted = 0;
    is_counting = true;
}

std::uint64_t stop_counting() noexcept {
    is_counting = false;
    return counted;
}

} // namespace lemuria::frame_allocations

void* operator new(std::size_t size) {
    return allocate_or_throw(size);
}

void* operator new[](std::size_t size) {
    return allocate_or_throw(size);
}

void* operator new(std::size_t size, const std::nothrow_t& /*tag*/) noexcept {
    return allocate(size);
}

void* operator new[](std::size_t size, const std::nothrow_t& /*tag*/) noexcept {
    return allocate(size);
}

void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_aligned_or_throw(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_aligned_or_throw(size, alignment);
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t& /*tag*/) noexcept {
    return allocate_aligned(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t& /*tag*/) noexcept {
    return allocate_aligned(size, alignment);
}

void operator delete(void* memory) noexcept {
    release(memory);
}

void operator delete[](void* memory) noexcept {
    release(memory);
}

void operator delete(void* memory, std::size_t /*size*/) noexcept {
    release(memory);
}

void operator delete[](void* memory, std::size_t /*size*/) noexcept {
    release(memory);
}

void operator delete(void* memory, const std::nothrow_t& /*tag*/) noexcept {
    release(memory);
}

void operator delete[](void* memory, const std::nothrow_t& /*tag*/) noexcept {
    release(memory);
}

void operator delete(void* memory, std::align_val_t /*alignment*/) noexcept {
    release_aligned(memory);
}

void operator delete[](void* memory, std::align_val_t /*alignment*/) noexcept {
    release_aligned(memory);
}

void operator delete(void* memory, std::size_t /*size*/, std::align_val_t /*alignment*/) noexcept {
    release_aligned(memory);
}

void operator delete[](void* memory, std::size_t /*size*/, std::align_val_t /*alignment*/) noexcept {
    release_aligned(memory);
}

void operator delete(void* memory, std::align_val_t /*alignment*/, const std::nothrow_t& /*tag*/) noexcept {
    release_aligned(memory);
}

void operator delete[](void* memory, std::align_val_t /*alignment*/, const std::nothrow_t& /*tag*/) noexcept {
    release_aligned(memory);
}
