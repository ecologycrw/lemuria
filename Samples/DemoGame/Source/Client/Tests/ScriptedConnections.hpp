#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <utility>
#include <vector>

#include "DemoProtocol/Framing.hpp"
#include "DemoProtocol/MessageCodec.hpp"
#include "DemoProtocol/Messages.hpp"
#include "Lemuria/Core/Connections.hpp"

// A connection written by each test, and the bytes the tests send through it.
namespace demo_client::testing {

using Bytes = std::vector<std::byte>;

[[nodiscard]] inline Bytes bytes_of(std::initializer_list<int> values) {
    Bytes bytes;
    for (const int value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

[[nodiscard]] inline Bytes bytes_of(const demo_protocol::ServerMessage& message) {
    const demo_protocol::EncodedFrame frame = demo_protocol::encode(message);
    return {frame.bytes().begin(), frame.bytes().end()};
}

[[nodiscard]] inline Bytes bytes_of(const demo_protocol::ClientMessage& message) {
    const demo_protocol::EncodedFrame frame = demo_protocol::encode(message);
    return {frame.bytes().begin(), frame.bytes().end()};
}

inline void append(Bytes& bytes, const Bytes& more) {
    bytes.insert(bytes.end(), more.begin(), more.end());
}

inline constexpr lemuria::core::ConnectionTraits kTcpTraits{lemuria::core::kTcpConnectionKind, false, false,
                                                            64 * 1024};

// A connection whose events are written by each test.
class ScriptedConnections final : public lemuria::core::Connections {
public:
    explicit ScriptedConnections(lemuria::core::ConnectionTraits traits = kTcpTraits) : traits_(traits) {}

    [[nodiscard]] const lemuria::core::ConnectionTraits& traits() const noexcept override { return traits_; }

    void arrive_connected() { script_.push_back(Step{StepKind::Connected, {}, {}}); }
    void arrive_bytes(Bytes bytes) { script_.push_back(Step{StepKind::Bytes, std::move(bytes), {}}); }

    void arrive_closed(lemuria::core::CloseReason reason) {
        script_.push_back(Step{StepKind::Closed, {}, reason});
    }

    std::vector<lemuria::core::ConnectionAddress> opened;
    std::vector<Bytes> sent;
    int stop_count = 0;

private:
    enum class StepKind : std::uint8_t { Connected, Bytes, Closed };

    struct Step {
        StepKind kind = StepKind::Connected;
        Bytes bytes;
        lemuria::core::CloseReason reason = lemuria::core::CloseReason::ConnectionLost;
    };

    lemuria::core::Result<void> start_connecting(lemuria::core::ConnectionId connection,
                                                 const lemuria::core::ConnectionAddress& address) override {
        connection_ = connection;
        opened.push_back(address);
        return {};
    }

    lemuria::core::Result<void> queue_bytes(lemuria::core::ConnectionId /*connection*/,
                                            std::span<const std::byte> bytes,
                                            lemuria::core::SendOptions /*options*/) override {
        sent.emplace_back(bytes.begin(), bytes.end());
        return {};
    }

    void stop_connection(lemuria::core::ConnectionId /*connection*/) noexcept override { ++stop_count; }

    void collect_events(lemuria::core::ConnectionEvents& events) override {
        const std::vector<Step> script = std::exchange(script_, {});
        for (const Step& step : script) {
            switch (step.kind) {
            case StepKind::Connected:
                events.on_connected(connection_);
                break;
            case StepKind::Bytes:
                events.on_received(lemuria::core::ReceivedBytes{connection_, step.bytes, false});
                break;
            case StepKind::Closed:
                events.on_closed(connection_, lemuria::core::ConnectionClosed{step.reason, 0});
                break;
            }
        }
    }

    lemuria::core::ConnectionTraits traits_;
    lemuria::core::ConnectionId connection_;
    std::vector<Step> script_;
};

} // namespace demo_client::testing
