#include <array>
#include <cstddef>
#include <optional>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Platform/TcpConnections.hpp"
#include "Lemuria/Platform/TcpListener.hpp"
#include "TcpTestSupport.hpp"

using lemuria::platform::TcpListener;
using lemuria::platform::TcpPeer;
using lemuria::platform::test::kFirstTestPort;
using lemuria::platform::test::kTestPortCount;
using lemuria::platform::test::wait_until;

namespace {

[[nodiscard]] TcpListener open_listener() {
    auto listener = TcpListener::open_on_this_computer(kFirstTestPort, kTestPortCount);
    REQUIRE(listener.has_value());
    return std::move(*listener);
}

class SilentEvents final : public lemuria::core::ConnectionEvents {
public:
    void on_connected(lemuria::core::ConnectionId /*connection*/) override {}
    void on_received(const lemuria::core::ReceivedBytes& /*block*/) override {}
    void on_closed(lemuria::core::ConnectionId /*connection*/,
                   const lemuria::core::ConnectionClosed& /*closed*/) override {}
};

} // namespace

TEST_CASE("a listener takes the next free port when the first one is busy") {
    const TcpListener first = open_listener();

    const auto second = TcpListener::open_on_this_computer(first.port(), kTestPortCount);

    REQUIRE(second.has_value());
    CHECK(second->port() > first.port());
}

TEST_CASE("a listener reports that no port is free") {
    const TcpListener busy = open_listener();

    const auto refused = TcpListener::open_on_this_computer(busy.port());

    CHECK_FALSE(refused.has_value());
}

TEST_CASE("a listener needs a port to try") {
    CHECK_FALSE(TcpListener::open_on_this_computer(0).has_value());
    CHECK_FALSE(TcpListener::open_on_this_computer(kFirstTestPort, 0).has_value());
}

TEST_CASE("a listener with nobody connecting returns no connection, without waiting") {
    TcpListener listener = open_listener();

    const auto accepted = listener.accept();

    REQUIRE(accepted.has_value());
    CHECK_FALSE(accepted->has_value());
}

TEST_CASE("a peer with nothing to read returns zero bytes, and fails after the other side leaves") {
    TcpListener listener = open_listener();
    std::optional<TcpPeer> peer;
    SilentEvents events;
    {
        lemuria::platform::TcpConnections connections;
        REQUIRE(connections.open({"127.0.0.1", listener.port()}).has_value());
        REQUIRE(wait_until([&] {
            connections.poll(events);
            auto accepted = listener.accept();
            REQUIRE(accepted.has_value());
            peer = std::move(*accepted);
            return peer.has_value();
        }));

        std::array<std::byte, 8> buffer{};
        const auto nothing = peer->read(buffer);
        REQUIRE(nothing.has_value());
        CHECK(*nothing == 0);
    }

    std::array<std::byte, 8> buffer{};
    CHECK(wait_until([&] { return !peer->read(buffer).has_value(); }));
}
