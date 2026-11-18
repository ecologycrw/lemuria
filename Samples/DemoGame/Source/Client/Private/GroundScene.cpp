#include "DemoClient/GroundScene.hpp"

#include <algorithm>

#include "DemoClient/DemoStyle.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Core/SeededRandom.hpp"

namespace demo_client {

namespace {

using demo_protocol::DemoMap;
using demo_protocol::TileKind;
using lemuria::core::Color;
using lemuria::core::ShapeInstance;

constexpr std::uint64_t kBrightnessSeed = 7;
constexpr float kLowestBrightness = 0.92F;
constexpr float kBrightnessRange = 0.16F;

[[nodiscard]] Color scaled(Color color, float brightness) noexcept {
    return {std::min(color.r * brightness, 1.0F), std::min(color.g * brightness, 1.0F),
            std::min(color.b * brightness, 1.0F), color.a};
}

} // namespace

Color tile_color(TileKind kind) noexcept {
    switch (kind) {
    case TileKind::DeepShadow:
        return lemuria::core::color_from_rgb(kDeepShadowRgb);
    case TileKind::Earth:
        return lemuria::core::color_from_rgb(kShadowedEarthRgb);
    case TileKind::Moss:
        return lemuria::core::color_from_rgb(kMossRgb);
    case TileKind::Foliage:
        return lemuria::core::color_from_rgb(kLitFoliageRgb);
    case TileKind::Trail:
        return lemuria::core::color_from_rgb(kTrailRgb);
    case TileKind::Stone:
        return lemuria::core::color_from_rgb(kStoneRgb);
    }
    return lemuria::core::color_from_rgb(kDeepShadowRgb);
}

void add_ground(const DemoMap& map, lemuria::core::SceneDescription& scene) {
    lemuria::core::SeededRandom random(kBrightnessSeed);
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            const TileKind kind = map.kind_at(x, z);
            const float brightness = kLowestBrightness + random.next_unit() * kBrightnessRange;
            const Color color = scaled(tile_color(kind), brightness);
            const auto center = lemuria::classic::tile_center({x, z});

            static_cast<void>(scene.ground_tiles.add(ShapeInstance{center, {1.0F, 0.0F, 1.0F}, color}));
            if (kind == TileKind::Stone) {
                static_cast<void>(
                    scene.boxes.add(ShapeInstance{center, {kRockWidth, kRockHeight, kRockWidth}, color}));
            }
        }
    }
}

void add_character(lemuria::core::Vec3 position, std::uint32_t rgb, lemuria::core::SceneDescription& scene) {
    static_cast<void>(scene.boxes.add(ShapeInstance{
        position, {kCharacterWidth, kCharacterHeight, kCharacterWidth}, lemuria::core::color_from_rgb(rgb)}));
}

} // namespace demo_client
