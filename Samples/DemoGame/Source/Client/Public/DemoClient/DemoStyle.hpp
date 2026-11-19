#pragma once

#include <cstdint>

#include "DemoProtocol/DemoMap.hpp"
#include "Lemuria/Classic/ThreeQuarterCamera.hpp"
#include "Lemuria/Core/SceneDescription.hpp"

// The look of the demonstration, taken from docs/VISUAL.md. These values belong to this game, and never to
// the engine (INV-06).
namespace demo_client {

inline constexpr std::uint32_t kDeepShadowRgb = 0x15120AU;
inline constexpr std::uint32_t kShadowedEarthRgb = 0x2B2110U;
inline constexpr std::uint32_t kMossRgb = 0x3A3A16U;
inline constexpr std::uint32_t kLitFoliageRgb = 0x5C5A24U;
inline constexpr std::uint32_t kTrailRgb = 0x6E5630U;
inline constexpr std::uint32_t kStoneRgb = 0x4A4A44U;

// Provisional colors of the simple shapes that stand in for the characters until slice 1.
inline constexpr std::uint32_t kOwnCharacterRgb = 0xE4D8BCU;
inline constexpr std::uint32_t kOtherCharacterRgb = 0xB5533CU;

inline constexpr lemuria::classic::CameraSettings kCameraSettings{
    .pitch_degrees = 50.0F,
    .yaw_degrees = 45.0F,
    .distance = 24.0F,
    .vertical_field_of_view_degrees = 30.0F,
    .near_distance = 1.0F,
    .far_distance = 200.0F,
};

inline constexpr float kCharacterHeight = 1.8F;
inline constexpr float kCharacterWidth = 0.7F;
inline constexpr float kRockHeight = 0.6F;
inline constexpr float kRockWidth = 0.92F;

[[nodiscard]] lemuria::core::Color tile_color(demo_protocol::TileKind kind) noexcept;

} // namespace demo_client
