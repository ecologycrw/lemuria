#pragma once

#include <string_view>

#include "Lemuria/Core/Result.hpp"
#include "Lemuria/Core/Version.hpp"

namespace custom_framework {

// A framework written by a developer: it only knows the public interface of the Core.
[[nodiscard]] lemuria::core::Result<lemuria::core::Version> read_required_engine(std::string_view text);

} // namespace custom_framework
