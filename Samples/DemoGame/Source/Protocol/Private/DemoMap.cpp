#include "DemoProtocol/DemoMap.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>

#include "Lemuria/Core/SeededRandom.hpp"

namespace demo_protocol {

namespace {

constexpr int kCoarseStep = 6;
constexpr std::uint32_t kCoarseRange = 1024;
constexpr std::size_t kTileCount = static_cast<std::size_t>(DemoMap::kSize) * DemoMap::kSize;

// The kinds of ground from the darkest patches to the rocks, with how many tiles each one covers.
struct GroundShare {
    TileKind kind;
    std::size_t tiles;
};

constexpr std::array<GroundShare, 5> kGroundShares{{
    {TileKind::DeepShadow, kTileCount * 30 / 100},
    {TileKind::Earth, kTileCount * 45 / 100},
    {TileKind::Moss, kTileCount * 17 / 100},
    {TileKind::Foliage, kTileCount * 5 / 100},
    {TileKind::Stone, kTileCount},
}};

// The strip of trail that only the server forbids.
constexpr int kTrailFirstX = 22;
constexpr int kTrailLastX = 42;
constexpr int kTrailFirstZ = 27;
constexpr int kTrailLastZ = 28;

// The area around the starting point is always free of rocks.
constexpr int kSpawnCenter = DemoMap::kSpawnCenter;
constexpr int kSpawnRadius = 4;

[[nodiscard]] std::size_t tile_index(int x, int z) noexcept {
    return static_cast<std::size_t>(z) * DemoMap::kSize + static_cast<std::size_t>(x);
}

[[nodiscard]] bool is_on_trail_strip(int x, int z) noexcept {
    return x >= kTrailFirstX && x <= kTrailLastX && z >= kTrailFirstZ && z <= kTrailLastZ;
}

[[nodiscard]] bool is_near_spawn(int x, int z) noexcept {
    return x >= kSpawnCenter - kSpawnRadius && x <= kSpawnCenter + kSpawnRadius &&
           z >= kSpawnCenter - kSpawnRadius && z <= kSpawnCenter + kSpawnRadius;
}

// Random values that change slowly across the map, so each kind of ground forms patches. Only whole numbers
// are used, so every compiler generates the same map.
[[nodiscard]] std::vector<std::uint32_t> patch_values(lemuria::core::SeededRandom& random) {
    constexpr int kCoarseSize = DemoMap::kSize / kCoarseStep + 2;
    constexpr std::uint32_t kStep = kCoarseStep;

    std::vector<std::uint32_t> coarse(static_cast<std::size_t>(kCoarseSize) * kCoarseSize);
    std::generate(coarse.begin(), coarse.end(), [&] { return random.next_below(kCoarseRange); });
    const auto coarse_at = [&](int x, int z) {
        return coarse[static_cast<std::size_t>(z) * kCoarseSize + static_cast<std::size_t>(x)];
    };

    std::vector<std::uint32_t> values(kTileCount);
    for (int z = 0; z < DemoMap::kSize; ++z) {
        for (int x = 0; x < DemoMap::kSize; ++x) {
            const int cell_x = x / kCoarseStep;
            const int cell_z = z / kCoarseStep;
            const auto part_x = static_cast<std::uint32_t>(x % kCoarseStep);
            const auto part_z = static_cast<std::uint32_t>(z % kCoarseStep);
            const std::uint32_t near_row =
                coarse_at(cell_x, cell_z) * (kStep - part_x) + coarse_at(cell_x + 1, cell_z) * part_x;
            const std::uint32_t far_row =
                coarse_at(cell_x, cell_z + 1) * (kStep - part_x) + coarse_at(cell_x + 1, cell_z + 1) * part_x;
            const std::uint32_t smooth = near_row * (kStep - part_z) + far_row * part_z;
            values[tile_index(x, z)] = smooth * 3 + random.next_below(kCoarseRange * kStep * kStep);
        }
    }
    return values;
}

// Gives each kind of ground its share of the tiles, from the lowest values to the highest.
[[nodiscard]] std::vector<TileKind> assign_ground(const std::vector<std::uint32_t>& values) {
    std::vector<std::size_t> order(values.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) { return values[a] < values[b]; });

    std::vector<TileKind> tiles(values.size(), TileKind::Stone);
    std::size_t rank = 0;
    for (const GroundShare& share : kGroundShares) {
        const std::size_t end = std::min(rank + share.tiles, order.size());
        for (; rank < end; ++rank) {
            tiles[order[rank]] = share.kind;
        }
    }
    return tiles;
}

} // namespace

DemoMap::DemoMap(std::uint64_t seed) {
    lemuria::core::SeededRandom random(seed);
    tiles_ = assign_ground(patch_values(random));

    for (int z = 0; z < kSize; ++z) {
        for (int x = 0; x < kSize; ++x) {
            TileKind& tile = tiles_[tile_index(x, z)];
            if (is_on_trail_strip(x, z)) {
                tile = TileKind::Trail;
            } else if (is_near_spawn(x, z) && tile == TileKind::Stone) {
                tile = TileKind::Earth;
            }
        }
    }
}

bool DemoMap::contains(int x, int z) const noexcept {
    return x >= 0 && x < kSize && z >= 0 && z < kSize;
}

TileKind DemoMap::kind_at(int x, int z) const noexcept {
    return contains(x, z) ? tiles_[tile_index(x, z)] : TileKind::Stone;
}

bool DemoMap::looks_walkable(int x, int z) const noexcept {
    return kind_at(x, z) != TileKind::Stone;
}

bool DemoMap::is_walkable_on_server(int x, int z) const noexcept {
    return looks_walkable(x, z) && !is_on_trail_strip(x, z);
}

} // namespace demo_protocol
