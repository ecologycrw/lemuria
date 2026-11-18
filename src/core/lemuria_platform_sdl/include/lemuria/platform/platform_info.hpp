#pragma once

#include <string>

#include "lemuria/core/result.hpp"
#include "lemuria/core/version.hpp"

namespace lemuria::platform {

// The version of the SDL library in use.
[[nodiscard]] core::Version sdl_version() noexcept;

// Opens the GPU, without any window, and returns the name of the graphics driver in use.
[[nodiscard]] core::Result<std::string> probe_gpu_driver();

} // namespace lemuria::platform
