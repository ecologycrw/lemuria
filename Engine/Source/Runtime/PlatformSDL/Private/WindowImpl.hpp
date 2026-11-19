#pragma once

#include <memory>

#include <SDL3/SDL.h>

#include "Lemuria/Platform/Window.hpp"
#include "VideoSystem.hpp"

namespace lemuria::platform {

struct SdlWindowDeleter {
    void operator()(SDL_Window* window) const noexcept { SDL_DestroyWindow(window); }
};

// The video system comes first so it is closed after the window.
struct Window::Impl {
    VideoSystem video;
    std::unique_ptr<SDL_Window, SdlWindowDeleter> window;
};

} // namespace lemuria::platform
