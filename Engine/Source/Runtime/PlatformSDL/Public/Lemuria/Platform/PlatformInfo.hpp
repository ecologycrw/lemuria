#pragma once

#include <string>

#include "Lemuria/Core/Result.hpp"
#include "Lemuria/Core/Version.hpp"

namespace lemuria::platform {

// The version of the SDL library in use.
[[nodiscard]] core::Version sdl_version() noexcept;

// Opens the GPU, without any window, and returns the name of the graphics driver in use.
[[nodiscard]] core::Result<std::string> probe_gpu_driver();

} // namespace lemuria::platform
