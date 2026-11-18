#include "Lemuria/Core/Version.hpp"

#include <charconv>
#include <cstddef>
#include <string>
#include <system_error>

namespace lemuria::core {

namespace {

// Reads the number at the start of the text and removes it from the text.
[[nodiscard]] Result<std::uint32_t> take_number(std::string_view& text) {
    std::uint32_t number = 0;
    const char* const begin = text.data();
    const auto [next, error] = std::from_chars(begin, begin + text.size(), number);
    if (error != std::errc{}) {
        return make_error("expected a number in the version, found '" + std::string(text) + "'");
    }
    text.remove_prefix(static_cast<std::size_t>(next - begin));
    return number;
}

// Removes the dot that separates two numbers.
[[nodiscard]] Result<void> take_dot(std::string_view& text) {
    if (!text.starts_with('.')) {
        return make_error("expected a dot in the version, found '" + std::string(text) + "'");
    }
    text.remove_prefix(1);
    return {};
}

} // namespace

Version engine_version() noexcept {
    return Version{LEMURIA_ENGINE_VERSION_MAJOR, LEMURIA_ENGINE_VERSION_MINOR, LEMURIA_ENGINE_VERSION_PATCH};
}

Result<Version> parse_version(std::string_view text) {
    Version version;
    for (std::uint32_t* const part : {&version.major, &version.minor, &version.patch}) {
        if (part != &version.major) {
            if (auto dot = take_dot(text); !dot) {
                return tl::unexpected(dot.error());
            }
        }
        auto number = take_number(text);
        if (!number) {
            return tl::unexpected(number.error());
        }
        *part = *number;
    }
    if (!text.empty()) {
        return make_error("unexpected text after the version: '" + std::string(text) + "'");
    }
    return version;
}

} // namespace lemuria::core
