#include <catch2/catch_test_macros.hpp>

#include "lemuria/classic/framework_info.hpp"

TEST_CASE("the framework reports its name") {
    CHECK(lemuria::classic::framework_name() == "Classic");
}
