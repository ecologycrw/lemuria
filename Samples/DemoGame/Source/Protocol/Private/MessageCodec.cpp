#include "DemoProtocol/MessageCodec.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <variant>

namespace demo_protocol {

namespace {

// The largest message is a walk of someone else: type, entity, origin, step count and the steps.
constexpr std::size_t kLargestBody = 1 + 4 + 4 + 1 + (4 * kMaxWalkSteps);
static_assert(kLargestBody <= kMaxBodyBytes, "every message must fit in one frame");

class FieldWriter {
public:
    explicit FieldWriter(std::uint8_t type) noexcept : frame_(type) {}

    void u8(std::uint8_t value) noexcept { put(std::array{static_cast<std::byte>(value)}); }

    void u16(std::uint16_t value) noexcept {
        put(std::array{static_cast<std::byte>(value & 0xFFU), static_cast<std::byte>(value >> 8U)});
    }

    void u32(std::uint32_t value) noexcept {
        put(std::array{static_cast<std::byte>(value & 0xFFU), static_cast<std::byte>((value >> 8U) & 0xFFU),
                       static_cast<std::byte>((value >> 16U) & 0xFFU), static_cast<std::byte>(value >> 24U)});
    }

    void tile(Tile value) noexcept {
        u16(value.x);
        u16(value.z);
    }

    void steps(const StepList& list) noexcept {
        u8(static_cast<std::uint8_t>(list.size()));
        for (const Tile step : list.view()) {
            tile(step);
        }
    }

    [[nodiscard]] const EncodedFrame& frame() const noexcept { return frame_; }

private:
    // Every message fits in a frame, as checked by `kLargestBody`.
    void put(std::span<const std::byte> bytes) noexcept { static_cast<void>(frame_.append(bytes)); }

    EncodedFrame frame_;
};

// Reads the fields in order. Reading past the end gives zeros and makes the size wrong, and a value the
// message cannot have makes it invalid, so each message checks everything once, at the end.
class FieldReader {
public:
    explicit FieldReader(std::span<const std::byte> fields) noexcept : fields_(fields) {}

    [[nodiscard]] std::uint8_t u8() noexcept {
        const auto bytes = take(1);
        return bytes.empty() ? std::uint8_t{0} : std::to_integer<std::uint8_t>(bytes[0]);
    }

    [[nodiscard]] std::uint16_t u16() noexcept {
        const auto bytes = take(2);
        if (bytes.empty()) {
            return 0;
        }
        return static_cast<std::uint16_t>(std::to_integer<unsigned>(bytes[0]) |
                                          (std::to_integer<unsigned>(bytes[1]) << 8U));
    }

    [[nodiscard]] std::uint32_t u32() noexcept {
        const auto bytes = take(4);
        std::uint32_t value = 0;
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            value |= std::to_integer<std::uint32_t>(bytes[index]) << (8U * index);
        }
        return value;
    }

    [[nodiscard]] Tile tile() noexcept {
        const Tile value{u16(), u16()};
        refuse_if(value.x > kMaxTileCoordinate || value.z > kMaxTileCoordinate);
        return value;
    }

    [[nodiscard]] std::uint32_t entity() noexcept {
        const std::uint32_t value = u32();
        refuse_if(value == 0);
        return value;
    }

    [[nodiscard]] bool flag() noexcept {
        const std::uint8_t value = u8();
        refuse_if(value > 1);
        return value == 1;
    }

    [[nodiscard]] LeaveReason leave_reason() noexcept {
        const std::uint8_t value = u8();
        refuse_if(value > 1);
        return value == 1 ? LeaveReason::Disconnected : LeaveReason::LeftView;
    }

    // With a count out of the limits, the steps are not read at all.
    [[nodiscard]] StepList steps(std::size_t min_count) noexcept {
        const std::size_t count = u8();
        StepList list;
        if (count < min_count || count > kMaxWalkSteps) {
            refuse_if(true);
            offset_ = fields_.size();
            return list;
        }
        for (std::size_t index = 0; index < count; ++index) {
            static_cast<void>(list.add(tile()));
        }
        return list;
    }

    template <typename Result>
    [[nodiscard]] tl::expected<Result, MessageError> finish(const auto& message) const noexcept {
        if (ran_past_end_ || offset_ != fields_.size()) {
            return tl::unexpected(MessageError::WrongSize);
        }
        if (has_invalid_value_) {
            return tl::unexpected(MessageError::InvalidValue);
        }
        return Result{message};
    }

private:
    [[nodiscard]] std::span<const std::byte> take(std::size_t count) noexcept {
        if (count > fields_.size() - offset_) {
            ran_past_end_ = true;
            offset_ = fields_.size();
            return {};
        }
        const auto bytes = fields_.subspan(offset_, count);
        offset_ += count;
        return bytes;
    }

    void refuse_if(bool is_invalid) noexcept { has_invalid_value_ = has_invalid_value_ || is_invalid; }

