#pragma once

// Loaded first-play tile state, separate from immutable source pixels and prototype identity.
// See rules/STARTUP.md; this is a static reconstruction, not an APK runtime capture.
#include "ark/simulation/rules/map_access.hpp"

namespace ark::simulation {
struct StartupEvidence;

struct LoadedStartupCell {
    int definition_id{};
    int legacy_state{};
    ark::simulation::rules::RouteCategory category{};
    int display_id{};
    int variant{};
    int road_mask{};
    int boundary_fragment{-1};
    int external_direction{-1};
    bool road_quad{};
    bool edge_road_pair{};
    std::optional<int> legacy_instance_id;
};
struct LoadedStartupInstance {
    int legacy_id{}; // Includes zero and may be reused by the original allocator.
    int definition_id{};
    ark::simulation::rules::Position anchor;
};
struct LoadedStartupMap {
    int width{};
    int height{};
    std::vector<LoadedStartupCell> cells;
    // Original vector order after entrance replacement, not numeric-ID order.
    std::vector<LoadedStartupInstance> instances;
};

// Restricted to the published single-cell, level-zero, no-inheritance first-play input.
// Reject unsupported evidence rather than silently generating a plausible map.
LoadedStartupMap reconstruct_startup_map(const StartupEvidence &evidence);
// The mapping raw_id + 1 is ONLY for this reset snapshot; later prototype IDs stay monotonic.
ark::simulation::rules::LegacyMap startup_route_map(const LoadedStartupMap &map);
} // namespace ark::simulation
