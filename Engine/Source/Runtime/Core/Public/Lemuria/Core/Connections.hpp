#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "Lemuria/Core/Result.hpp"

// Connections and bytes: the part of the protocol adapter contract that belongs to the engine
// (docs/adr/0017-contrato-do-adaptador.md and docs/adr/0018-conexoes-de-rede.md). The content of the bytes is
// never read here (INV-07).
namespace lemuria::core {

// Identifies one connection. A number is never used twice by the same `Connections`.
struct ConnectionId {
    std::uint64_t value = 0;

    friend constexpr bool operator==(ConnectionId, ConnectionId) noexcept = default;
};

struct ConnectionAddress {
    std::string host;
    std::uint16_t port = 0;
};

// How a kind of connection is named, so an adapter can declare the kind its server uses. New kinds of
// connection can name themselves without changing the engine.
inline constexpr std::string_view kTcpConnectionKind = "tcp";

// What a kind of connection offers.
struct ConnectionTraits {
    std::string_view kind;

    // Whether each block of received bytes is one whole message, instead of a piece of a stream.
    bool delivers_whole_messages = false;

    bool offers_unreliable_delivery = false;

    // The most bytes one send can carry.
    std::size_t max_send_bytes = 0;
};

enum class Delivery : std::uint8_t {
    // Every byte arrives, in the order it was sent.
    ReliableInOrder,

    // The bytes may never arrive. Only some kinds of connection offer this.
    Unreliable,
};

struct SendOptions {
    Delivery delivery = Delivery::ReliableInOrder;
};

// Why a connection ended, in the same words for every kind of connection.
enum class CloseReason : std::uint8_t {
    LocalRequest,
    PeerClosed,
    CouldNotConnect,
    ConnectionLost,
    TimedOut,
};

struct ConnectionClosed {
    CloseReason reason = CloseReason::ConnectionLost;

    // The code of the library in use, kept for diagnosis. Its meaning depends on the kind of connection.
    std::int64_t original_code = 0;
};

// A block of received bytes, which is only valid during the call that delivers it.
struct ReceivedBytes {
    ConnectionId connection;
    std::span<const std::byte> bytes;
    bool is_whole_message = false;
};

// Receives what happened to the connections.
class ConnectionEvents {
public:
    virtual ~ConnectionEvents() = default;

    virtual void on_connected(ConnectionId connection) = 0;
    virtual void on_received(const ReceivedBytes& block) = 0;

    // The last event of a connection.
    virtual void on_closed(ConnectionId connection, const ConnectionClosed& closed) = 0;

protected:
    ConnectionEvents() = default;
    ConnectionEvents(const ConnectionEvents&) = default;
    ConnectionEvents(ConnectionEvents&&) = default;
    ConnectionEvents& operator=(const ConnectionEvents&) = default;
    ConnectionEvents& operator=(ConnectionEvents&&) = default;
};

// The connections of one kind. Every kind derives from this class, which keeps the same promises for all of
// them. It is used from one thread only, and none of its functions waits for the network (INV-04):
//
// - a connection is ready only after `on_connected`, and `on_closed` is always its last event;
// - no data of a closed connection ever reaches the game, not even data that was already on its way;
// - a delivery option that the kind does not offer is an error, and is never replaced by another one.
class Connections {
public:
    static constexpr std::size_t kMaxOpenConnections = 8;

    Connections(const Connections&) = delete;
    Connections(Connections&&) = delete;
    Connections& operator=(const Connections&) = delete;
    Connections& operator=(Connections&&) = delete;
    virtual ~Connections() = default;

    [[nodiscard]] virtual const ConnectionTraits& traits() const noexcept = 0;

    // Starts connecting. The result arrives later, as `on_connected` or `on_closed`.
    [[nodiscard]] Result<ConnectionId> open(const ConnectionAddress& address);

    // Bytes sent on the same connection leave in the order of the calls.
    [[nodiscard]] Result<void> send(ConnectionId connection, std::span<const std::byte> bytes,
                                    SendOptions options = {});

    // Ends the connection. The next `poll` reports its end, and it can be called from inside an event.
    void close(ConnectionId connection) noexcept;

    // Delivers what happened since the previous call.
    void poll(ConnectionEvents& events);

    // Events left out because they broke the promises above, kept for diagnosis.
    [[nodiscard]] std::uint64_t ignored_event_count() const noexcept { return ignored_events_; }

protected:
    Connections() = default;

private:
    class EventGate;

    enum class SlotState : std::uint8_t { Free, Connecting, Connected, Closing };

    struct Slot {
        ConnectionId connection;
        SlotState state = SlotState::Free;
    };

    // What each kind of connection provides. The address and the bytes were already checked.
    [[nodiscard]] virtual Result<void> start_connecting(ConnectionId connection,
                                                        const ConnectionAddress& address) = 0;
    [[nodiscard]] virtual Result<void> queue_bytes(ConnectionId connection, std::span<const std::byte> bytes,
                                                   SendOptions options) = 0;
    virtual void stop_connection(ConnectionId connection) noexcept = 0;
    virtual void collect_events(ConnectionEvents& events) = 0;

    [[nodiscard]] Slot* find_slot(ConnectionId connection) noexcept;
    [[nodiscard]] Slot* find_free_slot() noexcept;
    [[nodiscard]] Result<void> check_send(std::size_t byte_count, SendOptions options) const;

    std::array<Slot, kMaxOpenConnections> slots_{};
    std::uint64_t last_connection_number_ = 0;
    std::uint64_t ignored_events_ = 0;
};

} // namespace lemuria::core
