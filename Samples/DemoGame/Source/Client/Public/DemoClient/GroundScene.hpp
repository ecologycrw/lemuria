#pragma once

#include <cstddef>
#include <cstdint>

#include "DemoProtocol/DemoMap.hpp"
#include "Lemuria/Core/SceneDescription.hpp"

namespace demo_client {

// How many shapes the demonstration can draw.
inline constexpr std::size_t kGroundTileBudget =
    static_cast<std::size_t>(demo_protocol::DemoMap::kSize) * demo_protocol::DemoMap::kSize;
inline constexpr std::size_t kBoxBudget = 256;

// Adds every tile of the map to the scene, with a small variation of brightness between neighbors, and a
// raised block on top of each rock.
void add_ground(const demo_protocol::DemoMap& map, lemuria::core::SceneDescription& scene);

// Adds the simple shape that stands in for a character.
void add_character(lemuria::core::Vec3 position, std::uint32_t rgb, lemuria::core::SceneDescription& scene);

} // namespace demo_client
