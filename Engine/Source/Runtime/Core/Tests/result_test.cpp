#include <catch2/catch_test_macros.hpp>

#include "lemuria/core/result.hpp"

using lemuria::core::make_error;
using lemuria::core::Result;

namespace {

Result<int> half_of(int value) {
    if (value % 2 != 0) {
        return make_error("the value is odd");
    }
    return value / 2;
}

} // namespace

TEST_CASE("a result carries the value when the operation succeeds") {
    const auto result = half_of(8);

    REQUIRE(result.has_value());
    CHECK(*result == 4);
}

TEST_CASE("a result carries the reason when the operation fails") {
    const auto result = half_of(7);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message == "the value is odd");
}
