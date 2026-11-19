#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "AdapterTestSupport.hpp"
#include "Lemuria/Classic/GameSession.hpp"
#include "Lemuria/Classic/World.hpp"
#include "WorldTestSupport.hpp"

// The whole framework, from the bytes to the world, facing servers that behave badly. Every scenario runs
// with two servers that work in different ways, and the game below never asks which one it is talking to:
// what changes between them comes only from the capabilities their adapters declare.

using lemuria::classic::AdapterCapabilities;
using lemuria::classic::Entity;
using lemuria::classic::EntityId;
using lemuria::classic::GameEvent;
using lemuria::classic::GameSession;
using lemuria::classic::MapEntered;
using lemuria::classic::MapLoadTicket;
using lemuria::classic::MovementRules;
using lemuria::classic::PathFailure;
using lemuria::classic::SessionEndReason;
using lemuria::classic::SessionLimits;
using lemuria::classic::SessionListener;
using lemuria::classic::SessionPhase;
using lemuria::classic::TileCoord;
using lemuria::classic::WalkRequestMode;
using lemuria::classic::World;
using lemuria::classic::testing::Bytes;
using lemuria::classic::testing::field_at;
using lemuria::classic::testing::framed;
using lemuria::classic::testing::kChangingMapMessage;
using lemuria::classic::testing::kEntityAppearedMessage;
using lemuria::classic::testing::kEntityLeftMessage;
using lemuria::classic::testing::kEntityWalkedMessage;
using lemuria::classic::testing::kMapChangeRefusedMessage;
using lemuria::classic::testing::kMapEnteredMessage;
using lemuria::classic::testing::kMapLoadedMessage;
using lemuria::classic::testing::kMessageTraits;
using lemuria::classic::testing::kMovementRules;
using lemuria::classic::testing::kPositionCorrectedMessage;
using lemuria::classic::testing::kStreamTraits;
using lemuria::classic::testing::kTilesChangedMessage;
using lemuria::classic::testing::kWalkConfirmedMessage;
using lemuria::classic::testing::kWalkDestinationMessage;
using lemuria::classic::testing::kWalkStepsMessage;
using lemuria::classic::testing::make_grid;
using lemuria::classic::testing::make_world;
using lemuria::classic::testing::ScriptedConnections;
using lemuria::classic::testing::SentBlock;
using lemuria::classic::testing::TestAdapter;
using lemuria::classic::testing::tile_at;
using lemuria::core::CloseReason;
using lemuria::core::ConnectionTraits;
using lemuria::core::Delivery;
using lemuria::core::FrameTime;

