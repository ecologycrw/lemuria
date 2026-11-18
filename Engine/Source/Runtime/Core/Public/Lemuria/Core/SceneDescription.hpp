#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Lemuria/Core/Math.hpp"

namespace lemuria::core {

// A color as it appears on the screen, with each channel from 0 to 1.
struct Color {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    float a = 1.0F;
};

// Reads a color written as 0xRRGGBB.
[[nodiscard]] constexpr Color color_from_rgb(std::uint32_t rgb) noexcept {
    constexpr float kChannelScale = 1.0F / 255.0F;
    return {static_cast<float>((rgb >> 16U) & 0xFFU) * kChannelScale,
            static_cast<float>((rgb >> 8U) & 0xFFU) * kChannelScale,
            static_cast<float>(rgb & 0xFFU) * kChannelScale, 1.0F};
}

// One shape to draw. The position is the center of its base.
struct ShapeInstance {
    Vec3 position;
    Vec3 size;
    Color color;
};

// A list of shapes with a fixed budget (INV-14). Adding never allocates memory (INV-13): what does not fit
// is left out and counted.
class ShapeList {
public:
    explicit ShapeList(std::size_t capacity);

    // Returns false when the budget is already used up.
    bool add(const ShapeInstance& shape) noexcept;

    // Empties the list and keeps its memory for the next frame.
    void clear() noexcept;

    [[nodiscard]] std::span<const ShapeInstance> shapes() const noexcept;
    [[nodiscard]] std::size_t capacity() const noexcept { return storage_.size(); }
    [[nodiscard]] std::size_t dropped_count() const noexcept { return dropped_count_; }

private:
    std::vector<ShapeInstance> storage_;
    std::size_t count_ = 0;
    std::size_t dropped_count_ = 0;
};

// Everything needed to draw one frame. It is only data: drawing reads it and never changes the game (INV-03).
struct SceneDescription {
    SceneDescription(std::size_t ground_tile_capacity, std::size_t box_capacity);

    // Empties the shape lists and keeps their memory for the next frame.
    void clear() noexcept;

    Mat4 view_projection = Mat4::identity();
    Color background;
    ShapeList ground_tiles;
    ShapeList boxes;
};

} // namespace lemuria::core
