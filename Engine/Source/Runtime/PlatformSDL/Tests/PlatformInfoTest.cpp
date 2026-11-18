#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Platform/PlatformInfo.hpp"

TEST_CASE("the platform layer runs on SDL 3") {
    CHECK(lemuria::platform::sdl_version().major == 3);
}
