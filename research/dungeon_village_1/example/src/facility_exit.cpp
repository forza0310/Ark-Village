#include "dungeon_village_reference/facility_exit.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {

FacilityUseResult prepare_facility_use_completion(std::int32_t definition_id,
                                                  LevelEndpoints upgrade_uses,
                                                  const FacilityUseProgress &progress) {
    if (definition_id < 0 || progress.level < 1 || progress.level > 5 ||
        progress.completed_uses < 0 || upgrade_uses.first < 0 || upgrade_uses.fifth < 0) {
        return {FacilityExitError::invalid_input, std::nullopt};
    }
    if (progress.completed_uses == std::numeric_limits<std::int32_t>::max()) {
        return {FacilityExitError::numeric_overflow, std::nullopt};
    }
    FacilityEconomyDefinition definition;
    definition.upgrade_uses = upgrade_uses;
    FacilityEconomyInput input;
    input.level = progress.level;
    input.completed_definition_uses = static_cast<std::uint64_t>(progress.completed_uses) + 1;
    const auto values = derive_facility_economy(definition, input);
    if (values.error != FacilityEconomyError::none || !values.values) {
        return {FacilityExitError::invalid_input, std::nullopt};
    }
    auto next = progress;
    ++next.completed_uses;
    next.upgrade_pending = next.upgrade_pending || values.values->upgrade_ready;
    return {FacilityExitError::none,
            FacilityUseCandidate{definition_id, next, values.values->upgrade_uses}};
}

FacilitySatisfactionResult prepare_facility_satisfaction(const FacilitySatisfactionInput &input) {
    if (input.character_id.value == 0 || input.instance_id.value == 0 || input.definition_id < 0 ||
        input.satisfaction < 0 || input.satisfaction > 100 || input.ticket < 0 ||
        input.ticket >= 10) {
        return {FacilityExitError::invalid_input, std::nullopt};
    }
    const auto first = static_cast<std::int64_t>(input.legacy_job_thresholds[0]);
    const auto last = static_cast<std::int64_t>(input.legacy_job_thresholds[1]);
    const auto threshold = first + input.satisfaction * (last - first) / 100 + input.ticket - 5;
    const std::int32_t gain = threshold <= input.resolved_instance_quality ? 1 : 0;
    const auto satisfaction = std::min(input.satisfaction + gain, 100);
    return {FacilityExitError::none,
            FacilitySatisfactionCandidate{
                input.character_id, input.instance_id, input.definition_id, threshold, satisfaction,
                satisfaction - input.satisfaction, PopularityRequest{gain, false, 0, 10}}};
}

} // namespace dungeon_village_reference
