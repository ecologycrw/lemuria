#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <tl/expected.hpp>

// How the messages of the demonstration travel on a stream of bytes. Each frame starts with two bytes that
// give the size of its body, least significant byte first. The body starts with one byte with the type of the
// message, followed by its fields.
namespace demo_protocol {

inline constexpr std::size_t kFrameHeaderBytes = 2;

// The largest body a frame can have. Anything larger breaks the format (INV-14).
inline constexpr std::size_t kMaxBodyBytes = 128;

inline constexpr std::size_t kMaxFrameBytes = kFrameHeaderBytes + kMaxBodyBytes;

// The bytes broke the format, and there is no way to find where the next frame starts. The connection must
// end.
enum class FrameError : std::uint8_t {
    EmptyBody,
    BodyTooLarge,
};

// One frame read whole. The fields are only valid during the call that delivers them.
struct Frame {
    std::uint8_t type = 0;
    std::span<const std::byte> fields;
};

// Receives the frames in the order they arrived.
class FrameSink {
public:
    virtual ~FrameSink() = default;

    virtual void on_frame(const Frame& frame) = 0;

protected:
    FrameSink() = default;
    FrameSink(const FrameSink&) = default;
    FrameSink(FrameSink&&) = default;
    FrameSink& operator=(const FrameSink&) = default;
    FrameSink& operator=(FrameSink&&) = default;
};

// Cuts a stream of bytes into frames. A frame may arrive in pieces, and several frames may arrive together.
// It keeps at most one unfinished frame, inside the object, so it never allocates memory (INV-13).
class FrameReader {
public:
    // Hands every frame completed by these bytes to the sink. After a format error, nothing else is read, now
    // or later.
    [[nodiscard]] tl::expected<void, FrameError> read(std::span<const std::byte> bytes, FrameSink& sink);

    // The bytes of a frame that has not arrived whole yet.
    [[nodiscard]] std::size_t pending_bytes() const noexcept { return filled_; }

    [[nodiscard]] bool is_broken() const noexcept { return error_.has_value(); }

private:
    [[nodiscard]] std::span<const std::byte> take_bytes(std::span<const std::byte> bytes) noexcept;
    void deliver_frame(FrameSink& sink);
    [[nodiscard]] std::size_t frame_size() const noexcept;
    [[nodiscard]] std::optional<FrameError> check_header() const noexcept;

    std::array<std::byte, kMaxFrameBytes> buffer_{};
    std::size_t filled_ = 0;
    std::optional<FrameError> error_;
};

// One frame ready to be sent, kept inside the object.
class EncodedFrame {
public:
    explicit EncodedFrame(std::uint8_t type) noexcept;

    // Adds fields to the body. False, with nothing changed, when the body would become too large.
    [[nodiscard]] bool append(std::span<const std::byte> fields) noexcept;

    // The whole frame, with its size at the start.
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return {buffer_.data(), size_}; }

private:
    std::array<std::byte, kMaxFrameBytes> buffer_{};
    std::size_t size_ = 0;
};

} // namespace demo_protocol
