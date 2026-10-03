#pragma once

// Immutable, reset-only published tile evidence. Runtime bindings come from current instances.
#include "ark/world/navigation.hpp"
namespace ark::world {
struct LoadedCell {
    int definition_id{}, legacy_state{};
    RouteCategory category{};
    int display_id{}, variant{}, road_mask{}, boundary_fragment{-1}, external_direction{-1};
    bool road_quad{}, edge_road_pair{};
    int legacy_instance_id{-1};  // -1 means absent; raw zero is valid.
    std::uint64_t instance_id{}; // Zero means absent; raw+1 applies only to this reset snapshot.
};
} // namespace ark::world
