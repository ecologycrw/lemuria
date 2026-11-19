#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "DemoProtocol/DemoMap.hpp"
#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/GameSession.hpp"
#include "Lemuria/Classic/ProtocolAdapter.hpp"
#include "Lemuria/Classic/SessionPhase.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/TileGrid.hpp"
#include "Lemuria/Classic/World.hpp"
#include "Lemuria/Core/FrameClock.hpp"
#include "Lemuria/Core/Math.hpp"
#include "Lemuria/Core/Result.hpp"
#include "Lemuria/Core/SceneDescription.hpp"

namespace demo_client {

// How long the demonstration waits for the server, and how much it holds while the map loads.
inline constexpr lemuria::classic::SessionLimits kDemoSessionLimits{
    .connecting_seconds = 15.0,
    .authenticating_seconds = 15.0,
    .map_entry_seconds = 10.0,
    .map_load_seconds = 10.0,
    .max_held_events = 256,
    .max_released_events_per_frame = 64,
};

// The test server lets in 8 players, so 7 others, with room to spare.
inline constexpr std::size_t kMaxOtherCharacters = 16;

// How everyone walks in the demonstration. It must agree with the rules of the test server.
[[nodiscard]] lemuria::classic::MovementRules demo_movement_rules() noexcept;

// What the client believes about the map: only stone blocks the way. The server forbids more.
[[nodiscard]] lemuria::core::Result<lemuria::classic::TileGrid>
client_grid(const demo_protocol::DemoMap& map);

// The state of the connection, in a few words for the title of the window (CA-14).
[[nodiscard]] std::string_view
connection_status(lemuria::classic::SessionPhase phase,
                  std::optional<lemuria::classic::SessionEndReason> end) noexcept;

// The game of one player, without the window: the session with the server, the world and the clicks. The
// program with the window and the tests drive it in the same way.
class DemoGame final : private lemuria::classic::SessionListener {
public:
    [[nodiscard]] static lemuria::core::Result<std::unique_ptr<DemoGame>>
    create(lemuria::classic::ProtocolAdapter& adapter);

    // Use `create`, which checks the rules and builds the map.
    DemoGame(lemuria::classic::ProtocolAdapter& adapter, lemuria::classic::World world,
             lemuria::classic::TileGrid grid) noexcept;

    DemoGame(const DemoGame&) = delete;
    DemoGame(DemoGame&&) = delete;
    DemoGame& operator=(const DemoGame&) = delete;
    DemoGame& operator=(DemoGame&&) = delete;
    ~DemoGame() override = default;

    // Starts connecting. A failure also ends the session, which the status shows.
    [[nodiscard]] lemuria::core::Result<void> start();

    // The player clicked a tile. It counts only while the player is on the map, and returns whether the
    // player started walking: a blocked or unreachable tile leaves it where it is (CA-03).
    [[nodiscard]] bool click(lemuria::classic::TileCoord tile);

    // Handles what arrived from the server, moves everyone and sends the walk of the player.
    void frame(const lemuria::core::FrameTime& time);

    void end() noexcept;

    // Fills the scene with the ground and everyone on the map. The camera is left to the caller.
    void describe(lemuria::core::SceneDescription& scene) const;

    // Where the camera looks: the player, or the middle of the map before the player arrives.
    [[nodiscard]] lemuria::core::Vec3 focus() const noexcept;

    [[nodiscard]] std::string_view status() const noexcept;
    [[nodiscard]] const lemuria::classic::World& world() const noexcept { return world_; }
    [[nodiscard]] const lemuria::classic::GameSession& session() const noexcept { return session_; }

    // Walk requests that could not leave, because the connection had already ended.
    [[nodiscard]] std::uint64_t unsent_request_count() const noexcept { return unsent_requests_; }

private:
    void on_map_entered(const lemuria::classic::MapEntered& entered,
                        lemuria::classic::MapLoadTicket ticket) override;
    void on_world_event(const lemuria::classic::GameEvent& event) override;

    void finish_map_load();
    void send_walk_request();

    lemuria::classic::ProtocolAdapter& adapter_;
    demo_protocol::DemoMap map_;
    lemuria::classic::TileGrid grid_;
    lemuria::classic::World world_;
    lemuria::classic::GameSession session_;

    // The map is loaded after the session hands over its events, never while it is handing them over.
    std::optional<lemuria::classic::MapLoadTicket> pending_load_;

    // Why the client left the map the server sent, when it did. The session records that end as asked by the
    // player, so the status needs the real reason.
    std::string_view map_refusal_;

    std::uint64_t unsent_requests_ = 0;
};

} // namespace demo_client
