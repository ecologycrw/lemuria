#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Lemuria/Core/Result.hpp"

namespace lemuria::classic {

// What a walk request carries.
enum class WalkRequestMode : std::uint8_t {
    // The client finds the path and the server checks each step.
    Steps,

    // The server finds the path by itself.
    DestinationOnly,
};

// How the server learns that the player is still there.
enum class PresenceSignal : std::uint8_t {
    None,
    ClientSendsPeriodically,
    ServerAsks,
};

// What happens when the connection falls during the game.
enum class ResumePolicy : std::uint8_t {
    EndSession,
    EnterAgainOnNewConnection,
};

// How the server of an adapter works. The framework follows this description and has no special path for any
// server (docs/adr/0017-contrato-do-adaptador.md).
struct AdapterCapabilities {
    // The kind of connection the server uses, as named by `core::ConnectionTraits`. The text must outlive the
    // adapter.
    std::string_view connection_kind;

    WalkRequestMode walk_requests = WalkRequestMode::Steps;

    // The most steps one walk request can carry, when requests carry steps.
    std::size_t max_steps_per_request = 0;

    bool needs_map_loaded_notice = false;

    PresenceSignal presence = PresenceSignal::None;

    // How often the client sends the signal, when the client is the one that sends it.
    double presence_period_seconds = 0.0;

    ResumePolicy resume = ResumePolicy::EndSession;
};

// Refuses a description the framework cannot follow, and says what is wrong with it.
[[nodiscard]] core::Result<void> check_capabilities(const AdapterCapabilities& capabilities);

} // namespace lemuria::classic
