// Product composition of research ai_perception and facility_departure, not an AI scheduler.
#include "ark/people/decision.hpp"
namespace ark::people {
DecisionResult prepare_decision(const world::DistanceField &field,
                                const ActivityCandidateSnapshot &snapshot,
                                const DepartureOverrideInput &priority,
                                const FacilityDepartureInput &ordinary) {
    if (!world::valid_field(field))
        return {DecisionError::invalid_field,
                FacilityDepartureError::none,
                world::RouteError::invalid_field,
                {}};
    if (!valid_activity_candidate_snapshot(snapshot) ||
        priority.activity != ordinary.legacy_activity || priority.flags != ordinary.legacy_flags ||
        priority.reachable.size() != snapshot.cells.size())
        return {DecisionError::invalid_input, {}, {}, {}};
    for (std::size_t i = 0; i < snapshot.cells.size(); ++i) {
        const auto &cell = snapshot.cells[i];
        if (priority.reachable[i] != cell.position || !field.map.contains(cell.position))
            return {DecisionError::invalid_input, {}, {}, {}};
        const auto index = field.map.index(cell.position);
        const auto &tile = field.map.cells[index];
        if (cell.cost != field.distances[index] ||
            cell.definition.definition_id != tile.definition_id ||
            bool(cell.instance) != bool(tile.facility) ||
            (cell.instance && cell.instance->instance_id != tile.facility->instance))
            return {DecisionError::invalid_input, {}, {}, {}};
    }
    const auto choice = prepare_departure_override(priority);
    if (!choice)
        return {DecisionError::invalid_input, {}, {}, {}};
    if (choice->kind != DepartureOverrideKind::ordinary)
        return {DecisionError::none, {}, {}, DecisionCandidate{*choice, {}}};
    const auto departure = prepare_facility_departure(field, snapshot, ordinary);
    if (!departure.departure)
        return {DecisionError::facility_refused, departure.error, departure.route_error, {}};
    return {DecisionError::none, {}, {}, DecisionCandidate{*choice, departure.departure}};
}
} // namespace ark::people
