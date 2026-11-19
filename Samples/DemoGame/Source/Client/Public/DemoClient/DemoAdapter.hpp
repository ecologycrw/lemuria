#pragma once

#include <cstdint>

#include "DemoProtocol/Framing.hpp"
#include "DemoProtocol/Messages.hpp"
#include "Lemuria/Classic/AdapterCapabilities.hpp"
#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/GameRequest.hpp"
#include "Lemuria/Classic/ProtocolAdapter.hpp"
#include "Lemuria/Core/Connections.hpp"
#include "Lemuria/Core/Result.hpp"

namespace demo_client {

// How the test server works. The client must agree with the server on these values, or the server would
// correct walks the client believes are right.
inline constexpr lemuria::classic::AdapterCapabilities kDemoCapabilities{
    .connection_kind = lemuria::core::kTcpConnectionKind,
    .walk_requests = lemuria::classic::WalkRequestMode::Steps,
    .max_steps_per_request = demo_protocol::kMaxWalkSteps,
    .needs_map_loaded_notice = true,
    .presence = lemuria::classic::PresenceSignal::None,
    .presence_period_seconds = 0.0,
    .resume = lemuria::classic::ResumePolicy::EndSession,
};

// Speaks the messages of the demonstration with the test server, over one TCP connection. It cuts the bytes
// into frames, checks every message (INV-12) and turns each one into an event of the framework. A broken
// frame ends the session, and a wrong message is only counted (CA-10, CA-11).
//
// Nothing here allocates memory after it is built, so it can run inside a warm frame (INV-13).
class DemoAdapter final : public lemuria::classic::ProtocolAdapter,
                          private lemuria::core::ConnectionEvents,
                          private demo_protocol::FrameSink {
public:
    DemoAdapter(lemuria::core::Connections& connections, lemuria::core::ConnectionAddress server) noexcept;

    [[nodiscard]] const lemuria::classic::AdapterCapabilities& capabilities() const noexcept override {
        return kDemoCapabilities;
    }

    // Refuses connections of another kind: the test server speaks only TCP.
    [[nodiscard]] lemuria::core::Result<void> begin_session() override;

    void poll(lemuria::classic::GameEventSink& events) override;

    // A walk that the messages cannot carry, such as one with too many steps, is refused with the reason.
    [[nodiscard]] lemuria::core::Result<void>
    send_request(const lemuria::classic::GameRequest& request) override;

    void end_session() noexcept override;

    // Messages left out because only they were wrong, kept for diagnosis.
    [[nodiscard]] std::uint64_t invalid_message_count() const noexcept { return invalid_messages_; }

private:
    void on_connected(lemuria::core::ConnectionId connection) override;
    void on_received(const lemuria::core::ReceivedBytes& block) override;
    void on_closed(lemuria::core::ConnectionId connection,
                   const lemuria::core::ConnectionClosed& closed) override;
    void on_frame(const demo_protocol::Frame& frame) override;

    [[nodiscard]] lemuria::core::Result<void> send_message(const demo_protocol::ClientMessage& message);
    [[nodiscard]] lemuria::core::Result<void> send_walk(const lemuria::classic::WalkRequest& request);
    void deliver(const lemuria::classic::GameEvent& event);

    // Ends the session at once: nothing the connection still delivers becomes an event.
    void end_with(lemuria::classic::SessionEndReason reason);

    lemuria::core::Connections& connections_;
    lemuria::core::ConnectionAddress server_;
    lemuria::core::ConnectionId connection_;
    demo_protocol::FrameReader frames_;

    // Only set during `poll`.
    lemuria::classic::GameEventSink* events_ = nullptr;

    std::uint64_t invalid_messages_ = 0;
    bool has_begun_ = false;
    bool has_ended_ = false;
};

} // namespace demo_client
