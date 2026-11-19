#include "DemoClient/ProofImage.hpp"

#include <utility>

#include "DemoClient/GroundScene.hpp"
#include "DemoClient/ProofScene.hpp"
#include "DemoProtocol/DemoMap.hpp"
#include "Lemuria/Core/SceneDescription.hpp"
#include "Lemuria/Platform/Renderer.hpp"

namespace demo_client {

lemuria::core::Result<ProofImage> render_proof_image(std::string graphics_api) {
    lemuria::platform::RendererOptions options;
    options.max_ground_tiles = kGroundTileBudget;
    options.max_boxes = kBoxBudget;
    options.graphics_api = std::move(graphics_api);

    auto renderer = lemuria::platform::Renderer::offscreen(kProofImageWidth, kProofImageHeight, options);
    if (!renderer) {
        return tl::unexpected(renderer.error());
    }

    const demo_protocol::DemoMap map;
    lemuria::core::SceneDescription scene(kGroundTileBudget, kBoxBudget);
    fill_proof_scene(map, scene);

    const auto drawn = renderer->draw(scene);
    if (!drawn) {
        return tl::unexpected(drawn.error());
    }
    auto image = renderer->read_image();
    if (!image) {
        return tl::unexpected(image.error());
    }
    return ProofImage{std::move(*image), std::string(renderer->graphics_api())};
}

} // namespace demo_client
