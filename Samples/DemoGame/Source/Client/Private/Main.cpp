#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "DemoClient/DemoAdapter.hpp"
#include "DemoClient/DemoGame.hpp"
#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"
#include "DemoClient/ProofImage.hpp"
#include "DemoClient/ProofScene.hpp"
#include "DemoClient/Smoothness.hpp"
#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Protocol.hpp"
#include "Lemuria/Classic/FrameworkInfo.hpp"
#include "Lemuria/Classic/SessionPhase.hpp"
#include "Lemuria/Classic/ThreeQuarterCamera.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/FrameClock.hpp"
#include "Lemuria/Core/FrameTimeMeter.hpp"
#include "Lemuria/Core/SceneDescription.hpp"
#include "Lemuria/Core/Version.hpp"
#include "Lemuria/Platform/MonotonicClock.hpp"
#include "Lemuria/Platform/PlatformInfo.hpp"
#include "Lemuria/Platform/Renderer.hpp"
#include "Lemuria/Platform/TcpConnections.hpp"
#include "Lemuria/Platform/Window.hpp"

namespace {

namespace core = lemuria::core;
namespace platform = lemuria::platform;

constexpr std::string_view kWindowTitle = "Lemuria Demo";

struct Options {
    bool show_version = false;
    bool measure_frame_times = false;
    std::optional<std::uint64_t> frame_limit;
    std::optional<std::string> proof_image_path;
    std::string graphics_api;
    std::uint16_t server_port = demo_protocol::kDefaultServerPort;
};

void print_version(const core::Version& version) {
    std::cout << version.major << '.' << version.minor << '.' << version.patch;
}

void print_versions() {
    std::cout << "Lemuria Demo: engine ";
    print_version(core::engine_version());
    std::cout << ", framework " << lemuria::classic::framework_name() << ", SDL ";
    print_version(platform::sdl_version());
    std::cout << ", protocol " << demo_protocol::protocol_version() << '\n';
}

// Reads a whole number from the command line, refusing anything else (INV-12).
[[nodiscard]] std::optional<std::uint64_t> parse_count(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return std::nullopt;
        }
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
            return std::nullopt;
        }
        value = value * 10 + digit;
    }
    return value;
}

[[nodiscard]] std::optional<Options> parse_options(std::span<char*> arguments) {
    Options options;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];
        if (argument == "--version") {
            options.show_version = true;
        } else if (argument == "--measure-frame-times") {
            options.measure_frame_times = true;
        } else if (argument == "--frames" && index + 1 < arguments.size()) {
            ++index;
            options.frame_limit = parse_count(arguments[index]);
            if (!options.frame_limit) {
                return std::nullopt;
            }
        } else if (argument == "--proof-image" && index + 1 < arguments.size()) {
            ++index;
            options.proof_image_path = arguments[index];
        } else if (argument == "--port" && index + 1 < arguments.size()) {
            ++index;
            const std::optional<std::uint64_t> port = parse_count(arguments[index]);
            if (!port || *port == 0 || *port > std::numeric_limits<std::uint16_t>::max()) {
                return std::nullopt;
            }
            options.server_port = static_cast<std::uint16_t>(*port);
        } else if (argument == "--graphics-api" && index + 1 < arguments.size()) {
            ++index;
            options.graphics_api = arguments[index];
        } else {
            return std::nullopt;
        }
    }
    return options;
}

[[nodiscard]] platform::RendererOptions renderer_options_for(const Options& options) {
    platform::RendererOptions renderer_options;
    renderer_options.max_ground_tiles = demo_client::kGroundTileBudget;
    renderer_options.max_boxes = demo_client::kBoxBudget;
    renderer_options.graphics_api = options.graphics_api;
    return renderer_options;
}

// Draws the scene used to check the look of the game and saves it as an image.
[[nodiscard]] core::Result<void> save_proof_image(const Options& options, const std::string& path) {
    const auto proof = demo_client::render_proof_image(options.graphics_api);
    if (!proof) {
        return tl::unexpected(proof.error());
    }
    std::cout << "Graphics API: " << proof->graphics_api << '\n';
    return platform::save_png(proof->image, path);
}

// Draws the proof scene on a window as fast as the computer can, without waiting for the display.
[[nodiscard]] core::Result<core::FrameTimeSummary> measure_frame_times(const Options& options) {
    auto window = platform::Window::open(platform::WindowOptions{std::string(kWindowTitle)});
    if (!window) {
        return tl::unexpected(window.error());
    }
    platform::RendererOptions renderer_options = renderer_options_for(options);
    renderer_options.wait_for_display = false;
    auto renderer = platform::Renderer::for_window(*window, renderer_options);
    if (!renderer) {
        return tl::unexpected(renderer.error());
    }

    const demo_protocol::DemoMap map;
    core::SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);
    demo_client::fill_proof_scene(map, scene);

    core::FrameTimeMeter meter(demo_client::kWarmUpFrames, demo_client::kMeasuredFrames);
    platform::InputFrame input;
    std::uint64_t previous = platform::monotonic_nanoseconds();
    while (!meter.finished()) {
        window->poll_input(input);
        if (input.quit_requested) {
            return core::make_error("the window was closed before the measurement finished");
        }
        const auto drawn = renderer->draw(scene);
        if (!drawn) {
            return tl::unexpected(drawn.error());
        }
        const std::uint64_t now = platform::monotonic_nanoseconds();
        meter.record(now - previous);
        previous = now;
    }
    if (renderer->skipped_frame_count() > 0) {
        return core::make_error("the window must stay visible during the measurement");
    }
    return meter.summary();
}

