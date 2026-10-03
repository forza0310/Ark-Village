#pragma once
// Pure full-footprint binding over unbound terrain; this is not construction approval.
#include "ark/facilities/facility.hpp"
#include "ark/world/navigation.hpp"
#include <map>
namespace ark::facilities {
world::RouteMap bind_map(world::RouteMap terrain, const std::vector<Definition> &definitions,
                         const std::map<InstanceId, Instance> &instances);
} // namespace ark::facilities
