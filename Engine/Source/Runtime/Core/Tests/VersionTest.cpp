#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lemuria/Core/Version.hpp"

using lemuria::core::engine_version;
using lemuria::core::parse_version;
using lemuria::core::Version;

TEST_CASE("the engine reports the version it was built with") {
    CHECK(engine_version() == Version{0, 0, 0});
}

TEST_CASE("a version is read from its three numbers") {
    const auto version = parse_version("1.20.300");

    REQUIRE(version.has_value());
    CHECK(*version == Version{1, 20, 300});
}

TEST_CASE("versions are ordered by major, then minor, then patch") {
    CHECK(Version{1, 0, 0} > Version{0, 9, 9});
    CHECK(Version{1, 2, 0} > Version{1, 1, 9});
    CHECK(Version{1, 1, 2} > Version{1, 1, 1});
}

TEST_CASE("invalid version text is refused with a reason") {
    const std::string_view text = GENERATE("", "1", "1.2", "1.2.", ".1.2", "1..2", "a.b.c", "1.2.3.4",
                                           "1.2.3 ", " 1.2.3", "-1.2.3", "1.2.99999999999");

    const auto version = parse_version(text);

    REQUIRE_FALSE(version.has_value());
    CHECK_FALSE(version.error().message.empty());
}
