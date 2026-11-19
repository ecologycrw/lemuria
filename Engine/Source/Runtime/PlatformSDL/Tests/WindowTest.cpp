#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Platform/Window.hpp"

using lemuria::platform::InputFrame;
using lemuria::platform::Window;
using lemuria::platform::WindowOptions;

TEST_CASE("a window opens with the requested size") {
    auto window = Window::open(WindowOptions{"Lemuria test", 320, 240, true});

    INFO((window ? "" : window.error().message));
    REQUIRE(window.has_value());
    CHECK(window->size().x == 320.0F);
    CHECK(window->size().y == 240.0F);
}

TEST_CASE("reading the input of a quiet window reports its size and no clicks") {
    auto window = Window::open(WindowOptions{"Lemuria test", 320, 240, true});
    REQUIRE(window.has_value());
    InputFrame input;

    window->poll_input(input);

    CHECK(input.clicks().empty());
    CHECK(input.window_size.x == 320.0F);
    CHECK(input.window_size.y == 240.0F);
}

TEST_CASE("the title of a window can change") {
    auto window = Window::open(WindowOptions{"Lemuria test", 320, 240, true});
    REQUIRE(window.has_value());

    const auto changed = window->set_title("Lemuria test: connected");

    INFO((changed ? "" : changed.error().message));
    CHECK(changed.has_value());
}
