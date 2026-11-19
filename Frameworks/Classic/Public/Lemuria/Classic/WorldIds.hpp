#pragma once

#include <cstdint>

namespace lemuria::classic {

// Names something in the world only from the moment it appears until it leaves, inside the same map. The
// server may give the same number to something else right after (docs/adr/0017-contrato-do-adaptador.md).
struct EntityId {
    std::uint32_t value = 0;

    friend constexpr bool operator==(EntityId, EntityId) noexcept = default;
};

// Names a map. Only the game knows what each number means.
struct MapId {
    std::uint32_t value = 0;

    friend constexpr bool operator==(MapId, MapId) noexcept = default;
};

} // namespace lemuria::classic
