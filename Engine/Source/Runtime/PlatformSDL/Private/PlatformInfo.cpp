#include "Lemuria/Platform/PlatformInfo.hpp"

#include <cstdint>
#include <memory>

#include <SDL3/SDL.h>

#include "VideoSystem.hpp"

namespace lemuria::platform {

namespace {

struct GpuDeviceDeleter {
    void operator()(SDL_GPUDevice* device) const noexcept { SDL_DestroyGPUDevice(device); }
};

using GpuDevice = std::unique_ptr<SDL_GPUDevice, GpuDeviceDeleter>;

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
