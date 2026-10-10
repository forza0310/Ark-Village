// Full snapshot selection separates the drawn active index, snapshot index and final goal index.
// Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/ai/rules/snapshot_facility_choice.hpp"

#include "ark/simulation/ai/rules/activity_choice.hpp"

namespace ark::simulation::rules {

SnapshotFacilityResult select_snapshot_facility(const ActivityCandidateSnapshot &snapshot,
                                                std::int32_t category, std::int64_t ticket) {
    if (category != 1 && category != 2 && category != 6 && category != 8) {
        return {SnapshotFacilityError::unsupported_category, std::nullopt};
    }
    if (!valid_activity_candidate_snapshot(snapshot)) {
        return {SnapshotFacilityError::invalid_snapshot, std::nullopt};
    }
    // Duplicate cells, including event appends, each contribute weight; do not deduplicate
    // instances.
    std::vector<std::size_t> active_indices;
    std::vector<std::int64_t> weights;
    active_indices.reserve(snapshot.cells.size());
    weights.reserve(snapshot.cells.size());
    for (std::size_t index = 0; index < snapshot.cells.size(); ++index) {
        const auto &cell = snapshot.cells[index];
        if (cell.instance && cell.instance->legacy_phase == 1) {
            const auto &definition = candidate_instance_definition(cell);
            active_indices.push_back(index);
            weights.push_back(definition.legacy_category == category ? definition.definition_charm
                                                                     : 0);
        }
    }
    const auto draw = select_weighted_ticket(weights, ticket);
    if (draw.error != WeightedTicketError::none) {
        const auto error = draw.error == WeightedTicketError::no_weight
                               ? SnapshotFacilityError::no_weight
                           : draw.error == WeightedTicketError::numeric_overflow
                               ? SnapshotFacilityError::numeric_overflow
                           : draw.error == WeightedTicketError::invalid_ticket
                               ? SnapshotFacilityError::invalid_ticket
                               : SnapshotFacilityError::invalid_snapshot;
        return {error, std::nullopt};
    }
    const auto drawn_active = *draw.index;
    const auto drawn_snapshot = active_indices[drawn_active];
    // The drawn cell determines identity, not the final destination in the full snapshot.
    const auto instance_id = snapshot.cells[drawn_snapshot].instance->instance_id;
    for (std::size_t index = 0; index < snapshot.cells.size(); ++index) {
        const auto &cell = snapshot.cells[index];
        if (cell.instance && cell.instance->instance_id == instance_id) {
            return {SnapshotFacilityError::none,
                    SnapshotFacilityTarget{drawn_active, drawn_snapshot, index, cell}};
        }
    }
    return {SnapshotFacilityError::invalid_snapshot, std::nullopt};
}

} // namespace ark::simulation::rules