[[nodiscard]] int report_frame_times(const Options& options) {
    const auto summary = measure_frame_times(options);
    if (!summary) {
        std::cerr << summary.error().message << '\n';
        return 1;
    }
    std::cout << demo_client::describe_frame_times(*summary) << '\n';
    return demo_client::is_within_frame_budget(*summary) ? 0 : 1;
}

// Each click becomes the tile under the cursor, as the player saw it in the previous frame. A blocked tile
// leaves the player where it is (CA-03).
void handle_clicks(const platform::InputFrame& input, const lemuria::classic::ThreeQuarterCamera& camera,
                   demo_client::DemoGame& game) {
    for (const core::Vec2 click : input.clicks()) {
        if (const auto tile = camera.tile_under(click)) {
            static_cast<void>(game.click(*tile));
        }
    }
}

// The title changes only when the state of the connection changes, so a frame does not build text (CA-14).
void show_status(platform::Window& window, std::string_view status, std::string_view& shown) {
    if (status == shown) {
        return;
    }
    shown = status;
    const std::string title = std::string(kWindowTitle) + " - " + std::string(status);
    if (const auto set = window.set_title(title); !set) {
        std::cerr << set.error().message << '\n';
    }
}

// Runs the game until the window is closed, or until the frame limit of the options.
[[nodiscard]] core::Result<void> play(const Options& options, platform::Window& window,
                                      platform::Renderer& renderer, demo_client::DemoGame& game) {
    core::SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);
    lemuria::classic::ThreeQuarterCamera camera(demo_client::kCameraSettings);
    camera.focus_on(game.focus(), window.size());
    core::FrameClock clock;
    platform::InputFrame input;
    std::string_view shown_status;
    for (std::uint64_t frame = 0; !options.frame_limit || frame < *options.frame_limit; ++frame) {
        window.poll_input(input);
        if (input.quit_requested) {
            break;
        }
        handle_clicks(input, camera, game);
        game.frame(clock.advance(platform::monotonic_nanoseconds()));
        show_status(window, game.status(), shown_status);

        game.describe(scene);
        camera.focus_on(game.focus(), input.window_size);
        scene.view_projection = camera.view_projection();
        if (const auto drawn = renderer.draw(scene); !drawn) {
            return tl::unexpected(drawn.error());
        }
    }
    return {};
}

[[nodiscard]] int run_game(const Options& options) {
    auto window = platform::Window::open(platform::WindowOptions{std::string(kWindowTitle)});
    if (!window) {
        std::cerr << window.error().message << '\n';
        return 1;
    }

    auto renderer = platform::Renderer::for_window(*window, renderer_options_for(options));
    if (!renderer) {
        std::cerr << renderer.error().message << '\n';
        return 1;
    }

    platform::TcpConnections connections;
    demo_client::DemoAdapter adapter(connections, core::ConnectionAddress{"127.0.0.1", options.server_port});
    auto game = demo_client::DemoGame::create(adapter);
    if (!game) {
        std::cerr << game.error().message << '\n';
        return 1;
    }
    // A failure to connect ends the session, and the title of the window shows it. A session that could not
    // even start is a mistake in the values of the demonstration, which the window would hide.
    if (const auto started = (*game)->start();
        !started && (*game)->session().phase() != lemuria::classic::SessionPhase::Ended) {
        std::cerr << started.error().message << '\n';
        return 1;
    }

    const auto played = play(options, *window, *renderer, **game);
    (*game)->end();
    if (!played) {
        std::cerr << played.error().message << '\n';
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    const auto options = parse_options(std::span<char*>(argv, static_cast<std::size_t>(argc)));
    if (!options) {
        std::cerr << "Usage: demo_client [--version] [--frames count] [--proof-image file] "
                     "[--measure-frame-times] [--graphics-api name] [--port number]\n";
        return 2;
    }
    if (options->show_version) {
        print_versions();
        return 0;
    }
    if (options->measure_frame_times) {
        return report_frame_times(*options);
    }
    if (options->proof_image_path) {
        const auto saved = save_proof_image(*options, *options->proof_image_path);
        if (!saved) {
            std::cerr << saved.error().message << '\n';
            return 1;
        }
        return 0;
    }
    return run_game(*options);
}
