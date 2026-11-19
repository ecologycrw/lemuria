#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lemuria/Classic/EntityMotion.hpp"
#include "Lemuria/Core/SeededRandom.hpp"

using Catch::Approx;
using lemuria::classic::EntityMotion;
using lemuria::classic::MotionSpeeds;
using lemuria::classic::TileCoord;
using lemuria::classic::WalkSteps;
using lemuria::core::Vec3;

namespace {

constexpr double kWalkSpeed = 4.0;
constexpr double kSlideSpeed = 8.0;
constexpr MotionSpeeds kSpeeds{kWalkSpeed, kSlideSpeed};
constexpr double kStraightStep = 1.0 / kWalkSpeed;
constexpr double kPrecision = 0.0001;

using Steps = std::vector<TileCoord>;

void run_frames(EntityMotion& motion, int frames, double frame_seconds) {
    for (int frame = 0; frame < frames; ++frame) {
        static_cast<void>(motion.advance(frame_seconds, kSpeeds));
    }
}

[[nodiscard]] double distance(Vec3 first, Vec3 second) {
    return std::hypot(static_cast<double>(first.x) - second.x, static_cast<double>(first.z) - second.z);
}

void check_position(const EntityMotion& motion, double x, double z) {
    CHECK(motion.position().x == Approx(x).margin(kPrecision));
    CHECK(motion.position().y == 0.0F);
    CHECK(motion.position().z == Approx(z).margin(kPrecision));
}

} // namespace

TEST_CASE("entity motion starts at the middle of its tile") {
    const EntityMotion motion({3, 7});

    CHECK(motion.tile() == TileCoord{3, 7});
    CHECK_FALSE(motion.is_walking());
    check_position(motion, 3.5, 7.5);
}

TEST_CASE("entity motion is at the same point at any frame rate", "[0000-CA-04]") {
    // A whole number of frames at 30, 60 and 144 frames per second.
    constexpr double kWalkTime = 1.5;
    const Steps route{{1, 0}, {2, 1}, {3, 1}, {4, 2}, {4, 3}, {5, 4}, {6, 4}, {7, 4}};
    EntityMotion reference({0, 0});
    reference.follow(route);
    static_cast<void>(reference.advance(kWalkTime, kSpeeds));

    const int frames_per_second = GENERATE(30, 60, 144);
    EntityMotion motion({0, 0});
    motion.follow(route);

    const int frames = static_cast<int>(std::lround(kWalkTime * frames_per_second));
    run_frames(motion, frames, 1.0 / frames_per_second);

    CHECK(distance(motion.position(), reference.position()) < kPrecision);
    CHECK(motion.tile() == reference.tile());
}

TEST_CASE("entity motion covers the same distance with frames of different lengths", "[0000-CA-04]") {
    const Steps route{{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}};
    EntityMotion reference({0, 0});
    reference.follow(route);
    static_cast<void>(reference.advance(1.5, kSpeeds));

    EntityMotion motion({0, 0});
    motion.follow(route);
    lemuria::core::SeededRandom random(20261118);
    double time_left = 1.5;
    while (time_left > 0.0) {
        const double frame = std::min(time_left, 0.001 + random.next_unit() * 0.05);
        static_cast<void>(motion.advance(frame, kSpeeds));
        time_left -= frame;
    }

    CHECK(distance(motion.position(), reference.position()) < kPrecision);
}

TEST_CASE("entity motion walks at the chosen speed", "[0000-CA-04]") {
    EntityMotion motion({0, 0});
    motion.follow(Steps{{1, 0}, {2, 0}, {3, 0}});

    static_cast<void>(motion.advance(kStraightStep * 1.5, kSpeeds));

    check_position(motion, 2.0, 0.5);
    CHECK(motion.tile() == TileCoord{1, 0});
    CHECK(motion.route_start() == TileCoord{2, 0});
}

TEST_CASE("entity motion takes longer on a diagonal step", "[0000-CA-04]") {
    EntityMotion motion({0, 0});
    motion.follow(Steps{{1, 1}});

    static_cast<void>(motion.advance(kStraightStep, kSpeeds));
    CHECK(motion.is_walking());
    CHECK(motion.tile() == TileCoord{0, 0});

    static_cast<void>(motion.advance(kStraightStep * (std::numbers::sqrt2 - 1.0) + 0.0001, kSpeeds));
    CHECK_FALSE(motion.is_walking());
    CHECK(motion.tile() == TileCoord{1, 1});
    check_position(motion, 1.5, 1.5);
}

