#pragma once
// Input-only fixture projection. All binding/footprint validation runs through product code.
#include "ark/facilities/exit.hpp"
#include "ark/facilities/map_binding.hpp"
#include <stdexcept>
namespace ark::test {
struct BindingFixture {
    facilities::Placement placement;
    int kind{};
};
struct BindingFixtureResult {
    std::optional<world::RouteMap> map;
};
inline BindingFixtureResult bind_fixture_map(world::RouteMap terrain,
                                             const std::vector<BindingFixture> &input) {
    std::vector<facilities::Definition> definitions;
    std::map<facilities::InstanceId, facilities::Instance> instances;
    for (const auto &f : input) {
        facilities::Definition d;
        d.id = f.placement.definition_id;
        d.shape = f.placement.shape;
        d.kind = f.kind;
        definitions.push_back(d);
        instances.emplace(f.placement.instance_id, facilities::Instance{f.placement.instance_id,
                                                                        d.id,
                                                                        f.placement.anchor,
                                                                        f.placement.orientation,
                                                                        0,
                                                                        false,
                                                                        {}});
    }
    return {facilities::bind_map(std::move(terrain), definitions, instances)};
}
} // namespace ark::test
