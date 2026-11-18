#pragma once

#include <compare>
#include <cstdint>
#include <string_view>

#include "Lemuria/Core/Result.hpp"

namespace lemuria::core {

struct Version {
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t patch = 0;

    friend auto operator<=>(const Version&, const Version&) = default;
};

// The version of the engine that was compiled.
[[nodiscard]] Version engine_version() noexcept;

// Reads a version written as "major.minor.patch". The text is treated as untrusted input (INV-12).
[[nodiscard]] Result<Version> parse_version(std::string_view text);

} // namespace lemuria::core
