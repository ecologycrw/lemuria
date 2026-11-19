#include "DemoProtocol/Framing.hpp"

#include <algorithm>

namespace demo_protocol {

namespace {

using HeaderView = std::span<const std::byte, kFrameHeaderBytes>;

[[nodiscard]] std::size_t body_size_in(HeaderView header) noexcept {
    return std::to_integer<std::size_t>(header[0]) | (std::to_integer<std::size_t>(header[1]) << 8U);
}

void write_body_size(std::span<std::byte, kFrameHeaderBytes> header, std::size_t body_size) noexcept {
    header[0] = static_cast<std::byte>(body_size & 0xFFU);
    header[1] = static_cast<std::byte>((body_size >> 8U) & 0xFFU);
}

} // namespace

tl::expected<void, FrameError> FrameReader::read(std::span<const std::byte> bytes, FrameSink& sink) {
    while (!error_ && !bytes.empty()) {
        bytes = take_bytes(bytes);
        if (filled_ == kFrameHeaderBytes) {
            error_ = check_header();
        }
        if (!error_ && filled_ > kFrameHeaderBytes && filled_ == frame_size()) {
            deliver_frame(sink);
        }
    }
    if (error_) {
        return tl::unexpected(*error_);
    }
    return {};
}

// Copies what fits in the frame being read and returns the rest.
std::span<const std::byte> FrameReader::take_bytes(std::span<const std::byte> bytes) noexcept {
    const std::size_t taken = std::min(bytes.size(), frame_size() - filled_);
    std::ranges::copy(bytes.first(taken), buffer_.begin() + static_cast<std::ptrdiff_t>(filled_));
    filled_ += taken;
    return bytes.subspan(taken);
}

// The frame is ready for the next one before the sink sees it, and its body stays untouched during the call.
void FrameReader::deliver_frame(FrameSink& sink) {
    const auto body =
        std::span<const std::byte>(buffer_).subspan(kFrameHeaderBytes, filled_ - kFrameHeaderBytes);
    filled_ = 0;
    sink.on_frame(Frame{std::to_integer<std::uint8_t>(body[0]), body.subspan(1)});
}

// How many bytes the frame being read has, as far as is known: only the header until it arrives.
std::size_t FrameReader::frame_size() const noexcept {
    if (filled_ < kFrameHeaderBytes) {
        return kFrameHeaderBytes;
    }
    return kFrameHeaderBytes + body_size_in(HeaderView(buffer_.data(), kFrameHeaderBytes));
}

std::optional<FrameError> FrameReader::check_header() const noexcept {
    const std::size_t body_size = body_size_in(HeaderView(buffer_.data(), kFrameHeaderBytes));
    if (body_size == 0) {
        return FrameError::EmptyBody;
    }
    if (body_size > kMaxBodyBytes) {
        return FrameError::BodyTooLarge;
    }
    return std::nullopt;
}

EncodedFrame::EncodedFrame(std::uint8_t type) noexcept : size_(kFrameHeaderBytes + 1) {
    buffer_[kFrameHeaderBytes] = static_cast<std::byte>(type);
    write_body_size(std::span<std::byte, kFrameHeaderBytes>(buffer_.data(), kFrameHeaderBytes), 1);
}

bool EncodedFrame::append(std::span<const std::byte> fields) noexcept {
    if (fields.size() > buffer_.size() - size_) {
        return false;
    }
    std::ranges::copy(fields, buffer_.begin() + static_cast<std::ptrdiff_t>(size_));
    size_ += fields.size();
    write_body_size(std::span<std::byte, kFrameHeaderBytes>(buffer_.data(), kFrameHeaderBytes),
                    size_ - kFrameHeaderBytes);
    return true;
}

} // namespace demo_protocol
