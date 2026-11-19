#include "DemoClient/DemoAdapter.hpp"

#include <optional>
#include <utility>
#include <variant>

#include "DemoProtocol/MessageCodec.hpp"
#include "DemoProtocol/Protocol.hpp"

namespace demo_client {

namespace {

namespace classic = lemuria::classic;
namespace core = lemuria::core;

static_assert(demo_protocol::kMaxWalkSteps <= classic::WalkSteps::kCapacity,
              "every walk the server sends must fit in an event");

[[nodiscard]] classic::TileCoord tile_of(demo_protocol::Tile tile) noexcept {
    return {tile.x, tile.z};
}

[[nodiscard]] classic::WalkSteps steps_of(const demo_protocol::StepList& steps) noexcept {
    classic::WalkSteps walk;
    for (const demo_protocol::Tile step : steps.view()) {
        // Always fits, as checked above.
        static_cast<void>(walk.add(tile_of(step)));
    }
    return walk;
}

// Turns a message of the test server into an event of the framework. Nothing when the message cannot be one.
struct EventFromMessage {
    [[nodiscard]] std::optional<classic::GameEvent>
    operator()(const demo_protocol::MapEntered& message) const noexcept {
        return classic::MapEntered{classic::MapId{message.map}, classic::EntityId{message.own_entity},
                                   tile_of(message.position)};
    }

    [[nodiscard]] std::optional<classic::GameEvent>
    operator()(const demo_protocol::EntityAppeared& message) const noexcept {
        std::optional<classic::TileCoord> walking_to;
        if (message.walking_to) {
            walking_to = tile_of(*message.walking_to);
        }
        return classic::EntityAppeared{classic::EntityId{message.entity}, tile_of(message.position),
                                       walking_to};
    }

    [[nodiscard]] std::optional<classic::GameEvent>
    operator()(const demo_protocol::EntityWalked& message) const noexcept {
        if (message.steps.empty()) {
            return std::nullopt;
        }
        return classic::EntityWalked{classic::EntityId{message.entity}, tile_of(message.origin),
                                     tile_of(message.steps.view().back()), steps_of(message.steps)};
    }

    [[nodiscard]] std::optional<classic::GameEvent>
    operator()(const demo_protocol::WalkConfirmed& message) const noexcept {
        return classic::OwnWalkConfirmed{tile_of(message.origin), tile_of(message.destination)};
    }

    [[nodiscard]] std::optional<classic::GameEvent>
    operator()(const demo_protocol::PositionCorrected& message) const noexcept {
        return classic::PositionCorrected{classic::EntityId{message.entity}, tile_of(message.position)};
    }

