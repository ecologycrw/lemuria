#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Platform/MonotonicClock.hpp"
#include "Lemuria/Platform/TcpConnections.hpp"
#include "Lemuria/Platform/TcpListener.hpp"
#include "TcpTestSupport.hpp"

using Catch::Matchers::ContainsSubstring;
using lemuria::core::CloseReason;
using lemuria::core::ConnectionClosed;
using lemuria::core::ConnectionEvents;
using lemuria::core::ConnectionId;
using lemuria::core::Delivery;
using lemuria::core::ReceivedBytes;
using lemuria::core::SendOptions;
using lemuria::platform::monotonic_nanoseconds;
using lemuria::platform::TcpConnections;
using lemuria::platform::TcpListener;
using lemuria::platform::TcpPeer;
using lemuria::platform::TcpTimeouts;
using lemuria::platform::test::kFirstTestPort;
using lemuria::platform::test::kTestPortCount;
using lemuria::platform::test::pattern_bytes;
using lemuria::platform::test::wait_until;

namespace {

constexpr const char* kHost = "127.0.0.1";
constexpr std::uint64_t kNanosecondsPerMillisecond = 1'000'000;

struct ClosedConnection {
    ConnectionId connection;
    ConnectionClosed closed;
};

// Writes down what reaches the game.
class RecordedEvents final : public ConnectionEvents {
public:
    void on_connected(ConnectionId connection) override { connected.push_back(connection); }

    void on_received(const ReceivedBytes& block) override {
        last_block_was_whole_message = block.is_whole_message;
        received_from.push_back(block.connection);
        received.insert(received.end(), block.bytes.begin(), block.bytes.end());
    }

    void on_closed(ConnectionId connection, const ConnectionClosed& closed_connection) override {
        closed.push_back(ClosedConnection{connection, closed_connection});
    }

    std::vector<ConnectionId> connected;
    std::vector<ConnectionId> received_from;
    std::vector<std::byte> received;
    std::vector<ClosedConnection> closed;
    bool last_block_was_whole_message = false;
};

[[nodiscard]] TcpListener open_listener() {
    auto listener = TcpListener::open_on_this_computer(kFirstTestPort, kTestPortCount);
    REQUIRE(listener.has_value());
    return std::move(*listener);
}

[[nodiscard]] TcpPeer accept_peer(TcpListener& listener, TcpConnections& connections,
                                  RecordedEvents& events) {
    std::optional<TcpPeer> peer;
    REQUIRE(wait_until([&] {
        connections.poll(events);
        if (!peer) {
            auto accepted = listener.accept();
            REQUIRE(accepted.has_value());
            peer = std::move(*accepted);
        }
        return peer.has_value() && !events.connected.empty();
    }));
    return std::move(*peer);
}

struct OpenConnection {
    ConnectionId connection;
    TcpPeer peer;
};

[[nodiscard]] OpenConnection connect(TcpListener& listener, TcpConnections& connections,
                                     RecordedEvents& events) {
    events.connected.clear();
    const auto opened = connections.open({kHost, listener.port()});
    REQUIRE(opened.has_value());
    TcpPeer peer = accept_peer(listener, connections, events);
    return OpenConnection{*opened, std::move(peer)};
}

[[nodiscard]] std::vector<std::byte> read_from_peer(TcpPeer& peer, std::size_t count) {
    std::vector<std::byte> bytes;
    std::vector<std::byte> buffer(std::size_t{64} * 1024);
    REQUIRE(wait_until([&] {
        const auto read = peer.read(std::span(buffer).first(std::min(buffer.size(), count - bytes.size())));
        REQUIRE(read.has_value());
        bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(*read));
        return bytes.size() == count;
    }));
    return bytes;
}

// Reads until the connection ends and returns everything that arrived.
[[nodiscard]] std::vector<std::byte> read_until_end(TcpPeer& peer) {
    std::vector<std::byte> bytes;
    std::vector<std::byte> buffer(std::size_t{64} * 1024);
    REQUIRE(wait_until([&] {
        const auto read = peer.read(buffer);
        if (!read) {
            return true;
        }
        bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(*read));
        return false;
    }));
    return bytes;
}

// Bytes the peer could not send at once only leave during its next calls.
void pump(TcpPeer& peer) {
    std::array<std::byte, 1> ignored{};
    static_cast<void>(peer.read(std::span(ignored).first(0)));
}

