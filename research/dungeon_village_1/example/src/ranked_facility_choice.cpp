// Earlier sorted-cell selector; snapshot_facility_choice preserves the later two-view contract.
// Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_reference/ranked_facility_choice.hpp"

#include "dungeon_village_reference/activity_choice.hpp"

#include <map>
#include <set>
#include <tuple>

namespace dungeon_village_reference {

RankedFacilityResult select_ranked_facility(const std::vector<RankedFacilityCell> &candidates,
                                            std::int32_t category, std::int64_t ticket) {
    if (category != 1 && category != 2 && category != 6 && category != 8) {
        return {RankedFacilityError::unsupported_category, std::nullopt};
    }
    if (candidates.size() > 1000000) {
        return {RankedFacilityError::invalid_input, std::nullopt};
    }
    std::map<BuildingId, std::pair<std::int32_t, std::int32_t>> instances;
    std::map<std::int32_t, std::pair<std::int32_t, std::int64_t>> definitions;
    std::set<std::pair<int, int>> coordinates;
    std::vector<std::int64_t> weights;
    weights.reserve(candidates.size());
    std::int64_t previous_cost = 0;
    for (const auto &cell : candidates) {
        if (cell.position.x < 0 || cell.position.y < 0 || cell.instance_id.value == 0 ||
            cell.definition_id < 0 || cell.legacy_category < 0 || cell.legacy_category >= 11 ||
            cell.legacy_phase < 0 || cell.definition_charm < 0 || cell.cost < previous_cost ||
            !coordinates.emplace(cell.position.x, cell.position.y).second) {
            return {RankedFacilityError::invalid_input, std::nullopt};
        }
        const auto instance_value = std::make_pair(cell.definition_id, cell.legacy_phase);
        const auto instance = instances.emplace(cell.instance_id, instance_value);
        const auto definition_value = std::make_pair(cell.legacy_category, cell.definition_charm);
        const auto definition = definitions.emplace(cell.definition_id, definition_value);
        if ((!instance.second && instance.first->second != instance_value) ||
            (!definition.second && definition.first->second != definition_value)) {
            return {RankedFacilityError::invalid_input, std::nullopt};
        }
        previous_cost = cell.cost;
        weights.push_back(
            cell.legacy_phase == 1 && cell.legacy_category == category ? cell.definition_charm : 0);
    }
    const auto selection = select_weighted_ticket(weights, ticket);
    if (selection.error != WeightedTicketError::none) {
        const auto error = selection.error == WeightedTicketError::no_weight
                               ? RankedFacilityError::no_weight
                           : selection.error == WeightedTicketError::numeric_overflow
                               ? RankedFacilityError::numeric_overflow
                           : selection.error == WeightedTicketError::invalid_ticket
                               ? RankedFacilityError::invalid_ticket
                               : RankedFacilityError::invalid_input;
        return {error, std::nullopt};
    }
    const auto drawn = *selection.index;
    const auto instance_id = candidates[drawn].instance_id;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (candidates[index].instance_id == instance_id) {
            return {RankedFacilityError::none,
                    RankedFacilityTarget{drawn, index, candidates[index]}};
        }
    }
    return {RankedFacilityError::invalid_input, std::nullopt};
}

} // namespace dungeon_village_reference
