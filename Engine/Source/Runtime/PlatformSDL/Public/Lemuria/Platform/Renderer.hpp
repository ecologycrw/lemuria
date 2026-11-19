#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "Lemuria/Core/Result.hpp"
#include "Lemuria/Core/SceneDescription.hpp"
#include "Lemuria/Platform/Image.hpp"
#include "Lemuria/Platform/Window.hpp"

namespace lemuria::platform {

struct RendererOptions {
    std::size_t max_ground_tiles = 8192;
    std::size_t max_boxes = 256;

    // Waits for the display between frames. Turn it off to measure how long a frame really takes.
    bool wait_for_display = true;

    // The graphics API to use: "vulkan" or "direct3d12". Empty lets the system choose.
    std::string graphics_api;
};

// Draws scene descriptions with the GPU, on a window or on an image in memory.
class Renderer {
public:
    // The window must stay open for as long as the renderer exists.
    [[nodiscard]] static core::Result<Renderer> for_window(Window& window, const RendererOptions& options);

    [[nodiscard]] static core::Result<Renderer> offscreen(std::uint32_t width, std::uint32_t height,
                                                          const RendererOptions& options);

    Renderer(Renderer&& other) noexcept;
    Renderer& operator=(Renderer&& other) noexcept;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer();

    // Draws one frame. The scene is only read (INV-03).
    [[nodiscard]] core::Result<void> draw(const core::SceneDescription& scene);

    // Copies the last frame of an offscreen renderer to memory.
    [[nodiscard]] core::Result<Image> read_image();

    // How many frames had no image to draw on, as happens while the window is minimized.
    [[nodiscard]] std::uint64_t skipped_frame_count() const noexcept;

    // The name of the graphics API in use.
    [[nodiscard]] std::string_view graphics_api() const noexcept;

private:
    struct Impl;

    explicit Renderer(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;
};

} // namespace lemuria::platform