void receive_all(TcpPeer& peer, TcpConnections& connections, RecordedEvents& events, std::size_t count) {
    REQUIRE(wait_until([&] {
        pump(peer);
        connections.poll(events);
        return events.received.size() >= count;
    }));
}

// The peer closes its side of the connection.
void leave(TcpPeer&& peer) {
    const TcpPeer leaving = std::move(peer);
}

[[nodiscard]] ClosedConnection wait_for_close(TcpConnections& connections, RecordedEvents& events) {
    REQUIRE(wait_until([&] {
        connections.poll(events);
        return !events.closed.empty();
    }));
    return events.closed.front();
}

struct FilledQueue {
    std::size_t accepted_bytes = 0;
    std::uint64_t slowest_call_nanoseconds = 0;
};

// Sends until nothing more fits between the game and a peer that does not read.
[[nodiscard]] FilledQueue fill_until_nothing_fits(TcpConnections& connections, RecordedEvents& events,
                                                  ConnectionId connection) {
    constexpr std::uint64_t kQuietNanoseconds = 300 * kNanosecondsPerMillisecond;
    FilledQueue filled;
    std::uint64_t last_accepted_at = monotonic_nanoseconds();
    REQUIRE(wait_until([&] {
        const auto block = pattern_bytes(filled.accepted_bytes, TcpConnections::kMaxSendBytes);
        const std::uint64_t before = monotonic_nanoseconds();
        const auto sent = connections.send(connection, block);
        connections.poll(events);
        const std::uint64_t after = monotonic_nanoseconds();
        filled.slowest_call_nanoseconds = std::max(filled.slowest_call_nanoseconds, after - before);
        if (sent) {
            filled.accepted_bytes += block.size();
            last_accepted_at = after;
        } else {
            CHECK_THAT(sent.error().message, ContainsSubstring("full"));
        }
        return after - last_accepted_at > kQuietNanoseconds;
    }));
    return filled;
}

} // namespace

TEST_CASE("tcp connections publish what they offer", "[0000-CA-16]") {
    const TcpConnections connections;

    CHECK(connections.traits().kind == lemuria::core::kTcpConnectionKind);
    CHECK_FALSE(connections.traits().delivers_whole_messages);
    CHECK_FALSE(connections.traits().offers_unreliable_delivery);
    CHECK(connections.traits().max_send_bytes == TcpConnections::kMaxSendBytes);
}

TEST_CASE("bytes travel both ways, in order, over a real connection") {
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);

    REQUIRE(connections.send(link.connection, pattern_bytes(0, 1000)).has_value());
    REQUIRE(connections.send(link.connection, pattern_bytes(1000, 2000)).has_value());
    CHECK(read_from_peer(link.peer, 3000) == pattern_bytes(0, 3000));

    const auto answer = pattern_bytes(5, 70'000);
    REQUIRE(link.peer.write(answer).has_value());
    receive_all(link.peer, connections, events, answer.size());

    CHECK(events.received == answer);
    CHECK_FALSE(events.last_block_was_whole_message);
    CHECK(std::ranges::all_of(events.received_from,
                              [&](ConnectionId from) { return from == link.connection; }));
}

TEST_CASE("two connections keep their bytes apart") {
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection first = connect(listener, connections, events);
    OpenConnection second = connect(listener, connections, events);

    const auto to_first = pattern_bytes(0, 100);
    const auto to_second = pattern_bytes(7, 200);
    REQUIRE(connections.send(first.connection, to_first).has_value());
    REQUIRE(connections.send(second.connection, to_second).has_value());
    CHECK(read_from_peer(first.peer, 100) == to_first);
    CHECK(read_from_peer(second.peer, 200) == to_second);

    const auto from_second = pattern_bytes(3, 50);
    REQUIRE(second.peer.write(from_second).has_value());
    receive_all(second.peer, connections, events, from_second.size());

    CHECK(events.received == from_second);
    CHECK(std::ranges::all_of(events.received_from,
                              [&](ConnectionId from) { return from == second.connection; }));
}

TEST_CASE("tcp refuses to send without the guarantee of delivery", "[0000-CA-16]") {
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    const OpenConnection link = connect(listener, connections, events);

    const auto refused =
        connections.send(link.connection, pattern_bytes(0, 8), SendOptions{Delivery::Unreliable});

    REQUIRE_FALSE(refused.has_value());
    CHECK_THAT(refused.error().message, ContainsSubstring("tcp"));
    CHECK_THAT(refused.error().message, ContainsSubstring("unreliable"));
}

