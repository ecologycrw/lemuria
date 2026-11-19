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

namespace classic = lemuria::classic;
namespace core = lemuria::core;

using classic::EntityId;
using classic::TileCoord;
using demo_client::DemoAdapter;
using demo_client::DemoGame;
using demo_protocol::DemoMap;
using test_server::ServerHost;

namespace {

constexpr double kFrameSeconds = 1.0 / 60.0;

// How many tiles each player walks. At the speed of the demonstration, the walk lasts 0.75 s of game time.
constexpr std::size_t kWalkLength = 3;

using WalkLine = std::array<TileCoord, kWalkLength>;

// One player, connected as in the program with the window: a real TCP connection, the demo adapter and the
// game.
class Player {
public:
    explicit Player(std::uint16_t port) : adapter_(connections_, core::ConnectionAddress{"127.0.0.1", port}) {
        auto created = DemoGame::create(adapter_);
        REQUIRE(created.has_value());
        game_ = std::move(*created);
        REQUIRE(game_->start().has_value());
    }

    [[nodiscard]] DemoGame& game() noexcept { return *game_; }
    [[nodiscard]] const DemoGame& game() const noexcept { return *game_; }
    [[nodiscard]] const DemoAdapter& adapter() const noexcept { return adapter_; }

    [[nodiscard]] bool is_on_map() const noexcept {
        return game_->session().phase() == classic::SessionPhase::InWorld && game_->world().own() != nullptr;
    }

    // The player itself, once it is on the map.
    [[nodiscard]] const classic::Entity& own() const {
        const classic::Entity* own = game_->world().own();
        REQUIRE(own != nullptr);
        return *own;
    }

    [[nodiscard]] bool sees(EntityId entity) const noexcept { return game_->world().find(entity) != nullptr; }

    [[nodiscard]] bool sees_walking(EntityId entity) const noexcept {
        const classic::Entity* found = game_->world().find(entity);
        return found != nullptr && found->motion.is_walking();
    }

    // Whether this player sees the entity on the tile and no longer walking. Works for the player itself too.
    [[nodiscard]] bool sees_standing_on(EntityId entity, TileCoord tile) const noexcept {
        const classic::Entity* found = game_->world().find(entity);
        return found != nullptr && found->tile() == tile && !found->motion.is_walking();
    }

private:
    lemuria::platform::TcpConnections connections_;
    DemoAdapter adapter_;
    std::unique_ptr<DemoGame> game_;
};

[[nodiscard]] ServerHost open_host() {
    auto listener = lemuria::platform::TcpListener::open_on_this_computer(demo_test_support::kFirstTestPort,
                                                                          demo_test_support::kTestPortCount);
    REQUIRE(listener.has_value());
    auto host = ServerHost::create(std::move(*listener), test_server::ServerGrid::from_demo_map(DemoMap{}),
                                   test_server::demo_server_config(), test_server::demo_host_limits());
    REQUIRE(host.has_value());
    return std::move(*host);
}

// The test server and two players in the same process, connected by real connections on this computer.
class Match {
public:
    Match() : host_(open_host()), alice_(host_.port()), bob_(host_.port()) {}

    [[nodiscard]] Player& alice() noexcept { return alice_; }
    [[nodiscard]] Player& bob() noexcept { return bob_; }
    [[nodiscard]] const ServerHost& host() const noexcept { return host_; }

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
        const core::FrameTime time{kFrameSeconds, elapsed_, frame_index_};
        alice_.game().frame(time);
        bob_.game().frame(time);
        elapsed_ += kFrameSeconds;
        ++frame_index_;
    }

    ServerHost host_;
    Player alice_;
    Player bob_;
    double started_at_ = demo_test_support::real_seconds();
    double elapsed_ = 0.0;
    std::uint64_t frame_index_ = 0;
};

// A walk in a straight line, away from the given tiles, on tiles the server allows. A straight line is the
// only shortest route to its end, because a diagonal step is longer, so the client walks exactly these tiles.
[[nodiscard]] std::optional<WalkLine> straight_walk(TileCoord start, std::span<const TileCoord> keep_away) {
    const DemoMap map;
    for (const auto [step_x, step_z] :
         std::array<std::array<int, 2>, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
        WalkLine line;
        bool fits = true;
        for (std::size_t index = 0; index < kWalkLength; ++index) {
            const int distance = static_cast<int>(index) + 1;
            const TileCoord tile{start.x + (step_x * distance), start.z + (step_z * distance)};
            fits = fits && map.is_walkable_on_server(tile.x, tile.z) &&
                   std::ranges::find(keep_away, tile) == keep_away.end();
            line[index] = tile;
        }
        if (fits) {
            return line;
        }
    }
    return std::nullopt;
}

