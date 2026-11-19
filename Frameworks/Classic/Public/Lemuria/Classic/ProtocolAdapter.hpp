#pragma once

#include "Lemuria/Classic/AdapterCapabilities.hpp"
#include "Lemuria/Classic/GameEvent.hpp"
#include "Lemuria/Classic/GameRequest.hpp"
#include "Lemuria/Core/Result.hpp"

namespace lemuria::classic {

// The part each game writes to talk to its server. The adapter owns everything about the bytes: how many
// connections exist, the format of the messages and the encryption. The framework only sees events and
// requests (docs/adr/0017-contrato-do-adaptador.md).
//
// All functions run on the frame thread, and none of them waits for the network (INV-04).
class ProtocolAdapter {
public:
    ProtocolAdapter(const ProtocolAdapter&) = delete;
    ProtocolAdapter(ProtocolAdapter&&) = delete;
    ProtocolAdapter& operator=(const ProtocolAdapter&) = delete;
    ProtocolAdapter& operator=(ProtocolAdapter&&) = delete;
    virtual ~ProtocolAdapter() = default;

    [[nodiscard]] virtual const AdapterCapabilities& capabilities() const noexcept = 0;

    // Starts connecting. How it goes arrives later, as events.
    [[nodiscard]] virtual core::Result<void> begin_session() = 0;

    // Turns what arrived since the previous call into events. A broken message format ends the session with
    // `SessionEnded`, and a problem in a single message is only recorded by the adapter.
    virtual void poll(GameEventSink& events) = 0;

    // Sends a request. Success only means it left: the server may refuse it without saying anything.
    [[nodiscard]] virtual core::Result<void> send_request(const GameRequest& request) = 0;

    // Ends the session. The next `poll` delivers `SessionEnded`.
    virtual void end_session() noexcept = 0;

protected:
    ProtocolAdapter() = default;
};

} // namespace lemuria::classic
