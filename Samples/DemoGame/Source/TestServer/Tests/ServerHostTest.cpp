#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Framing.hpp"
#include "DemoProtocol/MessageCodec.hpp"
#include "DemoProtocol/Messages.hpp"
#include "DemoProtocol/Protocol.hpp"
#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Platform/MonotonicClock.hpp"
#include "Lemuria/Platform/TcpConnections.hpp"
#include "Lemuria/Platform/TcpListener.hpp"
#include "TestServer/ServerGrid.hpp"
#include "TestServer/ServerHost.hpp"

using demo_protocol::ClientMessage;
using demo_protocol::DemoMap;
using demo_protocol::EntityAppeared;
using demo_protocol::EntityLeft;
using demo_protocol::EntityWalked;
using demo_protocol::Hello;
using demo_protocol::LeaveReason;
using demo_protocol::MapEntered;
using demo_protocol::MapLoaded;
using demo_protocol::protocol_version;
using demo_protocol::ServerMessage;
using demo_protocol::StepList;
using demo_protocol::Tile;
using demo_protocol::WalkConfirmed;
using demo_protocol::WalkRequest;
using test_server::demo_host_limits;
using test_server::demo_server_config;
using test_server::HostLimits;
using test_server::ServerGrid;
using test_server::ServerHost;

namespace {

// The same range the tests of the platform layer use. Each test looks for a free port in it.
constexpr std::uint16_t kFirstTestPort = 24'100;
constexpr std::uint16_t kTestPortCount = 400;

// Real connections take real time, so the tests wait for a condition, always with a limit.
constexpr std::uint64_t kWaitLimitNanoseconds = 20'000'000'000;

// The time the rules see advances by this much in each round, whatever the real clock says (INV-05).
constexpr double kSecondsPerRound = 0.002;

// A client that speaks the demo messages over a real connection.
class TestClient final : public lemuria::core::ConnectionEvents, public demo_protocol::FrameSink {
public:
    explicit TestClient(std::uint16_t port) {
        const auto opened = connections_.open({"127.0.0.1", port});
        REQUIRE(opened.has_value());
        connection_ = *opened;
    }

    void send(const ClientMessage& message) { send_bytes(demo_protocol::encode(message).bytes()); }

    void send_bytes(std::span<const std::byte> bytes) {
        REQUIRE(connections_.send(connection_, bytes).has_value());
    }

    void leave() { connections_.close(connection_); }

    void poll() { connections_.poll(*this); }

    [[nodiscard]] bool is_connected() const noexcept { return is_connected_; }
    [[nodiscard]] bool is_closed() const noexcept { return is_closed_; }

    template <typename Message>
    [[nodiscard]] std::vector<Message> received() const {
        std::vector<Message> found;
        for (const ServerMessage& message : messages_) {
            if (const auto* typed = std::get_if<Message>(&message)) {
                found.push_back(*typed);
            }
        }
        return found;
    }

    [[nodiscard]] std::uint64_t unreadable_message_count() const noexcept { return unreadable_messages_; }

private:
    void on_connected(lemuria::core::ConnectionId /*connection*/) override { is_connected_ = true; }

    void on_received(const lemuria::core::ReceivedBytes& block) override {
        REQUIRE(frames_.read(block.bytes, *this).has_value());
    }

    void on_closed(lemuria::core::ConnectionId /*connection*/,
                   const lemuria::core::ConnectionClosed& /*closed*/) override {
        is_closed_ = true;
    }

    void on_frame(const demo_protocol::Frame& frame) override {
        const auto message = demo_protocol::read_server_message(frame);
        if (message) {
            messages_.push_back(*message);
        } else {
            ++unreadable_messages_;
        }
    }

    lemuria::platform::TcpConnections connections_;
    lemuria::core::ConnectionId connection_;
    demo_protocol::FrameReader frames_;
    std::vector<ServerMessage> messages_;
    std::uint64_t unreadable_messages_ = 0;
    bool is_connected_ = false;
    bool is_closed_ = false;
};

[[nodiscard]] ServerHost open_host(const HostLimits& limits) {
    auto listener = lemuria::platform::TcpListener::open_on_this_computer(kFirstTestPort, kTestPortCount);
    REQUIRE(listener.has_value());
    auto host = ServerHost::create(std::move(*listener), ServerGrid::from_demo_map(DemoMap{}),
                                   demo_server_config(), limits);
    REQUIRE(host.has_value());
    return std::move(*host);
}

// The test server and its clients, all driven by the test, round after round.
class HostFixture {
public:
    explicit HostFixture(const HostLimits& limits = demo_host_limits()) : host(open_host(limits)) {}

