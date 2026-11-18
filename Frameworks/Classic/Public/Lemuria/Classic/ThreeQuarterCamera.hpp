#pragma once

#include <optional>

#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Core/Math.hpp"

namespace lemuria::classic {

// How the camera looks at the world. Every game chooses its own values.
struct CameraSettings {
    float pitch_degrees = 0.0F;
    float yaw_degrees = 0.0F;
    float distance = 0.0F;
    float vertical_field_of_view_degrees = 0.0F;
    float near_distance = 0.0F;
    float far_distance = 0.0F;
};

// A high camera that looks down at a point of the ground, as in classic online role-playing games.
class ThreeQuarterCamera {
public:
    explicit ThreeQuarterCamera(const CameraSettings& settings) noexcept;

    // Points the camera at `focus`, for a screen of the given size.
    void focus_on(core::Vec3 focus, core::Vec2 screen_size) noexcept;

    [[nodiscard]] const core::Mat4& view_projection() const noexcept { return view_projection_; }
    [[nodiscard]] core::Vec3 eye() const noexcept { return eye_; }

    // The tile seen at a screen position, in pixels from the top-left corner. Nothing when that position
    // does not show the ground.
    [[nodiscard]] std::optional<TileCoord> tile_under(core::Vec2 screen_position) const noexcept;

private:
    CameraSettings settings_;
    core::Vec2 screen_size_;
    core::Vec3 eye_;
    core::Mat4 view_projection_ = core::Mat4::identity();
    std::optional<core::Mat4> inverse_view_projection_;
};

} // namespace lemuria::classic
