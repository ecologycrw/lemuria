#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "DemoClient/DemoAdapter.hpp"
#include "DemoProtocol/Framing.hpp"
#include "DemoProtocol/MessageCodec.hpp"
#include "DemoProtocol/Messages.hpp"
#include "Lemuria/Classic/AdapterCapabilities.hpp"
#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/GameRequest.hpp"
#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/SeededRandom.hpp"
#include "ScriptedConnections.hpp"

namespace classic = lemuria::classic;
namespace core = lemuria::core;
namespace protocol = demo_protocol;

using demo_client::DemoAdapter;
using demo_client::kDemoCapabilities;
using demo_client::testing::append;
using demo_client::testing::Bytes;
using demo_client::testing::bytes_of;
using demo_client::testing::ScriptedConnections;

namespace {

class RecordingSink final : public classic::GameEventSink {
public:
    void deliver(const classic::GameEvent& event) override { events.push_back(event); }

    std::vector<classic::GameEvent> events;
};

class AdapterFixture {
public:
    AdapterFixture() : adapter(connections, core::ConnectionAddress{"127.0.0.1", 24'000}) {}

    // Connects, and forgets the hello and the first event, which other tests check.
    void connect() {
        REQUIRE(adapter.begin_session().has_value());
        connections.arrive_connected();
        poll();
        connections.sent.clear();
        sink.events.clear();
    }

    void poll() { adapter.poll(sink); }

    ScriptedConnections connections;
    DemoAdapter adapter;
    RecordingSink sink;
};

[[nodiscard]] protocol::StepList message_steps(std::initializer_list<protocol::Tile> tiles) {
    protocol::StepList steps;
    for (const protocol::Tile tile : tiles) {
        REQUIRE(steps.add(tile));
    }
    return steps;
}

[[nodiscard]] classic::WalkSteps walk_steps(std::initializer_list<classic::TileCoord> tiles) {
    classic::WalkSteps steps;
    for (const classic::TileCoord tile : tiles) {
        REQUIRE(steps.add(tile));
    }
    return steps;
}

// One message of each kind the server sends, and the event each one must become.
struct Conversation {
    Bytes bytes;
    std::vector<classic::GameEvent> events;
};

[[nodiscard]] Conversation every_kind_of_message() {
    const std::vector<std::pair<protocol::ServerMessage, classic::GameEvent>> pairs{
        {protocol::MapEntered{1, 7, {32, 33}},
         classic::MapEntered{classic::MapId{1}, classic::EntityId{7}, {32, 33}}},
        {protocol::EntityAppeared{8, {30, 31}, std::nullopt},
         classic::EntityAppeared{classic::EntityId{8}, {30, 31}, std::nullopt}},
        {protocol::EntityAppeared{9, {1023, 0}, protocol::Tile{1022, 1}},
         classic::EntityAppeared{classic::EntityId{9}, {1023, 0}, classic::TileCoord{1022, 1}}},
        {protocol::EntityWalked{8, {30, 31}, message_steps({{31, 31}, {32, 32}})},
         classic::EntityWalked{
             classic::EntityId{8}, classic::TileCoord{30, 31}, {32, 32}, walk_steps({{31, 31}, {32, 32}})}},
        {protocol::WalkConfirmed{{32, 33}, {34, 33}}, classic::OwnWalkConfirmed{{32, 33}, {34, 33}}},
        {protocol::PositionCorrected{7, {33, 33}},
         classic::PositionCorrected{classic::EntityId{7}, {33, 33}}},
        {protocol::EntityLeft{8, protocol::LeaveReason::Disconnected},
         classic::EntityLeft{classic::EntityId{8}, classic::EntityLeaveReason::Removed}},
        {protocol::EntityLeft{9, protocol::LeaveReason::LeftView},
         classic::EntityLeft{classic::EntityId{9}, classic::EntityLeaveReason::LeftView}},
    };
    Conversation conversation;
    for (const auto& [message, event] : pairs) {
        append(conversation.bytes, bytes_of(message));
        conversation.events.push_back(event);
    }
    return conversation;
}

[[nodiscard]] bool ends_the_session(const classic::GameEvent& event) {
    return std::holds_alternative<classic::SessionEnded>(event);
}

} // namespace

TEST_CASE("the demo adapter declares what the framework can follow, and what the test server needs") {
    CHECK(classic::check_capabilities(kDemoCapabilities).has_value());
    CHECK(kDemoCapabilities.connection_kind == core::kTcpConnectionKind);
    CHECK(kDemoCapabilities.max_steps_per_request == protocol::kMaxWalkSteps);
    CHECK(kDemoCapabilities.needs_map_loaded_notice);
}

TEST_CASE("the demo adapter says hello with the protocol version as soon as it connects") {
    AdapterFixture fixture;
    REQUIRE(fixture.adapter.begin_session().has_value());
    REQUIRE(fixture.connections.opened.size() == 1);
    CHECK(fixture.connections.opened.front().host == "127.0.0.1");
    CHECK(fixture.connections.opened.front().port == 24'000);
    CHECK(fixture.connections.sent.empty());

    fixture.connections.arrive_connected();
    fixture.poll();

    // Size of the body (3), type of the hello (1) and version 1.
    REQUIRE(fixture.connections.sent.size() == 1);
    CHECK(fixture.connections.sent.front() == bytes_of({0x03, 0x00, 0x01, 0x01, 0x00}));
    CHECK(fixture.sink.events ==
          std::vector<classic::GameEvent>{classic::SessionPhaseReached{classic::SessionPhase::EnteringMap}});
}

TEST_CASE("a hello that cannot leave ends the session as a lost connection, before the map is awaited") {
    AdapterFixture fixture;
    REQUIRE(fixture.adapter.begin_session().has_value());
    fixture.connections.refuses_sends = true;

    fixture.connections.arrive_connected();
    fixture.poll();

    CHECK(fixture.connections.sent.empty());
    CHECK(fixture.sink.events ==
          std::vector<classic::GameEvent>{classic::SessionEnded{classic::SessionEndReason::ConnectionLost}});
    CHECK(fixture.connections.stop_count == 1);
}

TEST_CASE("the demo adapter begins only once, and only over TCP") {
    SECTION("twice") {
        AdapterFixture fixture;
        REQUIRE(fixture.adapter.begin_session().has_value());
        CHECK_FALSE(fixture.adapter.begin_session().has_value());
        CHECK(fixture.connections.opened.size() == 1);
    }
    SECTION("another kind of connection") {
        ScriptedConnections messages(core::ConnectionTraits{"whole-messages", true, true, 256});
        DemoAdapter adapter(messages, core::ConnectionAddress{"127.0.0.1", 24'000});
        CHECK_FALSE(adapter.begin_session().has_value());
        CHECK(messages.opened.empty());
    }
}

TEST_CASE("messages that arrive one byte at a time become events, in order", "[0000-CA-10]") {
    AdapterFixture fixture;
    fixture.connect();
    const Conversation conversation = every_kind_of_message();

    for (const std::byte byte : conversation.bytes) {
        fixture.connections.arrive_bytes(Bytes{byte});
        fixture.poll();
    }

    CHECK(fixture.sink.events == conversation.events);
    CHECK(fixture.adapter.invalid_message_count() == 0);
}

TEST_CASE("several messages that arrive together become events, in order", "[0000-CA-10]") {
    AdapterFixture fixture;
    fixture.connect();
    const Conversation conversation = every_kind_of_message();

    fixture.connections.arrive_bytes(conversation.bytes);
    fixture.poll();

    CHECK(fixture.sink.events == conversation.events);
}

TEST_CASE("bytes that break the format end the session, and nothing after them becomes an event",
          "[0000-CA-11]") {
    AdapterFixture fixture;
    fixture.connect();
    Bytes bytes = bytes_of(protocol::PositionCorrected{7, {33, 33}});
    // A frame with an empty body: there is no way to know where the next one starts.
    append(bytes, bytes_of({0x00, 0x00}));
    append(bytes, bytes_of(protocol::PositionCorrected{7, {34, 34}}));

    fixture.connections.arrive_bytes(bytes);
    fixture.connections.arrive_bytes(bytes_of(protocol::EntityLeft{8, protocol::LeaveReason::LeftView}));
    fixture.connections.arrive_closed(core::CloseReason::PeerClosed);
    fixture.poll();
    fixture.poll();

    CHECK(fixture.sink.events ==
          std::vector<classic::GameEvent>{classic::PositionCorrected{classic::EntityId{7}, {33, 33}},
                                          classic::SessionEnded{classic::SessionEndReason::InvalidData}});
    CHECK(fixture.connections.stop_count == 1);
}

TEST_CASE("a frame larger than the format allows ends the session", "[0000-CA-11]") {
    AdapterFixture fixture;
    fixture.connect();

    // A body of 129 bytes, one more than the largest one.
    fixture.connections.arrive_bytes(bytes_of({0x81, 0x00, 0x0B}));
    fixture.poll();

    CHECK(fixture.sink.events ==
          std::vector<classic::GameEvent>{classic::SessionEnded{classic::SessionEndReason::InvalidData}});
}

TEST_CASE("a wrong message is only counted, and the messages after it still become events", "[0000-CA-11]") {
    AdapterFixture fixture;
    fixture.connect();
    // A type the client does not know, a hello sent to the client, an entity numbered 0 and a walk without
    // steps: each one is wrong only by itself.
    Bytes bytes = bytes_of({0x01, 0x00, 0x63});
    append(bytes, bytes_of({0x03, 0x00, 0x01, 0x01, 0x00}));
    append(bytes, bytes_of({0x0D, 0x00, 0x0B, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
                            0x21, 0x00}));
    append(bytes, bytes_of({0x0A, 0x00, 0x0D, 0x08, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x1F, 0x00, 0x00}));
    append(bytes, bytes_of(protocol::PositionCorrected{7, {33, 33}}));

    fixture.connections.arrive_bytes(bytes);
    fixture.poll();

    CHECK(fixture.sink.events ==
          std::vector<classic::GameEvent>{classic::PositionCorrected{classic::EntityId{7}, {33, 33}}});
    CHECK(fixture.adapter.invalid_message_count() == 4);
    CHECK(fixture.connections.stop_count == 0);
}

TEST_CASE("random bytes never break the demo adapter, and the end of the session is always the last event",
          "[0000-CA-11]") {
    core::SeededRandom random(20261119);
    for (int round = 0; round < 300; ++round) {
        AdapterFixture fixture;
        fixture.connect();
        const std::uint32_t block_count = 1 + random.next_below(8);
        for (std::uint32_t block = 0; block < block_count; ++block) {
            Bytes bytes(1 + random.next_below(64));
            for (std::byte& byte : bytes) {
                byte = static_cast<std::byte>(random.next_below(256));
            }
            fixture.connections.arrive_bytes(std::move(bytes));
        }
        fixture.poll();
        fixture.poll();

        const auto& events = fixture.sink.events;
        for (std::size_t index = 0; index + 1 < events.size(); ++index) {
            CHECK_FALSE(ends_the_session(events[index]));
        }
    }
}

TEST_CASE("the end of the connection ends the session, with the reason in the words of the framework") {
    const auto [close_reason, end_reason] =
        GENERATE(std::pair{core::CloseReason::PeerClosed, classic::SessionEndReason::ClosedByServer},
                 std::pair{core::CloseReason::CouldNotConnect, classic::SessionEndReason::CouldNotConnect},
                 std::pair{core::CloseReason::TimedOut, classic::SessionEndReason::TimedOut},
                 std::pair{core::CloseReason::ConnectionLost, classic::SessionEndReason::ConnectionLost});
    AdapterFixture fixture;
    fixture.connect();

    fixture.connections.arrive_closed(close_reason);
    fixture.poll();

    CHECK(fixture.sink.events == std::vector<classic::GameEvent>{classic::SessionEnded{end_reason}});
}

TEST_CASE("ending the session delivers its end on the next poll, and nothing that arrives later") {
    AdapterFixture fixture;
    fixture.connect();

    fixture.adapter.end_session();
    fixture.connections.arrive_bytes(bytes_of(protocol::PositionCorrected{7, {33, 33}}));
    fixture.poll();

    CHECK(fixture.sink.events ==
          std::vector<classic::GameEvent>{classic::SessionEnded{classic::SessionEndReason::ClosedByPlayer}});
}

TEST_CASE("a walk request carries the origin and every step") {
    AdapterFixture fixture;
    fixture.connect();

    REQUIRE(fixture.adapter
                .send_request(classic::WalkRequest{{32, 33}, {34, 34}, walk_steps({{33, 33}, {34, 34}})})
                .has_value());

    // Size (14), type (2), origin (32, 33), two steps: (33, 33) and (34, 34).
    REQUIRE(fixture.connections.sent.size() == 1);
    CHECK(fixture.connections.sent.front() == bytes_of({0x0E, 0x00, 0x02, 0x20, 0x00, 0x21, 0x00, 0x02, 0x21,
                                                        0x00, 0x21, 0x00, 0x22, 0x00, 0x22, 0x00}));
}

TEST_CASE("a walk without steps asks to stop at its origin") {
    AdapterFixture fixture;
    fixture.connect();

    REQUIRE(fixture.adapter.send_request(classic::WalkRequest{{32, 33}, {32, 33}, {}}).has_value());

    REQUIRE(fixture.connections.sent.size() == 1);
    CHECK(fixture.connections.sent.front() == bytes_of({0x06, 0x00, 0x02, 0x20, 0x00, 0x21, 0x00, 0x00}));
}

TEST_CASE("the notice that the map is loaded carries the number of the map") {
    AdapterFixture fixture;
    fixture.connect();

    REQUIRE(fixture.adapter.send_request(classic::MapLoadedNotice{classic::MapId{1}}).has_value());

    REQUIRE(fixture.connections.sent.size() == 1);
    CHECK(fixture.connections.sent.front() == bytes_of({0x05, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00}));
}

TEST_CASE("a walk the messages cannot carry is refused, and nothing is sent") {
    AdapterFixture fixture;
    fixture.connect();

    SECTION("an origin outside the map") {
        CHECK_FALSE(fixture.adapter.send_request(classic::WalkRequest{{-1, 0}, {0, 0}, walk_steps({{0, 0}})})
                        .has_value());
    }
    SECTION("a step beyond the tiles a message can carry") {
        CHECK_FALSE(
            fixture.adapter.send_request(classic::WalkRequest{{1023, 0}, {1024, 0}, walk_steps({{1024, 0}})})
                .has_value());
    }
    SECTION("more steps than the server accepts") {
        classic::WalkSteps steps;
        for (int step = 1; step <= static_cast<int>(protocol::kMaxWalkSteps) + 1; ++step) {
            REQUIRE(steps.add({step, 0}));
        }
        const int last = static_cast<int>(protocol::kMaxWalkSteps) + 1;
        CHECK_FALSE(fixture.adapter.send_request(classic::WalkRequest{{0, 0}, {last, 0}, steps}).has_value());
    }
    CHECK(fixture.connections.sent.empty());
}