    [[nodiscard]] std::optional<classic::GameEvent>
    operator()(const demo_protocol::EntityLeft& message) const noexcept {
        const classic::EntityLeaveReason reason = message.reason == demo_protocol::LeaveReason::Disconnected
                                                      ? classic::EntityLeaveReason::Removed
                                                      : classic::EntityLeaveReason::LeftView;
        return classic::EntityLeft{classic::EntityId{message.entity}, reason};
    }
};

[[nodiscard]] classic::SessionEndReason end_reason_of(core::CloseReason reason) noexcept {
    switch (reason) {
    case core::CloseReason::LocalRequest:
        return classic::SessionEndReason::ClosedByPlayer;
    case core::CloseReason::PeerClosed:
        return classic::SessionEndReason::ClosedByServer;
    case core::CloseReason::CouldNotConnect:
        return classic::SessionEndReason::CouldNotConnect;
    case core::CloseReason::TimedOut:
        return classic::SessionEndReason::TimedOut;
    case core::CloseReason::ConnectionLost:
        break;
    }
    return classic::SessionEndReason::ConnectionLost;
}

// Nothing when the tile is beyond what a message can carry.
[[nodiscard]] std::optional<demo_protocol::Tile> message_tile(classic::TileCoord tile) noexcept {
    return demo_protocol::tile_from(tile.x, tile.z);
}

} // namespace

DemoAdapter::DemoAdapter(core::Connections& connections, core::ConnectionAddress server) noexcept
    : connections_(connections), server_(std::move(server)) {}

core::Result<void> DemoAdapter::begin_session() {
    if (has_begun_) {
        return core::make_error("a session of the demo adapter can begin only once");
    }
    if (connections_.traits().kind != kDemoCapabilities.connection_kind) {
        return core::make_error("the test server speaks only over TCP");
    }
    auto opened = connections_.open(server_);
    if (!opened) {
        return tl::unexpected(std::move(opened.error()));
    }
    connection_ = *opened;
    has_begun_ = true;
    return {};
}

void DemoAdapter::poll(classic::GameEventSink& events) {
    events_ = &events;
    connections_.poll(*this);
    events_ = nullptr;
}

core::Result<void> DemoAdapter::send_request(const classic::GameRequest& request) {
    if (const auto* walk = std::get_if<classic::WalkRequest>(&request)) {
        return send_walk(*walk);
    }
    const auto& notice = std::get<classic::MapLoadedNotice>(request);
    return send_message(demo_protocol::MapLoaded{notice.map.value});
}

void DemoAdapter::end_session() noexcept {
    connections_.close(connection_);
}

// The server hears the hello first, and the session waits for the map from here.
void DemoAdapter::on_connected(core::ConnectionId /*connection*/) {
    if (has_ended_) {
        return;
    }
    if (!send_message(demo_protocol::Hello{demo_protocol::protocol_version()})) {
        end_with(classic::SessionEndReason::ConnectionLost);
        return;
    }
    deliver(classic::SessionPhaseReached{classic::SessionPhase::EnteringMap});
}

void DemoAdapter::on_received(const core::ReceivedBytes& block) {
    if (has_ended_) {
        return;
    }
    if (!frames_.read(block.bytes, *this)) {
        end_with(classic::SessionEndReason::InvalidData);
    }
}

void DemoAdapter::on_closed(core::ConnectionId /*connection*/, const core::ConnectionClosed& closed) {
    if (has_ended_) {
        return;
    }
    has_ended_ = true;
    deliver(classic::SessionEnded{end_reason_of(closed.reason)});
}

void DemoAdapter::on_frame(const demo_protocol::Frame& frame) {
    if (has_ended_) {
        return;
    }
    const auto message = demo_protocol::read_server_message(frame);
    const std::optional<classic::GameEvent> event =
        message ? std::visit(EventFromMessage{}, *message) : std::nullopt;
    if (!event) {
        ++invalid_messages_;
        return;
    }
    deliver(*event);
}

core::Result<void> DemoAdapter::send_message(const demo_protocol::ClientMessage& message) {
    const demo_protocol::EncodedFrame frame = demo_protocol::encode(message);
    return connections_.send(connection_, frame.bytes());
}

// The server reads the destination from the last step, so the steps are all it needs.
core::Result<void> DemoAdapter::send_walk(const classic::WalkRequest& request) {
    const std::optional<demo_protocol::Tile> origin = message_tile(request.origin);
    if (!origin) {
        return core::make_error("the walk starts beyond the tiles a message can carry");
    }
    if (request.steps.size() > demo_protocol::kMaxWalkSteps) {
        return core::make_error("the walk has more steps than the test server accepts");
    }
    demo_protocol::StepList steps;
    for (const classic::TileCoord step : request.steps.view()) {
        const std::optional<demo_protocol::Tile> tile = message_tile(step);
        if (!tile || !steps.add(*tile)) {
            return core::make_error("a step of the walk is beyond the tiles a message can carry");
        }
    }
    return send_message(demo_protocol::WalkRequest{*origin, steps});
}

void DemoAdapter::deliver(const classic::GameEvent& event) {
    if (events_ != nullptr) {
        events_->deliver(event);
    }
}

void DemoAdapter::end_with(classic::SessionEndReason reason) {
    has_ended_ = true;
    deliver(classic::SessionEnded{reason});
    connections_.close(connection_);
}

} // namespace demo_client