    // Returns when the connection is ready, or when it has already ended.
    [[nodiscard]] TestClient& connect() {
        clients_.push_back(std::make_unique<TestClient>(host.port()));
        TestClient& client = *clients_.back();
        REQUIRE(serve_until([&] { return client.is_connected() || client.is_closed(); }));
        return client;
    }

    // Puts a client on the map, as the demo client does: hello, then the map is loaded.
    [[nodiscard]] TestClient& join() {
        TestClient& client = connect();
        client.send(Hello{protocol_version()});
        REQUIRE(serve_until([&] { return !client.received<MapEntered>().empty(); }));
        client.send(MapLoaded{DemoMap::kId});
        return client;
    }

    template <typename Condition>
    [[nodiscard]] bool serve_until(Condition condition) {
        const std::uint64_t give_up_at = lemuria::platform::monotonic_nanoseconds() + kWaitLimitNanoseconds;
        while (!condition()) {
            if (lemuria::platform::monotonic_nanoseconds() > give_up_at) {
                return false;
            }
            serve_round();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    void serve_round() {
        REQUIRE(host.serve(now_).has_value());
        now_ += kSecondsPerRound;
        for (const auto& client : clients_) {
            client->poll();
        }
    }

    ServerHost host;

private:
    std::vector<std::unique_ptr<TestClient>> clients_;
    double now_ = 0.0;
};

[[nodiscard]] std::uint32_t own_entity(const TestClient& client) {
    const auto entered = client.received<MapEntered>();
    REQUIRE(entered.size() == 1);
    return entered.front().own_entity;
}

[[nodiscard]] Tile own_tile(const TestClient& client) {
    const auto entered = client.received<MapEntered>();
    REQUIRE(entered.size() == 1);
    return entered.front().position;
}

// A tile beside `tile` that the server lets players walk on.
[[nodiscard]] std::optional<Tile> walkable_neighbor(Tile tile) {
    const DemoMap map;
    for (const auto [step_x, step_z] :
         std::array<std::pair<int, int>, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
        const int x = tile.x + step_x;
        const int z = tile.z + step_z;
        if (map.is_walkable_on_server(x, z)) {
            return demo_protocol::tile_from(x, z);
        }
    }
    return std::nullopt;
}

} // namespace

TEST_CASE("the test server puts a client that says hello on the demo map, over a real connection") {
    HostFixture fixture;
    TestClient& client = fixture.connect();

    client.send(Hello{protocol_version()});

    REQUIRE(fixture.serve_until([&] { return !client.received<MapEntered>().empty(); }));
    const MapEntered entered = client.received<MapEntered>().front();
    CHECK(entered.map == DemoMap::kId);
    CHECK(entered.own_entity != 0);
    CHECK(DemoMap{}.is_walkable_on_server(entered.position.x, entered.position.z));
    CHECK(fixture.host.world().player_count() == 1);
    CHECK(client.unreadable_message_count() == 0);
}

TEST_CASE("players on the test server see each other arrive, walk and leave") {
    HostFixture fixture;
    TestClient& alice = fixture.join();
    TestClient& bob = fixture.join();
    const std::uint32_t bob_entity = own_entity(bob);

    REQUIRE(fixture.serve_until([&] { return !alice.received<EntityAppeared>().empty(); }));
    CHECK(alice.received<EntityAppeared>().front().entity == bob_entity);
    REQUIRE(fixture.serve_until([&] { return !bob.received<EntityAppeared>().empty(); }));
    CHECK(bob.received<EntityAppeared>().front().entity == own_entity(alice));

    const Tile start = own_tile(bob);
    const std::optional<Tile> neighbor = walkable_neighbor(start);
    REQUIRE(neighbor.has_value());
    const Tile destination = *neighbor;
    StepList steps;
    REQUIRE(steps.add(destination));
    bob.send(WalkRequest{start, steps});

    REQUIRE(fixture.serve_until([&] { return !alice.received<EntityWalked>().empty(); }));
    CHECK(alice.received<EntityWalked>().front() == EntityWalked{bob_entity, start, steps});
    REQUIRE(fixture.serve_until([&] { return !bob.received<WalkConfirmed>().empty(); }));
    CHECK(bob.received<WalkConfirmed>().front() == WalkConfirmed{start, destination});

    bob.leave();

    REQUIRE(fixture.serve_until([&] { return !alice.received<EntityLeft>().empty(); }));
    CHECK(alice.received<EntityLeft>().front() == EntityLeft{bob_entity, LeaveReason::Disconnected});
    CHECK(fixture.host.connection_count() == 1);
    CHECK(fixture.host.world().player_count() == 1);
}

TEST_CASE("the test server closes a connection whose bytes break the format, and the others keep playing") {
    HostFixture fixture;
    TestClient& alice = fixture.join();
    TestClient& intruder = fixture.connect();

    // A frame with an empty body: there is no way to know where the next frame starts.
    intruder.send_bytes(std::array{std::byte{0x00}, std::byte{0x00}});

    REQUIRE(fixture.serve_until([&] { return intruder.is_closed(); }));
    CHECK(fixture.host.broken_connection_count() == 1);
    CHECK(fixture.host.connection_count() == 1);
    CHECK_FALSE(alice.is_closed());
    CHECK(fixture.host.world().player_count() == 1);
}

TEST_CASE("the test server leaves out a wrong message and goes on reading the same connection") {
    HostFixture fixture;
    TestClient& client = fixture.connect();
    const demo_protocol::EncodedFrame unknown(200);

    client.send_bytes(unknown.bytes());
    client.send(Hello{protocol_version()});

    REQUIRE(fixture.serve_until([&] { return !client.received<MapEntered>().empty(); }));
    CHECK(fixture.host.invalid_message_count() == 1);
    CHECK(fixture.host.broken_connection_count() == 0);
    CHECK_FALSE(client.is_closed());
}

TEST_CASE("the test server reads a message that arrives one byte at a time") {
    HostFixture fixture;
    TestClient& client = fixture.connect();
    const demo_protocol::EncodedFrame hello = demo_protocol::encode(ClientMessage{Hello{protocol_version()}});

    for (const std::byte byte : hello.bytes()) {
        client.send_bytes(std::span(&byte, 1));
        fixture.serve_round();
    }

    REQUIRE(fixture.serve_until([&] { return !client.received<MapEntered>().empty(); }));
    CHECK(fixture.host.invalid_message_count() == 0);
}

TEST_CASE("the test server ends the connection of a client that speaks another version") {
    HostFixture fixture;
    TestClient& client = fixture.connect();

    client.send(Hello{static_cast<std::uint16_t>(protocol_version() + 1)});

    REQUIRE(fixture.serve_until([&] { return client.is_closed(); }));
    CHECK(client.received<MapEntered>().empty());
    CHECK(fixture.host.connection_count() == 0);
    CHECK(fixture.host.world().player_count() == 0);
}

TEST_CASE("the test server closes the connections beyond its limit as soon as they arrive") {
    HostFixture fixture(HostLimits{.max_connections = 1});
    TestClient& first = fixture.connect();
    REQUIRE(fixture.serve_until([&] { return fixture.host.connection_count() == 1; }));

    TestClient& second = fixture.connect();

    REQUIRE(fixture.serve_until([&] { return second.is_closed(); }));
    CHECK(fixture.host.refused_connection_count() == 1);
    first.send(Hello{protocol_version()});
    REQUIRE(fixture.serve_until([&] { return !first.received<MapEntered>().empty(); }));
    CHECK_FALSE(first.is_closed());
}

TEST_CASE("the test server refuses limits and rules it cannot follow") {
    auto listener = lemuria::platform::TcpListener::open_on_this_computer(kFirstTestPort, kTestPortCount);
    REQUIRE(listener.has_value());

    SECTION("no room for connections") {
        CHECK_FALSE(ServerHost::create(std::move(*listener), ServerGrid::from_demo_map(DemoMap{}),
                                       demo_server_config(), HostLimits{.max_connections = 0})
                        .has_value());
    }
    SECTION("no room for players") {
        test_server::ServerConfig config = demo_server_config();
        config.max_players = 0;
        CHECK_FALSE(ServerHost::create(std::move(*listener), ServerGrid::from_demo_map(DemoMap{}), config,
                                       demo_host_limits())
                        .has_value());
    }
}
