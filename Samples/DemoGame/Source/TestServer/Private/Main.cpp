#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <thread>
#include <utility>

#include "DemoProtocol/DemoMap.hpp"
#include "DemoProtocol/Protocol.hpp"
#include "Lemuria/Platform/MonotonicClock.hpp"
#include "Lemuria/Platform/TcpListener.hpp"
#include "TestServer/ServerGrid.hpp"
#include "TestServer/ServerHost.hpp"

namespace {

namespace platform = lemuria::platform;

// How long the server rests between rounds. A message waits at most this long before it is handled.
constexpr std::chrono::milliseconds kRestBetweenRounds{2};

constexpr double kNanosecondsPerSecond = 1e9;

struct Options {
    bool show_version = false;
    std::uint16_t port = demo_protocol::kDefaultServerPort;
};

// Reads a port number from the command line, refusing anything else (INV-12).
[[nodiscard]] std::optional<std::uint16_t> parse_port(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return std::nullopt;
        }
        value = value * 10 + static_cast<std::uint32_t>(character - '0');
        if (value > std::numeric_limits<std::uint16_t>::max()) {
            return std::nullopt;
        }
    }
    if (value == 0) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(value);
}

[[nodiscard]] std::optional<Options> parse_options(std::span<char*> arguments) {
    Options options;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        const std::string_view argument = arguments[index];
        if (argument == "--version") {
            options.show_version = true;
        } else if (argument == "--port" && index + 1 < arguments.size()) {
            ++index;
            const std::optional<std::uint16_t> port = parse_port(arguments[index]);
            if (!port) {
                return std::nullopt;
            }
            options.port = *port;
        } else {
            return std::nullopt;
        }
    }
    return options;
}

void print_version() {
    std::cout << "Lemuria test server: protocol " << demo_protocol::protocol_version() << '\n';
}

[[nodiscard]] double seconds_since(std::uint64_t start) {
    return static_cast<double>(platform::monotonic_nanoseconds() - start) / kNanosecondsPerSecond;
}

// Serves until the window is closed, or until the server can no longer accept connections.
[[nodiscard]] int run_server(test_server::ServerHost& host) {
    // Flushed at once, so a program that reads this output knows the server is ready.
    std::cout << "Listening on 127.0.0.1:" << host.port()
              << ", for this computer only. Close this window to stop.\n"
              << std::flush;
    const std::uint64_t start = platform::monotonic_nanoseconds();
    std::size_t players = 0;
    while (true) {
        if (const auto served = host.serve(seconds_since(start)); !served) {
            std::cerr << "The server stopped: " << served.error().message << '\n';
            return 1;
        }
        if (host.world().player_count() != players) {
            players = host.world().player_count();
            std::cout << "Players on the map: " << players << '\n' << std::flush;
        }
        std::this_thread::sleep_for(kRestBetweenRounds);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    const auto options = parse_options(std::span(argv, static_cast<std::size_t>(argc)));
    if (!options) {
        std::cerr << "Usage: test_server [--version] [--port <1-65535>]\n";
        return 2;
    }
    print_version();
    if (options->show_version) {
        return 0;
    }

    auto listener = platform::TcpListener::open_on_this_computer(options->port);
    if (!listener) {
        std::cerr << "Could not listen on port " << options->port << ": " << listener.error().message << '\n';
        return 1;
    }
    auto host = test_server::ServerHost::create(
        std::move(*listener), test_server::ServerGrid::from_demo_map(demo_protocol::DemoMap{}),
        test_server::demo_server_config(), test_server::demo_host_limits());
    if (!host) {
        std::cerr << host.error().message << '\n';
        return 1;
    }
    return run_server(*host);
}
