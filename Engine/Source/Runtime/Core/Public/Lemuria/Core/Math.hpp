#pragma once

#include <array>
#include <cstddef>
#include <optional>

// World convention: right-handed, Y points up and the ground is the XZ plane.
// Clip space convention: Y points up and depth goes from 0 (near) to 1 (far).
namespace lemuria::core {

struct Vec2 {
    float x = 0.0F;
    float y = 0.0F;
};

struct Vec3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct Vec4 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 0.0F;
};

[[nodiscard]] constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

[[nodiscard]] constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

[[nodiscard]] constexpr Vec3 operator*(Vec3 v, float scale) noexcept {
    return {v.x * scale, v.y * scale, v.z * scale};
}

[[nodiscard]] constexpr float dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] constexpr Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

[[nodiscard]] float length(Vec3 v) noexcept;

// A vector of length 1 in the same direction. The zero vector stays zero.
[[nodiscard]] Vec3 normalized(Vec3 v) noexcept;

// A 4x4 matrix stored column by column, the layout the shaders read.
struct Mat4 {
    std::array<float, 16> values{};

    [[nodiscard]] constexpr float at(std::size_t column, std::size_t row) const noexcept {
        return values[column * 4 + row];
    }

    [[nodiscard]] constexpr float& at(std::size_t column, std::size_t row) noexcept {
        return values[column * 4 + row];
    }

    [[nodiscard]] static constexpr Mat4 identity() noexcept {
        Mat4 result;
        for (std::size_t index = 0; index < 4; ++index) {
            result.at(index, index) = 1.0F;
        }
        return result;
    }
};

[[nodiscard]] Mat4 operator*(const Mat4& a, const Mat4& b) noexcept;
[[nodiscard]] Vec4 operator*(const Mat4& matrix, Vec4 v) noexcept;

// The matrix that undoes this one, or nothing when the matrix cannot be undone.
[[nodiscard]] std::optional<Mat4> inverse(const Mat4& matrix) noexcept;

// A camera at `eye` looking at `target`.
[[nodiscard]] Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) noexcept;

[[nodiscard]] Mat4 perspective(float vertical_field_of_view_radians, float aspect_ratio, float near_distance,
                               float far_distance) noexcept;

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

// Where a world point lands on the screen, in pixels from the top-left corner.
// Nothing when the point is behind the camera or the screen has no area.
[[nodiscard]] std::optional<Vec2> project_to_screen(const Mat4& view_projection, Vec3 point,
                                                    Vec2 screen_size) noexcept;

// The line of sight that passes through a screen position. Nothing when the screen has no area.
[[nodiscard]] std::optional<Ray> screen_ray(const Mat4& inverse_view_projection, Vec2 screen_position,
                                            Vec2 screen_size) noexcept;

// Where a ray meets the horizontal plane at `height`. Nothing when it never does.
[[nodiscard]] std::optional<Vec3> intersect_horizontal_plane(const Ray& ray, float height) noexcept;

} // namespace lemuria::core
