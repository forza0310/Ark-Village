#include "startup_world_facility_update_private.hpp"

#include "ark/simulation/world/startup_world_runtime.hpp"
#include <stdexcept>

namespace ark::simulation {
std::optional<StartupFacilityUpdateStep>
try_consume_startup_world_facility_update(StartupWorldRuntimeState &owner, std::uint64_t identity,
                                          const rules::WorldScriptCatalog &catalog) {
    const auto facility = owner.scene.world.world.facilities.find(identity);
    const auto progress = owner.dungeon_facilities.find(identity);
    const auto detail = owner.facility_details.find(identity);
    if (!identity || facility == owner.scene.world.world.facilities.end() ||
        progress == owner.dungeon_facilities.end() || detail == owner.facility_details.end())
        return {};
    const auto definition =
        owner.facility_definitions.find(facility->second.placement.definition_id);
    if (definition == owner.facility_definitions.end())
        return {};

    // Classify without incrementing an int (the existing rule owns overflow and
    // notice validation). Completion and real dungeon callbacks retain the full
    // consumer, including fresh shared-state reads at each actual callback point.
    const auto next_updates = static_cast<std::int64_t>(progress->second.updates) + 1;
    if ((facility->second.status == 0 && next_updates >= detail->second.construction_limit) ||
        (facility->second.status == 1 && facility->second.category == 5 &&
         !facility->second.occupants.empty()) ||
        (facility->second.status == 2 && next_updates >= 10))
        return {};

    if (!valid_startup_world_facility_projection(owner))
        throw std::invalid_argument("共同人物缺原任务旗标投影");

    // This branch of c/m only visits this instance, its definition, and the full
    // script validator. Keep the original rule as the sole update/notice oracle;
    // omit unrelated world/maps/routes, never script pages or continuations.
    rules::WorldFacilityUpdateState projected;
    projected.finish.dungeon.world.facilities.emplace(*facility);
    projected.finish.dungeon.facilities.emplace(*progress);
    projected.details.emplace(*detail);
    projected.definitions.emplace(*definition);
    projected.finish.event_calls = owner.scripts.event_calls;
    projected.finish.dungeon.world.ai.pending_completion =
        owner.scene.world.world.ai.pending_completion;
    projected.scripts = startup_world_runtime_scripts(owner);
    projected.random = owner.scene.random;
    projected.cycle_length = owner.clock_parameter;
    const auto result = rules::prepare_world_facility_update(projected, identity, catalog);
    if (!result.candidate)
        return StartupFacilityUpdateStep{result.error, false};

    // Do the legacy writer's unrelated normalization before applying the new
    // detail notices: its route projection still sees the old shop notice view.
    // A failure is safe only because the outer frame discards this private owner.
    if (!normalize_startup_world_facility_writeback(owner) ||
        !write_startup_world_runtime_scripts(owner, result.candidate->state.scripts))
        return StartupFacilityUpdateStep{rules::WorldFacilityUpdateError::none, false};
    const auto &next = result.candidate->state;
    owner.scene.world.world.facilities.at(identity) =
        next.finish.dungeon.world.facilities.at(identity);
    owner.dungeon_facilities.at(identity) = next.finish.dungeon.facilities.at(identity);
    owner.facility_details.at(identity) = next.details.at(identity);
    owner.facility_definitions.at(definition->first) = next.definitions.at(definition->first);
    owner.scene.random = next.random;
    return StartupFacilityUpdateStep{rules::WorldFacilityUpdateError::none, true};
}
} // namespace ark::simulation
