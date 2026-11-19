#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "AllocationCounter.hpp"
#include "DemoClient/DemoAdapter.hpp"
#include "DemoClient/DemoGame.hpp"
#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"
#include "DemoProtocol/DemoMap.hpp"
#include "DemoTestSupport/RealNetwork.hpp"
#include "Lemuria/Classic/SessionPhase.hpp"
#include "Lemuria/Classic/ThreeQuarterCamera.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Classic/World.hpp"
#include "Lemuria/Classic/WorldIds.hpp"
#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/FrameClock.hpp"
#include "Lemuria/Core/Math.hpp"
#include "Lemuria/Core/SceneDescription.hpp"
#include "Lemuria/Platform/Renderer.hpp"
#include "Lemuria/Platform/TcpConnections.hpp"
#include "Lemuria/Platform/TcpListener.hpp"
#include "TestServer/ServerGrid.hpp"
#include "TestServer/ServerHost.hpp"

// The demonstration played by three players over real connections, while the frame of one of them is
// measured.
namespace lemuria::frame_allocations {

inline constexpr double kFrameSeconds = 1.0 / 60.0;
inline constexpr core::Vec2 kScreenSize{1280.0F, 720.0F};

// How many tiles each walk takes. At the speed of the demonstration, a walk lasts 0.5 s of game time.
inline constexpr std::size_t kWalkLength = 2;

using WalkLine = std::array<classic::TileCoord, kWalkLength>;

// The frame warms up by doing everything it will do while measured: the buffers that grow reach their size.
inline constexpr int kWarmUpCycles = 2;
inline constexpr int kMeasuredCycles = 2;

// One player, connected as in the program with the window: a real TCP connection, the demo adapter and the
// game.
class Player {
public:
    explicit Player(std::uint16_t port) : adapter_(connections_, core::ConnectionAddress{"127.0.0.1", port}) {
        auto created = demo_client::DemoGame::create(adapter_);
        REQUIRE(created.has_value());
        game_ = std::move(*created);
        REQUIRE(game_->start().has_value());
    }

    [[nodiscard]] demo_client::DemoGame& game() noexcept { return *game_; }
    [[nodiscard]] const demo_client::DemoGame& game() const noexcept { return *game_; }

    [[nodiscard]] bool is_on_map() const noexcept {
        return game_->session().phase() == classic::SessionPhase::InWorld && game_->world().own() != nullptr;
    }

    // The number of the player itself, once it is on the map.
    [[nodiscard]] classic::EntityId own_id() const {
        const classic::Entity* own = game_->world().own();
        REQUIRE(own != nullptr);
        return own->id;
    }

    [[nodiscard]] classic::TileCoord own_tile() const {
        const classic::Entity* own = game_->world().own();
        REQUIRE(own != nullptr);
        return own->tile();
    }

    [[nodiscard]] bool sees(classic::EntityId entity) const noexcept {
        return game_->world().find(entity) != nullptr;
    }

