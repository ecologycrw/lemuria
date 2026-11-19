#include "Lemuria/Core/Connections.hpp"

#include <string>
#include <utility>

namespace lemuria::core {

// Stands between a kind of connection and the game, and lets through only the events that keep the promises.
class Connections::EventGate final : public ConnectionEvents {
public:
    EventGate(Connections& connections, ConnectionEvents& game) noexcept
        : connections_(connections), game_(game) {}

    void on_connected(ConnectionId connection) override {
        Slot* slot = connections_.find_slot(connection);
        if (slot == nullptr || slot->state != SlotState::Connecting) {
            ++connections_.ignored_events_;
            return;
        }
        slot->state = SlotState::Connected;
        game_.on_connected(connection);
    }

    void on_received(const ReceivedBytes& block) override {
        const Slot* slot = connections_.find_slot(block.connection);
        if (slot == nullptr || slot->state != SlotState::Connected) {
            ++connections_.ignored_events_;
            return;
        }
        game_.on_received(block);
    }

    void on_closed(ConnectionId connection, const ConnectionClosed& closed) override {
        Slot* slot = connections_.find_slot(connection);
        if (slot == nullptr) {
            ++connections_.ignored_events_;
            return;
        }
        if (slot->state == SlotState::Closing) {
            return; // The game asked for this end, and `poll` reports it.
        }
        *slot = Slot{};
        game_.on_closed(connection, closed);
    }

private:
    Connections& connections_;
    ConnectionEvents& game_;
};

Connections::Slot* Connections::find_slot(ConnectionId connection) noexcept {
    for (Slot& slot : slots_) {
        if (slot.state != SlotState::Free && slot.connection == connection) {
            return &slot;
        }
    }
    return nullptr;
}

Connections::Slot* Connections::find_free_slot() noexcept {
    for (Slot& slot : slots_) {
        if (slot.state == SlotState::Free) {
            return &slot;
        }
    }
    return nullptr;
}

Result<ConnectionId> Connections::open(const ConnectionAddress& address) {
    if (address.host.empty() || address.port == 0) {
        return make_error("a connection needs a host and a port");
    }
    Slot* slot = find_free_slot();
    if (slot == nullptr) {
        return make_error("too many open connections");
    }

    const ConnectionId connection{++last_connection_number_};
    auto started = start_connecting(connection, address);
    if (!started) {
        return tl::unexpected(std::move(started.error()));
    }
    *slot = Slot{connection, SlotState::Connecting};
    return connection;
}

Result<void> Connections::check_send(std::size_t byte_count, SendOptions options) const {
    const ConnectionTraits& offered = traits();
    if (byte_count == 0) {
        return make_error("there are no bytes to send");
    }
    if (byte_count > offered.max_send_bytes) {
        return make_error("the bytes are above the send limit of the connection kind '" +
                          std::string(offered.kind) + "'");
    }
    if (options.delivery == Delivery::Unreliable && !offered.offers_unreliable_delivery) {
        return make_error("the connection kind '" + std::string(offered.kind) +
                          "' does not offer unreliable delivery");
    }
    return {};
}

Result<void> Connections::send(ConnectionId connection, std::span<const std::byte> bytes,
                               SendOptions options) {
    const Slot* slot = find_slot(connection);
    if (slot == nullptr || slot->state == SlotState::Closing) {
        return make_error("the connection is closed");
    }
    if (slot->state != SlotState::Connected) {
        return make_error("the connection is still connecting");
    }
    auto allowed = check_send(bytes.size(), options);
    if (!allowed) {
        return allowed;
    }
    return queue_bytes(connection, bytes, options);
}

void Connections::close(ConnectionId connection) noexcept {
    Slot* slot = find_slot(connection);
    if (slot == nullptr || slot->state == SlotState::Closing) {
        return;
    }
    slot->state = SlotState::Closing;
    stop_connection(connection);
}

void Connections::poll(ConnectionEvents& events) {
    EventGate gate(*this, events);
    collect_events(gate);

    for (Slot& slot : slots_) {
        if (slot.state == SlotState::Closing) {
            const ConnectionId connection = slot.connection;
            slot = Slot{};
            events.on_closed(connection, ConnectionClosed{CloseReason::LocalRequest, 0});
        }
    }
}

} // namespace lemuria::core
