#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "AdapterTestSupport.hpp"

using Catch::Matchers::ContainsSubstring;
using lemuria::classic::AdapterCapabilities;
using lemuria::classic::check_capabilities;
using lemuria::classic::EntityAppeared;
using lemuria::classic::EntityId;
using lemuria::classic::EntityLeaveReason;
using lemuria::classic::EntityLeft;
using lemuria::classic::EntityWalked;
using lemuria::classic::GameEvent;
using lemuria::classic::GameEventSink;
using lemuria::classic::GameRequest;
using lemuria::classic::MapEntered;
using lemuria::classic::MapId;
using lemuria::classic::MapLoadedNotice;
using lemuria::classic::OwnWalkConfirmed;
using lemuria::classic::PositionCorrected;
using lemuria::classic::ProtocolAdapter;
using lemuria::classic::SessionEnded;
using lemuria::classic::SessionEndReason;
using lemuria::classic::SessionPhase;
using lemuria::classic::SessionPhaseReached;
using lemuria::classic::TileCoord;
using lemuria::classic::WalkRequest;
using lemuria::classic::WalkRequestMode;
using lemuria::classic::WalkSteps;
using lemuria::classic::testing::Bytes;
using lemuria::classic::testing::framed;
using lemuria::classic::testing::kEntityAppearedMessage;
using lemuria::classic::testing::kEntityLeftMessage;
using lemuria::classic::testing::kEntityWalkedMessage;
using lemuria::classic::testing::kMapEnteredMessage;
using lemuria::classic::testing::kMapLoadedMessage;
using lemuria::classic::testing::kMessageTraits;
using lemuria::classic::testing::kPositionCorrectedMessage;
using lemuria::classic::testing::kStreamTraits;
using lemuria::classic::testing::kWalkConfirmedMessage;
using lemuria::classic::testing::kWalkDestinationMessage;
using lemuria::classic::testing::kWalkStepsMessage;
using lemuria::classic::testing::ScriptedConnections;
using lemuria::classic::testing::TestAdapter;
using lemuria::core::CloseReason;
using lemuria::core::Delivery;

namespace {

constexpr AdapterCapabilities kStepsCapabilities{
    .connection_kind = kStreamTraits.kind,
    .walk_requests = WalkRequestMode::Steps,
    .max_steps_per_request = 2,
    .needs_map_loaded_notice = true,
};

constexpr AdapterCapabilities kDestinationCapabilities{
    .connection_kind = kMessageTraits.kind,
    .walk_requests = WalkRequestMode::DestinationOnly,
};

class EventLog final : public GameEventSink {
public:
    void deliver(const GameEvent& event) override { events.push_back(event); }

    std::vector<GameEvent> events;
};

// Starts a session and leaves the connection ready.
void connect(ProtocolAdapter& adapter, ScriptedConnections& connections, EventLog& log) {
    REQUIRE(adapter.begin_session().has_value());
    connections.arrive_connected();
    adapter.poll(log);
}

// Builds a walk from what the adapter declares, as the framework does: the code knows no adapter in
// particular.
[[nodiscard]] WalkRequest walk_for(const ProtocolAdapter& adapter, TileCoord origin,
                                   std::span<const TileCoord> path) {
    const AdapterCapabilities& capabilities = adapter.capabilities();
    WalkRequest request{origin, path.back(), {}};
    if (capabilities.walk_requests == WalkRequestMode::Steps) {
        const auto part = path.first(std::min(path.size(), capabilities.max_steps_per_request));
        for (const TileCoord step : part) {
            REQUIRE(request.steps.add(step));
        }
        request.destination = part.back();
    }
    return request;
}

[[nodiscard]] std::vector<Bytes> world_messages() {
    return {
        framed({kMapEnteredMessage, 7, 1, 10, 11}),
        framed({kEntityAppearedMessage, 2, 12, 13}),
        framed({kEntityWalkedMessage, 2, 14, 13}),
        framed({kEntityWalkedMessage, 2, 15, 13, 14, 13, 15, 13}),
        framed({kWalkConfirmedMessage, 10, 11, 12, 11}),
        framed({kPositionCorrectedMessage, 1, 10, 10}),
        framed({kEntityLeftMessage, 2}),
    };
}

[[nodiscard]] std::vector<GameEvent> world_events() {
    WalkSteps steps;
    REQUIRE(steps.add({14, 13}));
    REQUIRE(steps.add({15, 13}));
    return {
        SessionPhaseReached{SessionPhase::EnteringMap},
        MapEntered{MapId{7}, EntityId{1}, TileCoord{10, 11}},
        EntityAppeared{EntityId{2}, TileCoord{12, 13}, std::nullopt},
        EntityWalked{EntityId{2}, std::nullopt, TileCoord{14, 13}, WalkSteps{}},
        EntityWalked{EntityId{2}, std::nullopt, TileCoord{15, 13}, steps},
        OwnWalkConfirmed{TileCoord{10, 11}, TileCoord{12, 11}},
        PositionCorrected{EntityId{1}, TileCoord{10, 10}},
        EntityLeft{EntityId{2}, EntityLeaveReason::LeftView},
    };
}

} // namespace

