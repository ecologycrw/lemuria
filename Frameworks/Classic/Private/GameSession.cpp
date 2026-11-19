#include "Lemuria/Classic/GameSession.hpp"

#include <cmath>
#include <variant>

namespace lemuria::classic {

namespace {

[[nodiscard]] bool is_usable_wait(double seconds) noexcept {
    return std::isfinite(seconds) && seconds > 0.0;
}

[[nodiscard]] core::Result<void> check_limits(const SessionLimits& limits) {
    const bool waits_are_usable =
        is_usable_wait(limits.connecting_seconds) && is_usable_wait(limits.authenticating_seconds) &&
        is_usable_wait(limits.map_entry_seconds) && is_usable_wait(limits.map_load_seconds);
    if (!waits_are_usable) {
        return core::make_error("every wait of a game session needs a limit above zero");
    }
    if (limits.max_held_events == 0 || limits.max_released_events_per_frame == 0) {
        return core::make_error("a game session must be able to hold and release events");
    }
    return {};
}

} // namespace

GameSession::GameSession(ProtocolAdapter& adapter, const SessionLimits& limits,
                         SessionListener& listener) noexcept
    : adapter_(adapter), limits_(limits), listener_(listener) {}

core::Result<void> GameSession::start() {
    if (has_started_) {
        return core::make_error("a game session can start only once");
    }
    if (auto valid = check_capabilities(adapter_.capabilities()); !valid) {
        return valid;
    }
    if (auto valid = check_limits(limits_); !valid) {
        return valid;
    }

    held_events_ = GameEventQueue(limits_.max_held_events);
    has_started_ = true;
    auto begun = adapter_.begin_session();
    if (!begun) {
        finish(SessionEndReason::CouldNotConnect);
    }
    return begun;
}

void GameSession::update(core::FrameTime time) {
    if (!has_started_) {
        return;
    }
    adapter_.poll(*this);
    release_held_events();
    watch_current_wait(time.delta_seconds);
}

void GameSession::finish_map_load(MapLoadTicket ticket) {
    if (!is_loading_map_ || ticket.epoch != map_epoch_) {
        return;
    }
    is_loading_map_ = false;
    set_phase(SessionPhase::InWorld);
    if (adapter_.capabilities().needs_map_loaded_notice &&
        !adapter_.send_request(MapLoadedNotice{ticket.map})) {
        give_up(SessionEndReason::ConnectionLost);
    }
}

void GameSession::end() noexcept {
    if (has_started_ && phase_ != SessionPhase::Ended) {
        give_up(SessionEndReason::ClosedByPlayer);
    }
}

void GameSession::deliver(const GameEvent& event) {
    if (phase_ == SessionPhase::Ended) {
        if (!std::holds_alternative<SessionEnded>(event)) {
            ++discarded_events_;
        }
        return;
    }

    if (const auto* reached = std::get_if<SessionPhaseReached>(&event)) {
        handle_phase_report(reached->phase);
    } else if (const auto* ended = std::get_if<SessionEnded>(&event)) {
        finish(ended->reason);
    } else if (const auto* entered = std::get_if<MapEntered>(&event)) {
        enter_map(*entered);
    } else if (std::holds_alternative<MapChangeRefused>(event)) {
        refuse_map_change();
    } else {
        handle_world_event(event);
    }
}

// The session itself decides when the world is ready and when it is over.
void GameSession::handle_phase_report(SessionPhase phase) {
    const bool is_decided_here = phase == SessionPhase::InWorld || phase == SessionPhase::Ended;
    if (is_decided_here || is_loading_map_) {
        ++discarded_events_;
        return;
    }
    set_phase(phase);
}

void GameSession::enter_map(const MapEntered& entered) {
    discarded_events_ += held_events_.size();
    held_events_.clear();

    phase_ = has_map_ ? SessionPhase::ChangingMap : SessionPhase::EnteringMap;
    waited_seconds_ = 0.0;
    has_map_ = true;
    is_loading_map_ = true;
    ++map_epoch_;
    listener_.on_map_entered(entered, MapLoadTicket{entered.map, map_epoch_});
}

void GameSession::refuse_map_change() noexcept {
    if (phase_ == SessionPhase::ChangingMap && has_map_ && !is_loading_map_) {
        set_phase(SessionPhase::InWorld);
    } else {
        ++discarded_events_;
    }
}

void GameSession::handle_world_event(const GameEvent& event) {
    if (!has_map_) {
        ++discarded_events_;
        return;
    }
    // Events that arrive while others wait go behind them, so the order never changes.
    if (!is_loading_map_ && held_events_.empty()) {
        listener_.on_world_event(event);
    } else if (!held_events_.push(event)) {
        give_up(SessionEndReason::TooManyHeldEvents);
    }
}

void GameSession::release_held_events() {
    if (is_loading_map_) {
        return;
    }
    for (std::size_t released = 0; released < limits_.max_released_events_per_frame && !held_events_.empty();
         ++released) {
        const GameEvent event = held_events_.front();
        held_events_.pop();
        listener_.on_world_event(event);
    }
}

void GameSession::watch_current_wait(double elapsed_seconds) noexcept {
    const std::optional<double> limit = current_wait_limit();
    if (!limit) {
        return;
    }
    waited_seconds_ += elapsed_seconds;
    if (waited_seconds_ >= *limit) {
        give_up(SessionEndReason::TimedOut);
    }
}

std::optional<double> GameSession::current_wait_limit() const noexcept {
    if (is_loading_map_) {
        return limits_.map_load_seconds;
    }
    switch (phase_) {
    case SessionPhase::Connecting:
    case SessionPhase::Reconnecting:
        return limits_.connecting_seconds;
    case SessionPhase::Authenticating:
        return limits_.authenticating_seconds;
    case SessionPhase::EnteringMap:
    case SessionPhase::ChangingMap:
        return limits_.map_entry_seconds;
    case SessionPhase::ChoosingCharacter:
    case SessionPhase::InWorld:
    case SessionPhase::Ended:
        break;
    }
    return std::nullopt;
}

void GameSession::set_phase(SessionPhase phase) noexcept {
    if (phase != phase_) {
        phase_ = phase;
        waited_seconds_ = 0.0;
    }
}

void GameSession::finish(SessionEndReason reason) noexcept {
    phase_ = SessionPhase::Ended;
    end_reason_ = reason;
    is_loading_map_ = false;
    discarded_events_ += held_events_.size();
    held_events_.clear();
}

// The session stops by its own decision, and the adapter is told to close everything.
void GameSession::give_up(SessionEndReason reason) noexcept {
    finish(reason);
    adapter_.end_session();
}

} // namespace lemuria::classic
