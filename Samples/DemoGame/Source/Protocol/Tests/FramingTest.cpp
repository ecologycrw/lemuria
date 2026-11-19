#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "DemoProtocol/Framing.hpp"

using demo_protocol::EncodedFrame;
using demo_protocol::Frame;
using demo_protocol::FrameError;
using demo_protocol::FrameReader;
using demo_protocol::FrameSink;
using demo_protocol::kFrameHeaderBytes;
using demo_protocol::kMaxBodyBytes;
using demo_protocol::kMaxFrameBytes;

namespace {

using Bytes = std::vector<std::byte>;

[[nodiscard]] Bytes bytes_of(std::initializer_list<int> values) {
    Bytes bytes;
    for (const int value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

// A frame as the sink saw it, copied so it outlives the call.
struct ReadFrame {
    std::uint8_t type = 0;
    Bytes fields;

    friend bool operator==(const ReadFrame&, const ReadFrame&) = default;
};

class FrameLog final : public FrameSink {
public:
    void on_frame(const Frame& frame) override {
        frames.push_back(ReadFrame{frame.type, Bytes(frame.fields.begin(), frame.fields.end())});
    }

    std::vector<ReadFrame> frames;
};

// Three frames of different sizes, one after the other.
[[nodiscard]] Bytes three_frames() {
    return bytes_of({1, 0, 7, 3, 0, 9, 1, 2, 5, 0, 4, 10, 20, 30, 40});
}

[[nodiscard]] std::vector<ReadFrame> three_frames_read() {
    return {
        ReadFrame{7, {}},
        ReadFrame{9, bytes_of({1, 2})},
        ReadFrame{4, bytes_of({10, 20, 30, 40})},
    };
}

} // namespace

TEST_CASE("frames that arrive byte by byte are read whole and in order", "[0000-CA-10]") {
    FrameReader reader;
    FrameLog log;

    for (const std::byte value : three_frames()) {
        REQUIRE(reader.read(std::array{value}, log).has_value());
    }

    CHECK(log.frames == three_frames_read());
    CHECK(reader.pending_bytes() == 0);
}

TEST_CASE("frames that arrive together are read in order", "[0000-CA-10]") {
    FrameReader reader;
    FrameLog log;

    REQUIRE(reader.read(three_frames(), log).has_value());

    CHECK(log.frames == three_frames_read());
}

TEST_CASE("frames cut at any point are read the same way", "[0000-CA-10]") {
    const Bytes stream = three_frames();
    const auto cut = GENERATE_COPY(range(std::size_t{1}, stream.size()));
    FrameReader reader;
    FrameLog log;
    const std::span<const std::byte> all(stream);

    REQUIRE(reader.read(all.first(cut), log).has_value());
    REQUIRE(reader.read(all.subspan(cut), log).has_value());

    CHECK(log.frames == three_frames_read());
}

TEST_CASE("an unfinished frame waits for the rest and hands nothing over", "[0000-CA-10]") {
    FrameReader reader;
    FrameLog log;

    REQUIRE(reader.read(bytes_of({3, 0, 9, 1}), log).has_value());

    CHECK(log.frames.empty());
    CHECK(reader.pending_bytes() == 4);

    REQUIRE(reader.read(bytes_of({2}), log).has_value());
    CHECK(log.frames == std::vector<ReadFrame>{ReadFrame{9, bytes_of({1, 2})}});
}

TEST_CASE("a frame with the largest body is read") {
    FrameReader reader;
    FrameLog log;
    Bytes frame = bytes_of({static_cast<int>(kMaxBodyBytes), 0, 5});
    frame.resize(kMaxFrameBytes, std::byte{8});

    REQUIRE(reader.read(frame, log).has_value());

    REQUIRE(log.frames.size() == 1);
    CHECK(log.frames[0].type == 5);
    CHECK(log.frames[0].fields.size() == kMaxBodyBytes - 1);
}

TEST_CASE("a frame without a body breaks the format", "[0000-CA-11]") {
    FrameReader reader;
    FrameLog log;

    const auto read = reader.read(bytes_of({0, 0, 1, 0, 7}), log);

    REQUIRE_FALSE(read.has_value());
    CHECK(read.error() == FrameError::EmptyBody);
    CHECK(log.frames.empty());
    CHECK(reader.is_broken());
}

TEST_CASE("a frame with a body above the limit breaks the format", "[0000-CA-11]") {
    // Sizes whose second byte matters: 256 and 257 have a first byte of 0 and 1.
    const int body_size = GENERATE(static_cast<int>(kMaxBodyBytes) + 1, 0x0100, 0x0101, 0xFFFF);
    FrameReader reader;
    FrameLog log;

    const auto read = reader.read(bytes_of({body_size & 0xFF, body_size >> 8}), log);

    REQUIRE_FALSE(read.has_value());
    CHECK(read.error() == FrameError::BodyTooLarge);
    CHECK(reader.is_broken());
}

TEST_CASE("frames before a broken one are still handed over, and nothing after it", "[0000-CA-11]") {
    FrameReader reader;
    FrameLog log;

    const auto read = reader.read(bytes_of({1, 0, 7, 0, 0, 1, 0, 8}), log);

    REQUIRE_FALSE(read.has_value());
    CHECK(log.frames == std::vector<ReadFrame>{ReadFrame{7, {}}});
}

TEST_CASE("a broken reader refuses everything that comes later", "[0000-CA-11]") {
    FrameReader reader;
    FrameLog log;
    REQUIRE_FALSE(reader.read(bytes_of({0, 0}), log).has_value());

    const auto read = reader.read(bytes_of({1, 0, 7}), log);

    REQUIRE_FALSE(read.has_value());
    CHECK(read.error() == FrameError::EmptyBody);
    CHECK(log.frames.empty());
}

TEST_CASE("an encoded frame starts with the size of its body") {
    EncodedFrame frame(9);
    REQUIRE(frame.append(bytes_of({1, 2, 3})));

    const auto bytes = frame.bytes();

    CHECK(Bytes(bytes.begin(), bytes.end()) == bytes_of({4, 0, 9, 1, 2, 3}));
}

TEST_CASE("an encoded frame refuses fields that would pass the limit") {
    EncodedFrame frame(9);
    const Bytes almost_full(kMaxBodyBytes - 2, std::byte{1});
    REQUIRE(frame.append(almost_full));

    CHECK_FALSE(frame.append(bytes_of({1, 2})));
    CHECK(frame.bytes().size() == kFrameHeaderBytes + kMaxBodyBytes - 1);
    CHECK(frame.append(bytes_of({1})));
    CHECK(frame.bytes().size() == kMaxFrameBytes);
}

TEST_CASE("an encoded frame is read back as it was written", "[0000-CA-10]") {
    EncodedFrame frame(42);
    REQUIRE(frame.append(bytes_of({5, 6, 7})));
    FrameReader reader;
    FrameLog log;

    REQUIRE(reader.read(frame.bytes(), log).has_value());

    CHECK(log.frames == std::vector<ReadFrame>{ReadFrame{42, bytes_of({5, 6, 7})}});
}
