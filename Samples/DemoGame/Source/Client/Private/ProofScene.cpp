#include "DemoClient/ProofScene.hpp"

#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"
#include "Lemuria/Classic/ThreeQuarterCamera.hpp"
#include "Lemuria/Classic/TileCoord.hpp"

namespace demo_client {

namespace {

constexpr lemuria::classic::TileCoord kOwnCharacterTile{32, 32};
constexpr lemuria::classic::TileCoord kOtherCharacterTile{34, 31};

} // namespace

void fill_proof_scene(const demo_protocol::DemoMap& map, lemuria::core::SceneDescription& scene) {
    scene.clear();
    scene.background = lemuria::core::color_from_rgb(kDeepShadowRgb);
    add_ground(map, scene);

    const auto own_position = lemuria::classic::tile_center(kOwnCharacterTile);
    add_character(own_position, kOwnCharacterRgb, scene);
    add_character(lemuria::classic::tile_center(kOtherCharacterTile), kOtherCharacterRgb, scene);

    lemuria::classic::ThreeQuarterCamera camera(kCameraSettings);
    camera.focus_on(own_position,
                    {static_cast<float>(kProofImageWidth), static_cast<float>(kProofImageHeight)});
    scene.view_projection = camera.view_projection();
}

} // namespace demo_client