namespace {

constexpr int kMapSide = 16;
constexpr int kFirstMap = 7;
constexpr int kPlayer = 1;
constexpr int kOther = 2;
constexpr TileCoord kStart{3, 3};

// Six tiles away, so a server that checks steps receives the walk in two parts.
constexpr TileCoord kFar{9, 3};

constexpr int kFramesPerSecond = 60;
constexpr double kServerWait = 5.0;
constexpr double kEnoughToArrive = 3.0;
constexpr std::size_t kMaxOthers = 8;

constexpr SessionLimits kLimits{
    .connecting_seconds = kServerWait,
    .authenticating_seconds = kServerWait,
    .map_entry_seconds = kServerWait,
    .map_load_seconds = kServerWait,
    .max_held_events = 16,
    .max_released_events_per_frame = 2,
};

// How one server works, and the connection it uses.
struct ServerKind {
    const char* name = "";
    ConnectionTraits traits;
    AdapterCapabilities capabilities;
    Delivery walk_delivery = Delivery::ReliableInOrder;
};

// Checks every step, receives the route in parts and must be told when the map is loaded.
constexpr ServerKind kStepsOverStream{
    .name = "steps over a stream of bytes",
    .traits = kStreamTraits,
    .capabilities = {.connection_kind = kStreamTraits.kind,
                     .walk_requests = WalkRequestMode::Steps,
                     .max_steps_per_request = 4,
                     .needs_map_loaded_notice = true},
    .walk_delivery = Delivery::ReliableInOrder,
};

// Finds the path by itself and sends whole messages, and walks go without a delivery guarantee.
constexpr ServerKind kDestinationOverMessages{
    .name = "destination over whole messages",
    .traits = kMessageTraits,
    .capabilities = {.connection_kind = kMessageTraits.kind,
                     .walk_requests = WalkRequestMode::DestinationOnly},
    .walk_delivery = Delivery::Unreliable,
};

// The movement follows what the adapter declares.
[[nodiscard]] MovementRules rules_for(const AdapterCapabilities& capabilities) {
    MovementRules rules = kMovementRules;
    rules.walk_requests = capabilities.walk_requests;
    rules.max_steps_per_request = capabilities.max_steps_per_request;
    return rules;
}

// A walk as the server reads it. Servers that find the path receive no origin.
struct ReceivedWalk {
    std::optional<TileCoord> origin;
    TileCoord destination;
    Delivery delivery = Delivery::ReliableInOrder;
};

// What the server read from the client.
struct ServerInbox {
    std::vector<ReceivedWalk> walks;
    int map_loaded_notices = 0;
};

void read_request(const SentBlock& block, ServerInbox& inbox) {
    REQUIRE(block.bytes.size() >= 2);
    const auto message = std::span<const std::byte>(block.bytes).subspan(1);
    const int type = field_at(message, 0);
    const auto fields = message.subspan(1);
    if (type == kWalkStepsMessage) {
        REQUIRE(fields.size() >= 4);
        inbox.walks.push_back({tile_at(fields, 0), tile_at(fields, 2), block.delivery});
    } else if (type == kWalkDestinationMessage) {
        REQUIRE(fields.size() == 2);
        inbox.walks.push_back({std::nullopt, tile_at(fields, 0), block.delivery});
    } else {
        REQUIRE(type == kMapLoadedMessage);
        ++inbox.map_loaded_notices;
    }
}

// The part of a game that joins the session and the world, as the demonstration client will do.
class TestGame final : public SessionListener {
public:
    explicit TestGame(const ServerKind& server)
        : connections(server.traits), adapter(connections, server.capabilities, server.walk_delivery),
          world(make_world(kMaxOthers, rules_for(server.capabilities))), session(adapter, kLimits, *this) {
        REQUIRE(session.start().has_value());
    }

    // Bytes from the server, which arrive one by one on a stream and as one block when messages are whole.
    void arrive(const Bytes& message) {
        if (connections.traits().delivers_whole_messages) {
            connections.arrive_bytes(message, true);
            return;
        }
        for (const std::byte value : message) {
            connections.arrive_bytes(Bytes{value}, false);
        }
    }

    void receive(const Bytes& message) {
        arrive(message);
        frame();
    }

    void server_accepts_connection() {
        connections.arrive_connected();
        frame();
    }

    // The map of the latest change of map is ready.
    void finish_loading() {
        REQUIRE(ticket_.has_value());
        world.set_grid(make_grid(kMapSide));
        session.finish_map_load(*ticket_);
    }

    void enter_world() {
        server_accepts_connection();
        receive(framed({kMapEnteredMessage, kFirstMap, kPlayer, kStart.x, kStart.z}));
        finish_loading();
        frame();
    }

    void click(TileCoord tile) { REQUIRE(world.walk_own_to(tile).has_value()); }

    void frame() {
        constexpr double kSeconds = 1.0 / kFramesPerSecond;
        session.update(FrameTime{kSeconds, elapsed_seconds_, frame_index_});
        world.update(kSeconds);
        if (const auto request = world.take_walk_request(); request && !adapter.send_request(*request)) {
            ++refused_requests;
        }
        elapsed_seconds_ += kSeconds;
        ++frame_index_;
    }

    void run(double seconds) {
        const auto frames = static_cast<int>(std::lround(seconds * kFramesPerSecond));
        for (int index = 0; index < frames; ++index) {
            frame();
        }
    }

    [[nodiscard]] const Entity& own() const {
        REQUIRE(world.own() != nullptr);
        return *world.own();
    }

    [[nodiscard]] ServerInbox inbox() const {
        ServerInbox inbox;
        for (const SentBlock& block : connections.sent) {
            read_request(block, inbox);
        }
        return inbox;
    }