TEST_CASE("a server that does not exist is reported, and the game keeps running", "[0000-CA-14]") {
    TcpConnections connections;
    RecordedEvents events;

    const auto opened = connections.open({kHost, lemuria::platform::test::kPortWithoutServer});
    REQUIRE(opened.has_value());
    const ClosedConnection end = wait_for_close(connections, events);

    CHECK(end.connection == *opened);
    CHECK(end.closed.reason == CloseReason::CouldNotConnect);
    CHECK(events.connected.empty());
}

TEST_CASE("a connection that takes too long to start gives up", "[0000-CA-14]") {
    const TcpListener listener = open_listener();
    TcpConnections connections(TcpTimeouts{0, 0});
    RecordedEvents events;

    REQUIRE(connections.open({kHost, listener.port()}).has_value());
    const ClosedConnection end = wait_for_close(connections, events);

    CHECK(end.closed.reason == CloseReason::TimedOut);
    CHECK(events.connected.empty());
}

TEST_CASE("bytes sent before the server leaves arrive before the end of the connection", "[0000-CA-14]") {
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);

    const auto farewell = pattern_bytes(0, 5000);
    REQUIRE(link.peer.write(farewell).has_value());
    leave(std::move(link.peer));
    const ClosedConnection end = wait_for_close(connections, events);

    CHECK(events.received == farewell);
    CHECK(end.connection == link.connection);
    CHECK(end.closed.reason == CloseReason::PeerClosed);
}

TEST_CASE("a connection that breaks is reported as lost", "[0000-CA-14]") {
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);

    // Leaving with unread bytes makes the system break the connection, instead of ending it normally.
    REQUIRE(connections.send(link.connection, pattern_bytes(0, 4096)).has_value());
    CHECK(read_from_peer(link.peer, 1).size() == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    leave(std::move(link.peer));

    CHECK(wait_for_close(connections, events).closed.reason == CloseReason::ConnectionLost);
}

TEST_CASE("data of a connection closed by the game never reaches it", "[0000-CA-14]") {
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);

    REQUIRE(link.peer.write(pattern_bytes(0, 1000)).has_value());
    connections.close(link.connection);
    const ClosedConnection end = wait_for_close(connections, events);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    connections.poll(events);

    CHECK(end.closed.reason == CloseReason::LocalRequest);
    CHECK(events.closed.size() == 1);
    CHECK(events.received.empty());
}

TEST_CASE("a connection closed while it is still connecting ends once") {
    const TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;

    const auto opened = connections.open({kHost, listener.port()});
    REQUIRE(opened.has_value());
    connections.close(*opened);
    const ClosedConnection end = wait_for_close(connections, events);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    connections.poll(events);

    CHECK(end.closed.reason == CloseReason::LocalRequest);
    CHECK(events.closed.size() == 1);
    CHECK(events.connected.empty());
}

TEST_CASE("bytes sent right before closing still reach the server") {
    constexpr std::size_t kBlocks = 8;
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);

    for (std::size_t block = 0; block < kBlocks; ++block) {
        const auto bytes =
            pattern_bytes(block * TcpConnections::kMaxSendBytes, TcpConnections::kMaxSendBytes);
        REQUIRE(connections.send(link.connection, bytes).has_value());
    }
    connections.close(link.connection);

    CHECK(read_until_end(link.peer) == pattern_bytes(0, kBlocks * TcpConnections::kMaxSendBytes));
}

TEST_CASE("the frame never waits for a server that stopped reading", "[0000-CA-17]") {
    constexpr std::uint64_t kSlowestAllowedNanoseconds = 1'000 * kNanosecondsPerMillisecond;
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);

    // A call that waited for the network would never return here, because this server reads nothing yet.
    const FilledQueue filled = fill_until_nothing_fits(connections, events, link.connection);

    CHECK(filled.slowest_call_nanoseconds < kSlowestAllowedNanoseconds);
    CHECK(filled.accepted_bytes > TcpConnections::kMaxQueuedSendBytes - TcpConnections::kMaxSendBytes);
    CHECK(events.closed.empty());

    // Once the server wakes up, every accepted byte arrives, in order.
    CHECK(read_from_peer(link.peer, filled.accepted_bytes) == pattern_bytes(0, filled.accepted_bytes));
}

