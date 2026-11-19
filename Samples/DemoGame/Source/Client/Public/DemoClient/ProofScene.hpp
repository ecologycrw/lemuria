#pragma once

#include <cstdint>

#include "DemoProtocol/DemoMap.hpp"
#include "Lemuria/Core/SceneDescription.hpp"

namespace demo_client {

inline constexpr std::uint32_t kProofImageWidth = 1280;
inline constexpr std::uint32_t kProofImageHeight = 720;

// Fills the scene used to check the look of the game (docs/VISUAL.md): always the same map, camera,
// characters and time, so it always gives the same image.
void fill_proof_scene(const demo_protocol::DemoMap& map, lemuria::core::SceneDescription& scene);

} // namespace demo_client
