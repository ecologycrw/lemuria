#pragma once

#include <string>

#include "Lemuria/Core/Result.hpp"
#include "Lemuria/Platform/Image.hpp"

namespace demo_client {

struct ProofImage {
    lemuria::platform::Image image;

    // The name of the graphics API that drew the image.
    std::string graphics_api;
};

// Draws the proof scene on an image in memory. An empty graphics API lets the system choose.
[[nodiscard]] lemuria::core::Result<ProofImage> render_proof_image(std::string graphics_api);

} // namespace demo_client