    std::span<const std::byte> fields_;
    std::size_t offset_ = 0;
    bool ran_past_end_ = false;
    bool has_invalid_value_ = false;
};

[[nodiscard]] EncodedFrame encode_one(const Hello& message) noexcept {
    FieldWriter fields(message_type::kHello);
    fields.u16(message.protocol_version);
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const WalkRequest& message) noexcept {
    FieldWriter fields(message_type::kWalkRequest);
    fields.tile(message.origin);
    fields.steps(message.steps);
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const MapLoaded& message) noexcept {
    FieldWriter fields(message_type::kMapLoaded);
    fields.u32(message.map);
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const MapEntered& message) noexcept {
    FieldWriter fields(message_type::kMapEntered);
    fields.u32(message.map);
    fields.u32(message.own_entity);
    fields.tile(message.position);
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const EntityAppeared& message) noexcept {
    FieldWriter fields(message_type::kEntityAppeared);
    fields.u32(message.entity);
    fields.tile(message.position);
    fields.u8(message.walking_to ? std::uint8_t{1} : std::uint8_t{0});
    if (message.walking_to) {
        fields.tile(*message.walking_to);
    }
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const EntityWalked& message) noexcept {
    FieldWriter fields(message_type::kEntityWalked);
    fields.u32(message.entity);
    fields.tile(message.origin);
    fields.steps(message.steps);
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const WalkConfirmed& message) noexcept {
    FieldWriter fields(message_type::kWalkConfirmed);
    fields.tile(message.origin);
    fields.tile(message.destination);
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const PositionCorrected& message) noexcept {
    FieldWriter fields(message_type::kPositionCorrected);
    fields.u32(message.entity);
    fields.tile(message.position);
    return fields.frame();
}

[[nodiscard]] EncodedFrame encode_one(const EntityLeft& message) noexcept {
    FieldWriter fields(message_type::kEntityLeft);
    fields.u32(message.entity);
    fields.u8(static_cast<std::uint8_t>(message.reason));
    return fields.frame();
}

using ClientResult = tl::expected<ClientMessage, MessageError>;
using ServerResult = tl::expected<ServerMessage, MessageError>;

[[nodiscard]] ClientResult read_hello(FieldReader& fields) noexcept {
    const Hello message{fields.u16()};
    return fields.finish<ClientMessage>(message);
}

// A walk request may have no step: the player wants to stop where the walk starts.
[[nodiscard]] ClientResult read_walk_request(FieldReader& fields) noexcept {
    const WalkRequest message{fields.tile(), fields.steps(0)};
    return fields.finish<ClientMessage>(message);
}

[[nodiscard]] ClientResult read_map_loaded(FieldReader& fields) noexcept {
    const MapLoaded message{fields.u32()};
    return fields.finish<ClientMessage>(message);
}

[[nodiscard]] ServerResult read_map_entered(FieldReader& fields) noexcept {
    const MapEntered message{fields.u32(), fields.entity(), fields.tile()};
    return fields.finish<ServerMessage>(message);
}

[[nodiscard]] ServerResult read_entity_appeared(FieldReader& fields) noexcept {
    EntityAppeared message{fields.entity(), fields.tile(), std::nullopt};
    if (fields.flag()) {
        message.walking_to = fields.tile();
    }
    return fields.finish<ServerMessage>(message);
}

[[nodiscard]] ServerResult read_entity_walked(FieldReader& fields) noexcept {
    const EntityWalked message{fields.entity(), fields.tile(), fields.steps(1)};
    return fields.finish<ServerMessage>(message);
}

[[nodiscard]] ServerResult read_walk_confirmed(FieldReader& fields) noexcept {
    const WalkConfirmed message{fields.tile(), fields.tile()};
    return fields.finish<ServerMessage>(message);
}

[[nodiscard]] ServerResult read_position_corrected(FieldReader& fields) noexcept {
    const PositionCorrected message{fields.entity(), fields.tile()};
    return fields.finish<ServerMessage>(message);
}

[[nodiscard]] ServerResult read_entity_left(FieldReader& fields) noexcept {
    const EntityLeft message{fields.entity(), fields.leave_reason()};
    return fields.finish<ServerMessage>(message);
}

} // namespace

EncodedFrame encode(const ClientMessage& message) noexcept {
    return std::visit([](const auto& value) { return encode_one(value); }, message);
}

EncodedFrame encode(const ServerMessage& message) noexcept {
    return std::visit([](const auto& value) { return encode_one(value); }, message);
}

tl::expected<ClientMessage, MessageError> read_client_message(const Frame& frame) noexcept {
    FieldReader fields(frame.fields);
    switch (frame.type) {
    case message_type::kHello:
        return read_hello(fields);
    case message_type::kWalkRequest:
        return read_walk_request(fields);
    case message_type::kMapLoaded:
        return read_map_loaded(fields);
    default:
        break;
    }
    return tl::unexpected(MessageError::UnknownType);
}

tl::expected<ServerMessage, MessageError> read_server_message(const Frame& frame) noexcept {
    FieldReader fields(frame.fields);
    switch (frame.type) {
    case message_type::kMapEntered:
        return read_map_entered(fields);
    case message_type::kEntityAppeared:
        return read_entity_appeared(fields);
    case message_type::kEntityWalked:
        return read_entity_walked(fields);
    case message_type::kWalkConfirmed:
        return read_walk_confirmed(fields);
    case message_type::kPositionCorrected:
        return read_position_corrected(fields);
    case message_type::kEntityLeft:
        return read_entity_left(fields);
    default:
        break;
    }
    return tl::unexpected(MessageError::UnknownType);
}

} // namespace demo_protocol
