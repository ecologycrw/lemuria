#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "lemuria/platform/platform_info.hpp"

TEST_CASE("the platform layer opens the GPU without a window") {
    const auto driver = lemuria::platform::probe_gpu_driver();

    INFO((driver ? *driver : driver.error().message));
    REQUIRE(driver.has_value());
    CHECK_FALSE(driver->empty());
}