    ScriptedConnections connections;
    TestAdapter adapter;
    World world;
    GameSession session;
    int refused_requests = 0;

private:
    void on_map_entered(const MapEntered& entered, MapLoadTicket ticket) override {
        world.enter_map(entered);
        ticket_ = ticket;
    }

    void on_world_event(const GameEvent& event) override { world.apply(event); }

    std::optional<MapLoadTicket> ticket_;
    double elapsed_seconds_ = 0.0;
    std::uint64_t frame_index_ = 0;
};

[[nodiscard]] Bytes appeared(int entity, TileCoord tile) {
    return framed({kEntityAppearedMessage, entity, tile.x, tile.z});
}

[[nodiscard]] Bytes walked(int entity, TileCoord destination) {
    return framed({kEntityWalkedMessage, entity, destination.x, destination.z});
}

[[nodiscard]] Bytes corrected(int entity, TileCoord tile) {
    return framed({kPositionCorrectedMessage, entity, tile.x, tile.z});
}

[[nodiscard]] Bytes confirmed(TileCoord origin, TileCoord destination) {
    return framed({kWalkConfirmedMessage, origin.x, origin.z, destination.x, destination.z});
}

[[nodiscard]] Bytes tiles_changed(TileCoord first, TileCoord last, bool walkable) {
    return framed({kTilesChangedMessage, first.x, first.z, last.x, last.z, walkable ? 1 : 0});
}

// The stages of a session in which a server can misbehave.
enum class Stage : std::uint8_t {
    Connecting,
    EnteringMap,
    LoadingMap,
    InWorld,
    ChangingMap,
};

[[nodiscard]] SessionPhase phase_of(Stage stage) noexcept {
    switch (stage) {
    case Stage::Connecting:
        return SessionPhase::Connecting;
    case Stage::EnteringMap:
    case Stage::LoadingMap:
        return SessionPhase::EnteringMap;
    case Stage::InWorld:
        return SessionPhase::InWorld;
    case Stage::ChangingMap:
        break;
    }
    return SessionPhase::ChangingMap;
}

// Takes the game to a stage with something happening: someone waiting to appear while the map loads, and the
// player walking in the world.
void reach(TestGame& game, Stage stage) {
    if (stage == Stage::Connecting) {
        return;
    }
    game.server_accepts_connection();
    if (stage == Stage::EnteringMap) {
        return;
    }
    game.receive(framed({kMapEnteredMessage, kFirstMap, kPlayer, kStart.x, kStart.z}));
    game.receive(appeared(kOther, {6, 6}));
    if (stage == Stage::LoadingMap) {
        return;
    }
    game.finish_loading();
    game.frame();
    game.click(kFar);
    game.frame();
    if (stage == Stage::ChangingMap) {
        game.receive(framed({kChangingMapMessage}));
    }
}

} // namespace

TEST_CASE("a server that checks steps receives the route in parts", "[0000-CA-15]") {
    TestGame game(kStepsOverStream);
    game.enter_world();

    game.click(kFar);
    game.run(kEnoughToArrive);

    const ServerInbox inbox = game.inbox();
    REQUIRE(inbox.walks.size() == 2);
    CHECK(inbox.walks[0].origin == kStart);
    CHECK(inbox.walks[0].destination == TileCoord{7, 3});
    CHECK(inbox.walks[1].origin == inbox.walks[0].destination);
    CHECK(inbox.walks[1].destination == kFar);
    CHECK(inbox.walks[0].delivery == Delivery::ReliableInOrder);
    CHECK(inbox.map_loaded_notices == 1);
}

TEST_CASE("a server that finds the path receives only the destination, without a delivery guarantee",
          "[0000-CA-15][0000-CA-16]") {
    TestGame game(kDestinationOverMessages);
    game.enter_world();

    game.click(kFar);
    game.run(kEnoughToArrive);

    const ServerInbox inbox = game.inbox();
    REQUIRE(inbox.walks.size() == 1);
    CHECK_FALSE(inbox.walks[0].origin.has_value());
    CHECK(inbox.walks[0].destination == kFar);
    CHECK(inbox.walks[0].delivery == Delivery::Unreliable);
    CHECK(inbox.map_loaded_notices == 0);
}

