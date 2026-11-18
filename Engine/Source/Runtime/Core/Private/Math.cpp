#include "Lemuria/Core/Math.hpp"

#include <cmath>

namespace lemuria::core {

namespace {

constexpr float kSmallestDeterminant = 1.0e-18F;
constexpr float kSmallestDivisor = 1.0e-7F;

[[nodiscard]] bool has_area(Vec2 screen_size) noexcept {
    return screen_size.x > 0.0F && screen_size.y > 0.0F;
}

// Converts a point from clip space back to world space.
[[nodiscard]] std::optional<Vec3> unproject(const Mat4& inverse_view_projection, Vec4 clip_point) noexcept {
    const Vec4 world = inverse_view_projection * clip_point;
    if (std::abs(world.w) < kSmallestDivisor) {
        return std::nullopt;
    }
    return Vec3{world.x / world.w, world.y / world.w, world.z / world.w};
}

} // namespace

float length(Vec3 v) noexcept {
    return std::sqrt(dot(v, v));
}

Vec3 normalized(Vec3 v) noexcept {
    const float size = length(v);
    if (size < kSmallestDivisor) {
        return {};
    }
    return v * (1.0F / size);
}

Mat4 operator*(const Mat4& a, const Mat4& b) noexcept {
    Mat4 result;
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            float sum = 0.0F;
            for (std::size_t index = 0; index < 4; ++index) {
                sum += a.at(index, row) * b.at(column, index);
            }
            result.at(column, row) = sum;
        }
    }
    return result;
}

Vec4 operator*(const Mat4& matrix, Vec4 v) noexcept {
    const auto row = [&](std::size_t index) {
        return matrix.at(0, index) * v.x + matrix.at(1, index) * v.y + matrix.at(2, index) * v.z +
               matrix.at(3, index) * v.w;
    };
    return {row(0), row(1), row(2), row(3)};
}

std::optional<Mat4> inverse(const Mat4& matrix) noexcept {
    const auto a = [&](std::size_t row, std::size_t column) { return matrix.at(column, row); };

    const float s0 = a(0, 0) * a(1, 1) - a(1, 0) * a(0, 1);
    const float s1 = a(0, 0) * a(1, 2) - a(1, 0) * a(0, 2);
    const float s2 = a(0, 0) * a(1, 3) - a(1, 0) * a(0, 3);
    const float s3 = a(0, 1) * a(1, 2) - a(1, 1) * a(0, 2);
    const float s4 = a(0, 1) * a(1, 3) - a(1, 1) * a(0, 3);
    const float s5 = a(0, 2) * a(1, 3) - a(1, 2) * a(0, 3);

    const float c0 = a(2, 0) * a(3, 1) - a(3, 0) * a(2, 1);
    const float c1 = a(2, 0) * a(3, 2) - a(3, 0) * a(2, 2);
    const float c2 = a(2, 0) * a(3, 3) - a(3, 0) * a(2, 3);
    const float c3 = a(2, 1) * a(3, 2) - a(3, 1) * a(2, 2);
    const float c4 = a(2, 1) * a(3, 3) - a(3, 1) * a(2, 3);
    const float c5 = a(2, 2) * a(3, 3) - a(3, 2) * a(2, 3);

    const float determinant = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    if (std::abs(determinant) < kSmallestDeterminant) {
        return std::nullopt;
    }

    Mat4 result;
    const auto set = [&](std::size_t row, std::size_t column, float cofactor) {
        result.at(column, row) = cofactor / determinant;
    };
    set(0, 0, a(1, 1) * c5 - a(1, 2) * c4 + a(1, 3) * c3);
    set(0, 1, -a(0, 1) * c5 + a(0, 2) * c4 - a(0, 3) * c3);
    set(0, 2, a(3, 1) * s5 - a(3, 2) * s4 + a(3, 3) * s3);
    set(0, 3, -a(2, 1) * s5 + a(2, 2) * s4 - a(2, 3) * s3);
    set(1, 0, -a(1, 0) * c5 + a(1, 2) * c2 - a(1, 3) * c1);
    set(1, 1, a(0, 0) * c5 - a(0, 2) * c2 + a(0, 3) * c1);
    set(1, 2, -a(3, 0) * s5 + a(3, 2) * s2 - a(3, 3) * s1);
    set(1, 3, a(2, 0) * s5 - a(2, 2) * s2 + a(2, 3) * s1);
    set(2, 0, a(1, 0) * c4 - a(1, 1) * c2 + a(1, 3) * c0);
    set(2, 1, -a(0, 0) * c4 + a(0, 1) * c2 - a(0, 3) * c0);
    set(2, 2, a(3, 0) * s4 - a(3, 1) * s2 + a(3, 3) * s0);
    set(2, 3, -a(2, 0) * s4 + a(2, 1) * s2 - a(2, 3) * s0);
    set(3, 0, -a(1, 0) * c3 + a(1, 1) * c1 - a(1, 2) * c0);
    set(3, 1, a(0, 0) * c3 - a(0, 1) * c1 + a(0, 2) * c0);
    set(3, 2, -a(3, 0) * s3 + a(3, 1) * s1 - a(3, 2) * s0);
    set(3, 3, a(2, 0) * s3 - a(2, 1) * s1 + a(2, 2) * s0);
    return result;
}

Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) noexcept {
    const Vec3 forward = normalized(target - eye);
    const Vec3 right = normalized(cross(forward, up));
    const Vec3 camera_up = cross(right, forward);

    Mat4 result = Mat4::identity();
    result.at(0, 0) = right.x;
    result.at(1, 0) = right.y;
    result.at(2, 0) = right.z;
    result.at(3, 0) = -dot(right, eye);
    result.at(0, 1) = camera_up.x;
    result.at(1, 1) = camera_up.y;
    result.at(2, 1) = camera_up.z;
    result.at(3, 1) = -dot(camera_up, eye);
    result.at(0, 2) = -forward.x;
    result.at(1, 2) = -forward.y;
    result.at(2, 2) = -forward.z;
    result.at(3, 2) = dot(forward, eye);
    return result;
}

