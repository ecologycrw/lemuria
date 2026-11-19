#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <variant>

// The messages exchanged by the demo client and the test server. Every value is a plain copy, so messages can
// be read and written during a frame without allocating memory (INV-13).
namespace demo_protocol {

// The engine accepts maps of up to 1024 tiles on each side.
inline constexpr std::uint16_t kMaxTileCoordinate = 1023;

// The most steps one walk can carry. The client and the server both follow this limit.
inline constexpr std::size_t kMaxWalkSteps = 16;

struct Tile {
    std::uint16_t x = 0;
    std::uint16_t z = 0;

    friend constexpr bool operator==(Tile, Tile) noexcept = default;
};

// Nothing when the tile is outside the coordinates a message can carry.
[[nodiscard]] constexpr std::optional<Tile> tile_from(int x, int z) noexcept {
    const auto is_valid = [](int value) { return value >= 0 && value <= kMaxTileCoordinate; };
    if (!is_valid(x) || !is_valid(z)) {
        return std::nullopt;
    }
    return Tile{static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(z)};
}

// The steps of one walk, kept inside the object.
class StepList {
public:
    // False, with nothing changed, when the list is full.
    [[nodiscard]] constexpr bool add(Tile step) noexcept {
        if (count_ == kMaxWalkSteps) {
            return false;
        }
        steps_[count_] = step;
        ++count_;
        return true;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return count_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return count_ == 0; }

    [[nodiscard]] std::span<const Tile> view() const noexcept { return {steps_.data(), count_}; }

    friend bool operator==(const StepList& first, const StepList& second) noexcept {
        return std::ranges::equal(first.view(), second.view());
    }

private:
    std::array<Tile, kMaxWalkSteps> steps_{};
    std::size_t count_ = 0;
};

// From the client to the server.

// The first message of a connection.
struct Hello {
    std::uint16_t protocol_version = 0;

    friend bool operator==(const Hello&, const Hello&) = default;
};

// The player wants to walk these steps from `origin`. Without steps, the player wants to stop at `origin`.
struct WalkRequest {
    Tile origin;
    StepList steps;

    friend bool operator==(const WalkRequest&, const WalkRequest&) = default;
};

struct MapLoaded {
    std::uint32_t map = 0;

    friend bool operator==(const MapLoaded&, const MapLoaded&) = default;
};

using ClientMessage = std::variant<Hello, WalkRequest, MapLoaded>;

// From the server to the client.

// The number of an entity is never 0.
struct MapEntered {
    std::uint32_t map = 0;
    std::uint32_t own_entity = 0;
    Tile position;

    friend bool operator==(const MapEntered&, const MapEntered&) = default;
};

struct EntityAppeared {
    std::uint32_t entity = 0;
    Tile position;
    std::optional<Tile> walking_to;

    friend bool operator==(const EntityAppeared&, const EntityAppeared&) = default;
};

// Someone walks these steps from `origin`. A walk always has at least one step.
struct EntityWalked {
    std::uint32_t entity = 0;
    Tile origin;
    StepList steps;

    friend bool operator==(const EntityWalked&, const EntityWalked&) = default;
};

// The server accepted the walk of the player that started at `origin`, up to `destination`.
struct WalkConfirmed {
    Tile origin;
    Tile destination;

    friend bool operator==(const WalkConfirmed&, const WalkConfirmed&) = default;
};

struct PositionCorrected {
    std::uint32_t entity = 0;
    Tile position;

    friend bool operator==(const PositionCorrected&, const PositionCorrected&) = default;
};

enum class LeaveReason : std::uint8_t {
    LeftView,
    Disconnected,
};

struct EntityLeft {
    std::uint32_t entity = 0;
    LeaveReason reason = LeaveReason::LeftView;

    friend bool operator==(const EntityLeft&, const EntityLeft&) = default;
};

using ServerMessage =
    std::variant<MapEntered, EntityAppeared, EntityWalked, WalkConfirmed, PositionCorrected, EntityLeft>;

static_assert(std::is_trivially_copyable_v<ClientMessage>);
static_assert(std::is_trivially_copyable_v<ServerMessage>);

} // namespace demo_protocol