TEST_CASE("adapters with different capabilities are valid under the same contract", "[0000-CA-15]") {
    const AdapterCapabilities capabilities = GENERATE(kStepsCapabilities, kDestinationCapabilities);

    CHECK(check_capabilities(capabilities).has_value());
}

TEST_CASE("a walk request carries the steps when the adapter sends steps", "[0000-CA-15]") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);
    const std::vector<TileCoord> path{{2, 1}, {3, 1}, {4, 1}};

    REQUIRE(adapter.send_request(walk_for(adapter, {1, 1}, path)).has_value());

    REQUIRE(connections.sent.size() == 1);
    CHECK(connections.sent[0].bytes == framed({kWalkStepsMessage, 1, 1, 3, 1, 2, 1, 3, 1}));
}

TEST_CASE("a walk request carries only the destination when the adapter sends only that", "[0000-CA-15]") {
    ScriptedConnections connections(kMessageTraits);
    TestAdapter adapter(connections, kDestinationCapabilities);
    EventLog log;
    connect(adapter, connections, log);
    const std::vector<TileCoord> path{{2, 1}, {3, 1}, {4, 1}};

    REQUIRE(adapter.send_request(walk_for(adapter, {1, 1}, path)).has_value());

    REQUIRE(connections.sent.size() == 1);
    CHECK(connections.sent[0].bytes == framed({kWalkDestinationMessage, 4, 1}));
}

TEST_CASE("the map loaded notice reaches the server as a request", "[0000-CA-15]") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);

    REQUIRE(adapter.send_request(GameRequest{MapLoadedNotice{MapId{7}}}).has_value());

    REQUIRE(connections.sent.size() == 1);
    CHECK(connections.sent[0].bytes == framed({kMapLoadedMessage, 7}));
}

TEST_CASE("the same events come from a stream of bytes split in pieces", "[0000-CA-16]") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);

    for (const Bytes& message : world_messages()) {
        for (const std::byte value : message) {
            connections.arrive_bytes(Bytes{value}, false);
        }
    }
    adapter.poll(log);

    CHECK(log.events == world_events());
    CHECK(adapter.ignored_messages() == 0);
}

TEST_CASE("the same events come from several messages joined in one block", "[0000-CA-16]") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);

    Bytes joined;
    for (const Bytes& message : world_messages()) {
        joined.insert(joined.end(), message.begin(), message.end());
    }
    connections.arrive_bytes(std::move(joined), false);
    adapter.poll(log);

    CHECK(log.events == world_events());
}

TEST_CASE("the same events come from a connection that delivers whole messages", "[0000-CA-16]") {
    ScriptedConnections connections(kMessageTraits);
    TestAdapter adapter(connections, kDestinationCapabilities);
    EventLog log;
    connect(adapter, connections, log);

    for (Bytes& message : world_messages()) {
        connections.arrive_bytes(std::move(message), true);
    }
    adapter.poll(log);

    CHECK(log.events == world_events());
}

