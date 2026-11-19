#include <cstddef>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Classic/WalkSteps.hpp"

using lemuria::classic::TileCoord;
using lemuria::classic::WalkSteps;

namespace {

[[nodiscard]] WalkSteps steps_of(const std::vector<TileCoord>& tiles) {
    WalkSteps steps;
    for (const TileCoord tile : tiles) {
        REQUIRE(steps.add(tile));
    }
    return steps;
}

[[nodiscard]] WalkSteps full_steps() {
    WalkSteps steps;
    for (std::size_t index = 0; index < WalkSteps::kCapacity; ++index) {
        REQUIRE(steps.add({static_cast<int>(index), 0}));
    }
    return steps;
}

} // namespace

TEST_CASE("walk steps start empty") {
    const WalkSteps steps;

    CHECK(steps.empty());
    CHECK(steps.size() == 0);
    CHECK(steps.view().empty());
}

TEST_CASE("walk steps keep the order they were added in") {
    const WalkSteps steps = steps_of({{3, 1}, {4, 1}, {4, 2}});

    CHECK_FALSE(steps.empty());
    REQUIRE(steps.size() == 3);
    CHECK(steps.view()[0] == TileCoord{3, 1});
    CHECK(steps.view()[1] == TileCoord{4, 1});
    CHECK(steps.view()[2] == TileCoord{4, 2});
}

TEST_CASE("walk steps hold up to their capacity") {
    const WalkSteps steps = full_steps();

    CHECK(steps.size() == WalkSteps::kCapacity);
    CHECK(steps.view().back() == TileCoord{static_cast<int>(WalkSteps::kCapacity) - 1, 0});
}

TEST_CASE("walk steps refuse a step when they are full") {
    WalkSteps steps = full_steps();
    const WalkSteps before = steps;

    CHECK_FALSE(steps.add({99, 99}));

    CHECK(steps == before);
}

TEST_CASE("walk steps are equal when they list the same tiles") {
    CHECK(steps_of({{1, 2}, {2, 2}}) == steps_of({{1, 2}, {2, 2}}));
    CHECK_FALSE(steps_of({{1, 2}, {2, 2}}) == steps_of({{1, 2}, {2, 3}}));
    CHECK_FALSE(steps_of({{1, 2}}) == steps_of({{1, 2}, {2, 2}}));
}

TEST_CASE("walk steps never need memory of their own") {
    STATIC_REQUIRE(std::is_trivially_copyable_v<WalkSteps>);
}
