#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "DemoProtocol/Framing.hpp"
#include "DemoProtocol/MessageCodec.hpp"
#include "DemoProtocol/Messages.hpp"
#include "Lemuria/Core/SeededRandom.hpp"

using demo_protocol::ClientMessage;
using demo_protocol::encode;
using demo_protocol::EntityAppeared;
using demo_protocol::EntityLeft;
using demo_protocol::EntityWalked;
using demo_protocol::Frame;
using demo_protocol::FrameReader;
using demo_protocol::FrameSink;
using demo_protocol::Hello;
using demo_protocol::kMaxTileCoordinate;
using demo_protocol::kMaxWalkSteps;
using demo_protocol::LeaveReason;
using demo_protocol::MapEntered;
using demo_protocol::MapLoaded;
using demo_protocol::MessageError;
using demo_protocol::PositionCorrected;
using demo_protocol::read_client_message;
using demo_protocol::read_server_message;
using demo_protocol::ServerMessage;
using demo_protocol::StepList;
using demo_protocol::Tile;
using demo_protocol::tile_from;
using demo_protocol::WalkConfirmed;
using demo_protocol::WalkRequest;
using lemuria::core::SeededRandom;
namespace message_type = demo_protocol::message_type;

namespace {

using Bytes = std::vector<std::byte>;

[[nodiscard]] Bytes bytes_of(std::span<const int> values) {
    Bytes bytes;
    for (const int value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

[[nodiscard]] Bytes bytes_of(std::initializer_list<int> values) {
    return bytes_of(std::span<const int>(values.begin(), values.size()));
}

[[nodiscard]] Bytes bytes_of(std::span<const std::byte> bytes) {
    return {bytes.begin(), bytes.end()};
}

[[nodiscard]] StepList steps_of(std::initializer_list<Tile> tiles) {
    StepList steps;
    for (const Tile tile : tiles) {
        REQUIRE(steps.add(tile));
    }
    return steps;
}

// Each message next to its bytes, which are fixed so the format cannot change by accident.
[[nodiscard]] std::vector<std::pair<ClientMessage, Bytes>> client_samples() {
    return {
        {Hello{1}, bytes_of({3, 0, 1, 1, 0})},
        {WalkRequest{{10, 20}, steps_of({{11, 20}, {12, 20}})},
         bytes_of({14, 0, 2, 10, 0, 20, 0, 2, 11, 0, 20, 0, 12, 0, 20, 0})},
        {WalkRequest{{10, 20}, {}}, bytes_of({6, 0, 2, 10, 0, 20, 0, 0})},
        {MapLoaded{0x01020304}, bytes_of({5, 0, 3, 4, 3, 2, 1})},
    };
}

[[nodiscard]] std::vector<std::pair<ServerMessage, Bytes>> server_samples() {
    return {
        {MapEntered{7, 256, {300, 2}}, bytes_of({13, 0, 11, 7, 0, 0, 0, 0, 1, 0, 0, 44, 1, 2, 0})},
        {EntityAppeared{5, {1, 2}, Tile{3, 4}}, bytes_of({14, 0, 12, 5, 0, 0, 0, 1, 0, 2, 0, 1, 3, 0, 4, 0})},
        {EntityAppeared{5, {1, 2}, std::nullopt}, bytes_of({10, 0, 12, 5, 0, 0, 0, 1, 0, 2, 0, 0})},
        {EntityWalked{5, {1, 1}, steps_of({{2, 1}})},
         bytes_of({14, 0, 13, 5, 0, 0, 0, 1, 0, 1, 0, 1, 2, 0, 1, 0})},
        {WalkConfirmed{{1, 2}, {3, 4}}, bytes_of({9, 0, 14, 1, 0, 2, 0, 3, 0, 4, 0})},
        {PositionCorrected{5, {6, 7}}, bytes_of({9, 0, 15, 5, 0, 0, 0, 6, 0, 7, 0})},
        {EntityLeft{5, LeaveReason::Disconnected}, bytes_of({6, 0, 16, 5, 0, 0, 0, 1})},
        {EntityLeft{0x0A0B0C0D, LeaveReason::LeftView}, bytes_of({6, 0, 16, 13, 12, 11, 10, 0})},
    };
}

// The frame inside encoded bytes, which must hold exactly one frame.
class SingleFrame final : public FrameSink {
public:
    explicit SingleFrame(std::span<const std::byte> bytes) {
        FrameReader reader;
        REQUIRE(reader.read(bytes, *this).has_value());
        REQUIRE(count_ == 1);
        REQUIRE(reader.pending_bytes() == 0);
    }

    [[nodiscard]] Frame frame() const { return Frame{type_, fields_}; }

private:
    void on_frame(const Frame& frame) override {
        ++count_;
        type_ = frame.type;
        fields_ = bytes_of(frame.fields);
    }

    int count_ = 0;
    std::uint8_t type_ = 0;
    Bytes fields_;
};

// Reads the fields of a message written by hand, the way the other side would.
[[nodiscard]] auto read_client(std::uint8_t type, std::initializer_list<int> fields) {
    const Bytes bytes = bytes_of(fields);
    return read_client_message(Frame{type, bytes});
}

[[nodiscard]] auto read_server(std::uint8_t type, const std::vector<int>& fields) {
    const Bytes bytes = bytes_of(std::span<const int>(fields));
    return read_server_message(Frame{type, bytes});
}

// The bytes of a frame with its body replaced, keeping the header right.
[[nodiscard]] Bytes with_body(std::uint8_t type, const Bytes& fields) {
    Bytes bytes = bytes_of(
        {static_cast<int>((fields.size() + 1) & 0xFFU), static_cast<int>((fields.size() + 1) >> 8U), type});
    bytes.insert(bytes.end(), fields.begin(), fields.end());
    return bytes;
}

[[nodiscard]] bool is_client_type(std::uint8_t type) {
    return type == message_type::kHello || type == message_type::kWalkRequest ||
           type == message_type::kMapLoaded;
}

// Reads a body with the reader of its side. When the body is accepted, it must be written back with exactly
// the same bytes: the format has only one way to write each message.
[[nodiscard]] bool read_and_compare(std::uint8_t type, const Bytes& fields) {
    const Frame frame{type, fields};
    if (is_client_type(type)) {
        const auto message = read_client_message(frame);
        if (message) {
            CHECK(bytes_of(encode(*message).bytes()) == with_body(type, fields));
        }
        return message.has_value();
    }
    const auto message = read_server_message(frame);
    if (message) {
        CHECK(bytes_of(encode(*message).bytes()) == with_body(type, fields));
    }
    return message.has_value();
}

} // namespace

TEST_CASE("each client message is written with fixed bytes", "[0000-CA-10]") {
    for (const auto& [message, bytes] : client_samples()) {
        CHECK(bytes_of(encode(message).bytes()) == bytes);
    }
}

TEST_CASE("each server message is written with fixed bytes", "[0000-CA-10]") {
    for (const auto& [message, bytes] : server_samples()) {
        CHECK(bytes_of(encode(message).bytes()) == bytes);
    }
}

TEST_CASE("each client message is read back as it was written", "[0000-CA-10]") {
    for (const auto& [message, bytes] : client_samples()) {
        const SingleFrame frame(bytes);

        const auto read = read_client_message(frame.frame());

        REQUIRE(read.has_value());
        CHECK(*read == message);
    }
}

TEST_CASE("each server message is read back as it was written", "[0000-CA-10]") {
    for (const auto& [message, bytes] : server_samples()) {
        const SingleFrame frame(bytes);

        const auto read = read_server_message(frame.frame());

        REQUIRE(read.has_value());
        CHECK(*read == message);
    }
}

TEST_CASE("a walk carries up to the step limit, on both sides", "[0000-CA-10]") {
    StepList steps;
    for (std::size_t index = 0; index < kMaxWalkSteps; ++index) {
        REQUIRE(steps.add(Tile{static_cast<std::uint16_t>(index), kMaxTileCoordinate}));
    }
    CHECK_FALSE(steps.add(Tile{}));
    const WalkRequest request{{kMaxTileCoordinate, 0}, steps};
    const EntityWalked walked{0xFFFFFFFF, {0, kMaxTileCoordinate}, steps};

    const auto request_read = read_client_message(SingleFrame(encode(request).bytes()).frame());
    const auto walked_read = read_server_message(SingleFrame(encode(walked).bytes()).frame());

    REQUIRE(request_read.has_value());
    CHECK(*request_read == ClientMessage{request});
    REQUIRE(walked_read.has_value());
    CHECK(*walked_read == ServerMessage{walked});
}

TEST_CASE("a message of an unknown type is refused", "[0000-CA-11]") {
    SECTION("by the server") {
        const std::uint8_t type =
            GENERATE(as<std::uint8_t>{}, 0, 4, message_type::kMapEntered, message_type::kEntityLeft, 255);
        const auto read = read_client_message(Frame{type, {}});
        REQUIRE_FALSE(read.has_value());
        CHECK(read.error() == MessageError::UnknownType);
    }
    SECTION("by the client") {
        const std::uint8_t type = GENERATE(as<std::uint8_t>{}, 0, message_type::kHello,
                                           message_type::kWalkRequest, message_type::kMapLoaded, 17, 255);
        const auto read = read_server_message(Frame{type, {}});
        REQUIRE_FALSE(read.has_value());
        CHECK(read.error() == MessageError::UnknownType);
    }
}

TEST_CASE("a message with fields missing or left over is refused", "[0000-CA-11]") {
    for (const auto& [message, bytes] : client_samples()) {
        const SingleFrame single(bytes);
        const Frame frame = single.frame();
        Bytes longer = bytes_of(frame.fields);
        longer.push_back(std::byte{0});

        const auto shorter_read =
            read_client_message(Frame{frame.type, frame.fields.first(frame.fields.size() - 1)});
        const auto longer_read = read_client_message(Frame{frame.type, longer});

        REQUIRE_FALSE(shorter_read.has_value());
        CHECK(shorter_read.error() == MessageError::WrongSize);
        REQUIRE_FALSE(longer_read.has_value());
        CHECK(longer_read.error() == MessageError::WrongSize);
    }
    for (const auto& [message, bytes] : server_samples()) {
        const SingleFrame single(bytes);
        const Frame frame = single.frame();
        Bytes longer = bytes_of(frame.fields);
        longer.push_back(std::byte{0});

        const auto shorter_read =
            read_server_message(Frame{frame.type, frame.fields.first(frame.fields.size() - 1)});
        const auto longer_read = read_server_message(Frame{frame.type, longer});

        REQUIRE_FALSE(shorter_read.has_value());
        CHECK(shorter_read.error() == MessageError::WrongSize);
        REQUIRE_FALSE(longer_read.has_value());
        CHECK(longer_read.error() == MessageError::WrongSize);
    }
}

TEST_CASE("a message without any field is refused", "[0000-CA-11]") {
    for (const std::uint8_t type :
         {message_type::kHello, message_type::kWalkRequest, message_type::kMapLoaded}) {
        const auto read = read_client_message(Frame{type, {}});
        REQUIRE_FALSE(read.has_value());
        CHECK(read.error() == MessageError::WrongSize);
    }
    for (const std::uint8_t type :
         {message_type::kMapEntered, message_type::kEntityAppeared, message_type::kEntityWalked,
          message_type::kWalkConfirmed, message_type::kPositionCorrected, message_type::kEntityLeft}) {
        const auto read = read_server_message(Frame{type, {}});
        REQUIRE_FALSE(read.has_value());
        CHECK(read.error() == MessageError::WrongSize);
    }
}

TEST_CASE("a client message with a value it cannot have is refused", "[0000-CA-11]") {
    SECTION("a tile beyond the limit") {
        const auto read = read_client(message_type::kWalkRequest, {0, 4, 0, 0, 0});
        REQUIRE_FALSE(read.has_value());
        CHECK(read.error() == MessageError::InvalidValue);
    }
    SECTION("a step beyond the limit") {
        const auto read = read_client(message_type::kWalkRequest, {0, 0, 0, 0, 1, 0, 0, 0, 4});
        REQUIRE_FALSE(read.has_value());
        CHECK(read.error() == MessageError::InvalidValue);
    }
    SECTION("more steps than the limit, even with all their bytes") {
        Bytes fields = bytes_of({0, 0, 0, 0, static_cast<int>(kMaxWalkSteps) + 1});
        fields.resize(fields.size() + (4 * (kMaxWalkSteps + 1)), std::byte{0});
        const auto read = read_client_message(Frame{message_type::kWalkRequest, fields});
        REQUIRE_FALSE(read.has_value());
        CHECK(read.error() == MessageError::InvalidValue);
    }
}

TEST_CASE("a server message with a value it cannot have is refused", "[0000-CA-11]") {
    const auto [type, fields] = GENERATE(table<std::uint8_t, std::vector<int>>({
        // An entity numbered 0.
        {message_type::kMapEntered, {7, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0}},
        {message_type::kEntityAppeared, {0, 0, 0, 0, 1, 0, 1, 0, 0}},
        {message_type::kEntityWalked, {0, 0, 0, 0, 1, 0, 1, 0, 1, 2, 0, 1, 0}},
        {message_type::kPositionCorrected, {0, 0, 0, 0, 1, 0, 1, 0}},
        {message_type::kEntityLeft, {0, 0, 0, 0, 0}},
        // A tile beyond the limit.
        {message_type::kMapEntered, {7, 0, 0, 0, 1, 0, 0, 0, 0, 4, 1, 0}},
        {message_type::kEntityAppeared, {5, 0, 0, 0, 1, 0, 1, 0, 1, 0, 4, 0, 0}},
        {message_type::kEntityWalked, {5, 0, 0, 0, 1, 0, 1, 0, 1, 2, 0, 0, 4}},
        {message_type::kWalkConfirmed, {1, 0, 2, 0, 3, 0, 255, 255}},
        {message_type::kPositionCorrected, {5, 0, 0, 0, 0, 4, 1, 0}},
        // A walk without steps, or with more than the limit.
        {message_type::kEntityWalked, {5, 0, 0, 0, 1, 0, 1, 0, 0}},
        {message_type::kEntityWalked, {5, 0, 0, 0, 1, 0, 1, 0, 17}},
        // A choice that does not exist.
        {message_type::kEntityAppeared, {5, 0, 0, 0, 1, 0, 1, 0, 2}},
        {message_type::kEntityLeft, {5, 0, 0, 0, 2}},
    }));

    const auto read = read_server(type, fields);

    REQUIRE_FALSE(read.has_value());
    CHECK(read.error() == MessageError::InvalidValue);
}

TEST_CASE("only tiles inside the limits become message tiles") {
    CHECK(tile_from(0, 0) == Tile{0, 0});
    CHECK(tile_from(kMaxTileCoordinate, kMaxTileCoordinate) == Tile{kMaxTileCoordinate, kMaxTileCoordinate});
    CHECK_FALSE(tile_from(-1, 0).has_value());
    CHECK_FALSE(tile_from(0, -1).has_value());
    CHECK_FALSE(tile_from(kMaxTileCoordinate + 1, 0).has_value());
    CHECK_FALSE(tile_from(0, kMaxTileCoordinate + 1).has_value());
}

TEST_CASE("damaged messages are either refused or read as exactly what they say", "[0000-CA-11]") {
    constexpr int kRounds = 20'000;
    SeededRandom random(20261119);
    std::vector<std::pair<std::uint8_t, Bytes>> samples;
    for (const auto& [message, bytes] : client_samples()) {
        const SingleFrame single(bytes);
        samples.emplace_back(single.frame().type, bytes_of(single.frame().fields));
    }
    for (const auto& [message, bytes] : server_samples()) {
        const SingleFrame single(bytes);
        samples.emplace_back(single.frame().type, bytes_of(single.frame().fields));
    }
    int accepted = 0;
    int refused = 0;

    for (int round = 0; round < kRounds; ++round) {
        auto [type, fields] = samples[random.next_below(static_cast<std::uint32_t>(samples.size()))];
        const auto damaged_byte = static_cast<std::byte>(random.next_below(256));
        switch (random.next_below(3)) {
        case 0:
            fields[random.next_below(static_cast<std::uint32_t>(fields.size()))] = damaged_byte;
            break;
        case 1:
            fields.pop_back();
            break;
        default:
            fields.push_back(damaged_byte);
            break;
        }
        if (read_and_compare(type, fields)) {
            ++accepted;
        } else {
            ++refused;
        }
    }

    CHECK(accepted > 0);
    CHECK(refused > 0);
}

TEST_CASE("random bytes never break the reading", "[0000-CA-11]") {
    constexpr int kRounds = 2'000;
    constexpr std::uint32_t kLongestBlock = 300;
    SeededRandom random(1118);

    class ReadEverything final : public FrameSink {
    public:
        void on_frame(const Frame& frame) override {
            static_cast<void>(read_client_message(frame));
            static_cast<void>(read_server_message(frame));
            ++frames;
        }

        int frames = 0;
    };

    int broken_readers = 0;
    ReadEverything sink;
    for (int round = 0; round < kRounds; ++round) {
        Bytes block(random.next_below(kLongestBlock));
        for (std::byte& value : block) {
            value = static_cast<std::byte>(random.next_below(256));
        }
        // Small sizes at the start, so some frames are complete.
        if (block.size() >= 2) {
            block[1] = std::byte{0};
        }
        FrameReader reader;
        if (!reader.read(block, sink)) {
            ++broken_readers;
        }
    }

    CHECK(sink.frames > 0);
    CHECK(broken_readers > 0);
}