void check_nothing_was_refused(const Match& match, const Player& alice, const Player& bob) {
    for (int refusal = 0; refusal < test_server::kWalkRefusalCount; ++refusal) {
        CHECK(match.host().world().refused_walk_count(static_cast<test_server::WalkRefusal>(refusal)) == 0);
    }
    CHECK(match.host().invalid_message_count() == 0);
    CHECK(match.host().broken_connection_count() == 0);
    for (const Player* player : {&alice, &bob}) {
        CHECK(player->adapter().invalid_message_count() == 0);
        CHECK(player->game().world().refused_event_count() == 0);
        CHECK(player->game().world().unknown_entity_event_count() == 0);
        CHECK(player->game().unsent_request_count() == 0);
    }
}

void check_leaves(Match& match, Player& leaving, const Player& staying) {
    const EntityId leaving_entity = leaving.own().id;

    leaving.game().end();

    REQUIRE(match.play_until([&] { return !staying.sees(leaving_entity); }));
    CHECK(leaving.game().status() == "disconnected");
    CHECK(staying.game().status() == "on the map");
    CHECK(match.host().world().player_count() == 1);
    CHECK(match.host().connection_count() == 1);
}

} // namespace

TEST_CASE(
    "two players on the test server see each other arrive, walk to the end and leave, over real connections",
    "[0000-CA-09]") {
    Match match;
    Player& alice = match.alice();
    Player& bob = match.bob();

    // Each one sees the other appear, on the tile where the other is.
    REQUIRE(match.play_until([&] { return alice.is_on_map() && bob.is_on_map(); }));
    const EntityId alice_entity = alice.own().id;
    const EntityId bob_entity = bob.own().id;
    CHECK_FALSE(alice_entity == bob_entity);
    REQUIRE(match.play_until([&] { return alice.sees(bob_entity) && bob.sees(alice_entity); }));
    CHECK(alice.game().world().find(bob_entity)->tile() == bob.own().tile());
    CHECK(bob.game().world().find(alice_entity)->tile() == alice.own().tile());
    CHECK(match.host().world().player_count() == 2);

    // Both click at the same time, each away from the other, and each one sees the other walk to the end.
    const TileCoord alice_start = alice.own().tile();
    const TileCoord bob_start = bob.own().tile();
    const std::optional<WalkLine> bob_walk = straight_walk(bob_start, std::array{alice_start});
    REQUIRE(bob_walk.has_value());
    std::vector<TileCoord> taken{alice_start, bob_start};
    taken.insert(taken.end(), bob_walk->begin(), bob_walk->end());
    const std::optional<WalkLine> alice_walk = straight_walk(alice_start, taken);
    REQUIRE(alice_walk.has_value());
    const TileCoord alice_end = alice_walk->back();
    const TileCoord bob_end = bob_walk->back();

    REQUIRE(alice.game().click(alice_end));
    REQUIRE(bob.game().click(bob_end));

    bool alice_saw_bob_walking = false;
    bool bob_saw_alice_walking = false;
    REQUIRE(match.play_until([&] {
        alice_saw_bob_walking = alice_saw_bob_walking || alice.sees_walking(bob_entity);
        bob_saw_alice_walking = bob_saw_alice_walking || bob.sees_walking(alice_entity);
        return alice.sees_standing_on(bob_entity, bob_end) && bob.sees_standing_on(alice_entity, alice_end) &&
               alice.sees_standing_on(alice_entity, alice_end) && bob.sees_standing_on(bob_entity, bob_end);
    }));
    CHECK(alice_saw_bob_walking);
    CHECK(bob_saw_alice_walking);

    // The server agreed with every step, and nothing that arrived was wrong.
    check_nothing_was_refused(match, alice, bob);

    // Each one sees the other disappear when the other disconnects.
    SECTION("Bob disconnects, and Alice sees him disappear") {
        check_leaves(match, bob, alice);
    }
    SECTION("Alice disconnects, and Bob sees her disappear") {
        check_leaves(match, alice, bob);
    }
}
