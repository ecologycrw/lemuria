#pragma once

#include <cstdint>
#include <vector>

namespace demo_protocol {

enum class TileKind : std::uint8_t { DeepShadow, Earth, Moss, Foliage, Trail, Stone };

// The only map of the demonstration. The client and the test server generate the same map from the same seed.
class DemoMap {
public:
    static constexpr int kSize = 64;
    static constexpr std::uint64_t kDefaultSeed = 20261118;

    explicit DemoMap(std::uint64_t seed = kDefaultSeed);

    [[nodiscard]] bool contains(int x, int z) const noexcept;

    // Anything outside the map counts as stone.
    [[nodiscard]] TileKind kind_at(int x, int z) const noexcept;

    // What the client believes: only stone blocks the way.
    [[nodiscard]] bool looks_walkable(int x, int z) const noexcept;

    // What the server decides: it also forbids the trail strip, so the demonstration can show a correction.
    [[nodiscard]] bool is_walkable_on_server(int x, int z) const noexcept;

private:
    std::vector<TileKind> tiles_;
};

} // namespace demo_protocol