TEST_CASE("a server that refuses a walk without answering does not stop the player",
          "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    TestGame game(server);
    game.enter_world();

    game.click(kFar);
    game.run(kEnoughToArrive);

    CHECK(game.own().tile() == kFar);
    CHECK_FALSE(game.own().motion.is_walking());
    CHECK(game.session.phase() == SessionPhase::InWorld);
    CHECK(game.refused_requests == 0);
}

TEST_CASE("a server that shortens a walk without saying why stops the player earlier",
          "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    TestGame game(server);
    game.enter_world();
    game.click(kFar);
    game.frame();

    game.receive(confirmed(kStart, {5, 3}));
    game.run(kEnoughToArrive);

    CHECK(game.own().tile() == TileCoord{5, 3});
    CHECK_FALSE(game.own().motion.is_walking());
    CHECK(game.world.stale_confirmation_count() == 0);
    CHECK(game.world.refused_event_count() == 0);
    CHECK(game.session.phase() == SessionPhase::InWorld);
}

TEST_CASE("an answer the server sends late, after a correction, does not undo it",
          "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    TestGame game(server);
    game.enter_world();
    game.click(kFar);
    game.frame();

    game.receive(corrected(kPlayer, {3, 5}));
    game.receive(confirmed(kStart, {5, 3}));
    game.run(kEnoughToArrive);

    CHECK(game.own().tile() == TileCoord{3, 5});
    CHECK_FALSE(game.own().motion.is_walking());
    CHECK(game.world.stale_confirmation_count() == 1);
}

TEST_CASE("a server that reuses a number replaces what the number named", "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    TestGame game(server);
    game.enter_world();
    game.receive(appeared(kOther, {6, 6}));
    game.receive(walked(kOther, {12, 6}));

    game.receive(appeared(kOther, {10, 10}));
    game.run(kEnoughToArrive);

    CHECK(game.world.entities().size() == 2);
    const Entity* other = game.world.find(EntityId{kOther});
    REQUIRE(other != nullptr);
    CHECK(other->tile() == TileCoord{10, 10});
    CHECK_FALSE(other->motion.is_walking());
}

TEST_CASE("a server that talks about an unknown number creates nothing", "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    constexpr int kUnknown = 9;
    TestGame game(server);
    game.enter_world();

    game.receive(walked(kUnknown, {8, 8}));
    game.receive(corrected(kUnknown, {8, 8}));
    game.receive(framed({kEntityLeftMessage, kUnknown}));

    CHECK(game.world.entities().size() == 1);
    CHECK(game.world.unknown_entity_event_count() == 3);
    CHECK(game.session.phase() == SessionPhase::InWorld);
    CHECK(game.adapter.ignored_messages() == 0);
}

TEST_CASE("a server that sends things while the map loads has them applied in order once it loads",
          "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    constexpr int kPasserby = 3;
    TestGame game(server);
    game.server_accepts_connection();
    game.receive(framed({kMapEnteredMessage, kFirstMap, kPlayer, kStart.x, kStart.z}));

    game.arrive(appeared(kOther, {6, 6}));
    game.arrive(walked(kOther, {8, 6}));
    game.arrive(appeared(kPasserby, {7, 7}));
    game.arrive(framed({kEntityLeftMessage, kPasserby}));
    game.run(1.0);
    CHECK(game.world.entities().size() == 1);

    game.finish_loading();
    game.run(kEnoughToArrive);

    const Entity* other = game.world.find(EntityId{kOther});
    REQUIRE(other != nullptr);
    CHECK(other->tile() == TileCoord{8, 6});
    CHECK(game.world.find(EntityId{kPasserby}) == nullptr);
    CHECK(game.world.unknown_entity_event_count() == 0);
    CHECK(game.world.refused_event_count() == 0);
    CHECK(game.session.discarded_event_count() == 0);
    CHECK(game.inbox().map_loaded_notices == (server.capabilities.needs_map_loaded_notice ? 1 : 0));
}

