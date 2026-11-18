#include <catch2/catch_test_macros.hpp>

#include "Lemuria/Classic/FrameworkInfo.hpp"

TEST_CASE("the framework reports its name") {
    CHECK(lemuria::classic::framework_name() == "Classic");
}
