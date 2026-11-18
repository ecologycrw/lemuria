#include <catch2/catch_test_macros.hpp>

#include "custom_framework/custom_framework.hpp"

TEST_CASE("a custom framework works with the Core alone") {
    const auto required = custom_framework::read_required_engine("0.0.0");

    REQUIRE(required.has_value());
    CHECK(*required <= lemuria::core::engine_version());
}