    // Whether this player sees the entity on the tile and no longer walking. Works for the player itself too.
    [[nodiscard]] bool sees_standing_on(classic::EntityId entity, classic::TileCoord tile) const noexcept {
        const classic::Entity* found = game_->world().find(entity);
        return found != nullptr && found->tile() == tile && !found->motion.is_walking();
    }

private:
    platform::TcpConnections connections_;
    demo_client::DemoAdapter adapter_;
    std::unique_ptr<demo_client::DemoGame> game_;
};

// A walk in a straight line, away from the given tiles, on tiles the server allows. A straight line is the
// only shortest route to its end, because a diagonal step is longer, so the client walks exactly these tiles.
[[nodiscard]] inline std::optional<WalkLine> straight_walk(classic::TileCoord start,
                                                           std::span<const classic::TileCoord> keep_away) {
    const demo_protocol::DemoMap map;
    for (const auto [step_x, step_z] :
         std::array<std::array<int, 2>, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
        WalkLine line;
        bool fits = true;
        for (std::size_t index = 0; index < kWalkLength; ++index) {
            const int distance = static_cast<int>(index) + 1;
            const classic::TileCoord tile{start.x + (step_x * distance), start.z + (step_z * distance)};
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

[[nodiscard]] inline test_server::ServerHost open_host() {
    auto listener = platform::TcpListener::open_on_this_computer(demo_test_support::kFirstTestPort,
                                                                 demo_test_support::kTestPortCount);
    REQUIRE(listener.has_value());
    auto host = test_server::ServerHost::create(
        std::move(*listener), test_server::ServerGrid::from_demo_map(demo_protocol::DemoMap{}),
        test_server::demo_server_config(), test_server::demo_host_limits());
    REQUIRE(host.has_value());
    return std::move(*host);
}

// The parts of the measured frame, in the order they run.
enum class FramePart : std::uint8_t { Click, Game, Scene, Camera, Drawing };
inline constexpr std::size_t kFramePartCount = 5;

// What the measurement saw.
struct Measurement {
    std::uint64_t frames = 0;
    std::uint64_t allocations = 0;
    std::uint64_t frames_with_allocations = 0;

    // The allocations of each part of the frame, to tell where the memory was asked for.
    std::array<std::uint64_t, kFramePartCount> by_part{};
};

inline constexpr std::array<std::uint64_t, kFramePartCount> kNoAllocationInAnyPart{};

// The test server, the measured player (Alice), a second player who walks with her (Bob), and a third who
// arrives and leaves (Carol). Alice's frame is the frame of the program with the window, except reading the
// window: the click, the game, the scene, the camera and, when there is a renderer, the drawing.
class MeasuredMatch {
public:
    explicit MeasuredMatch(platform::Renderer* renderer = nullptr)
        : host_(open_host()), alice_(host_.port()), bob_(host_.port()), renderer_(renderer) {
        REQUIRE(play_until([&] { return alice_.is_on_map() && bob_.is_on_map(); }));
        const classic::EntityId bob = bob_.own_id();
        REQUIRE(play_until([&] { return alice_.sees(bob) && bob_.sees(alice_.own_id()); }));

        alice_start_ = alice_.own_tile();
        bob_start_ = bob_.own_tile();
        const std::optional<WalkLine> bob_walk = straight_walk(bob_start_, std::array{alice_start_});
        REQUIRE(bob_walk.has_value());
        std::vector<classic::TileCoord> taken{alice_start_, bob_start_};
        taken.insert(taken.end(), bob_walk->begin(), bob_walk->end());
        const std::optional<WalkLine> alice_walk = straight_walk(alice_start_, taken);
        REQUIRE(alice_walk.has_value());
        alice_end_ = alice_walk->back();
        bob_end_ = bob_walk->back();
    }

    // Alice and Bob walk away and back, while Carol arrives and leaves: characters walking and events
    // arriving.
    void play_cycle() {
        const classic::EntityId alice = alice_.own_id();
        const classic::EntityId bob = bob_.own_id();

        alice_click_ = alice_end_;
        REQUIRE(bob_.game().click(bob_end_));
        carol_ = std::make_unique<Player>(host_.port());
        REQUIRE(play_until([&] {
            return alice_.sees_standing_on(alice, alice_end_) && alice_.sees_standing_on(bob, bob_end_) &&
                   carol_->is_on_map() && alice_.sees(carol_->own_id());
        }));
        const classic::EntityId carol = carol_->own_id();

        alice_click_ = alice_start_;
        REQUIRE(bob_.game().click(bob_start_));
        carol_->game().end();
        REQUIRE(play_until([&] {
            return alice_.sees_standing_on(alice, alice_start_) && alice_.sees_standing_on(bob, bob_start_) &&
                   !alice_.sees(carol);
        }));
        carol_.reset();
        REQUIRE(alice_clicks_refused_ == 0);
        REQUIRE_FALSE(draw_failed_);
    }

    void start_measuring() noexcept {
        measuring_ = true;
        measurement_ = {};
    }

    [[nodiscard]] Measurement stop_measuring() noexcept {
        measuring_ = false;
        return measurement_;
    }

    // Warms the frame up, then measures it while the same things happen again.
    [[nodiscard]] Measurement measure_after_warming_up() {
        for (int cycle = 0; cycle < kWarmUpCycles; ++cycle) {
            play_cycle();
        }
        start_measuring();
        for (int cycle = 0; cycle < kMeasuredCycles; ++cycle) {
            play_cycle();
        }
        return stop_measuring();
    }

    [[nodiscard]] Player& alice() noexcept { return alice_; }

    template <typename Condition>
    [[nodiscard]] bool play_until(Condition condition) {
        return demo_test_support::wait_until(condition, [this] { play_round(); });
    }

private:
    // One frame of the server and of every game, with the time given by the test (INV-05). The game time
    // never runs ahead of the real clock, so the time limits of the sessions mean what they say.
    void play_round() {
        if (elapsed_ > demo_test_support::real_seconds() - started_at_) {
            return;
        }
        REQUIRE(host_.serve(elapsed_).has_value());
        const core::FrameTime time{kFrameSeconds, elapsed_, frame_index_};
        alice_frame(time);
        bob_.game().frame(time);
        if (carol_ != nullptr) {
            carol_->game().frame(time);
        }
        elapsed_ += kFrameSeconds;
        ++frame_index_;
    }

    // The frame of the program with the window, in the same order. While measuring, each part is counted
    // apart.
    void alice_frame(const core::FrameTime& time) {
        std::uint64_t frame_allocations = 0;
        const auto part = [&](FramePart counted, auto&& work) {
            if (!measuring_) {
                work();
                return;
            }
            const std::uint64_t allocations = allocations_during(work);
            measurement_.by_part[static_cast<std::size_t>(counted)] += allocations;
            frame_allocations += allocations;
        };

        part(FramePart::Click, [&] {
            if (alice_click_) {
                alice_clicks_refused_ += alice_.game().click(*alice_click_) ? 0 : 1;
                alice_click_.reset();
            }
        });
        part(FramePart::Game, [&] { alice_.game().frame(time); });
        part(FramePart::Scene, [&] {
            static_cast<void>(alice_.game().status());
            alice_.game().describe(scene_);
        });
        part(FramePart::Camera, [&] {
            camera_.focus_on(alice_.game().focus(), kScreenSize);
            scene_.view_projection = camera_.view_projection();
        });
        part(FramePart::Drawing, [&] {
            if (renderer_ != nullptr && !renderer_->draw(scene_).has_value()) {
                draw_failed_ = true;
            }
        });

        if (measuring_) {
            ++measurement_.frames;
            measurement_.allocations += frame_allocations;
            measurement_.frames_with_allocations += frame_allocations > 0 ? 1 : 0;
        }
    }

    test_server::ServerHost host_;
    Player alice_;
    Player bob_;
    std::unique_ptr<Player> carol_;
    platform::Renderer* renderer_;
    core::SceneDescription scene_{demo_client::kGroundTileBudget, demo_client::kBoxBudget};
    classic::ThreeQuarterCamera camera_{demo_client::kCameraSettings};

    classic::TileCoord alice_start_;
    classic::TileCoord alice_end_;
    classic::TileCoord bob_start_;
    classic::TileCoord bob_end_;
    std::optional<classic::TileCoord> alice_click_;
    std::uint64_t alice_clicks_refused_ = 0;
    bool draw_failed_ = false;

    bool measuring_ = false;
    Measurement measurement_;
    double started_at_ = demo_test_support::real_seconds();
    double elapsed_ = 0.0;
    std::uint64_t frame_index_ = 0;
};

} // namespace lemuria::frame_allocations
