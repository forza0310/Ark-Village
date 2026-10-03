#pragma once

// Definitions, shared definition progress and placed instances have separate ownership.
#include "ark/facilities/economy.hpp"
#include "ark/world/grid.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ark::facilities {
using InstanceId = std::uint64_t;
struct Modifier {
    int slot{};
    std::int64_t delta{};
};
struct Definition {
    int id{};
    std::string name;
    int kind{}, tab{-1}, price{}, construction_ticks{}, display_id{}, shape{};
    int icon{}, activity_category{}, activity_detail{};
    EconomyDefinition economy{};
    std::vector<Modifier> neighbours{};
    // Preserve z/A independently: several unopened definitions have nonparallel source lists.
    std::vector<int> effect_icons{}, effect_markers{};
};
struct Progress {
    int level{1};
    std::array<std::int32_t, 4> improvements{};
    std::uint64_t completed_uses{};
    bool upgrade_pending{};
}; // All definition-wide; no instance-local level or automatic upgrade from readiness.
struct Instance {
    InstanceId id{};
    int definition_id{};
    world::Cell anchor;
    int orientation{}, remaining_ticks{};
    bool source_seed{};
};
struct Binding {
    world::Cell cell;
    int fragment{};
};
// FACILITIES geometry: preserve binding order, including the negative-x rotated anchor.
// Invalid shape/orientation throws before any world mutation.
std::vector<Binding> footprint(int shape, int orientation, world::Cell anchor);
// Ordered external ring from research/geometry; includes diagonals and clips only the ring.
std::vector<world::Cell> surroundings(int shape, int orientation, world::Cell anchor,
                                      const world::SourceMap &map);
} // namespace ark::facilities
