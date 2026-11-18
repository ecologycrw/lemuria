#include <catch2/catch_test_macros.hpp>

#include "DemoProtocol/Protocol.hpp"

TEST_CASE("the demo protocol starts at version 1") {
    CHECK(demo_protocol::protocol_version() == 1);
}