TEST_CASE("closing a connection to a server that stopped reading gives up on the bytes left",
          "[0000-CA-17]") {
    TcpListener listener = open_listener();
    TcpConnections connections(TcpTimeouts{10'000, 0});
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);
    const FilledQueue filled = fill_until_nothing_fits(connections, events, link.connection);

    connections.close(link.connection);
    const ClosedConnection end = wait_for_close(connections, events);

    CHECK(end.closed.reason == CloseReason::LocalRequest);
    CHECK(read_until_end(link.peer).size() < filled.accepted_bytes);
}

TEST_CASE("the send queue frees up as the bytes leave") {
    constexpr std::size_t kTotal = 3 * TcpConnections::kMaxQueuedSendBytes;
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);

    std::size_t accepted = 0;
    std::vector<std::byte> arrived;
    std::vector<std::byte> buffer(TcpConnections::kMaxSendBytes);
    REQUIRE(wait_until([&] {
        const std::size_t count = std::min(kTotal - accepted, TcpConnections::kMaxSendBytes);
        if (count > 0 && connections.send(link.connection, pattern_bytes(accepted, count)).has_value()) {
            accepted += count;
        }
        const auto read = link.peer.read(buffer);
        REQUIRE(read.has_value());
        arrived.insert(arrived.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(*read));
        return arrived.size() == kTotal;
    }));

    CHECK(arrived == pattern_bytes(0, kTotal));
}

TEST_CASE("connections that ended free their places for new ones") {
    constexpr std::size_t kConnections = 3 * TcpConnections::kMaxOpenConnections;
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;

    for (std::size_t index = 0; index < kConnections; ++index) {
        OpenConnection link = connect(listener, connections, events);
        if (index % 2 == 0) {
            connections.close(link.connection);
        } else {
            leave(std::move(link.peer));
        }
        static_cast<void>(wait_for_close(connections, events));
        events.closed.clear();
    }
}

TEST_CASE("received bytes that the game has not read have a budget") {
    constexpr std::size_t kTotal = 3 * TcpConnections::kMaxUndeliveredBytes;
    TcpListener listener = open_listener();
    TcpConnections connections;
    RecordedEvents events;
    OpenConnection link = connect(listener, connections, events);
    const auto sent = pattern_bytes(0, kTotal);
    REQUIRE(link.peer.write(sent).has_value());

    std::size_t largest_poll = 0;
    REQUIRE(wait_until([&] {
        // The game stays away for a while, and the server keeps trying to send.
        for (int pause = 0; pause < 50; ++pause) {
            pump(link.peer);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        const std::size_t before = events.received.size();
        connections.poll(events);
        largest_poll = std::max(largest_poll, events.received.size() - before);
        return events.received.size() == kTotal;
    }));

    CHECK(largest_poll <= TcpConnections::kMaxUndeliveredBytes);
    CHECK(events.received == sent);
    CHECK(events.closed.empty());
}

TEST_CASE("polling from inside an event loses nothing") {
    class PollingEvents final : public ConnectionEvents {
    public:
        explicit PollingEvents(TcpConnections& connections) : connections_(connections) {}

        void on_connected(ConnectionId connection) override { recorded.on_connected(connection); }

        void on_received(const ReceivedBytes& block) override {
            recorded.on_received(block);
            connections_.poll(*this);
        }

        void on_closed(ConnectionId connection, const ConnectionClosed& closed) override {
            recorded.on_closed(connection, closed);
        }

        RecordedEvents recorded;

    private:
        TcpConnections& connections_;
    };

    TcpListener listener = open_listener();
    TcpConnections connections;
    PollingEvents events(connections);
    OpenConnection link = connect(listener, connections, events.recorded);

    const auto sent = pattern_bytes(0, 40'000);
    REQUIRE(link.peer.write(sent).has_value());
    REQUIRE(wait_until([&] {
        pump(link.peer);
        connections.poll(events);
        return events.recorded.received.size() >= sent.size();
    }));

    CHECK(events.recorded.received == sent);
}

TEST_CASE("destroying the connections ends them for the server") {
    TcpListener listener = open_listener();
    std::optional<TcpConnections> connections(std::in_place);
    RecordedEvents events;
    OpenConnection link = connect(listener, *connections, events);

    connections.reset();

    CHECK(read_until_end(link.peer).empty());
}
