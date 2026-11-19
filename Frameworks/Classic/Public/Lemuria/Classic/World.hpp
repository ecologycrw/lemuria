#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <tl/expected.hpp>

#include "Lemuria/Classic/AdapterCapabilities.hpp"
#include "Lemuria/Classic/EntityMotion.hpp"
#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/GameRequest.hpp"
#include "Lemuria/Classic/PathFinder.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/TileGrid.hpp"
#include "Lemuria/Classic/WorldIds.hpp"
#include "Lemuria/Core/Result.hpp"

namespace lemuria::classic {

// How things walk in a game. The walk requests follow what the adapter declares in its capabilities.
struct MovementRules {
    PathRules paths;
    MotionSpeeds speeds;
    WalkRequestMode walk_requests = WalkRequestMode::Steps;

    // The most steps one walk request can carry, when requests carry steps.
    std::size_t max_steps_per_request = 0;
};

// Something that exists on the current map.
struct Entity {
    EntityId id;
    EntityMotion motion;

    // Where it is walking to, while it is walking.
    std::optional<TileCoord> destination;

    [[nodiscard]] TileCoord tile() const noexcept { return motion.tile(); }
};

// What the client knows about the current map. The numbers of the entities are temporary: they only mean
// something from the moment an entity appears until it leaves (docs/adr/0017-contrato-do-adaptador.md).
//
// The player starts walking at once, without waiting for the server. The server still decides (INV-01): a
// correction always wins, and an answer that belongs to an older walk changes nothing.
class World {
public:
    // Keeps room for the player and for `max_other_entities` others, so it never allocates during the game
    // (INV-13, INV-14).
    [[nodiscard]] static core::Result<World> create(std::size_t max_other_entities,
                                                    const MovementRules& rules);

    // Removes everything from the previous map and places the player. Events are refused until `set_grid`.
    void enter_map(const MapEntered& entered);

    // The tiles of the current map, which are ready after the map loads.
    void set_grid(TileGrid grid);

    // Applies something that happened on the current map.
    void apply(const GameEvent& event);

    // Moves everything for the time of one frame.
    void update(double seconds);

    // Starts walking the player to a tile. On a failure the player keeps doing what it was doing.
    [[nodiscard]] tl::expected<void, PathFailure> walk_own_to(TileCoord destination);

    // The walk request the server must receive now, if there is one. It is handed over only once.
    [[nodiscard]] std::optional<WalkRequest> take_walk_request() noexcept;

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

    // Answers that belong to a walk that is no longer the current one, kept for diagnosis.
    [[nodiscard]] std::uint64_t stale_confirmation_count() const noexcept { return stale_confirmations_; }

private:
    // The walk the server was last told about.
    struct OwnWalk {
        TileCoord origin;
        TileCoord end;
        WalkSteps steps;
    };

    World(std::size_t max_other_entities, const MovementRules& rules);

    [[nodiscard]] Entity* find_entity(EntityId id) noexcept;

    void place(const EntityAppeared& appeared, const TileGrid& grid);
    void start_walk(const EntityWalked& walked, const TileGrid& grid);
    void correct_position(const PositionCorrected& corrected, const TileGrid& grid) noexcept;
    void remove(const EntityLeft& left) noexcept;
    void change_tiles(const TilesChanged& changed, TileGrid& grid) noexcept;

    void confirm_own_walk(const OwnWalkConfirmed& confirmed, const TileGrid& grid);
    [[nodiscard]] bool answers_current_walk(const OwnWalkConfirmed& confirmed) const noexcept;
    void adopt_shorter_walk(Entity& own, TileCoord accepted, const TileGrid& grid);
    void forget_own_walk() noexcept;

    void advance_entity(Entity& entity, double seconds, const TileGrid& grid);
    [[nodiscard]] bool continue_walk(Entity& entity, const TileGrid& grid);
    void plan_steps(Entity& entity, TileCoord destination, const TileGrid& grid);
    void walk_next_steps(Entity& entity, std::span<const TileCoord> path);
    void announce_own_steps(TileCoord origin, std::span<const TileCoord> steps) noexcept;
    [[nodiscard]] std::size_t route_step_limit() const noexcept;

    std::size_t max_other_entities_;
    MovementRules rules_;
    PathFinder path_finder_;
    std::vector<Entity> entities_;
    std::optional<TileGrid> grid_;
    std::optional<OwnWalk> own_walk_;
    std::optional<WalkRequest> pending_request_;
    MapId map_;
    EntityId own_id_;
    std::uint64_t unknown_entity_events_ = 0;
    std::uint64_t refused_events_ = 0;
    std::uint64_t stale_confirmations_ = 0;
};

} // namespace lemuria::classic
