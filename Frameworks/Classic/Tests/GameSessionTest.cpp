#include <cstdint>
#include <initializer_list>
#include <limits>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "AdapterTestSupport.hpp"
#include "Lemuria/Classic/GameSession.hpp"

using Catch::Matchers::ContainsSubstring;
using lemuria::classic::AdapterCapabilities;
using lemuria::classic::EntityId;
using lemuria::classic::EntityLeaveReason;
using lemuria::classic::EntityLeft;
using lemuria::classic::GameEvent;
using lemuria::classic::GameRequest;
using lemuria::classic::GameSession;
using lemuria::classic::MapChangeRefused;
using lemuria::classic::MapEntered;
using lemuria::classic::MapId;
using lemuria::classic::MapLoadedNotice;
using lemuria::classic::MapLoadTicket;
using lemuria::classic::SessionEnded;
using lemuria::classic::SessionEndReason;
using lemuria::classic::SessionLimits;
using lemuria::classic::SessionListener;
using lemuria::classic::SessionPhase;
using lemuria::classic::SessionPhaseReached;
using lemuria::classic::TileCoord;
using lemuria::classic::testing::framed;
using lemuria::classic::testing::kEntityLeftMessage;
using lemuria::classic::testing::kMapEnteredMessage;
using lemuria::classic::testing::kScriptedCapabilities;
using lemuria::classic::testing::kStreamTraits;
using lemuria::classic::testing::ScriptedAdapter;
using lemuria::classic::testing::ScriptedConnections;
using lemuria::classic::testing::TestAdapter;
using lemuria::core::CloseReason;
using lemuria::core::FrameTime;

namespace {

constexpr double kConnecting = 5.0;
constexpr double kAuthenticating = 6.0;
constexpr double kMapEntry = 7.0;
constexpr double kMapLoad = 8.0;
constexpr double kVeryLongWait = 1'000'000.0;

constexpr SessionLimits kLimits{
    .connecting_seconds = kConnecting,
    .authenticating_seconds = kAuthenticating,
    .map_entry_seconds = kMapEntry,
    .map_load_seconds = kMapLoad,
    .max_held_events = 4,
    .max_released_events_per_frame = 2,
};

constexpr MapEntered kFirstMap{MapId{1}, EntityId{100}, TileCoord{3, 3}};
constexpr MapEntered kSecondMap{MapId{2}, EntityId{100}, TileCoord{9, 9}};

[[nodiscard]] GameEvent left(std::uint32_t entity) {
    return EntityLeft{EntityId{entity}, EntityLeaveReason::LeftView};
}

[[nodiscard]] std::vector<GameEvent> left_list(std::initializer_list<std::uint32_t> entities) {
    std::vector<GameEvent> events;
    for (const std::uint32_t entity : entities) {
        events.push_back(left(entity));
    }
    return events;
}

class RecordingListener final : public SessionListener {
public:
    void on_map_entered(const MapEntered& entered, MapLoadTicket ticket) override {
        maps.push_back(entered);
        tickets.push_back(ticket);
    }

    void on_world_event(const GameEvent& event) override { world.push_back(event); }

    std::vector<MapEntered> maps;
    std::vector<MapLoadTicket> tickets;
    std::vector<GameEvent> world;
};

class SessionFixture {
public:
    explicit SessionFixture(const AdapterCapabilities& capabilities = kScriptedCapabilities,
                            const SessionLimits& limits = kLimits)
        : adapter(capabilities), session(adapter, limits, listener) {
        REQUIRE(session.start().has_value());
    }

    void frame(double seconds = 0.0) { session.update(FrameTime{seconds, 0.0, 0}); }

    void receive(std::initializer_list<GameEvent> events) {
        for (const GameEvent& event : events) {
            adapter.arrive(event);
        }
        frame();
    }

    void finish_load() { session.finish_map_load(listener.tickets.back()); }

    void enter_world() {
        receive({kFirstMap});
        finish_load();
        frame();
    }

    ScriptedAdapter adapter;
    RecordingListener listener;
    GameSession session;
};

} // namespace

