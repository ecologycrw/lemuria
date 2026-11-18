#pragma once

#include <cstdint>

namespace lemuria::platform {

// A reading of the real clock, which never goes backwards. Only the program's main loop should read it:
// everything else receives the time from the outside (INV-05).
[[nodiscard]] std::uint64_t monotonic_nanoseconds() noexcept;

} // namespace lemuria::platform
