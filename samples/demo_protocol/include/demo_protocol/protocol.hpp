#pragma once

#include <cstdint>

namespace demo_protocol {

// The version of the messages exchanged by the demo client and the test server.
[[nodiscard]] std::uint16_t protocol_version() noexcept;

} // namespace demo_protocol