TEST_CASE("session tells the game which map to load", "[0000-CA-13]") {
    SessionFixture fixture;

    fixture.receive({kFirstMap});

    REQUIRE(fixture.listener.maps.size() == 1);
    CHECK(fixture.listener.maps[0] == kFirstMap);
    CHECK(fixture.listener.tickets[0].map == kFirstMap.map);
}

TEST_CASE("session holds the events that arrive while the map loads", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});

    fixture.receive({left(1), left(2)});
    fixture.frame(1.0);
    CHECK(fixture.listener.world.empty());

    fixture.finish_load();
    fixture.frame();

    CHECK(fixture.listener.world == left_list({1, 2}));
}

TEST_CASE("session releases held events a few at a time and in order", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});
    fixture.receive({left(1), left(2), left(3)});
    fixture.finish_load();

    fixture.receive({left(4)});
    CHECK(fixture.listener.world == left_list({1, 2}));

    fixture.frame();
    CHECK(fixture.listener.world == left_list({1, 2, 3, 4}));

    fixture.receive({left(5)});
    CHECK(fixture.listener.world == left_list({1, 2, 3, 4, 5}));
}

TEST_CASE("session delivers events right away once the world is ready", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.enter_world();

    fixture.receive({left(1)});

    CHECK(fixture.listener.world == left_list({1}));
}

TEST_CASE("session drops events that arrive before any map", "[0000-CA-13]") {
    SessionFixture fixture;

    fixture.receive({left(1)});
    fixture.receive({kFirstMap});
    fixture.finish_load();
    fixture.frame();

    CHECK(fixture.listener.world.empty());
    CHECK(fixture.session.discarded_event_count() == 1);
}

TEST_CASE("session discards events held for the previous map", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});
    fixture.receive({left(1), left(2)});

    fixture.receive({kSecondMap});
    fixture.receive({left(3)});
    fixture.finish_load();
    fixture.frame();

    CHECK(fixture.listener.world == left_list({3}));
    CHECK(fixture.session.discarded_event_count() == 2);
}

TEST_CASE("session discards events of the previous map that were still being released", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});
    fixture.receive({left(1), left(2), left(3)});
    fixture.finish_load();
    fixture.frame();
    REQUIRE(fixture.listener.world == left_list({1, 2}));

    fixture.receive({kSecondMap});
    fixture.finish_load();
    fixture.frame();

    CHECK(fixture.listener.world == left_list({1, 2}));
    CHECK(fixture.session.discarded_event_count() == 1);
}

TEST_CASE("session ignores the end of a load that no longer matters", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});
    fixture.receive({kSecondMap});
    fixture.receive({left(1)});

    fixture.session.finish_map_load(fixture.listener.tickets[0]);
    fixture.frame();

    CHECK(fixture.session.phase() == SessionPhase::ChangingMap);
    CHECK(fixture.listener.world.empty());

    fixture.session.finish_map_load(fixture.listener.tickets[1]);
    fixture.frame();

    CHECK(fixture.session.phase() == SessionPhase::InWorld);
    CHECK(fixture.listener.world == left_list({1}));
}

TEST_CASE("session gives every map load its own ticket", "[0000-CA-13]") {
    SessionFixture fixture;

    fixture.receive({kFirstMap});
    fixture.receive({kFirstMap});

    REQUIRE(fixture.listener.tickets.size() == 2);
    CHECK_FALSE(fixture.listener.tickets[0] == fixture.listener.tickets[1]);
}

TEST_CASE("session goes through the stages of the first entry", "[0000-CA-13]") {
    SessionFixture fixture;
    CHECK(fixture.session.phase() == SessionPhase::Connecting);

    fixture.receive({SessionPhaseReached{SessionPhase::EnteringMap}});
    CHECK(fixture.session.phase() == SessionPhase::EnteringMap);

    fixture.receive({kFirstMap});
    CHECK(fixture.session.phase() == SessionPhase::EnteringMap);

    fixture.finish_load();
    CHECK(fixture.session.phase() == SessionPhase::InWorld);
}

TEST_CASE("session goes through the stages of a change of map", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.enter_world();

    fixture.receive({kSecondMap});
    CHECK(fixture.session.phase() == SessionPhase::ChangingMap);

    fixture.finish_load();
    CHECK(fixture.session.phase() == SessionPhase::InWorld);
}

