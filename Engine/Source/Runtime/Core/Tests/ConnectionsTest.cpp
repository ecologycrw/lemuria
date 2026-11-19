#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "Lemuria/Core/Connections.hpp"

using Catch::Matchers::ContainsSubstring;
using lemuria::core::CloseReason;
using lemuria::core::ConnectionAddress;
using lemuria::core::ConnectionClosed;
using lemuria::core::ConnectionEvents;
using lemuria::core::ConnectionId;
using lemuria::core::Connections;
using lemuria::core::ConnectionTraits;
using lemuria::core::Delivery;
using lemuria::core::ReceivedBytes;
using lemuria::core::Result;
using lemuria::core::SendOptions;

namespace {

constexpr std::size_t kSendLimit = 8;
const ConnectionAddress kAddress{"localhost", 4000};

constexpr ConnectionTraits kStreamTraits{lemuria::core::kTcpConnectionKind, false, false, kSendLimit};
constexpr ConnectionTraits kMessageTraits{"whole-messages", true, true, kSendLimit};

[[nodiscard]] std::vector<std::byte> bytes_of(std::string_view text) {
    std::vector<std::byte> bytes;
    for (const char character : text) {
        bytes.push_back(static_cast<std::byte>(character));
    }
    return bytes;
}

[[nodiscard]] std::string text_of(std::span<const std::byte> bytes) {
    std::string text;
    for (const std::byte value : bytes) {
        text.push_back(static_cast<char>(value));
    }
    return text;
}

struct SentBytes {
    ConnectionId connection;
    std::string text;
    Delivery delivery = Delivery::ReliableInOrder;
};

// A kind of connection whose events are written by each test.
class ScriptedConnections final : public Connections {
public:
    explicit ScriptedConnections(ConnectionTraits traits = kStreamTraits) : traits_(traits) {}

    [[nodiscard]] const ConnectionTraits& traits() const noexcept override { return traits_; }

    void arrive_connected(ConnectionId connection) {
        script_.emplace_back([connection](ConnectionEvents& events) { events.on_connected(connection); });
    }

    void arrive_bytes(ConnectionId connection, std::string_view text, bool is_whole_message = false) {
        script_.emplace_back([connection, data = bytes_of(text), is_whole_message](ConnectionEvents& events) {
            events.on_received(ReceivedBytes{connection, data, is_whole_message});
        });
    }

    void arrive_closed(ConnectionId connection, ConnectionClosed closed) {
        script_.emplace_back(
            [connection, closed](ConnectionEvents& events) { events.on_closed(connection, closed); });
    }

    std::vector<ConnectionId> started;
    std::vector<ConnectionId> stopped;
    std::vector<SentBytes> sent;
    std::optional<std::string> open_failure;
    std::optional<std::string> send_failure;

private:
    Result<void> start_connecting(ConnectionId connection, const ConnectionAddress& /*address*/) override {
        if (open_failure) {
            return lemuria::core::make_error(*open_failure);
        }
        started.push_back(connection);
        return {};
    }

    Result<void> queue_bytes(ConnectionId connection, std::span<const std::byte> bytes,
                             SendOptions options) override {
        if (send_failure) {
            return lemuria::core::make_error(*send_failure);
        }
        sent.push_back(SentBytes{connection, text_of(bytes), options.delivery});
        return {};
    }

    void stop_connection(ConnectionId connection) noexcept override { stopped.push_back(connection); }

    void collect_events(ConnectionEvents& events) override {
        const auto script = std::exchange(script_, {});
        for (const auto& event : script) {
            event(events);
        }
    }

    ConnectionTraits traits_;
    std::vector<std::function<void(ConnectionEvents&)>> script_;
};

// Writes down what reaches the game, in order.
class EventLog final : public ConnectionEvents {
public:
    void on_connected(ConnectionId connection) override {
        lines.push_back("connected " + std::to_string(connection.value));
        run_reaction();
    }

    void on_received(const ReceivedBytes& block) override {
        lines.push_back("received " + std::to_string(block.connection.value) + " " + text_of(block.bytes) +
                        (block.is_whole_message ? " whole" : ""));
        run_reaction();
    }

