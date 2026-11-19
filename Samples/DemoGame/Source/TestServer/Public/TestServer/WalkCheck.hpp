#pragma once

#include <cstdint>

#include <tl/expected.hpp>

#include "DemoProtocol/Messages.hpp"
#include "TestServer/ServerGrid.hpp"
#include "TestServer/ServerMotion.hpp"

namespace test_server {

// Why the server refused a walk.
enum class WalkRefusal : std::uint8_t {
    // The walk starts away from the route of the player and away from where the server believes it is.
    OriginTooFar,

    // The player could only be at the start of the walk by walking faster than the speed allows.
    TooFast,

    // A step does not touch the tile before it.
    NotNeighbor,

    // A step ends on a tile the server forbids.
    BlockedStep,

    // A diagonal step passes beside a forbidden tile.
    CutsCorner,
};

inline constexpr int kWalkRefusalCount = 5;

// How much the server forgives, so the delay of the network does not turn right walks into corrections.
struct WalkLimits {
    // A walk that starts away from the route may start this many steps from where the server believes the
    // player is.
    int origin_tolerance_steps = 0;

    // A walk may start this much earlier than the speed would allow. The client starts walking before the
    // server hears about it, and its walks start from the tile it is walking to.
    double early_tolerance_seconds = 0.0;
};

// Checks a walk of a player against the map and against how it was walking, and returns when the walk starts
// on the server. The server decides (INV-01): the client is only right when every rule agrees.
[[nodiscard]] tl::expected<double, WalkRefusal> check_walk(const demo_protocol::WalkRequest& request,
                                                           const ServerMotion& motion, double now,
                                                           const ServerGrid& grid,
                                                           const WalkLimits& limits) noexcept;

} // namespace test_server