TEST_CASE("a walk can go without a delivery guarantee on a connection that offers it", "[0000-CA-16]") {
    ScriptedConnections connections(kMessageTraits);
    TestAdapter adapter(connections, kDestinationCapabilities, Delivery::Unreliable);
    EventLog log;
    connect(adapter, connections, log);
    const std::vector<TileCoord> path{{2, 1}};

    REQUIRE(adapter.send_request(walk_for(adapter, {1, 1}, path)).has_value());

    REQUIRE(connections.sent.size() == 1);
    CHECK(connections.sent[0].delivery == Delivery::Unreliable);
}

TEST_CASE("a delivery option the connection does not offer is a clear error", "[0000-CA-16]") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities, Delivery::Unreliable);
    EventLog log;
    connect(adapter, connections, log);
    const std::vector<TileCoord> path{{2, 1}};

    const auto sent = adapter.send_request(walk_for(adapter, {1, 1}, path));

    REQUIRE_FALSE(sent.has_value());
    CHECK_THAT(sent.error().message, ContainsSubstring("unreliable"));
    CHECK(connections.sent.empty());
}

TEST_CASE("a request before the connection is ready is an error and does not wait") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    REQUIRE(adapter.begin_session().has_value());
    const std::vector<TileCoord> path{{2, 1}};

    const auto sent = adapter.send_request(walk_for(adapter, {1, 1}, path));

    REQUIRE_FALSE(sent.has_value());
    CHECK(connections.sent.empty());
}

TEST_CASE("a broken message format ends the session once") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);
    log.events.clear();

    connections.arrive_bytes(Bytes{std::byte{0}}, false);
    connections.arrive_bytes(framed({kEntityLeftMessage, 2}), false);
    adapter.poll(log);
    adapter.poll(log);

    CHECK(log.events == std::vector<GameEvent>{SessionEnded{SessionEndReason::InvalidData}});
    CHECK(connections.stopped.size() == 1);
}

TEST_CASE("a block that is not a whole message ends the session", "[0000-CA-16]") {
    ScriptedConnections connections(kMessageTraits);
    TestAdapter adapter(connections, kDestinationCapabilities);
    EventLog log;
    connect(adapter, connections, log);
    log.events.clear();

    connections.arrive_bytes(Bytes{std::byte{5}, std::byte{1}}, true);
    adapter.poll(log);

    CHECK(log.events == std::vector<GameEvent>{SessionEnded{SessionEndReason::InvalidData}});
}

TEST_CASE("a problem in a single message is only counted") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);
    log.events.clear();

    connections.arrive_bytes(framed({99, 1, 2}), false);
    connections.arrive_bytes(framed({kEntityLeftMessage}), false);
    connections.arrive_bytes(framed({kEntityLeftMessage, 2}), false);
    adapter.poll(log);

    CHECK(adapter.ignored_messages() == 2);
    CHECK(log.events == std::vector<GameEvent>{EntityLeft{EntityId{2}, EntityLeaveReason::LeftView}});
    CHECK(connections.stopped.empty());
}

TEST_CASE("the end of the connection reaches the game as the end of the session") {
    const auto [closed, ended] = GENERATE(table<CloseReason, SessionEndReason>({
        {CloseReason::PeerClosed, SessionEndReason::ClosedByServer},
        {CloseReason::CouldNotConnect, SessionEndReason::CouldNotConnect},
        {CloseReason::ConnectionLost, SessionEndReason::ConnectionLost},
        {CloseReason::TimedOut, SessionEndReason::TimedOut},
    }));
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);
    log.events.clear();

    connections.arrive_closed(closed);
    adapter.poll(log);

    CHECK(log.events == std::vector<GameEvent>{SessionEnded{ended}});
}

TEST_CASE("the player ending the session is reported on the next poll") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kStepsCapabilities);
    EventLog log;
    connect(adapter, connections, log);
    log.events.clear();

    adapter.end_session();
    CHECK(log.events.empty());
    adapter.poll(log);

    CHECK(log.events == std::vector<GameEvent>{SessionEnded{SessionEndReason::ClosedByPlayer}});
}