    void on_closed(ConnectionId connection, const ConnectionClosed& closed) override {
        lines.push_back("closed " + std::to_string(connection.value) + " reason " +
                        std::to_string(static_cast<int>(closed.reason)) + " code " +
                        std::to_string(closed.original_code));
    }

    std::vector<std::string> lines;
    std::function<void()> reaction;

private:
    void run_reaction() {
        if (reaction) {
            std::exchange(reaction, {})();
        }
    }
};

[[nodiscard]] std::string closed_line(ConnectionId connection, CloseReason reason, std::int64_t code = 0) {
    return "closed " + std::to_string(connection.value) + " reason " +
           std::to_string(static_cast<int>(reason)) + " code " + std::to_string(code);
}

[[nodiscard]] ConnectionId open_connection(Connections& connections) {
    const auto opened = connections.open(kAddress);
    REQUIRE(opened.has_value());
    return *opened;
}

[[nodiscard]] ConnectionId connected(ScriptedConnections& connections, EventLog& log) {
    const ConnectionId connection = open_connection(connections);
    connections.arrive_connected(connection);
    connections.poll(log);
    log.lines.clear();
    return connection;
}

} // namespace

TEST_CASE("every connection gets a number that is never used again") {
    ScriptedConnections connections;
    EventLog log;

    const ConnectionId first = open_connection(connections);
    connections.close(first);
    connections.poll(log);
    const ConnectionId second = open_connection(connections);

    CHECK(first.value != 0);
    CHECK(second.value > first.value);
}

TEST_CASE("opening a connection needs a host and a port") {
    ScriptedConnections connections;

    CHECK_FALSE(connections.open(ConnectionAddress{"", 4000}).has_value());
    CHECK_FALSE(connections.open(ConnectionAddress{"localhost", 0}).has_value());
    CHECK(connections.started.empty());
}

TEST_CASE("a connection that cannot start reports why") {
    ScriptedConnections connections;
    connections.open_failure = "no network";

    const auto opened = connections.open(kAddress);

    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error().message == "no network");
}

TEST_CASE("a connection that could not start does not use up the budget") {
    ScriptedConnections connections;
    connections.open_failure = "no network";
    for (std::size_t attempt = 0; attempt < Connections::kMaxOpenConnections; ++attempt) {
        CHECK_FALSE(connections.open(kAddress).has_value());
    }
    connections.open_failure.reset();

    CHECK(connections.open(kAddress).has_value());
}

TEST_CASE("the number of open connections has a budget") {
    ScriptedConnections connections;
    EventLog log;
    ConnectionId last;
    for (std::size_t index = 0; index < Connections::kMaxOpenConnections; ++index) {
        last = open_connection(connections);
    }

    const auto refused = connections.open(kAddress);

    REQUIRE_FALSE(refused.has_value());
    CHECK_THAT(refused.error().message, ContainsSubstring("too many"));

    connections.close(last);
    CHECK_FALSE(connections.open(kAddress).has_value());
    connections.poll(log);
    CHECK(connections.open(kAddress).has_value());
}

TEST_CASE("bytes are sent in the order they were asked for, with every byte guaranteed by default") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);

    REQUIRE(connections.send(connection, bytes_of("first")).has_value());
    REQUIRE(connections.send(connection, bytes_of("second")).has_value());

    REQUIRE(connections.sent.size() == 2);
    CHECK(connections.sent[0].text == "first");
    CHECK(connections.sent[1].text == "second");
    CHECK(connections.sent[0].connection == connection);
    CHECK(connections.sent[0].delivery == Delivery::ReliableInOrder);
}

TEST_CASE("a kind of connection that always guarantees delivery refuses the other option", "[0000-CA-16]") {
    ScriptedConnections connections(kStreamTraits);
    EventLog log;
    const ConnectionId connection = connected(connections, log);

    const auto sent = connections.send(connection, bytes_of("move"), SendOptions{Delivery::Unreliable});

    REQUIRE_FALSE(sent.has_value());
    CHECK_THAT(sent.error().message, ContainsSubstring("tcp"));
    CHECK_THAT(sent.error().message, ContainsSubstring("unreliable"));
    CHECK(connections.sent.empty());
}

