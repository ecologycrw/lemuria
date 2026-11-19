#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

#include "DemoClient/DemoGame.hpp"
#include "Lemuria/Classic/ThreeQuarterCamera.hpp"
#include "Lemuria/Core/FrameClock.hpp"
#include "Lemuria/Core/Math.hpp"
#include "Lemuria/Core/SceneDescription.hpp"

namespace demo_client {

// The steps of one frame of the program with the window, in its order, without the window itself: the
// clicks become walks, the game moves, the scene is described and the camera follows the player. The program
// reads the window before and draws after; the test of the warm frame (CA-18) calls the same steps.
class DemoFrame {
public:
    // The camera starts on the player, or on the middle of the map before the player arrives.
    DemoFrame(DemoGame& game, lemuria::core::Vec2 screen_size);

    // Each click becomes the tile under the cursor, as the player saw it in the previous frame. A blocked
    // tile leaves the player where it is (CA-03). Returns how many clicks started a walk.
    std::size_t handle_clicks(std::span<const lemuria::core::Vec2> clicks);

    // Handles what arrived from the server, moves everyone and sends the walk of the player.
    void advance(const lemuria::core::FrameTime& time);

    // The state of the connection, only when it changed since the last call, so a frame builds no text
    // (CA-14).
    [[nodiscard]] std::optional<std::string_view> take_status_change() noexcept;

    // Fills the scene with the ground and everyone on the map.
    void describe_scene();

    // Points the camera at the player, for a screen of this size.
    void follow_player(lemuria::core::Vec2 screen_size) noexcept;

    [[nodiscard]] const lemuria::core::SceneDescription& scene() const noexcept { return scene_; }
    [[nodiscard]] const lemuria::classic::ThreeQuarterCamera& camera() const noexcept { return camera_; }

private:
    DemoGame& game_;
    lemuria::core::SceneDescription scene_;
    lemuria::classic::ThreeQuarterCamera camera_;
    std::string_view shown_status_;
};

} // namespace demo_client
