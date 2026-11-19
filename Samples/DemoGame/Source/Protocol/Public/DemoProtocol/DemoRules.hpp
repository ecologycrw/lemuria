#pragma once

// The rules of movement that the demo client and the test server must share, or the server would correct
// walks that the client believes are right.
namespace demo_protocol {

// How fast everyone walks. A diagonal step is longer than a straight one, by the square root of two, and
// takes longer in the same proportion.
//
// A diagonal step also needs both tiles beside it to be walkable, so it never cuts a corner.
inline constexpr double kWalkTilesPerSecond = 4.0;

} // namespace demo_protocol
