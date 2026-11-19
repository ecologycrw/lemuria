#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/WorldIds.hpp"
#include "RealMatch.hpp"

namespace classic = lemuria::classic;

using classic::EntityId;
using classic::TileCoord;
using demo_client::testing::Match;
using demo_client::testing::Player;
using demo_client::testing::straight_walk;

namespace {

// How many tiles each player walks. At the speed of the demonstration, the walk lasts 0.75 s of game time.
constexpr std::size_t kWalkLength = 3;

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
    const auto bob_walk = straight_walk(bob_start, kWalkLength, std::array{alice_start});
    REQUIRE(bob_walk.has_value());
    std::vector<TileCoord> taken{alice_start, bob_start};
    taken.insert(taken.end(), bob_walk->begin(), bob_walk->end());
    const auto alice_walk = straight_walk(alice_start, kWalkLength, taken);
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
    demo_client::testing::check_nothing_was_refused(match);

    // Each one sees the other disappear when the other disconnects.
    SECTION("Bob disconnects, and Alice sees him disappear") {
        check_leaves(match, bob, alice);
    }
    SECTION("Alice disconnects, and Bob sees her disappear") {
        check_leaves(match, alice, bob);
    }
}
