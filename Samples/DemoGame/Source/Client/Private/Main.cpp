#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "DemoClient/DemoStyle.hpp"
#include "DemoClient/GroundScene.hpp"
#include "DemoClient/ProofScene.hpp"
#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Protocol.hpp"
#include "Lemuria/Classic/FrameworkInfo.hpp"
#include "Lemuria/Classic/ThreeQuarterCamera.hpp"
#include "Lemuria/Classic/TileCoord.hpp"
#include "Lemuria/Core/SceneDescription.hpp"
#include "Lemuria/Core/Version.hpp"
#include "Lemuria/Platform/PlatformInfo.hpp"
#include "Lemuria/Platform/Renderer.hpp"
#include "Lemuria/Platform/Window.hpp"

namespace {

namespace core = lemuria::core;
namespace platform = lemuria::platform;

struct Options {
    bool show_version = false;
    std::optional<std::uint64_t> frame_limit;
    std::optional<std::string> proof_image_path;
    std::string graphics_api;
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
        } else if (argument == "--frames" && index + 1 < arguments.size()) {
            ++index;
            options.frame_limit = parse_count(arguments[index]);
            if (!options.frame_limit) {
                return std::nullopt;
            }
        } else if (argument == "--proof-image" && index + 1 < arguments.size()) {
            ++index;
            options.proof_image_path = arguments[index];
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
    auto renderer = platform::Renderer::offscreen(
        demo_client::kProofImageWidth, demo_client::kProofImageHeight, renderer_options_for(options));
    if (!renderer) {
        return tl::unexpected(renderer.error());
    }

    const demo_protocol::DemoMap map;
    core::SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);
    demo_client::fill_proof_scene(map, scene);

    auto drawn = renderer->draw(scene);
    if (!drawn) {
        return drawn;
    }
    const auto image = renderer->read_image();
    if (!image) {
        return tl::unexpected(image.error());
    }
    std::cout << "Graphics API: " << renderer->graphics_api() << '\n';
    return platform::save_png(*image, path);
}

[[nodiscard]] int run_game(const Options& options) {
    auto window = platform::Window::open(platform::WindowOptions{"Lemuria Demo"});
    if (!window) {
        std::cerr << window.error().message << '\n';
        return 1;
    }

    auto renderer = platform::Renderer::for_window(*window, renderer_options_for(options));
    if (!renderer) {
        std::cerr << renderer.error().message << '\n';
        return 1;
    }

    const demo_protocol::DemoMap map;
    core::SceneDescription scene(demo_client::kGroundTileBudget, demo_client::kBoxBudget);
    scene.background = core::color_from_rgb(demo_client::kDeepShadowRgb);
    demo_client::add_ground(map, scene);
    const auto start =
        lemuria::classic::tile_center({demo_protocol::DemoMap::kSize / 2, demo_protocol::DemoMap::kSize / 2});
    demo_client::add_character(start, demo_client::kOwnCharacterRgb, scene);

    lemuria::classic::ThreeQuarterCamera camera(demo_client::kCameraSettings);
    platform::InputFrame input;
    std::uint64_t frame = 0;
    while (!options.frame_limit || frame < *options.frame_limit) {
        window->poll_input(input);
        if (input.quit_requested) {
            break;
        }
        camera.focus_on(start, input.window_size);
        scene.view_projection = camera.view_projection();

        const auto drawn = renderer->draw(scene);
        if (!drawn) {
            std::cerr << drawn.error().message << '\n';
            return 1;
        }
        ++frame;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    const auto options = parse_options(std::span<char*>(argv, static_cast<std::size_t>(argc)));
    if (!options) {
        std::cerr
            << "Usage: demo_client [--version] [--frames count] [--proof-image file] [--graphics-api name]\n";
        return 2;
    }
    if (options->show_version) {
        print_versions();
        return 0;
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
