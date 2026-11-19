#pragma once

#include <type_traits>
#include <variant>

#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/WalkSteps.hpp"
#include "Lemuria/Classic/WorldIds.hpp"

// What the player wants, in words that do not depend on any server. A request may never get an answer, and
// the silence counts as a provisional yes (docs/adr/0017-contrato-do-adaptador.md).
namespace lemuria::classic {

struct WalkRequest {
    TileCoord origin;
    TileCoord destination;

    // Empty when the adapter declares that its server receives only the destination.
    WalkSteps steps;

    friend bool operator==(const WalkRequest&, const WalkRequest&) = default;
};

// The map finished loading. It is sent only to servers that ask for it.
struct MapLoadedNotice {
    MapId map;

    friend bool operator==(const MapLoadedNotice&, const MapLoadedNotice&) = default;
};

using GameRequest = std::variant<WalkRequest, MapLoadedNotice>;

static_assert(std::is_trivially_copyable_v<GameRequest>);

} // namespace lemuria::classic
