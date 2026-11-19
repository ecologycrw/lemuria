#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "Lemuria/Classic/ProtocolAdapter.hpp"
#include "Lemuria/Core/Connections.hpp"

// A small adapter and a connection written by each test. Together they prove that the contract can be used
// without the framework knowing anything about bytes.
namespace lemuria::classic::testing {

using Bytes = std::vector<std::byte>;

inline constexpr std::size_t kSendLimit = 256;

// A stream of bytes that only offers reliable delivery.
inline constexpr core::ConnectionTraits kStreamTraits{core::kTcpConnectionKind, false, false, kSendLimit};

// Whole messages that may also be sent without a delivery guarantee.
inline constexpr core::ConnectionTraits kMessageTraits{"whole-messages", true, true, kSendLimit};

// The messages of the test protocol: one byte with the size of the rest, one byte with the type, then the
// fields.
inline constexpr int kMapEnteredMessage = 1;
inline constexpr int kEntityAppearedMessage = 2;
inline constexpr int kEntityWalkedMessage = 3;
inline constexpr int kPositionCorrectedMessage = 4;
inline constexpr int kEntityLeftMessage = 5;
inline constexpr int kWalkConfirmedMessage = 6;
inline constexpr int kWalkStepsMessage = 10;
inline constexpr int kWalkDestinationMessage = 11;
inline constexpr int kMapLoadedMessage = 12;

[[nodiscard]] inline Bytes framed(std::span<const int> content) {
    Bytes bytes{static_cast<std::byte>(content.size())};
    for (const int value : content) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

[[nodiscard]] inline Bytes framed(std::initializer_list<int> content) {
    return framed(std::span<const int>(content.begin(), content.size()));
}

struct SentBlock {
    Bytes bytes;
    core::Delivery delivery = core::Delivery::ReliableInOrder;
};

// A kind of connection whose events are written by each test.
class ScriptedConnections final : public core::Connections {
public:
    explicit ScriptedConnections(core::ConnectionTraits traits) : traits_(traits) {}

    [[nodiscard]] const core::ConnectionTraits& traits() const noexcept override { return traits_; }

    void arrive_connected() {
        script_.emplace_back([this](core::ConnectionEvents& events) { events.on_connected(started.back()); });
    }

    void arrive_bytes(Bytes bytes, bool is_whole_message) {
        script_.emplace_back(
            [this, data = std::move(bytes), is_whole_message](core::ConnectionEvents& events) {
                events.on_received(core::ReceivedBytes{started.back(), data, is_whole_message});
            });
    }

    void arrive_closed(core::CloseReason reason) {
        script_.emplace_back([this, reason](core::ConnectionEvents& events) {
            events.on_closed(started.back(), core::ConnectionClosed{reason, 0});
        });
    }

    std::vector<core::ConnectionId> started;
    std::vector<core::ConnectionId> stopped;
    std::vector<SentBlock> sent;

private:
    core::Result<void> start_connecting(core::ConnectionId connection,
                                        const core::ConnectionAddress& /*address*/) override {
        started.push_back(connection);
        return {};
    }

    core::Result<void> queue_bytes(core::ConnectionId /*connection*/, std::span<const std::byte> bytes,
                                   core::SendOptions options) override {
        sent.push_back(SentBlock{Bytes(bytes.begin(), bytes.end()), options.delivery});
        return {};
    }

    void stop_connection(core::ConnectionId connection) noexcept override { stopped.push_back(connection); }

    void collect_events(core::ConnectionEvents& events) override {
        const auto script = std::exchange(script_, {});
        for (const auto& event : script) {
            event(events);
        }
    }

    core::ConnectionTraits traits_;
    std::vector<std::function<void(core::ConnectionEvents&)>> script_;
};

[[nodiscard]] inline int field_at(std::span<const std::byte> fields, std::size_t index) {
    return std::to_integer<int>(fields[index]);
}

[[nodiscard]] inline std::uint32_t number_at(std::span<const std::byte> fields, std::size_t index) {
    return std::to_integer<std::uint32_t>(fields[index]);
}

[[nodiscard]] inline TileCoord tile_at(std::span<const std::byte> fields, std::size_t index) {
    return {field_at(fields, index), field_at(fields, index + 1)};
}

// Reads the walk of someone else: who, the destination, and then the steps, when the server sends them.
[[nodiscard]] inline std::optional<GameEvent> read_entity_walked(std::span<const std::byte> fields) {
    constexpr std::size_t kFixedFields = 3;
    if (fields.size() < kFixedFields || (fields.size() - kFixedFields) % 2 != 0) {
        return std::nullopt;
    }
    EntityWalked walked{EntityId{number_at(fields, 0)}, std::nullopt, tile_at(fields, 1), {}};
    for (std::size_t index = kFixedFields; index < fields.size(); index += 2) {
        if (!walked.steps.add(tile_at(fields, index))) {
            return std::nullopt;
        }
    }
    return GameEvent{walked};
}

// Nothing when the type is unknown or the fields do not match it.
[[nodiscard]] inline std::optional<GameEvent> read_message(int type, std::span<const std::byte> fields) {
    const std::size_t size = fields.size();
    if (type == kMapEnteredMessage && size == 4) {
        return GameEvent{
            MapEntered{MapId{number_at(fields, 0)}, EntityId{number_at(fields, 1)}, tile_at(fields, 2)}};
    }
    if (type == kEntityAppearedMessage && size == 3) {
        return GameEvent{EntityAppeared{EntityId{number_at(fields, 0)}, tile_at(fields, 1), std::nullopt}};
    }
    if (type == kEntityWalkedMessage) {
        return read_entity_walked(fields);
    }
    if (type == kPositionCorrectedMessage && size == 3) {
        return GameEvent{PositionCorrected{EntityId{number_at(fields, 0)}, tile_at(fields, 1)}};
    }
    if (type == kEntityLeftMessage && size == 1) {
        return GameEvent{EntityLeft{EntityId{number_at(fields, 0)}, EntityLeaveReason::LeftView}};
    }
    if (type == kWalkConfirmedMessage && size == 4) {
        return GameEvent{OwnWalkConfirmed{tile_at(fields, 0), tile_at(fields, 2)}};
    }
    return std::nullopt;
}

[[nodiscard]] inline SessionEndReason end_reason_of(core::CloseReason reason) noexcept {
    switch (reason) {
    case core::CloseReason::LocalRequest:
        return SessionEndReason::ClosedByPlayer;
    case core::CloseReason::PeerClosed:
        return SessionEndReason::ClosedByServer;
    case core::CloseReason::CouldNotConnect:
        return SessionEndReason::CouldNotConnect;
    case core::CloseReason::TimedOut:
        return SessionEndReason::TimedOut;
    case core::CloseReason::ConnectionLost:
        break;
    }
    return SessionEndReason::ConnectionLost;
}

// Speaks the test protocol over any kind of connection.
class TestAdapter final : public ProtocolAdapter, private core::ConnectionEvents {
public:
    TestAdapter(core::Connections& connections, const AdapterCapabilities& capabilities,
                core::Delivery walk_delivery = core::Delivery::ReliableInOrder)
        : connections_(connections), capabilities_(capabilities), walk_delivery_(walk_delivery) {}

    [[nodiscard]] const AdapterCapabilities& capabilities() const noexcept override { return capabilities_; }

    [[nodiscard]] core::Result<void> begin_session() override {
        auto opened = connections_.open(core::ConnectionAddress{"test.invalid", 1000});
        if (!opened) {
            return tl::unexpected(std::move(opened.error()));
        }
        connection_ = *opened;
        return {};
    }

    void poll(GameEventSink& events) override {
        events_ = &events;
        connections_.poll(*this);
        events_ = nullptr;
    }

    [[nodiscard]] core::Result<void> send_request(const GameRequest& request) override {
        return std::visit([this](const auto& value) { return send(value); }, request);
    }

    void end_session() noexcept override { connections_.close(connection_); }

    // Messages left out because only that message was wrong.
    [[nodiscard]] std::size_t ignored_messages() const noexcept { return ignored_messages_; }

private:
    void on_connected(core::ConnectionId /*connection*/) override {
        events_->deliver(SessionPhaseReached{SessionPhase::EnteringMap});
    }

    void on_received(const core::ReceivedBytes& block) override {
        pending_.insert(pending_.end(), block.bytes.begin(), block.bytes.end());
        read_pending_messages();
        if (block.is_whole_message && !pending_.empty()) {
            end_with_invalid_data();
        }
    }

    void on_closed(core::ConnectionId /*connection*/, const core::ConnectionClosed& closed) override {
        if (!has_ended_) {
            has_ended_ = true;
            events_->deliver(SessionEnded{end_reason_of(closed.reason)});
        }
    }

    void read_pending_messages() {
        std::size_t offset = 0;
        while (offset < pending_.size()) {
            const auto size = std::to_integer<std::size_t>(pending_[offset]);
            if (size == 0) {
                end_with_invalid_data();
                return;
            }
            if (pending_.size() - offset - 1 < size) {
                break;
            }
            handle_message(std::span<const std::byte>(pending_).subspan(offset + 1, size));
            offset += size + 1;
        }
        pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    void handle_message(std::span<const std::byte> message) {
        const auto event = read_message(field_at(message, 0), message.subspan(1));
        if (event) {
            events_->deliver(*event);
        } else {
            ++ignored_messages_;
        }
    }

    void end_with_invalid_data() {
        pending_.clear();
        has_ended_ = true;
        events_->deliver(SessionEnded{SessionEndReason::InvalidData});
        connections_.close(connection_);
    }

    [[nodiscard]] core::Result<void> send(const WalkRequest& request) {
        std::vector<int> content;
        if (capabilities_.walk_requests == WalkRequestMode::Steps) {
            content = {kWalkStepsMessage, request.origin.x, request.origin.z, request.destination.x,
                       request.destination.z};
            for (const TileCoord step : request.steps.view()) {
                content.push_back(step.x);
                content.push_back(step.z);
            }
        } else {
            content = {kWalkDestinationMessage, request.destination.x, request.destination.z};
        }
        return connections_.send(connection_, framed(content), core::SendOptions{walk_delivery_});
    }

    [[nodiscard]] core::Result<void> send(const MapLoadedNotice& notice) {
        return connections_.send(connection_,
                                 framed({kMapLoadedMessage, static_cast<int>(notice.map.value)}));
    }

    core::Connections& connections_;
    AdapterCapabilities capabilities_;
    core::Delivery walk_delivery_;
    core::ConnectionId connection_;
    GameEventSink* events_ = nullptr;
    Bytes pending_;
    std::size_t ignored_messages_ = 0;
    bool has_ended_ = false;
};

} // namespace lemuria::classic::testing
