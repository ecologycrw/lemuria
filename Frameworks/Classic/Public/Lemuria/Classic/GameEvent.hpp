#pragma once

#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>

#include "Lemuria/Classic/SessionPhase.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/WalkSteps.hpp"
#include "Lemuria/Classic/WorldIds.hpp"

// What the server said, in words that do not depend on any server. The adapter checks the data before it
// becomes an event (INV-12), and the framework decides what each event changes
// (docs/adr/0017-contrato-do-adaptador.md).
namespace lemuria::classic {

enum class SessionEndReason : std::uint8_t {
    ClosedByPlayer,
    ClosedByServer,
    CouldNotConnect,
    ConnectionLost,
    TimedOut,

    // The bytes broke the format of the messages, and there is no way to find the next message.
    InvalidData,

    // More events arrived during a map load than the session can hold, so its world can no longer be trusted.
    TooManyHeldEvents,
};

enum class EntityLeaveReason : std::uint8_t {
    LeftView,
    Removed,
    Died,
};

// The session moved to another stage.
struct SessionPhaseReached {
    SessionPhase phase = SessionPhase::Connecting;

    friend bool operator==(const SessionPhaseReached&, const SessionPhaseReached&) = default;
};

// Always the last event of a session.
struct SessionEnded {
    SessionEndReason reason = SessionEndReason::ConnectionLost;

    friend bool operator==(const SessionEnded&, const SessionEnded&) = default;
};

// The player was placed on a map, for the first time or after a change of map.
struct MapEntered {
    MapId map;
    EntityId own_entity;
    TileCoord own_position;

    friend bool operator==(const MapEntered&, const MapEntered&) = default;
};

// The server refused a change of map that had already started.
struct MapChangeRefused {
    friend bool operator==(const MapChangeRefused&, const MapChangeRefused&) = default;
};

// Something came into view. A number already in use replaces what had it before.
struct EntityAppeared {
    EntityId entity;
    TileCoord position;

    // Where it is already walking to, when it appears in the middle of a walk.
    std::optional<TileCoord> walking_to;

    friend bool operator==(const EntityAppeared&, const EntityAppeared&) = default;
};

// Someone else started walking. Servers that do not send the steps leave them empty.
struct EntityWalked {
    EntityId entity;
    std::optional<TileCoord> origin;
    TileCoord destination;
    WalkSteps steps;

    friend bool operator==(const EntityWalked&, const EntityWalked&) = default;
};

// The server accepted a walk of the player. The destination may be closer than the one requested, and the
// origin tells which walk this answer belongs to.
struct OwnWalkConfirmed {
    TileCoord origin;
    TileCoord destination;

    friend bool operator==(const OwnWalkConfirmed&, const OwnWalkConfirmed&) = default;
};

// The server decided where something is. This always wins over what the client expected (INV-01).
struct PositionCorrected {
    EntityId entity;
    TileCoord position;

    friend bool operator==(const PositionCorrected&, const PositionCorrected&) = default;
};

struct EntityLeft {
    EntityId entity;
    EntityLeaveReason reason = EntityLeaveReason::LeftView;

    friend bool operator==(const EntityLeft&, const EntityLeft&) = default;
};

// Every tile from `first` to `last`, in both directions, can now be walked on or not.
struct TilesChanged {
    TileCoord first;
    TileCoord last;
    bool walkable = true;

    friend bool operator==(const TilesChanged&, const TilesChanged&) = default;
};

using GameEvent =
    std::variant<SessionPhaseReached, SessionEnded, MapEntered, MapChangeRefused, EntityAppeared,
                 EntityWalked, OwnWalkConfirmed, PositionCorrected, EntityLeft, TilesChanged>;

// Events are plain values, so they can wait in a queue without allocating memory (INV-13).
static_assert(std::is_trivially_copyable_v<GameEvent>);

// Receives the events of an adapter, in the order the server sent them.
class GameEventSink {
public:
    virtual ~GameEventSink() = default;

    virtual void deliver(const GameEvent& event) = 0;

protected:
    GameEventSink() = default;
    GameEventSink(const GameEventSink&) = default;
    GameEventSink(GameEventSink&&) = default;
    GameEventSink& operator=(const GameEventSink&) = default;
    GameEventSink& operator=(GameEventSink&&) = default;
};

} // namespace lemuria::classic
