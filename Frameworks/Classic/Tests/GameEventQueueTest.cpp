#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Classic/GameEventQueue.hpp"

using lemuria::classic::EntityId;
using lemuria::classic::EntityLeaveReason;
using lemuria::classic::EntityLeft;
using lemuria::classic::GameEvent;
using lemuria::classic::GameEventQueue;

namespace {

[[nodiscard]] GameEvent event_number(std::uint32_t number) {
    return EntityLeft{EntityId{number}, EntityLeaveReason::LeftView};
}

} // namespace

TEST_CASE("game event queue starts empty") {
    const GameEventQueue queue(3);

    CHECK(queue.empty());
    CHECK(queue.size() == 0);
}

TEST_CASE("game event queue gives events back in the order they arrived") {
    GameEventQueue queue(3);
    REQUIRE(queue.push(event_number(1)));
    REQUIRE(queue.push(event_number(2)));

    CHECK(queue.size() == 2);
    CHECK(queue.front() == event_number(1));
    queue.pop();
    CHECK(queue.front() == event_number(2));
    queue.pop();
    CHECK(queue.empty());
}

TEST_CASE("game event queue refuses an event when it is full") {
    GameEventQueue queue(2);
    REQUIRE(queue.push(event_number(1)));
    REQUIRE(queue.push(event_number(2)));

    CHECK_FALSE(queue.push(event_number(3)));

    CHECK(queue.size() == 2);
    CHECK(queue.front() == event_number(1));
}

TEST_CASE("game event queue keeps the order after going around its memory") {
    GameEventQueue queue(3);
    for (std::uint32_t number = 1; number <= 10; ++number) {
        REQUIRE(queue.push(event_number(number)));
        REQUIRE(queue.push(event_number(number + 100)));
        CHECK(queue.front() == event_number(number));
        queue.pop();
        CHECK(queue.front() == event_number(number + 100));
        queue.pop();
    }
    CHECK(queue.empty());
}

TEST_CASE("game event queue can be emptied at once") {
    GameEventQueue queue(2);
    REQUIRE(queue.push(event_number(1)));
    REQUIRE(queue.push(event_number(2)));

    queue.clear();

    CHECK(queue.empty());
    REQUIRE(queue.push(event_number(3)));
    REQUIRE(queue.push(event_number(4)));
    CHECK(queue.front() == event_number(3));
}

TEST_CASE("game event queue ignores a pop when it is empty") {
    GameEventQueue queue(2);

    queue.pop();

    CHECK(queue.empty());
    REQUIRE(queue.push(event_number(1)));
    CHECK(queue.size() == 1);
}

TEST_CASE("game event queue without room holds nothing") {
    GameEventQueue queue;

    CHECK_FALSE(queue.push(event_number(1)));
    queue.pop();
    CHECK(queue.empty());
}
