#include <cstddef>

#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Platform/Window.hpp"

using lemuria::platform::InputFrame;

TEST_CASE("clicks are kept in the order they happened") {
    InputFrame input;

    CHECK(input.add_click({10.0F, 20.0F}));
    CHECK(input.add_click({30.0F, 40.0F}));

    REQUIRE(input.clicks().size() == 2);
    CHECK(input.clicks()[0].x == 10.0F);
    CHECK(input.clicks()[1].y == 40.0F);
}

TEST_CASE("clicks beyond the limit of a frame are left out") {
    InputFrame input;
    for (std::size_t index = 0; index < InputFrame::kMaxClicks; ++index) {
        REQUIRE(input.add_click({1.0F, 1.0F}));
    }

    CHECK_FALSE(input.add_click({2.0F, 2.0F}));
    CHECK(input.clicks().size() == InputFrame::kMaxClicks);
}

TEST_CASE("a new frame forgets the clicks and the quit request of the previous one") {
    InputFrame input;
    static_cast<void>(input.add_click({1.0F, 1.0F}));
    input.quit_requested = true;

    input.begin_frame();

    CHECK(input.clicks().empty());
    CHECK_FALSE(input.quit_requested);
}
