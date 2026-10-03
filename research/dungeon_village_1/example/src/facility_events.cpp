#include "dungeon_village_reference/facility_events.hpp"

#include <limits>

namespace dungeon_village_reference {
namespace {

bool valid(const FacilityEventCounters &counters) {
    return counters.item_confirmations >= 0 && counters.months_since_trigger >= 0;
}

} // namespace

FacilityEventResult advance_facility_event_months(const FacilityEventCounters &counters,
                                                  std::int64_t crossed_months) {
    if (!valid(counters) || crossed_months < 0) {
        return {FacilityEventError::invalid_input, std::nullopt};
    }
    if (crossed_months > std::numeric_limits<std::int64_t>::max() - counters.months_since_trigger) {
        return {FacilityEventError::numeric_overflow, std::nullopt};
    }
    auto next = counters;
    next.months_since_trigger += crossed_months;
    return {FacilityEventError::none, FacilityEventTransition{next, false}};
}

FacilityEventResult confirm_facility_item(const FacilityEventCounters &counters,
                                          std::int32_t legacy_icon) {
    if (!valid(counters) || legacy_icon < 0) {
        return {FacilityEventError::invalid_input, std::nullopt};
    }
    if (counters.item_confirmations == std::numeric_limits<std::int64_t>::max()) {
        return {FacilityEventError::numeric_overflow, std::nullopt};
    }
    auto next = counters;
    ++next.item_confirmations;
    const bool triggered = (legacy_icon == 2 || legacy_icon == 3) && next.item_confirmations >= 5 &&
                           next.months_since_trigger >= 36;
    if (triggered) {
        next = {};
    }
    return {FacilityEventError::none, FacilityEventTransition{next, triggered}};
}

FacilityPlanResult inspect_facility_legend_plan(std::int32_t definition_id,
                                                std::int32_t legacy_icon,
                                                const FacilityEventProgram &program) {
    if (definition_id < 0 || legacy_icon < 0) {
        return {FacilityPlanStatus::invalid_input, std::nullopt};
    }
    if (program.empty()) {
        return {FacilityPlanStatus::no_script, std::nullopt};
    }
    if (program.size() != 2 || program[0].size() != 2 || program[1].size() != 2 ||
        program[0][0] != 6 || program[0][1] < 0 || program[1][0] != 40 ||
        program[1][1] != definition_id || (legacy_icon != 2 && legacy_icon != 3)) {
        return {FacilityPlanStatus::unsupported, std::nullopt};
    }
    const auto mode = legacy_icon == 2 ? LegendPresentationMode::legacy_icon_2
                                       : LegendPresentationMode::legacy_icon_3;
    return {FacilityPlanStatus::supported, FacilityLegendPlan{definition_id, program[0][1], mode}};
}

} // namespace dungeon_village_reference
