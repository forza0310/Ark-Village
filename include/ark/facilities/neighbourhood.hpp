#pragma once

// Derived view only, adapted from research/example/neighbourhood. Stable source identity is
// retained even when its modifier list is empty; this is not a second mutable state store.
#include "ark/facilities/facility.hpp"
#include <map>
namespace ark::facilities {
struct NeighbourSource {
    InstanceId instance{};
    int definition_id{};
};
struct Neighbourhood {
    InstanceId instance{};
    std::array<std::int64_t, 3> modifiers{};
    std::vector<NeighbourSource> sources;
    std::size_t road_cells{};
};
// Fully validates identities/footprints/roads, then recomputes once per source/target pair.
std::map<InstanceId, Neighbourhood>
derive_neighbourhood(const std::vector<Definition> &definitions,
                     const std::map<InstanceId, Instance> &instances,
                     const std::vector<world::Cell> &roads, const world::SourceMap &map);
EconomyInput with_neighbours(const EconomyInput &base, const Neighbourhood &neighbours);
} // namespace ark::facilities
