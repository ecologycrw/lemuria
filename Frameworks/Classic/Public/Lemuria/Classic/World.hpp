#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/TileGrid.hpp"
#include "Lemuria/Classic/WorldIds.hpp"

namespace lemuria::classic {

// Something that exists on the current map.
struct Entity {
    EntityId id;
    TileCoord tile;

    // Where it is walking to, while it is walking.
    std::optional<TileCoord> destination;

    friend bool operator==(const Entity&, const Entity&) = default;
};

// What the client knows about the current map. The numbers of the entities are temporary: they only mean
// something from the moment an entity appears until it leaves (docs/adr/0017-contrato-do-adaptador.md).
class World {
public:
    // Keeps room for the player and for `max_other_entities` others, so it never allocates during the game
    // (INV-13, INV-14).
    explicit World(std::size_t max_other_entities);

    // Removes everything from the previous map and places the player. Events are refused until `set_grid`.
    void enter_map(const MapEntered& entered);

    // The tiles of the current map, which are ready after the map loads.
    void set_grid(TileGrid grid) noexcept;

    // Applies something that happened on the current map.
    void apply(const GameEvent& event);

    [[nodiscard]] MapId map() const noexcept { return map_; }

    // Nothing while the map is not ready.
    [[nodiscard]] const TileGrid* grid() const noexcept { return grid_ ? &*grid_ : nullptr; }

    [[nodiscard]] std::span<const Entity> entities() const noexcept { return entities_; }

    // Nothing when no entity has this number.
    [[nodiscard]] const Entity* find(EntityId id) const noexcept;

    // The player. Nothing before the first map.
    [[nodiscard]] const Entity* own() const noexcept { return find(own_id_); }

    // Events about a number that no entity has, kept for diagnosis.
    [[nodiscard]] std::uint64_t unknown_entity_event_count() const noexcept { return unknown_entity_events_; }

    // Events that could not be used because of their data or because the world is full, kept for diagnosis.
    [[nodiscard]] std::uint64_t refused_event_count() const noexcept { return refused_events_; }

private:
    [[nodiscard]] Entity* find_entity(EntityId id) noexcept;

    void place(const EntityAppeared& appeared, const TileGrid& grid);
    void start_walk(const EntityWalked& walked, const TileGrid& grid) noexcept;
    void correct_position(const PositionCorrected& corrected, const TileGrid& grid) noexcept;
    void remove(const EntityLeft& left) noexcept;
    void change_tiles(const TilesChanged& changed, TileGrid& grid) noexcept;

    std::size_t max_other_entities_;
    std::vector<Entity> entities_;
    std::optional<TileGrid> grid_;
    MapId map_;
    EntityId own_id_;
    std::uint64_t unknown_entity_events_ = 0;
    std::uint64_t refused_events_ = 0;
};

} // namespace lemuria::classic
