#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string>

#include "Lemuria/Core/Math.hpp"
#include "Lemuria/Core/Result.hpp"

namespace lemuria::platform {

// What the player did since the previous frame. It has a fixed size, so reading input never allocates
// (INV-13).
class InputFrame {
public:
    static constexpr std::size_t kMaxClicks = 8;

    // Forgets the previous frame.
    void begin_frame() noexcept;

    // Returns false when the frame already holds as many clicks as it can.
    bool add_click(core::Vec2 position) noexcept;

    [[nodiscard]] std::span<const core::Vec2> clicks() const noexcept;

    bool quit_requested = false;
    core::Vec2 window_size;

private:
    std::array<core::Vec2, kMaxClicks> clicks_{};
    std::size_t click_count_ = 0;
};

struct WindowOptions {
    std::string title;
    int width = 1280;
    int height = 720;
    bool hidden = false;
};

// The window of the game.
class Window {
public:
    [[nodiscard]] static core::Result<Window> open(const WindowOptions& options);

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window();

    // Collects what happened since the previous call. It never waits.
    void poll_input(InputFrame& input) noexcept;

    [[nodiscard]] core::Result<void> set_title(const std::string& title);

    // The size of the drawing area, in pixels.
    [[nodiscard]] core::Vec2 size() const noexcept;

private:
    friend class Renderer;
    struct Impl;

    explicit Window(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> impl_;
};

} // namespace lemuria::platform