TEST_CASE("entity motion stops at the last step and returns the time it did not use", "[0000-CA-04]") {
    EntityMotion motion({0, 0});
    motion.follow(Steps{{1, 0}, {2, 0}});

    const double time_left = motion.advance(kStraightStep * 2.0 + 0.5, kSpeeds);

    CHECK(time_left == Approx(0.5));
    CHECK_FALSE(motion.is_walking());
    CHECK(motion.tile() == TileCoord{2, 0});
    check_position(motion, 2.5, 0.5);
}

TEST_CASE("entity motion uses all the time while it is walking", "[0000-CA-04]") {
    EntityMotion motion({0, 0});
    motion.follow(Steps{{1, 0}, {2, 0}});

    CHECK(motion.advance(kStraightStep, kSpeeds) == 0.0);
}

TEST_CASE("entity motion returns all the time when it has nowhere to go", "[0000-CA-04]") {
    EntityMotion motion({4, 4});

    CHECK(motion.advance(0.25, kSpeeds) == Approx(0.25));
    check_position(motion, 4.5, 4.5);
}

TEST_CASE("entity motion does not move without time", "[0000-CA-04]") {
    EntityMotion motion({0, 0});
    motion.follow(Steps{{1, 0}});
    const double seconds = GENERATE(0.0, -1.0);

    CHECK(motion.advance(seconds, kSpeeds) == 0.0);

    check_position(motion, 0.5, 0.5);
}

TEST_CASE("entity motion finishes the current step before following a new route", "[0000-CA-05]") {
    EntityMotion motion({0, 0});
    motion.follow(Steps{{1, 0}, {2, 0}});
    static_cast<void>(motion.advance(kStraightStep / 2.0, kSpeeds));

    motion.follow(Steps{{1, 1}});

    CHECK(motion.route_start() == TileCoord{1, 0});
    static_cast<void>(motion.advance(kStraightStep / 2.0, kSpeeds));
    CHECK(motion.tile() == TileCoord{1, 0});
    check_position(motion, 1.5, 0.5);
    CHECK(motion.route_start() == TileCoord{1, 1});

    static_cast<void>(motion.advance(kStraightStep, kSpeeds));
    CHECK(motion.tile() == TileCoord{1, 1});
    CHECK_FALSE(motion.is_walking());
}

TEST_CASE("entity motion walks only as many steps as it can hold") {
    Steps route;
    for (std::size_t step = 1; step <= WalkSteps::kCapacity + 5; ++step) {
        route.push_back({static_cast<int>(step), 0});
    }
    EntityMotion motion({0, 0});
    motion.follow(route);

    static_cast<void>(motion.advance(1000.0, kSpeeds));

    CHECK(motion.tile() == TileCoord{static_cast<int>(WalkSteps::kCapacity), 0});
}

TEST_CASE("entity motion takes the corrected tile at once and cancels the route", "[0000-CA-06]") {
    EntityMotion motion({0, 0});
    motion.follow(Steps{{1, 0}, {2, 0}, {3, 0}});
    static_cast<void>(motion.advance(kStraightStep * 1.5, kSpeeds));
    const Vec3 before = motion.position();

    motion.place_at({1, 5});

    CHECK(motion.tile() == TileCoord{1, 5});
    CHECK_FALSE(motion.is_walking());
    CHECK(motion.route_start() == TileCoord{1, 5});
    CHECK(distance(motion.position(), before) == 0.0);
}

TEST_CASE("entity motion glides to the corrected tile without jumping", "[0000-CA-06]") {
    constexpr double kFrame = 1.0 / 60.0;
    EntityMotion motion({0, 0});
    motion.place_at({0, 4});

    for (int frame = 0; frame < 20; ++frame) {
        const Vec3 before = motion.position();
        static_cast<void>(motion.advance(kFrame, kSpeeds));
        CHECK(distance(motion.position(), before) == Approx(kSlideSpeed * kFrame).margin(kPrecision));
    }

    static_cast<void>(motion.advance(1.0, kSpeeds));
    check_position(motion, 0.5, 4.5);
}

TEST_CASE("entity motion returns the time left after gliding", "[0000-CA-06]") {
    EntityMotion motion({0, 0});
    motion.place_at({0, 4});

    CHECK(motion.advance(1.0, kSpeeds) == Approx(0.5));
}

TEST_CASE("entity motion can start walking again while it glides", "[0000-CA-06]") {
    EntityMotion motion({0, 0});
    motion.place_at({0, 4});
    static_cast<void>(motion.advance(0.1, kSpeeds));

    motion.follow(Steps{{1, 4}});
    static_cast<void>(motion.advance(100.0, kSpeeds));

    CHECK(motion.tile() == TileCoord{1, 4});
    check_position(motion, 1.5, 4.5);
}
