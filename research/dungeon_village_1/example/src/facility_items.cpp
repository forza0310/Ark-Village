#include "dungeon_village_reference/facility_items.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
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

FacilityItemResult prepare_facility_item(const FacilityItemDefinition &facility,
                                         const ImprovementItemDefinition &item,
                                         const FacilityEconomyInput &shared_input,
                                         const FacilityEventCounters &instance_counters,
                                         std::int32_t inventory) {
    if (facility.definition_id < 0 || facility.legacy_icon < 0 || item.item_id < 0 ||
        item.legacy_category < 0 ||
        static_cast<std::size_t>(item.legacy_category) >= facility.category_affinities.size() ||
        std::any_of(
            facility.category_affinities.begin(), facility.category_affinities.end(),
            [](std::int32_t value) { return value < 0 || value > 2; }) ||
        inventory < 0 || inventory > 999) {
        return {FacilityItemError::invalid_input, std::nullopt};
    }
    if (inventory == 0) {
        return {FacilityItemError::no_inventory, std::nullopt};
    }
    const auto before = derive_facility_economy(facility.economy, shared_input);
    if (before.error != FacilityEconomyError::none) {
        return {economy_error(before.error), std::nullopt};
    }
    FacilityItemCandidate candidate;
    candidate.definition_id = facility.definition_id;
    candidate.item_id = item.item_id;
    candidate.remaining_inventory = inventory - 1;
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
    const auto event = confirm_facility_item(instance_counters, facility.legacy_icon);
    if (event.error != FacilityEventError::none) {
        return {event.error == FacilityEventError::numeric_overflow
                    ? FacilityItemError::numeric_overflow
                    : FacilityItemError::invalid_input,
                std::nullopt};
    }
    candidate.before = *before.values;
    candidate.after = *after.values;
    candidate.instance_event = *event.transition;
    bool changed = false;
    for (std::size_t slot = 0; slot < candidate.visible_deltas.size(); ++slot) {
        candidate.visible_deltas[slot] =
            candidate.after.instance_attributes[slot] - candidate.before.instance_attributes[slot];
        changed = changed || candidate.visible_deltas[slot] != 0;
    }
    candidate.legacy_response = changed ? affinity : -1;
    return {FacilityItemError::none, candidate};
}

} // namespace dungeon_village_reference
