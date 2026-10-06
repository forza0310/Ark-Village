// Pure improvement transaction candidate: definition-shared attributes, inventory and instance
// event counters. Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/rules/facility_items.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
namespace {

bool fits_integer(std::int64_t value) {
    return value >= std::numeric_limits<std::int32_t>::min() &&
           value <= std::numeric_limits<std::int32_t>::max();
}

FacilityItemError economy_error(FacilityEconomyError error) {
    return error == FacilityEconomyError::numeric_overflow ? FacilityItemError::numeric_overflow
                                                           : FacilityItemError::invalid_input;
}

} // namespace

FacilityImprovementResult prepare_facility_improvement(const FacilityItemDefinition &facility,
                                                       const ImprovementItemDefinition &item,
                                                       const FacilityEconomyInput &shared_input) {
    if (facility.definition_id < 0 || facility.legacy_icon < 0 || item.item_id < 0 ||
        item.legacy_category < 0 ||
        static_cast<std::size_t>(item.legacy_category) >= facility.category_affinities.size() ||
        std::any_of(
            facility.category_affinities.begin(), facility.category_affinities.end(),
            [](std::int32_t value) { return value < 0 || value > 2; })) {
        return {FacilityItemError::invalid_input, std::nullopt};
    }
    const auto before = derive_facility_economy(facility.economy, shared_input);
    if (before.error != FacilityEconomyError::none) {
        return {economy_error(before.error), std::nullopt};
    }
    FacilityImprovementCandidate candidate;
    candidate.definition_improvements = shared_input.definition_improvements;
    const auto affinity =
        facility.category_affinities[static_cast<std::size_t>(item.legacy_category)];
    for (std::size_t slot = 0; slot < item.improvements.size(); ++slot) {
        std::int64_t delta = item.improvements[slot];
        if (affinity == 1) {
            delta /= 2;
        } else if (affinity == 2) {
            delta *= 2;
        }
        const auto improved = shared_input.definition_improvements[slot] + delta;
        if (!fits_integer(delta) || !fits_integer(improved)) {
            return {FacilityItemError::numeric_overflow, std::nullopt};
        }
        candidate.applied_improvements[slot] = static_cast<std::int32_t>(delta);
        candidate.definition_improvements[slot] = static_cast<std::int32_t>(improved);
    }
    auto next_input = shared_input;
    next_input.definition_improvements = candidate.definition_improvements;
    const auto after = derive_facility_economy(facility.economy, next_input);
    if (after.error != FacilityEconomyError::none) {
        return {economy_error(after.error), std::nullopt};
    }
    candidate.before = *before.values;
    candidate.after = *after.values;
    bool changed = false;
    for (std::size_t slot = 0; slot < candidate.visible_deltas.size(); ++slot) {
        candidate.visible_deltas[slot] =
            candidate.after.instance_attributes[slot] - candidate.before.instance_attributes[slot];
        changed = changed || candidate.visible_deltas[slot] != 0;
    }
    candidate.legacy_response = changed ? affinity : -1;
    return {FacilityItemError::none, candidate};
}

FacilityItemResult prepare_facility_item(const FacilityItemDefinition &facility,
                                         const ImprovementItemDefinition &item,
                                         const FacilityEconomyInput &shared_input,
                                         const FacilityEventCounters &instance_counters,
                                         std::int32_t inventory) {
    // 保留既有一体API的输入拒绝顺序；展示不变也照常扣库存、推进实例计数。
    if (facility.definition_id < 0 || facility.legacy_icon < 0 || item.item_id < 0 ||
        item.legacy_category < 0 ||
        static_cast<std::size_t>(item.legacy_category) >= facility.category_affinities.size() ||
        std::any_of(
            facility.category_affinities.begin(), facility.category_affinities.end(),
            [](std::int32_t value) { return value < 0 || value > 2; }) ||
        inventory < 0 || inventory > 999)
        return {FacilityItemError::invalid_input, std::nullopt};
    if (inventory == 0)
        return {FacilityItemError::no_inventory, std::nullopt};
    const auto improvement = prepare_facility_improvement(facility, item, shared_input);
    if (!improvement.candidate)
        return {improvement.error, std::nullopt};
    const auto event = confirm_facility_item(instance_counters, facility.legacy_icon);
    if (!event.transition)
        return {event.error == FacilityEventError::numeric_overflow
                    ? FacilityItemError::numeric_overflow
                    : FacilityItemError::invalid_input,
                std::nullopt};
    const auto &i = *improvement.candidate;
    return {FacilityItemError::none,
            FacilityItemCandidate{facility.definition_id, item.item_id, inventory - 1,
                                  i.definition_improvements, *event.transition,
                                  i.applied_improvements, i.visible_deltas, i.legacy_response,
                                  i.before, i.after}};
}

} // namespace ark::simulation::rules
