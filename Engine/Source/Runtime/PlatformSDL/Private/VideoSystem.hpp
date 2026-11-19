#pragma once

#include <SDL3/SDL.h>

namespace lemuria::platform {

// Keeps the SDL video system open while it exists.
class VideoSystem {
public:
    VideoSystem() : ready_(SDL_InitSubSystem(SDL_INIT_VIDEO)) {}
    ~VideoSystem() {
        if (ready_) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }
    }
    VideoSystem(const VideoSystem&) = delete;
    VideoSystem& operator=(const VideoSystem&) = delete;
    VideoSystem(VideoSystem&&) = delete;
    VideoSystem& operator=(VideoSystem&&) = delete;

    [[nodiscard]] bool ready() const noexcept { return ready_; }

private:
    bool ready_;
};

} // namespace lemuria::platform