Mat4 perspective(float vertical_field_of_view_radians, float aspect_ratio, float near_distance,
                 float far_distance) noexcept {
    const float focal_length = 1.0F / std::tan(vertical_field_of_view_radians * 0.5F);

    Mat4 result;
    result.at(0, 0) = focal_length / aspect_ratio;
    result.at(1, 1) = focal_length;
    result.at(2, 2) = far_distance / (near_distance - far_distance);
    result.at(2, 3) = -1.0F;
    result.at(3, 2) = near_distance * far_distance / (near_distance - far_distance);
    return result;
}

std::optional<Vec2> project_to_screen(const Mat4& view_projection, Vec3 point, Vec2 screen_size) noexcept {
    if (!has_area(screen_size)) {
        return std::nullopt;
    }
    const Vec4 clip = view_projection * Vec4{point.x, point.y, point.z, 1.0F};
    if (clip.w < kSmallestDivisor) {
        return std::nullopt;
    }
    return Vec2{(clip.x / clip.w + 1.0F) * 0.5F * screen_size.x,
                (1.0F - clip.y / clip.w) * 0.5F * screen_size.y};
}

std::optional<Ray> screen_ray(const Mat4& inverse_view_projection, Vec2 screen_position,
                              Vec2 screen_size) noexcept {
    if (!has_area(screen_size)) {
        return std::nullopt;
    }
    const float clip_x = 2.0F * screen_position.x / screen_size.x - 1.0F;
    const float clip_y = 1.0F - 2.0F * screen_position.y / screen_size.y;

    const auto near_point = unproject(inverse_view_projection, {clip_x, clip_y, 0.0F, 1.0F});
    const auto far_point = unproject(inverse_view_projection, {clip_x, clip_y, 1.0F, 1.0F});
    if (!near_point || !far_point) {
        return std::nullopt;
    }
    return Ray{*near_point, normalized(*far_point - *near_point)};
}

std::optional<Vec3> intersect_horizontal_plane(const Ray& ray, float height) noexcept {
    if (std::abs(ray.direction.y) < kSmallestDivisor) {
        return std::nullopt;
    }
    const float distance = (height - ray.origin.y) / ray.direction.y;
    if (distance < 0.0F) {
        return std::nullopt;
    }
    return ray.origin + ray.direction * distance;
}

} // namespace lemuria::core