TEST_CASE("a connection that offers delivery without guarantee keeps the option asked for", "[0000-CA-16]") {
    ScriptedConnections connections(kMessageTraits);
    EventLog log;
    const ConnectionId connection = connected(connections, log);

    REQUIRE(connections.send(connection, bytes_of("move"), SendOptions{Delivery::Unreliable}).has_value());
    REQUIRE(connections.send(connection, bytes_of("chat")).has_value());

    REQUIRE(connections.sent.size() == 2);
    CHECK(connections.sent[0].delivery == Delivery::Unreliable);
    CHECK(connections.sent[1].delivery == Delivery::ReliableInOrder);
}

TEST_CASE("each kind of connection publishes its limit for one send", "[0000-CA-16]") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);

    CHECK(connections.send(connection, std::vector<std::byte>(kSendLimit)).has_value());

    const auto refused = connections.send(connection, std::vector<std::byte>(kSendLimit + 1));
    REQUIRE_FALSE(refused.has_value());
    CHECK_THAT(refused.error().message, ContainsSubstring("limit"));
    CHECK(connections.sent.size() == 1);
}

TEST_CASE("sending nothing is refused") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);

    CHECK_FALSE(connections.send(connection, {}).has_value());
    CHECK(connections.sent.empty());
}

TEST_CASE("bytes wait until the connection is ready") {
    ScriptedConnections connections;
    const ConnectionId connection = open_connection(connections);

    const auto sent = connections.send(connection, bytes_of("early"));

    REQUIRE_FALSE(sent.has_value());
    CHECK_THAT(sent.error().message, ContainsSubstring("connecting"));
    CHECK(connections.sent.empty());
}

TEST_CASE("sending on a connection that does not exist is refused") {
    ScriptedConnections connections;

    CHECK_FALSE(connections.send(ConnectionId{99}, bytes_of("lost")).has_value());
}

TEST_CASE("sending on a closed connection is refused", "[0000-CA-14]") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId by_us = connected(connections, log);
    const ConnectionId by_peer = connected(connections, log);

    connections.close(by_us);
    connections.arrive_closed(by_peer, ConnectionClosed{CloseReason::PeerClosed, 0});

    const auto sent_while_closing = connections.send(by_us, bytes_of("late"));
    REQUIRE_FALSE(sent_while_closing.has_value());
    CHECK_THAT(sent_while_closing.error().message, ContainsSubstring("closed"));
    connections.poll(log);
    CHECK_FALSE(connections.send(by_peer, bytes_of("late")).has_value());
    CHECK(connections.sent.empty());
}

TEST_CASE("the empty connection number never means a connection") {
    ScriptedConnections connections;
    EventLog log;

    connections.close(ConnectionId{});
    connections.poll(log);

    CHECK(connections.stopped.empty());
    CHECK(log.lines.empty());
    CHECK_FALSE(connections.send(ConnectionId{}, bytes_of("lost")).has_value());
}

TEST_CASE("a failure of the connection while sending reaches the caller") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);
    connections.send_failure = "queue is full";

    const auto sent = connections.send(connection, bytes_of("data"));

    REQUIRE_FALSE(sent.has_value());
    CHECK(sent.error().message == "queue is full");
}

TEST_CASE("events reach the game in order and marked with their connection") {
    ScriptedConnections connections(kMessageTraits);
    EventLog log;
    const ConnectionId first = open_connection(connections);
    const ConnectionId second = open_connection(connections);
    connections.arrive_connected(second);
    connections.arrive_connected(first);
    connections.arrive_bytes(first, "abc", true);
    connections.arrive_bytes(second, "de");
    connections.arrive_bytes(first, "f", true);

    connections.poll(log);

    const std::string one = std::to_string(first.value);
    const std::string two = std::to_string(second.value);
    CHECK(log.lines == std::vector<std::string>{"connected " + two, "connected " + one,
                                                "received " + one + " abc whole", "received " + two + " de",
                                                "received " + one + " f whole"});
}

TEST_CASE("the reason of a closed connection and its original code reach the game") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);
    connections.arrive_closed(connection, ConnectionClosed{CloseReason::ConnectionLost, 10054});

    connections.poll(log);

    CHECK(log.lines == std::vector<std::string>{closed_line(connection, CloseReason::ConnectionLost, 10054)});
}

