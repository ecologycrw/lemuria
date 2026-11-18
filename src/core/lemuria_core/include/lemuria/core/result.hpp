#pragma once

#include <string>
#include <utility>

#include <tl/expected.hpp>

namespace lemuria::core {

// Why an operation failed, in words a developer can act on.
struct Error {
    std::string message;
};

// The return type of every operation that can fail.
template <typename Value>
using Result = tl::expected<Value, Error>;

[[nodiscard]] inline tl::unexpected<Error> make_error(std::string message) {
    return tl::unexpected<Error>(Error{std::move(message)});
}

} // namespace lemuria::core
