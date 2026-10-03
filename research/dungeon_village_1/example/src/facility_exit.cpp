// Exit helpers model shared uses and satisfaction requests, not the entire exit sequence.
// Package responsibilities and evidence boundaries: ../README.md.

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

FacilityExitPositionResult prepare_facility_exit_position(const LegacyMap &map,
                                                          const FacilityPlacement &facility,
                                                          WorldPosition current) {
    const auto logical = character_world_cell(current);
    if (!valid_legacy_map(map) || !logical || facility.instance_id.value == 0 ||
        facility.definition_id < 0)
        return {FacilityExitError::invalid_input, std::nullopt};
    const auto footprint = facility_footprint(facility.shape, facility.orientation, facility.anchor,
                                              map.width, map.height);
    if (footprint.error != GeometryError::none)
        return {FacilityExitError::invalid_input, std::nullopt};
    const auto at = [&](Position p) -> const LegacyMapCell & {
        return map.cells[static_cast<std::size_t>(p.y) * map.width + p.x];
    };
    bool contains_current = false;
    for (const auto &part : footprint.cells) {
        const auto &binding = at(part.position).facility;
        if (!binding || !(binding->instance_id == facility.instance_id) ||
            binding->definition_id != facility.definition_id ||
            binding->fragment_index != part.fragment_index)
            return {FacilityExitError::invalid_input, std::nullopt};
        contains_current = contains_current || part.position == *logical;
    }
    // Reject a stale/partial placement even if its current fragment happens to match.
    const auto bindings = std::count_if(map.cells.begin(), map.cells.end(), [&](const auto &cell) {
        return cell.facility && cell.facility->instance_id == facility.instance_id;
    });
    if (!contains_current || static_cast<std::size_t>(bindings) != footprint.cells.size())
        return {FacilityExitError::invalid_input, std::nullopt};
    constexpr std::array<Position, 4> directions{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
    const auto exit_near = [&](Position p) -> std::optional<Position> {
        for (const auto d : directions) {
            const Position next{p.x + d.x, p.y + d.y};
            if (next.x >= 0 && next.x < map.width && next.y >= 0 && next.y < map.height &&
                (at(next).legacy_state == 3 || at(next).legacy_state == 4))
                return next;
        }
        return std::nullopt;
    };
    FacilityExitPositionCandidate result{facility.instance_id, *logical, current,
                                         FacilityExitPositionStatus::retained};
    if (facility.shape != FacilityShape::single && !exit_near(*logical)) {
        for (const auto &part : footprint.cells) {
            if (part.position == *logical)
                continue;
            const auto exit = exit_near(part.position);
            if (!exit)
                continue;
            const auto centre = character_waypoint(*exit, 4, 0);
            if (!centre.target)
                return {FacilityExitError::invalid_input, std::nullopt};
            result.logical_cell = *exit;
            result.position = *centre.target;
            result.status = FacilityExitPositionStatus::relocated;
            break;
        }
    }
    return {FacilityExitError::none, result};
}

OrdinaryExitTailResult
prepare_ordinary_exit_tail(int category, int detail,
                           const std::vector<FacilityAttributeEffect> &effects,
                           std::optional<int> ticket) {
    if ((category != 1 && category != 2) || detail != 0)
        return {FacilityExitError::unsupported_branch, std::nullopt};
    for (const auto &effect : effects)
        if (effect.attribute_index < 0 || effect.attribute_index >= 6)
            return {FacilityExitError::invalid_input, std::nullopt};
    OrdinaryExitTail result{category == 1, {{ExitDeferredAction::choose_activity, 0, 0}}};
    if (category == 2) {
        result.requests.push_back({ExitDeferredAction::expression, 9, 0});
    } else {
        result.requests.push_back({ExitDeferredAction::shop_marker, 1, 0});
        if (!effects.empty()) {
            if (!ticket)
                return {FacilityExitError::missing_ticket, std::nullopt};
            if (*ticket < 0 || static_cast<std::size_t>(*ticket) >= effects.size())
                return {FacilityExitError::invalid_ticket, std::nullopt};
            const auto &effect = effects[static_cast<std::size_t>(*ticket)];
            result.requests.push_back(
                {ExitDeferredAction::attribute, effect.attribute_index, effect.delta});
        }
    }
    return {FacilityExitError::none, std::move(result)};
}

} // namespace dungeon_village_reference
