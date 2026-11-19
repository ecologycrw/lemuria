#include "Lemuria/Platform/Image.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>

#include <SDL3/SDL.h>

namespace lemuria::platform {

namespace {

constexpr std::size_t kBytesPerPixel = 4;

struct SurfaceDeleter {
    void operator()(SDL_Surface* surface) const noexcept { SDL_DestroySurface(surface); }
};

[[nodiscard]] bool fits_in_int(std::uint32_t value) noexcept {
    return value <= static_cast<std::uint32_t>(std::numeric_limits<int>::max());
}

[[nodiscard]] bool has_valid_size(const Image& image) noexcept {
    const std::size_t expected = std::size_t{image.width} * image.height * kBytesPerPixel;
    return image.width > 0 && image.height > 0 && fits_in_int(image.width) && fits_in_int(image.height) &&
           image.pixels.size() == expected;
}

} // namespace

core::Result<void> save_png(const Image& image, const std::string& path) {
    if (!has_valid_size(image)) {
        return core::make_error("the image size does not match its pixels");
    }

    const std::unique_ptr<SDL_Surface, SurfaceDeleter> surface(SDL_CreateSurface(
        static_cast<int>(image.width), static_cast<int>(image.height), SDL_PIXELFORMAT_RGBA32));
    if (!surface) {
        return core::make_error(std::string("could not prepare the image: ") + SDL_GetError());
    }

    const std::size_t row_size = std::size_t{image.width} * kBytesPerPixel;
    auto* destination = static_cast<std::uint8_t*>(surface->pixels);
    const std::uint8_t* source = image.pixels.data();
    for (std::uint32_t row = 0; row < image.height; ++row) {
        std::memcpy(destination, source, row_size);
        destination += surface->pitch;
        source += row_size;
    }

    if (!SDL_SavePNG(surface.get(), path.c_str())) {
        return core::make_error("could not save '" + path + "': " + SDL_GetError());
    }
    return {};
}

} // namespace lemuria::platform
