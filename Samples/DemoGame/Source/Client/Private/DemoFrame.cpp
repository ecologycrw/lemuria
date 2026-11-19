#include "DemoClient/DemoFrame.hpp"

#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"

namespace demo_client {

namespace core = lemuria::core;

DemoFrame::DemoFrame(DemoGame& game, core::Vec2 screen_size)
    : game_(game), scene_(kGroundTileBudget, kBoxBudget), camera_(kCameraSettings) {
    camera_.focus_on(game_.focus(), screen_size);
}

std::size_t DemoFrame::handle_clicks(std::span<const core::Vec2> clicks) {
    std::size_t walks = 0;
    for (const core::Vec2 click : clicks) {
        if (const auto tile = camera_.tile_under(click); tile && game_.click(*tile)) {
            ++walks;
        }
    }
    return walks;
}

void DemoFrame::advance(const core::FrameTime& time) {
    game_.frame(time);
}

std::optional<std::string_view> DemoFrame::take_status_change() noexcept {
    const std::string_view status = game_.status();
    if (status == shown_status_) {
        return std::nullopt;
    }
    shown_status_ = status;
    return status;
}

void DemoFrame::describe_scene() {
    game_.describe(scene_);
}

void DemoFrame::follow_player(core::Vec2 screen_size) noexcept {
    camera_.focus_on(game_.focus(), screen_size);
    scene_.view_projection = camera_.view_projection();
}

} // namespace demo_client
