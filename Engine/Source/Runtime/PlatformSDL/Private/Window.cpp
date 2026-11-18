#include "Lemuria/Platform/Window.hpp"

#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "WindowImpl.hpp"

namespace lemuria::platform {

void InputFrame::begin_frame() noexcept {
    quit_requested = false;
    click_count_ = 0;
}

bool InputFrame::add_click(core::Vec2 position) noexcept {
    if (click_count_ == clicks_.size()) {
        return false;
    }
    clicks_[click_count_] = position;
    ++click_count_;
    return true;
}

std::span<const core::Vec2> InputFrame::clicks() const noexcept {
    return std::span<const core::Vec2>(clicks_).first(click_count_);
}

Window::Window(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Window::Window(Window&& other) noexcept = default;
Window& Window::operator=(Window&& other) noexcept = default;
Window::~Window() = default;

core::Result<Window> Window::open(const WindowOptions& options) {
    auto impl = std::make_unique<Impl>();
    if (!impl->video.ready()) {
        return core::make_error(std::string("could not start the video system: ") + SDL_GetError());
    }

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (options.hidden) {
        flags |= SDL_WINDOW_HIDDEN;
    }
    impl->window.reset(SDL_CreateWindow(options.title.c_str(), options.width, options.height, flags));
    if (!impl->window) {
        return core::make_error(std::string("could not open the window: ") + SDL_GetError());
    }
    return Window(std::move(impl));
}

void Window::poll_input(InputFrame& input) noexcept {
    input.begin_frame();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            input.quit_requested = true;
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            input.add_click({event.button.x, event.button.y});
        }
    }
    input.window_size = size();
}

core::Result<void> Window::set_title(const std::string& title) {
    if (!SDL_SetWindowTitle(impl_->window.get(), title.c_str())) {
        return core::make_error(std::string("could not change the window title: ") + SDL_GetError());
    }
    return {};
}

core::Vec2 Window::size() const noexcept {
    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(impl_->window.get(), &width, &height)) {
        return {};
    }
    return {static_cast<float>(width), static_cast<float>(height)};
}

} // namespace lemuria::platform
