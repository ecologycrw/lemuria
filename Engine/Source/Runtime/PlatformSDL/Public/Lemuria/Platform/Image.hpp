#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Lemuria/Core/Result.hpp"

namespace lemuria::platform {

// An image in memory: four bytes per pixel (red, green, blue, alpha), row by row from the top.
struct Image {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> pixels;
};

// Saves the image as a PNG file.
[[nodiscard]] core::Result<void> save_png(const Image& image, const std::string& path);

} // namespace lemuria::platform