TEST_CASE("a server that refuses a change of map leaves the world as it was", "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    TestGame game(server);
    game.enter_world();
    game.receive(appeared(kOther, {6, 6}));
    game.receive(framed({kChangingMapMessage}));
    REQUIRE(game.session.phase() == SessionPhase::ChangingMap);
    game.receive(walked(kOther, {8, 6}));

    game.receive(framed({kMapChangeRefusedMessage}));
    game.click(kFar);
    game.run(kEnoughToArrive);

    CHECK(game.session.phase() == SessionPhase::InWorld);
    const Entity* other = game.world.find(EntityId{kOther});
    REQUIRE(other != nullptr);
    CHECK(other->tile() == TileCoord{8, 6});
    CHECK(game.own().tile() == kFar);
    CHECK_FALSE(game.inbox().walks.empty());
    CHECK(game.session.discarded_event_count() == 0);
}

TEST_CASE("a server that drops the connection at any stage ends the session, and the game keeps running",
          "[0000-CA-14][0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    const Stage stage = GENERATE(Stage::Connecting, Stage::EnteringMap, Stage::LoadingMap, Stage::InWorld,
                                 Stage::ChangingMap);
    INFO("server: " << server.name << ", stage: " << static_cast<int>(stage));
    TestGame game(server);
    reach(game, stage);
    REQUIRE(game.session.phase() == phase_of(stage));

    game.connections.arrive_closed(CloseReason::ConnectionLost);
    game.frame();
    game.arrive(appeared(5, {8, 8}));
    game.run(1.0);

    CHECK(game.session.phase() == SessionPhase::Ended);
    CHECK(game.session.end_reason() == SessionEndReason::ConnectionLost);
    CHECK(game.world.find(EntityId{5}) == nullptr);
    if (stage == Stage::LoadingMap) {
        CHECK(game.session.discarded_event_count() == 1);
        game.finish_loading();
        game.frame();
        CHECK(game.session.phase() == SessionPhase::Ended);
        CHECK(game.world.find(EntityId{kOther}) == nullptr);
    }
}

TEST_CASE("a request after the connection dropped is refused without waiting", "[0000-CA-14][0000-CA-15]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    TestGame game(server);
    game.enter_world();
    game.connections.arrive_closed(CloseReason::ConnectionLost);
    game.frame();

    game.click(kFar);
    game.frame();

    CHECK(game.refused_requests == 1);
    CHECK(game.inbox().walks.empty());
}

TEST_CASE("a server that goes silent before the world, without closing, is given up on in time",
          "[0000-CA-13][0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    const Stage stage = GENERATE(Stage::Connecting, Stage::EnteringMap);
    INFO("server: " << server.name << ", stage: " << static_cast<int>(stage));
    TestGame game(server);
    reach(game, stage);

    game.run(kServerWait - 0.5);
    CHECK(game.session.phase() == phase_of(stage));
    game.run(1.0);

    CHECK(game.session.phase() == SessionPhase::Ended);
    CHECK(game.session.end_reason() == SessionEndReason::TimedOut);
    CHECK(game.connections.stopped == game.connections.started);
}

TEST_CASE("a server that goes silent in the world does not end the session", "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    TestGame game(server);
    game.enter_world();

    game.click(kFar);
    game.run(kServerWait * 4);

    CHECK(game.session.phase() == SessionPhase::InWorld);
    CHECK(game.own().tile() == kFar);
    CHECK(game.connections.stopped.empty());
}

TEST_CASE("a server that changes the grid during the game changes where the player can go",
          "[0000-CA-15][0000-CA-16]") {
    const ServerKind server = GENERATE(kStepsOverStream, kDestinationOverMessages);
    INFO("server: " << server.name);
    constexpr TileCoord kWallStart{6, 0};
    constexpr TileCoord kWallEnd{6, kMapSide - 1};
    TestGame game(server);
    game.enter_world();

    game.receive(tiles_changed(kWallStart, kWallEnd, false));
    const auto blocked = game.world.walk_own_to(kFar);
    REQUIRE_FALSE(blocked.has_value());
    CHECK(blocked.error() == PathFailure::Unreachable);
    game.frame();
    CHECK(game.inbox().walks.empty());

    game.receive(tiles_changed(kWallStart, kWallEnd, true));
    game.click(kFar);
    game.run(kEnoughToArrive);

    CHECK(game.own().tile() == kFar);
    CHECK(game.adapter.ignored_messages() == 0);
}
