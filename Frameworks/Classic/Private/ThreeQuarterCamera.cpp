#include "Lemuria/Classic/ThreeQuarterCamera.hpp"

#include <cmath>
#include <numbers>

namespace lemuria::classic {

namespace {

// Beyond this distance a ground point is near the horizon and is not a tile anyone can click.
constexpr float kFarthestTile = 1'000'000.0F;

[[nodiscard]] float radians(float degrees) noexcept {
    return degrees * std::numbers::pi_v<float> / 180.0F;
}

} // namespace

ThreeQuarterCamera::ThreeQuarterCamera(const CameraSettings& settings) noexcept : settings_(settings) {}

void ThreeQuarterCamera::focus_on(core::Vec3 focus, core::Vec2 screen_size) noexcept {
    const float pitch = radians(settings_.pitch_degrees);
    const float yaw = radians(settings_.yaw_degrees);
    const float horizontal_distance = std::cos(pitch) * settings_.distance;

    screen_size_ = screen_size;
    eye_ = focus + core::Vec3{std::sin(yaw) * horizontal_distance, std::sin(pitch) * settings_.distance,
                              std::cos(yaw) * horizontal_distance};

    const float aspect_ratio = screen_size.y > 0.0F ? screen_size.x / screen_size.y : 1.0F;
    const core::Mat4 view = core::look_at(eye_, focus, {0.0F, 1.0F, 0.0F});
    const core::Mat4 projection =
        core::perspective(radians(settings_.vertical_field_of_view_degrees), aspect_ratio,
                          settings_.near_distance, settings_.far_distance);
    view_projection_ = projection * view;
    inverse_view_projection_ = core::inverse(view_projection_);
}

std::optional<TileCoord> ThreeQuarterCamera::tile_under(core::Vec2 screen_position) const noexcept {
    if (!inverse_view_projection_) {
        return std::nullopt;
    }
    const auto ray = core::screen_ray(*inverse_view_projection_, screen_position, screen_size_);
    if (!ray) {
        return std::nullopt;
    }
    const auto ground = core::intersect_horizontal_plane(*ray, 0.0F);
    if (!ground || std::abs(ground->x) > kFarthestTile || std::abs(ground->z) > kFarthestTile) {
        return std::nullopt;
    }
    return TileCoord{static_cast<int>(std::floor(ground->x)), static_cast<int>(std::floor(ground->z))};
}

} // namespace lemuria::classic
