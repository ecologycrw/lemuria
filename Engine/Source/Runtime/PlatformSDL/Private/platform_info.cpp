#include "lemuria/platform/platform_info.hpp"

#include <cstdint>
#include <memory>

#include <SDL3/SDL.h>

namespace lemuria::platform {

namespace {

struct GpuDeviceDeleter {
    void operator()(SDL_GPUDevice* device) const noexcept { SDL_DestroyGPUDevice(device); }
};

using GpuDevice = std::unique_ptr<SDL_GPUDevice, GpuDeviceDeleter>;

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

} // namespace

core::Version sdl_version() noexcept {
    const int version = SDL_GetVersion();
    return core::Version{static_cast<std::uint32_t>(SDL_VERSIONNUM_MAJOR(version)),
                         static_cast<std::uint32_t>(SDL_VERSIONNUM_MINOR(version)),
                         static_cast<std::uint32_t>(SDL_VERSIONNUM_MICRO(version))};
}

core::Result<std::string> probe_gpu_driver() {
    const VideoSystem video;
    if (!video.ready()) {
        return core::make_error(std::string("could not start the video system: ") + SDL_GetError());
    }
    const GpuDevice device(
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr));
    if (!device) {
        return core::make_error(std::string("could not open the GPU: ") + SDL_GetError());
    }
    return std::string(SDL_GetGPUDeviceDriver(device.get()));
}

} // namespace lemuria::platform
