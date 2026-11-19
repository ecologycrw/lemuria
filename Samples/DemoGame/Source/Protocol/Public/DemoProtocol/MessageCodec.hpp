#pragma once

#include <cstdint>

#include <tl/expected.hpp>

#include "DemoProtocol/Framing.hpp"
#include "DemoProtocol/Messages.hpp"

// Turns messages into frames and frames into messages. Numbers are written with the least significant byte
// first, and every field read is checked before use (INV-12).
namespace demo_protocol {

// The type of each message, as written in the first byte of its body.
namespace message_type {
inline constexpr std::uint8_t kHello = 1;
inline constexpr std::uint8_t kWalkRequest = 2;
inline constexpr std::uint8_t kMapLoaded = 3;
inline constexpr std::uint8_t kMapEntered = 11;
inline constexpr std::uint8_t kEntityAppeared = 12;
inline constexpr std::uint8_t kEntityWalked = 13;
inline constexpr std::uint8_t kWalkConfirmed = 14;
inline constexpr std::uint8_t kPositionCorrected = 15;
inline constexpr std::uint8_t kEntityLeft = 16;
} // namespace message_type

// Only this message is wrong. It is left out, and the next messages are still read.
enum class MessageError : std::uint8_t {
    UnknownType,

    // The fields are fewer or more than the type needs.
    WrongSize,

    // A field holds a value the message cannot have, such as a tile beyond `kMaxTileCoordinate`.
    InvalidValue,
};

[[nodiscard]] EncodedFrame encode(const ClientMessage& message) noexcept;
[[nodiscard]] EncodedFrame encode(const ServerMessage& message) noexcept;

// What the server reads. Messages meant for the client are of an unknown type here.
[[nodiscard]] tl::expected<ClientMessage, MessageError> read_client_message(const Frame& frame) noexcept;

// What the client reads. Messages meant for the server are of an unknown type here.
[[nodiscard]] tl::expected<ServerMessage, MessageError> read_server_message(const Frame& frame) noexcept;

} // namespace demo_protocol
