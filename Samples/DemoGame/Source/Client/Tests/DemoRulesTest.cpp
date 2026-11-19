#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <optional>

#include <catch2/catch_test_macros.hpp>

#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Messages.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/WorldIds.hpp"
#include "Lemuria/Core/Math.hpp"
#include "RealMatch.hpp"
#include "TestServer/WalkCheck.hpp"

// The demo client and the test server, each with the values of the demonstration, agree on every walk a
// player can make, and the server corrects only what it forbids.

namespace classic = lemuria::classic;
namespace core = lemuria::core;

using classic::EntityId;
using classic::TileCoord;
using demo_client::testing::check_nothing_was_refused;
using demo_client::testing::Match;
using demo_client::testing::Player;
using demo_client::testing::straight_walk;
using demo_protocol::DemoMap;
using test_server::WalkRefusal;

namespace {

[[nodiscard]] float distance(core::Vec3 first, core::Vec3 second) noexcept {
    return std::hypot(first.x - second.x, first.z - second.z);
}

// How far the player is along the step from one tile to the next, from 0 to 1.
[[nodiscard]] float progress_of_step(const Player& player, TileCoord from, TileCoord to) {
    return distance(player.own().motion.position(), classic::tile_center(from)) /
           distance(classic::tile_center(to), classic::tile_center(from));
}

// The tile the server forbids and the client believes walkable that is closest to `start`.
[[nodiscard]] std::optional<TileCoord> nearest_forbidden_tile(TileCoord start) {
    const DemoMap map;
    std::optional<TileCoord> nearest;
    int nearest_steps = 0;
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            if (!map.looks_walkable(x, z) || map.is_walkable_on_server(x, z)) {
                continue;
            }
            const int steps = std::max(std::abs(x - start.x), std::abs(z - start.z));
            if (!nearest || steps < nearest_steps) {
                nearest = TileCoord{x, z};
                nearest_steps = steps;
            }
        }
    }
    return nearest;
}

} // namespace

TEST_CASE(
    "a click in the middle of a step starts the new walk where the player is heading, and the test server "
    "agrees") {
    Match match;
    REQUIRE(match.play_until([&] { return match.alice().is_on_map() && match.bob().is_on_map(); }));
    Player& alice = match.alice();
    const Player& bob = match.bob();
    const EntityId alice_entity = alice.own().id;
    const TileCoord start = alice.own().tile();
    const TileCoord bob_tile = bob.own().tile();
    const auto first = straight_walk(start, 3, std::array{bob_tile});
    REQUIRE(first.has_value());
    const TileCoord heading = first->front();

    REQUIRE(alice.game().click(first->back()));
    REQUIRE(match.play_until([&] { return progress_of_step(alice, start, heading) >= 0.4F; }));
    REQUIRE(alice.own().motion.route_start() == heading);
    REQUIRE(progress_of_step(alice, start, heading) < 1.0F);

    const auto second = straight_walk(heading, 3, std::array{bob_tile, first->back()});
    REQUIRE(second.has_value());
    REQUIRE(alice.game().click(second->back()));

    REQUIRE(match.play_until([&] {
        return alice.sees_standing_on(alice_entity, second->back()) &&
               bob.sees_standing_on(alice_entity, second->back());
    }));
    check_nothing_was_refused(match);
}

TEST_CASE("a walk longer than one request reaches its end, and the test server accepts every part") {
    Match match;
    REQUIRE(match.play_until([&] { return match.alice().is_on_map() && match.bob().is_on_map(); }));
    Player& alice = match.alice();
    const Player& bob = match.bob();
    const EntityId alice_entity = alice.own().id;
    constexpr std::size_t kLength = demo_protocol::kMaxWalkSteps + 4;
    const auto walk = straight_walk(alice.own().tile(), kLength, std::array{bob.own().tile()});
    REQUIRE(walk.has_value());

    REQUIRE(alice.game().click(walk->back()));

    REQUIRE(match.play_until([&] {
        return alice.sees_standing_on(alice_entity, walk->back()) &&
               bob.sees_standing_on(alice_entity, walk->back());
    }));
    check_nothing_was_refused(match);
}

TEST_CASE("a walk onto the strip the test server forbids is corrected, and both players see the player where "
          "the server says",
          "[0000-CA-06]") {
    Match match;
    REQUIRE(match.play_until([&] { return match.alice().is_on_map() && match.bob().is_on_map(); }));
    Player& alice = match.alice();
    const Player& bob = match.bob();
    const EntityId alice_entity = alice.own().id;
    REQUIRE(match.play_until([&] { return bob.sees(alice_entity); }));
    const std::optional<TileCoord> forbidden = nearest_forbidden_tile(alice.own().tile());
    REQUIRE(forbidden.has_value());

    REQUIRE(alice.game().click(*forbidden));

    const auto refusals = [&] {
        return match.host().world().refused_walk_count(WalkRefusal::BlockedStep) +
               match.host().world().refused_walk_count(WalkRefusal::CutsCorner);
    };
    bool alice_walked = false;
    REQUIRE(match.play_until([&] {
        alice_walked = alice_walked || alice.own().motion.is_walking();
        return refusals() > 0 && !alice.own().motion.is_walking() &&
               bob.sees_standing_on(alice_entity, alice.own().tile());
    }));
    CHECK(alice_walked);
    const TileCoord corrected = alice.own().tile();
    CHECK_FALSE(corrected == *forbidden);
    CHECK(DemoMap{}.is_walkable_on_server(corrected.x, corrected.z));
    CHECK(alice.game().status() == "on the map");
}