TEST_CASE("session keeps the world running while the server answers a change of map", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.enter_world();

    fixture.receive({SessionPhaseReached{SessionPhase::ChangingMap}});
    fixture.receive({left(1)});

    CHECK(fixture.session.phase() == SessionPhase::ChangingMap);
    CHECK(fixture.listener.world == left_list({1}));
}

TEST_CASE("session returns to the world when a change of map is refused", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.enter_world();
    fixture.receive({SessionPhaseReached{SessionPhase::ChangingMap}});

    fixture.receive({MapChangeRefused{}});

    CHECK(fixture.session.phase() == SessionPhase::InWorld);
    CHECK(fixture.session.discarded_event_count() == 0);
}

TEST_CASE("session ignores a refusal when no change of map can be refused", "[0000-CA-13]") {
    SessionFixture fixture;

    SECTION("in the world") {
        fixture.enter_world();
        fixture.receive({MapChangeRefused{}});
        CHECK(fixture.session.phase() == SessionPhase::InWorld);
    }
    SECTION("while the new map loads") {
        fixture.enter_world();
        fixture.receive({kSecondMap});
        fixture.receive({MapChangeRefused{}});
        CHECK(fixture.session.phase() == SessionPhase::ChangingMap);
    }
    SECTION("before any map") {
        fixture.receive({SessionPhaseReached{SessionPhase::ChangingMap}});
        fixture.receive({MapChangeRefused{}});
        CHECK(fixture.session.phase() == SessionPhase::ChangingMap);
    }

    CHECK(fixture.session.discarded_event_count() == 1);
}

TEST_CASE("session ends when more events arrive than it can hold", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});

    fixture.receive({left(1), left(2), left(3), left(4), left(5)});

    CHECK(fixture.session.phase() == SessionPhase::Ended);
    CHECK(fixture.session.end_reason() == SessionEndReason::TooManyHeldEvents);
    CHECK(fixture.adapter.end_requests == 1);
    CHECK(fixture.listener.world.empty());
}

TEST_CASE("session tells the server the map is loaded only when the server asks for it", "[0000-CA-13]") {
    AdapterCapabilities capabilities = kScriptedCapabilities;
    capabilities.needs_map_loaded_notice = true;
    SessionFixture asking(capabilities);
    SessionFixture silent;

    asking.enter_world();
    silent.enter_world();

    CHECK(asking.adapter.requests == std::vector<GameRequest>{MapLoadedNotice{kFirstMap.map}});
    CHECK(silent.adapter.requests.empty());
}

TEST_CASE("session ends when the map loaded notice cannot be sent", "[0000-CA-13]") {
    AdapterCapabilities capabilities = kScriptedCapabilities;
    capabilities.needs_map_loaded_notice = true;
    SessionFixture fixture(capabilities);
    fixture.adapter.refuses_to_send = true;

    fixture.enter_world();

    CHECK(fixture.session.phase() == SessionPhase::Ended);
    CHECK(fixture.session.end_reason() == SessionEndReason::ConnectionLost);
    CHECK(fixture.adapter.end_requests == 1);
}

TEST_CASE("session gives up on every wait for the server when its time is over", "[0000-CA-13]") {
    const auto [phase, limit] = GENERATE(table<SessionPhase, double>({
        {SessionPhase::Connecting, kConnecting},
        {SessionPhase::Reconnecting, kConnecting},
        {SessionPhase::Authenticating, kAuthenticating},
        {SessionPhase::EnteringMap, kMapEntry},
        {SessionPhase::ChangingMap, kMapEntry},
    }));
    SessionFixture fixture;
    fixture.receive({SessionPhaseReached{phase}});

    fixture.frame(limit - 0.5);
    CHECK(fixture.session.phase() == phase);
    CHECK(fixture.adapter.end_requests == 0);

    fixture.frame(0.5);
    CHECK(fixture.session.phase() == SessionPhase::Ended);
    CHECK(fixture.session.end_reason() == SessionEndReason::TimedOut);
    CHECK(fixture.adapter.end_requests == 1);
}

