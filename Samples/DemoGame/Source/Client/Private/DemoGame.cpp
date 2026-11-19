#include "DemoClient/DemoGame.hpp"

#include <utility>

#include "DemoClient/DemoAdapter.hpp"
#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"
#include "DemoProtocol/DemoRules.hpp"

namespace demo_client {

namespace {

namespace classic = lemuria::classic;
namespace core = lemuria::core;

using demo_protocol::DemoMap;

// How fast a character glides to the place the server decided. Faster than walking, so a correction is quick,
// but still a glide and never a jump (CA-06).
constexpr double kSlideTilesPerSecond = 8.0;

} // namespace

classic::MovementRules demo_movement_rules() noexcept {
    return classic::MovementRules{
        .paths = {.diagonals = classic::DiagonalRule::WhenBothSidesAreFree,
                  .max_searched_tiles = static_cast<std::size_t>(DemoMap::kSize) * DemoMap::kSize},
        .speeds = {.walk_tiles_per_second = demo_protocol::kWalkTilesPerSecond,
                   .slide_tiles_per_second = kSlideTilesPerSecond},
        .walk_requests = kDemoCapabilities.walk_requests,
        .max_steps_per_request = kDemoCapabilities.max_steps_per_request,
    };
}

core::Result<classic::TileGrid> client_grid(const DemoMap& map) {
    auto grid = classic::TileGrid::create(DemoMap::kSize, DemoMap::kSize);
    if (!grid) {
        return grid;
    }
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            static_cast<void>(grid->set_walkable({x, z}, map.looks_walkable(x, z)));
        }
    }
    return grid;
}

std::string_view connection_status(classic::SessionPhase phase,
                                   std::optional<classic::SessionEndReason> end) noexcept {
    switch (phase) {
    case classic::SessionPhase::Connecting:
        return "connecting to the test server";
    case classic::SessionPhase::InWorld:
        return "on the map";
    case classic::SessionPhase::Authenticating:
    case classic::SessionPhase::ChoosingCharacter:
    case classic::SessionPhase::EnteringMap:
    case classic::SessionPhase::ChangingMap:
    case classic::SessionPhase::Reconnecting:
        return "entering the map";
    case classic::SessionPhase::Ended:
        break;
    }
    switch (end.value_or(classic::SessionEndReason::ConnectionLost)) {
    case classic::SessionEndReason::ClosedByPlayer:
        return "disconnected";
    case classic::SessionEndReason::CouldNotConnect:
        return "the test server could not be reached";
    case classic::SessionEndReason::TimedOut:
        return "the test server stopped answering";
    case classic::SessionEndReason::InvalidData:
    case classic::SessionEndReason::TooManyHeldEvents:
        return "the test server sent data the client could not use";
    case classic::SessionEndReason::ClosedByServer:
    case classic::SessionEndReason::ConnectionLost:
        break;
    }
    return "the connection to the test server was lost";
}

core::Result<std::unique_ptr<DemoGame>> DemoGame::create(classic::ProtocolAdapter& adapter) {
    auto world = classic::World::create(kMaxOtherCharacters, demo_movement_rules());
    if (!world) {
        return tl::unexpected(std::move(world.error()));
    }
    auto grid = client_grid(DemoMap{});
    if (!grid) {
        return tl::unexpected(std::move(grid.error()));
    }
    return std::make_unique<DemoGame>(adapter, std::move(*world), std::move(*grid));
}

DemoGame::DemoGame(classic::ProtocolAdapter& adapter, classic::World world, classic::TileGrid grid) noexcept
    : adapter_(adapter), grid_(std::move(grid)), world_(std::move(world)),
      session_(adapter, kDemoSessionLimits, *this) {}

core::Result<void> DemoGame::start() {
    return session_.start();
}

bool DemoGame::click(classic::TileCoord tile) {
    if (session_.phase() != classic::SessionPhase::InWorld) {
        return false;
    }
    return world_.walk_own_to(tile).has_value();
}

void DemoGame::frame(const core::FrameTime& time) {
    session_.update(time);
    finish_map_load();
    world_.update(time.delta_seconds);
    send_walk_request();
}

void DemoGame::end() noexcept {
    session_.end();
}

void DemoGame::describe(core::SceneDescription& scene) const {
    scene.clear();
    scene.background = core::color_from_rgb(kDeepShadowRgb);
    add_ground(map_, scene);
    const classic::Entity* own = world_.own();
    for (const classic::Entity& entity : world_.entities()) {
        const bool is_own = own != nullptr && entity.id == own->id;
        add_character(entity.motion.position(), is_own ? kOwnCharacterRgb : kOtherCharacterRgb, scene);
    }
}

core::Vec3 DemoGame::focus() const noexcept {
    if (const classic::Entity* own = world_.own()) {
        return own->motion.position();
    }
    return classic::tile_center({DemoMap::kSpawnCenter, DemoMap::kSpawnCenter});
}

std::string_view DemoGame::status() const noexcept {
    if (!map_refusal_.empty()) {
        return map_refusal_;
    }
    return connection_status(session_.phase(), session_.end_reason());
}

void DemoGame::on_map_entered(const classic::MapEntered& entered, classic::MapLoadTicket ticket) {
    world_.enter_map(entered);
    pending_load_ = ticket;
}

void DemoGame::on_world_event(const classic::GameEvent& event) {
    world_.apply(event);
}

// The demonstration has a single map, which is always ready. A map the client does not know, or a place
// outside the map, ends the session.
void DemoGame::finish_map_load() {
    if (!pending_load_) {
        return;
    }
    const classic::MapLoadTicket ticket = *std::exchange(pending_load_, std::nullopt);
    if (ticket.map != classic::MapId{DemoMap::kId}) {
        map_refusal_ = "the test server sent a map the client does not know";
        session_.end();
        return;
    }
    if (!world_.set_grid(grid_)) {
        map_refusal_ = "the test server placed the player outside the map";
        session_.end();
        return;
    }
    session_.finish_map_load(ticket);
}

void DemoGame::send_walk_request() {
    const std::optional<classic::WalkRequest> request = world_.take_walk_request();
    if (request && !adapter_.send_request(*request)) {
        ++unsent_requests_;
    }
}

} // namespace demo_client
