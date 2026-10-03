#pragma once

// Definitions, shared definition progress and placed instances have separate ownership.
#include "ark/world/grid.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace ark::facilities {
using InstanceId = std::uint64_t;
struct Definition {
    int id{};
    std::string name;
    int kind{}, tab{-1}, price{}, construction_ticks{}, display_id{}, shape{};
};
struct Progress {
    int level{1};
}; // Definition-wide; no instance-local upgrade state.
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
} // namespace ark::facilities
