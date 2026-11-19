#include "Lemuria/Classic/AdapterCapabilities.hpp"

#include <cmath>
#include <string>

#include "Lemuria/Classic/WalkSteps.hpp"

namespace lemuria::classic {

namespace {

[[nodiscard]] bool has_valid_step_limit(const AdapterCapabilities& capabilities) noexcept {
    return capabilities.walk_requests != WalkRequestMode::Steps ||
           (capabilities.max_steps_per_request > 0 &&
            capabilities.max_steps_per_request <= WalkSteps::kCapacity);
}

[[nodiscard]] bool has_valid_presence_period(const AdapterCapabilities& capabilities) noexcept {
    return capabilities.presence != PresenceSignal::ClientSendsPeriodically ||
           (std::isfinite(capabilities.presence_period_seconds) &&
            capabilities.presence_period_seconds > 0.0);
}

} // namespace

core::Result<void> check_capabilities(const AdapterCapabilities& capabilities) {
    if (capabilities.connection_kind.empty()) {
        return core::make_error("an adapter must declare the kind of connection its server uses");
    }
    if (!has_valid_step_limit(capabilities)) {
        return core::make_error("an adapter that sends steps must allow from 1 to " +
                                std::to_string(WalkSteps::kCapacity) + " steps per request");
    }
    if (!has_valid_presence_period(capabilities)) {
        return core::make_error("an adapter that sends a presence signal must declare a period above zero");
    }
    return {};
}

} // namespace lemuria::classic
