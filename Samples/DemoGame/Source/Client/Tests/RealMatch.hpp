#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "DemoClient/DemoAdapter.hpp"
#include "DemoClient/DemoGame.hpp"
#include "DemoProtocol/DemoMap.hpp"
#include "DemoTestSupport/RealNetwork.hpp"
#include "Lemuria/Classic/SessionPhase.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/World.hpp"
#include "Lemuria/Classic/WorldIds.hpp"
#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/FrameClock.hpp"
#include "Lemuria/Platform/TcpConnections.hpp"
#include "Lemuria/Platform/TcpListener.hpp"
#include "TestServer/ServerGrid.hpp"
#include "TestServer/ServerHost.hpp"
#include "TestServer/WalkCheck.hpp"

// The test server with the rules of the demonstration and two players, all in the same process and connected
// by real connections on this computer.
namespace demo_client::testing {

inline constexpr double kFrameSeconds = 1.0 / 60.0;

// One player, connected as in the program with the window: a real TCP connection, the demo adapter and the
// game.
class Player {
public:
    explicit Player(std::uint16_t port)
        : adapter_(connections_, lemuria::core::ConnectionAddress{"127.0.0.1", port}) {
        auto created = DemoGame::create(adapter_);
        REQUIRE(created.has_value());
        game_ = std::move(*created);
        REQUIRE(game_->start().has_value());
    }

    [[nodiscard]] DemoGame& game() noexcept { return *game_; }
    [[nodiscard]] const DemoGame& game() const noexcept { return *game_; }
    [[nodiscard]] const DemoAdapter& adapter() const noexcept { return adapter_; }

    [[nodiscard]] bool is_on_map() const noexcept {
        return game_->session().phase() == lemuria::classic::SessionPhase::InWorld &&
               game_->world().own() != nullptr;
    }

    // The player itself, once it is on the map.
    [[nodiscard]] const lemuria::classic::Entity& own() const {
        const lemuria::classic::Entity* own = game_->world().own();
        REQUIRE(own != nullptr);
        return *own;
    }

    [[nodiscard]] bool sees(lemuria::classic::EntityId entity) const noexcept {
        return game_->world().find(entity) != nullptr;
    }

    [[nodiscard]] bool sees_walking(lemuria::classic::EntityId entity) const noexcept {
        const lemuria::classic::Entity* found = game_->world().find(entity);
        return found != nullptr && found->motion.is_walking();
    }

    // Whether this player sees the entity on the tile and no longer walking. Works for the player itself too.
    [[nodiscard]] bool sees_standing_on(lemuria::classic::EntityId entity,
                                        lemuria::classic::TileCoord tile) const noexcept {
        const lemuria::classic::Entity* found = game_->world().find(entity);
        return found != nullptr && found->tile() == tile && !found->motion.is_walking();
    }

private:
    lemuria::platform::TcpConnections connections_;
    DemoAdapter adapter_;
    std::unique_ptr<DemoGame> game_;
};

[[nodiscard]] inline test_server::ServerHost open_demo_host() {
    auto listener = lemuria::platform::TcpListener::open_on_this_computer(demo_test_support::kFirstTestPort,
                                                                          demo_test_support::kTestPortCount);
    REQUIRE(listener.has_value());
    auto host = test_server::ServerHost::create(
        std::move(*listener), test_server::ServerGrid::from_demo_map(demo_protocol::DemoMap{}),
        test_server::demo_server_config(), test_server::demo_host_limits());
    REQUIRE(host.has_value());
    return std::move(*host);
}

class Match {
public:
    Match() : host_(open_demo_host()), alice_(host_.port()), bob_(host_.port()) {}

    [[nodiscard]] Player& alice() noexcept { return alice_; }
    [[nodiscard]] Player& bob() noexcept { return bob_; }
    [[nodiscard]] const test_server::ServerHost& host() const noexcept { return host_; }

    template <typename Condition>
    [[nodiscard]] bool play_until(Condition condition) {
        return demo_test_support::wait_until(condition, [this] { play_round(); });
    }

private:
    // One frame of the server and of both games, with the time given by the test (INV-05). The game time
    // never runs ahead of the real clock: the rounds are much faster than real frames, and the time limits of
    // the sessions must mean what they say while the real connections take real time.
    void play_round() {
        if (elapsed_ > demo_test_support::real_seconds() - started_at_) {
            return;
        }
        REQUIRE(host_.serve(elapsed_).has_value());
        const lemuria::core::FrameTime time{kFrameSeconds, elapsed_, frame_index_};
        alice_.game().frame(time);
        bob_.game().frame(time);
        elapsed_ += kFrameSeconds;
        ++frame_index_;
    }

    test_server::ServerHost host_;
    Player alice_;
    Player bob_;
    double started_at_ = demo_test_support::real_seconds();
    double elapsed_ = 0.0;
    std::uint64_t frame_index_ = 0;
};

// A walk in a straight line of `length` tiles, away from the given tiles, on tiles the server allows. A
// straight line is the only shortest route to its end, because a diagonal step is longer, so the client walks
// exactly these tiles.
[[nodiscard]] inline std::optional<std::vector<lemuria::classic::TileCoord>>
straight_walk(lemuria::classic::TileCoord start, std::size_t length,
              std::span<const lemuria::classic::TileCoord> keep_away) {
    const demo_protocol::DemoMap map;
    for (const auto [step_x, step_z] :
         std::array<std::array<int, 2>, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
        std::vector<lemuria::classic::TileCoord> line;
        bool fits = true;
        for (std::size_t index = 0; index < length; ++index) {
            const int distance = static_cast<int>(index) + 1;
            const lemuria::classic::TileCoord tile{start.x + (step_x * distance),
                                                   start.z + (step_z * distance)};
            fits = fits && map.is_walkable_on_server(tile.x, tile.z) &&
                   std::ranges::find(keep_away, tile) == keep_away.end();
            line.push_back(tile);
        }
        if (fits) {
            return line;
        }
    }
    return std::nullopt;
}

// The server agreed with every step, and nothing that arrived was wrong.
inline void check_nothing_was_refused(Match& match) {
    for (int refusal = 0; refusal < test_server::kWalkRefusalCount; ++refusal) {
        CHECK(match.host().world().refused_walk_count(static_cast<test_server::WalkRefusal>(refusal)) == 0);
    }
    CHECK(match.host().invalid_message_count() == 0);
    CHECK(match.host().broken_connection_count() == 0);
    for (const Player* player : {&match.alice(), &match.bob()}) {
        CHECK(player->adapter().invalid_message_count() == 0);
        CHECK(player->game().world().refused_event_count() == 0);
        CHECK(player->game().world().unknown_entity_event_count() == 0);
        CHECK(player->game().unsent_request_count() == 0);
    }
}

} // namespace demo_client::testing
