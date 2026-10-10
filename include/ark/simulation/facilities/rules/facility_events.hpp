#pragma once

// Instance item/month counters and a narrow legend plan recognizer; no generic script interpreter.

#include <cstdint>
#include <optional>
#include <vector>

namespace ark::simulation::rules {

struct FacilityEventCounters {
    std::int64_t item_confirmations{};
    std::int64_t months_since_trigger{};
};

struct FacilityEventTransition {
    FacilityEventCounters counters;
    bool script_triggered{};
};

enum class FacilityEventError { none, invalid_input, numeric_overflow };
struct FacilityEventResult {
    FacilityEventError error{FacilityEventError::none};
    std::optional<FacilityEventTransition> transition;
};

// Advance only month counters; crossing a month does not trigger the script by itself.
FacilityEventResult advance_facility_event_months(const FacilityEventCounters &counters,
                                                  std::int64_t crossed_months);
// Return counter resets and a trigger request after the verified icon-specific gates.
FacilityEventResult confirm_facility_item(const FacilityEventCounters &counters,
                                          std::int32_t legacy_icon);

using FacilityEventProgram = std::vector<std::vector<std::int32_t>>;

enum class LegendPresentationMode { legacy_icon_2, legacy_icon_3 };
struct FacilityLegendPlan {
    std::int32_t definition_id{};
    std::int32_t delay_ticks{};
    LegendPresentationMode mode{LegendPresentationMode::legacy_icon_2};
};

enum class FacilityPlanStatus { supported, no_script, unsupported, invalid_input };
struct FacilityPlanResult {
    FacilityPlanStatus status{FacilityPlanStatus::unsupported};
    std::optional<FacilityLegendPlan> plan;
};

// Recognize supported delay/presentation patterns; distinguish no_script, unsupported and
// invalid_input.
FacilityPlanResult inspect_facility_legend_plan(std::int32_t definition_id,
                                                std::int32_t legacy_icon,
                                                const FacilityEventProgram &program);

} // namespace ark::simulation::rules
