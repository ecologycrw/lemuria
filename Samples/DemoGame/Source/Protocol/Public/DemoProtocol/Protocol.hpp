#pragma once

#include <cstdint>

namespace demo_protocol {

// The version of the messages exchanged by the demo client and the test server.
[[nodiscard]] std::uint16_t protocol_version() noexcept;

// Where the test server listens and the demo client connects, unless told otherwise. It sits below the ports
// the tests use.
inline constexpr std::uint16_t kDefaultServerPort = 24'000;

} // namespace demo_protocol