TEST_CASE("a connection that never connected reports why it closed") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = open_connection(connections);
    connections.arrive_closed(connection, ConnectionClosed{CloseReason::CouldNotConnect, 111});

    connections.poll(log);

    CHECK(log.lines == std::vector<std::string>{closed_line(connection, CloseReason::CouldNotConnect, 111)});
}

TEST_CASE("data that arrives after a connection closed never reaches the game", "[0000-CA-14]") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);
    connections.arrive_closed(connection, ConnectionClosed{CloseReason::PeerClosed, 0});
    connections.arrive_bytes(connection, "late");
    connections.arrive_connected(connection);
    connections.arrive_closed(connection, ConnectionClosed{CloseReason::ConnectionLost, 0});

    connections.poll(log);

    CHECK(log.lines == std::vector<std::string>{closed_line(connection, CloseReason::PeerClosed)});
    CHECK(connections.ignored_event_count() == 3);
}

TEST_CASE("data that was on its way when the game closed the connection never reaches the game",
          "[0000-CA-14]") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);
    connections.arrive_bytes(connection, "on its way");

    connections.close(connection);
    connections.poll(log);

    CHECK(log.lines == std::vector<std::string>{closed_line(connection, CloseReason::LocalRequest)});
    CHECK(connections.stopped == std::vector<ConnectionId>{connection});
}

TEST_CASE("a connection closed by the game reports its end exactly once", "[0000-CA-14]") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);

    connections.close(connection);
    connections.close(connection);
    connections.arrive_closed(connection, ConnectionClosed{CloseReason::ConnectionLost, 7});
    connections.poll(log);
    connections.arrive_closed(connection, ConnectionClosed{CloseReason::ConnectionLost, 7});
    connections.poll(log);

    CHECK(log.lines == std::vector<std::string>{closed_line(connection, CloseReason::LocalRequest)});
    CHECK(connections.stopped.size() == 1);
}

TEST_CASE("the game can close a connection while it reads its data", "[0000-CA-14]") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = connected(connections, log);
    connections.arrive_bytes(connection, "broken");
    connections.arrive_bytes(connection, "next");
    log.reaction = [&] { connections.close(connection); };

    connections.poll(log);

    CHECK(log.lines == std::vector<std::string>{"received " + std::to_string(connection.value) + " broken",
                                                closed_line(connection, CloseReason::LocalRequest)});
}

TEST_CASE("closing a connection that does not exist changes nothing") {
    ScriptedConnections connections;
    EventLog log;

    connections.close(ConnectionId{99});
    connections.poll(log);

    CHECK(connections.stopped.empty());
    CHECK(log.lines.empty());
}

TEST_CASE("events about a connection that was never opened are ignored and counted", "[0000-CA-14]") {
    ScriptedConnections connections;
    EventLog log;
    connections.arrive_connected(ConnectionId{99});
    connections.arrive_bytes(ConnectionId{99}, "stray");
    connections.arrive_closed(ConnectionId{99}, ConnectionClosed{CloseReason::PeerClosed, 0});

    connections.poll(log);

    CHECK(log.lines.empty());
    CHECK(connections.ignored_event_count() == 3);
}

TEST_CASE("data that arrives before the connection is ready is ignored and counted") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = open_connection(connections);
    connections.arrive_bytes(connection, "early");

    connections.poll(log);

    CHECK(log.lines.empty());
    CHECK(connections.ignored_event_count() == 1);
}

TEST_CASE("a connection reports that it is ready only once") {
    ScriptedConnections connections;
    EventLog log;
    const ConnectionId connection = open_connection(connections);
    connections.arrive_connected(connection);
    connections.arrive_connected(connection);

    connections.poll(log);

    CHECK(log.lines == std::vector<std::string>{"connected " + std::to_string(connection.value)});
    CHECK(connections.ignored_event_count() == 1);
}

TEST_CASE("a closed connection frees its place in the budget") {
    ScriptedConnections connections;
    EventLog log;
    ConnectionId last;
    for (std::size_t index = 0; index < Connections::kMaxOpenConnections; ++index) {
        last = open_connection(connections);
    }
    connections.arrive_closed(last, ConnectionClosed{CloseReason::TimedOut, 0});

    connections.poll(log);

    CHECK(connections.open(kAddress).has_value());
}
