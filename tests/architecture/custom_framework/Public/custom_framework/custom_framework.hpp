#pragma once

#include <string_view>

#include "lemuria/core/result.hpp"
#include "lemuria/core/version.hpp"

namespace custom_framework {

// A framework written by a developer: it only knows the public interface of the Core.
[[nodiscard]] lemuria::core::Result<lemuria::core::Version> read_required_engine(std::string_view text);

} // namespace custom_framework
