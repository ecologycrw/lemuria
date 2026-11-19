#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/GameEventQueue.hpp"
#include "Lemuria/Classic/ProtocolAdapter.hpp"
#include "Lemuria/Classic/SessionPhase.hpp"
#include "Lemuria/Core/FrameClock.hpp"
#include "Lemuria/Core/Result.hpp"

namespace lemuria::classic {

// How long a session waits and how much it holds. Each game chooses its own values.
struct SessionLimits {
    double connecting_seconds = 0.0;
    double authenticating_seconds = 0.0;

    // How long the server may take to place the player on a map.
    double map_entry_seconds = 0.0;

    double map_load_seconds = 0.0;

    // The most events that can wait for a map to load.
    std::size_t max_held_events = 0;

    // The most held events handed to the world in one frame, so a long wait does not hold up a frame.
    std::size_t max_released_events_per_frame = 0;
};

// Identifies one map load. A new change of map makes the tickets of the older loads worthless.
struct MapLoadTicket {
    MapId map;
    std::uint64_t epoch = 0;

    friend bool operator==(const MapLoadTicket&, const MapLoadTicket&) = default;
};

// What the session hands to the game.
class SessionListener {
public:
    virtual ~SessionListener() = default;

    // The player is on a new map: remove everything from the world, load the map and then call
    // `GameSession::finish_map_load` with the ticket.
    virtual void on_map_entered(const MapEntered& entered, MapLoadTicket ticket) = 0;

    // Something happened in the current map. It never arrives while a map loads.
    virtual void on_world_event(const GameEvent& event) = 0;

protected:
    SessionListener() = default;
    SessionListener(const SessionListener&) = default;
    SessionListener(SessionListener&&) = default;
    SessionListener& operator=(const SessionListener&) = default;
    SessionListener& operator=(SessionListener&&) = default;
};

// Follows a game session from the connection to its end. Every wait for the server or for a map load has a
// deadline, and the only waits without one are those that depend on the player.
class GameSession final : private GameEventSink {
public:
    GameSession(ProtocolAdapter& adapter, const SessionLimits& limits, SessionListener& listener) noexcept;

    GameSession(const GameSession&) = delete;
    GameSession(GameSession&&) = delete;
    GameSession& operator=(const GameSession&) = delete;
    GameSession& operator=(GameSession&&) = delete;
    ~GameSession() override = default;

    // Checks the adapter and the limits, and starts connecting. A failure to connect also ends the session.
    [[nodiscard]] core::Result<void> start();

    // Handles what arrived since the previous frame. It never waits (INV-04).
    void update(core::FrameTime time);

    // The map of this ticket is ready. A ticket of an older load changes nothing.
    void finish_map_load(MapLoadTicket ticket);

    // Ends the session at the request of the player.
    void end() noexcept;

    [[nodiscard]] SessionPhase phase() const noexcept { return phase_; }

    // Why the session ended. Nothing while it is running.
    [[nodiscard]] std::optional<SessionEndReason> end_reason() const noexcept { return end_reason_; }

    // Events left out because they arrived at the wrong moment, kept for diagnosis.
    [[nodiscard]] std::uint64_t discarded_event_count() const noexcept { return discarded_events_; }

private:
    void deliver(const GameEvent& event) override;

    void handle_phase_report(SessionPhase phase);
    void enter_map(const MapEntered& entered);
    void refuse_map_change() noexcept;
    void handle_world_event(const GameEvent& event);
    void release_held_events();
    void watch_current_wait(double elapsed_seconds) noexcept;

    [[nodiscard]] std::optional<double> current_wait_limit() const noexcept;
    void set_phase(SessionPhase phase) noexcept;
    void finish(SessionEndReason reason) noexcept;
    void give_up(SessionEndReason reason) noexcept;

    ProtocolAdapter& adapter_;
    SessionLimits limits_;
    SessionListener& listener_;
    GameEventQueue held_events_;
    SessionPhase phase_ = SessionPhase::Connecting;
    std::optional<SessionEndReason> end_reason_;
    std::uint64_t map_epoch_ = 0;
    std::uint64_t discarded_events_ = 0;
    double waited_seconds_ = 0.0;
    bool has_started_ = false;
    bool has_map_ = false;
    bool is_loading_map_ = false;
};

} // namespace lemuria::classic