TEST_CASE("session gives up on a map that takes too long to load", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({SessionPhaseReached{SessionPhase::EnteringMap}});
    fixture.frame(kMapEntry - 1.0);
    fixture.receive({kFirstMap});

    fixture.frame(kMapLoad - 0.5);
    CHECK(fixture.session.phase() == SessionPhase::EnteringMap);

    fixture.frame(0.5);
    CHECK(fixture.session.phase() == SessionPhase::Ended);
    CHECK(fixture.session.end_reason() == SessionEndReason::TimedOut);
}

TEST_CASE("session does not limit the time with the player or in the world", "[0000-CA-13]") {
    SessionFixture fixture;

    fixture.receive({SessionPhaseReached{SessionPhase::ChoosingCharacter}});
    fixture.frame(kVeryLongWait);
    CHECK(fixture.session.phase() == SessionPhase::ChoosingCharacter);

    fixture.enter_world();
    fixture.frame(kVeryLongWait);
    CHECK(fixture.session.phase() == SessionPhase::InWorld);
}

TEST_CASE("session counts the time of each stage from its start", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.frame(kConnecting - 1.0);

    fixture.receive({SessionPhaseReached{SessionPhase::Authenticating}});
    fixture.frame(kAuthenticating - 1.0);
    CHECK(fixture.session.phase() == SessionPhase::Authenticating);

    fixture.frame(1.0);
    CHECK(fixture.session.phase() == SessionPhase::Ended);
}

TEST_CASE("session does not get more time when the same stage is reported again", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.frame(kConnecting - 1.0);

    fixture.receive({SessionPhaseReached{SessionPhase::Connecting}});
    fixture.frame(1.0);

    CHECK(fixture.session.phase() == SessionPhase::Ended);
}

TEST_CASE("session decides by itself when the world is ready or over", "[0000-CA-13]") {
    SessionFixture fixture;
    const SessionPhase phase = GENERATE(SessionPhase::InWorld, SessionPhase::Ended);

    fixture.receive({SessionPhaseReached{phase}});

    CHECK(fixture.session.phase() == SessionPhase::Connecting);
    CHECK(fixture.session.discarded_event_count() == 1);
}

TEST_CASE("session stays in the map load when another stage is reported", "[0000-CA-13]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});

    fixture.receive({SessionPhaseReached{SessionPhase::Authenticating}});

    CHECK(fixture.session.phase() == SessionPhase::EnteringMap);
    CHECK(fixture.session.discarded_event_count() == 1);
}

TEST_CASE("session ignores everything after its end", "[0000-CA-14]") {
    SessionFixture fixture;
    fixture.receive({SessionEnded{SessionEndReason::ClosedByServer}});

    fixture.receive({kFirstMap, left(1), SessionPhaseReached{SessionPhase::Authenticating}});
    fixture.receive({SessionEnded{SessionEndReason::TimedOut}});

    CHECK(fixture.session.phase() == SessionPhase::Ended);
    CHECK(fixture.session.end_reason() == SessionEndReason::ClosedByServer);
    CHECK(fixture.listener.maps.empty());
    CHECK(fixture.listener.world.empty());
    CHECK(fixture.session.discarded_event_count() == 3);
}

TEST_CASE("session drops what it was holding when the connection falls", "[0000-CA-14]") {
    SessionFixture fixture;
    fixture.receive({kFirstMap});
    fixture.receive({left(1), left(2)});

    fixture.receive({SessionEnded{SessionEndReason::ConnectionLost}});
    fixture.session.finish_map_load(fixture.listener.tickets.back());
    fixture.frame();

    CHECK(fixture.session.phase() == SessionPhase::Ended);
    CHECK(fixture.session.end_reason() == SessionEndReason::ConnectionLost);
    CHECK(fixture.listener.world.empty());
    CHECK(fixture.session.discarded_event_count() == 2);
}

TEST_CASE("session keeps answering after the connection falls", "[0000-CA-14]") {
    SessionFixture fixture;
    fixture.enter_world();
    fixture.receive({SessionEnded{SessionEndReason::ClosedByServer}});
    const int polls_before = fixture.adapter.poll_count;

    fixture.frame(kVeryLongWait);
    fixture.frame(kVeryLongWait);

    CHECK(fixture.adapter.poll_count == polls_before + 2);
    CHECK(fixture.session.end_reason() == SessionEndReason::ClosedByServer);
    CHECK(fixture.adapter.end_requests == 0);
}

