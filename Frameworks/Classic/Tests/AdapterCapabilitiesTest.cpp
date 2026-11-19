#include <cstddef>
#include <limits>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "Lemuria/Classic/AdapterCapabilities.hpp"
#include "Lemuria/Classic/WalkSteps.hpp"

using Catch::Matchers::ContainsSubstring;
using lemuria::classic::AdapterCapabilities;
using lemuria::classic::check_capabilities;
using lemuria::classic::PresenceSignal;
using lemuria::classic::WalkRequestMode;
using lemuria::classic::WalkSteps;

namespace {

constexpr AdapterCapabilities kValid{
    .connection_kind = "tcp",
    .walk_requests = WalkRequestMode::Steps,
    .max_steps_per_request = 8,
};

} // namespace

TEST_CASE("adapter capabilities accept a complete description") {
    CHECK(check_capabilities(kValid).has_value());
}

TEST_CASE("adapter capabilities need the kind of connection") {
    AdapterCapabilities capabilities = kValid;
    capabilities.connection_kind = {};

    const auto checked = check_capabilities(capabilities);

    REQUIRE_FALSE(checked.has_value());
    CHECK_THAT(checked.error().message, ContainsSubstring("kind of connection"));
}

TEST_CASE("adapter capabilities accept step limits the framework can carry") {
    AdapterCapabilities capabilities = kValid;
    capabilities.max_steps_per_request = GENERATE(std::size_t{1}, WalkSteps::kCapacity);

    CHECK(check_capabilities(capabilities).has_value());
}

TEST_CASE("adapter capabilities refuse step limits the framework cannot carry") {
    AdapterCapabilities capabilities = kValid;
    capabilities.max_steps_per_request = GENERATE(std::size_t{0}, WalkSteps::kCapacity + 1);

    const auto checked = check_capabilities(capabilities);

    REQUIRE_FALSE(checked.has_value());
    CHECK_THAT(checked.error().message, ContainsSubstring("steps per request"));
}

TEST_CASE("adapter capabilities do not need a step limit when only the destination is sent") {
    AdapterCapabilities capabilities = kValid;
    capabilities.walk_requests = WalkRequestMode::DestinationOnly;
    capabilities.max_steps_per_request = GENERATE(std::size_t{0}, WalkSteps::kCapacity + 1);

    CHECK(check_capabilities(capabilities).has_value());
}

TEST_CASE("adapter capabilities accept a presence signal with a period") {
    AdapterCapabilities capabilities = kValid;
    capabilities.presence = PresenceSignal::ClientSendsPeriodically;
    capabilities.presence_period_seconds = 20.0;

    CHECK(check_capabilities(capabilities).has_value());
}

TEST_CASE("adapter capabilities refuse a presence signal without a usable period") {
    AdapterCapabilities capabilities = kValid;
    capabilities.presence = PresenceSignal::ClientSendsPeriodically;
    capabilities.presence_period_seconds = GENERATE(0.0, -1.0, std::numeric_limits<double>::infinity(),
                                                    std::numeric_limits<double>::quiet_NaN());

    const auto checked = check_capabilities(capabilities);

    REQUIRE_FALSE(checked.has_value());
    CHECK_THAT(checked.error().message, ContainsSubstring("presence signal"));
}

TEST_CASE("adapter capabilities need a period only when the client sends the signal") {
    AdapterCapabilities capabilities = kValid;
    capabilities.presence = GENERATE(PresenceSignal::None, PresenceSignal::ServerAsks);

    CHECK(check_capabilities(capabilities).has_value());
}
