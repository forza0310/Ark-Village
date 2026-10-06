#pragma once

// Pure improvement transaction candidate: definition-shared attributes, inventory and instance
// event counters.

#include "dungeon_village_reference/facility_economy.hpp"
#include "dungeon_village_reference/facility_events.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace dungeon_village_reference {

struct FacilityItemDefinition {
    std::int32_t definition_id{};
    std::int32_t legacy_icon{};
    std::vector<std::int32_t> category_affinities;
    FacilityEconomyDefinition economy;
};

struct ImprovementItemDefinition {
    std::int32_t item_id{};
    std::int32_t legacy_category{};
    std::array<std::int32_t, 3> improvements{};
};

struct FacilityItemCandidate {
    std::int32_t definition_id{};
    std::int32_t item_id{};
    std::int32_t remaining_inventory{};
    std::array<std::int32_t, 4> definition_improvements{};
    FacilityEventTransition instance_event;
    std::array<std::int32_t, 3> applied_improvements{};
    std::array<std::int64_t, 3> visible_deltas{};
    std::int32_t legacy_response{};
    FacilityEconomyValues before;
    FacilityEconomyValues after;
};

enum class FacilityItemError { none, invalid_input, no_inventory, numeric_overflow };
struct FacilityItemResult {
    FacilityItemError error{FacilityItemError::none};
    std::optional<FacilityItemCandidate> candidate;
};

// raw76初始化只提交共享改良；raw75此前已经消耗库存并推进实例事件。
// 与一体候选共用计算，避免Owner复制适配、封顶和实际显示差值规则。
struct FacilityImprovementCandidate {
    std::array<std::int32_t, 4> definition_improvements{};
    std::array<std::int32_t, 3> applied_improvements{};
    std::array<std::int64_t, 3> visible_deltas{};
    std::int32_t legacy_response{};
    FacilityEconomyValues before;
    FacilityEconomyValues after;
};
struct FacilityImprovementResult {
    FacilityItemError error{FacilityItemError::none};
    std::optional<FacilityImprovementCandidate> candidate;
};
FacilityImprovementResult prepare_facility_improvement(const FacilityItemDefinition &facility,
                                                       const ImprovementItemDefinition &item,
                                                       const FacilityEconomyInput &shared_input);

// Consume one item even when capped display values do not change; the owner commits the complete
// candidate.
FacilityItemResult prepare_facility_item(const FacilityItemDefinition &facility,
                                         const ImprovementItemDefinition &item,
                                         const FacilityEconomyInput &shared_input,
                                         const FacilityEventCounters &instance_counters,
                                         std::int32_t inventory);

} // namespace dungeon_village_reference