TEST_CASE("session never turns data of a closed connection into events", "[0000-CA-14]") {
    ScriptedConnections connections(kStreamTraits);
    TestAdapter adapter(connections, kScriptedCapabilities);
    RecordingListener listener;
    GameSession session(adapter, kLimits, listener);
    REQUIRE(session.start().has_value());
    connections.arrive_connected();
    connections.arrive_closed(CloseReason::PeerClosed);
    connections.arrive_bytes(framed({kMapEnteredMessage, 1, 1, 2, 2}), false);
    connections.arrive_bytes(framed({kEntityLeftMessage, 2}), false);

    session.update(FrameTime{});

    CHECK(session.phase() == SessionPhase::Ended);
    CHECK(session.end_reason() == SessionEndReason::ClosedByServer);
    CHECK(listener.maps.empty());
    CHECK(listener.world.empty());
    CHECK(connections.ignored_event_count() == 2);
}

TEST_CASE("session reports a server that does not exist", "[0000-CA-14]") {
    ScriptedAdapter adapter;
    adapter.refuses_to_begin = true;
    RecordingListener listener;
    GameSession session(adapter, kLimits, listener);

    const auto started = session.start();

    REQUIRE_FALSE(started.has_value());
    CHECK_THAT(started.error().message, ContainsSubstring("does not exist"));
    CHECK(session.phase() == SessionPhase::Ended);
    CHECK(session.end_reason() == SessionEndReason::CouldNotConnect);
}

TEST_CASE("session ends when the player asks", "[0000-CA-14]") {
    SessionFixture fixture;
    fixture.enter_world();

    fixture.session.end();
    fixture.session.end();
    fixture.frame();

    CHECK(fixture.session.phase() == SessionPhase::Ended);
    CHECK(fixture.session.end_reason() == SessionEndReason::ClosedByPlayer);
    CHECK(fixture.adapter.end_requests == 1);
    CHECK(fixture.session.discarded_event_count() == 0);
}

TEST_CASE("session does nothing before it starts") {
    ScriptedAdapter adapter;
    RecordingListener listener;
    GameSession session(adapter, kLimits, listener);

    session.update(FrameTime{kVeryLongWait, 0.0, 0});
    session.end();

    CHECK(adapter.poll_count == 0);
    CHECK(adapter.end_requests == 0);
    CHECK(session.phase() == SessionPhase::Connecting);
    CHECK_FALSE(session.end_reason().has_value());
}

TEST_CASE("session starts only once") {
    SessionFixture fixture;

    const auto started = fixture.session.start();

    REQUIRE_FALSE(started.has_value());
    CHECK_THAT(started.error().message, ContainsSubstring("once"));
}

TEST_CASE("session refuses an adapter with an invalid description") {
    const AdapterCapabilities capabilities{};
    ScriptedAdapter adapter(capabilities);
    RecordingListener listener;
    GameSession session(adapter, kLimits, listener);

    const auto started = session.start();

    REQUIRE_FALSE(started.has_value());
    CHECK_THAT(started.error().message, ContainsSubstring("kind of connection"));
}

TEST_CASE("session refuses limits it cannot follow") {
    constexpr double kInfinite = std::numeric_limits<double>::infinity();
    constexpr double kNotANumber = std::numeric_limits<double>::quiet_NaN();
    const SessionLimits limits =
        GENERATE(SessionLimits{0.0, 6.0, 7.0, 8.0, 4, 2}, SessionLimits{5.0, 0.0, 7.0, 8.0, 4, 2},
                 SessionLimits{5.0, 6.0, 0.0, 8.0, 4, 2}, SessionLimits{5.0, 6.0, 7.0, 0.0, 4, 2},
                 SessionLimits{-1.0, 6.0, 7.0, 8.0, 4, 2}, SessionLimits{kInfinite, 6.0, 7.0, 8.0, 4, 2},
                 SessionLimits{5.0, 6.0, kNotANumber, 8.0, 4, 2}, SessionLimits{5.0, 6.0, 7.0, 8.0, 0, 2},
                 SessionLimits{5.0, 6.0, 7.0, 8.0, 4, 0});
    ScriptedAdapter adapter;
    RecordingListener listener;
    GameSession session(adapter, limits, listener);

    CHECK_FALSE(session.start().has_value());
}
