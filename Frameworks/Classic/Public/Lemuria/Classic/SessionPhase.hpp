#pragma once

#include <cstdint>

namespace lemuria::classic {

// The stages of a game session. Each adapter uses only the stages its server has.
enum class SessionPhase : std::uint8_t {
    Connecting,
    Authenticating,
    ChoosingCharacter,
    EnteringMap,
    InWorld,
    ChangingMap,
    Reconnecting,
    Ended,
};

} // namespace lemuria::classic
