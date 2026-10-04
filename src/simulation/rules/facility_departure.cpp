// Independent composition of maintained category, instance and path rules. Evidence and caller
// preconditions are linked from ../README.md; this is not a complete AI interpreter.
#include "ark/simulation/rules/facility_departure.hpp"

#include <utility>

namespace ark::simulation::rules {
namespace {
FacilityDepartureResult refuse(FacilityDepartureError error,
                               MapAccessError route_error = MapAccessError::none) {
    return {error, route_error, std::nullopt};
}
} // namespace

FacilityDepartureResult prepare_facility_departure(const LegacyDistanceField &field,
                                                   const ActivityCandidateSnapshot &snapshot,
                                                   const FacilityDepartureInput &input) {
    if (!valid_legacy_distance_field(field))
        return refuse(FacilityDepartureError::invalid_field, MapAccessError::invalid_field);
    if (!valid_activity_candidate_snapshot(snapshot))
        return refuse(FacilityDepartureError::invalid_snapshot);
    const auto plan = plan_activity_categories({input.legacy_activity, snapshot.category_counts,
                                                input.legacy_visit_counts, input.legacy_flags});
    if (plan.error != ActivityChoiceError::none)
        return refuse(plan.error == ActivityChoiceError::unsupported_activity
                          ? FacilityDepartureError::unsupported_activity
                          : FacilityDepartureError::invalid_input);

    int category{};
    if (plan.plan->forced_category) {
        category = *plan.plan->forced_category;
    } else {
        std::vector<std::int64_t> weights;
        for (const auto &option : plan.plan->options)
            weights.push_back(option.weight);
        const auto draw = select_weighted_ticket(weights, input.category_ticket);
        if (draw.error != WeightedTicketError::none)
            return refuse(draw.error == WeightedTicketError::no_weight
                              ? FacilityDepartureError::no_category
                          : draw.error == WeightedTicketError::numeric_overflow
                              ? FacilityDepartureError::numeric_overflow
                              : FacilityDepartureError::invalid_ticket);
        category = plan.plan->options[*draw.index].category;
    }
    if (category != 1 && category != 2 && category != 6 && category != 8)
        return refuse(FacilityDepartureError::unsupported_category);

    const auto selection = select_snapshot_facility(snapshot, category, input.facility_ticket);
    if (selection.error != SnapshotFacilityError::none)
        return refuse(selection.error == SnapshotFacilityError::no_weight
                          ? FacilityDepartureError::no_facility_weight
                      : selection.error == SnapshotFacilityError::numeric_overflow
                          ? FacilityDepartureError::numeric_overflow
                      : selection.error == SnapshotFacilityError::invalid_ticket
                          ? FacilityDepartureError::invalid_ticket
                          : FacilityDepartureError::invalid_snapshot);
    const auto &target = *selection.target;
    auto route = trace_legacy_path(field, target.goal.position);
    if (route.error != MapAccessError::none)
        return refuse(FacilityDepartureError::route_failure, route.error);
    // Reject mixing a candidate's old finite cost or identity with another map/field snapshot.
    if (target.goal.cost && *target.goal.cost != route.cost)
        return refuse(FacilityDepartureError::invalid_snapshot);
    const auto &instance = *target.goal.instance;
    const ArrivalBinding binding{target.goal.position, instance.instance_id,
                                 instance.definition_id};
    if (!arrival_binding_matches(field.map, binding, binding.goal))
        return refuse(FacilityDepartureError::binding_mismatch);

    std::optional<std::int32_t> direction;
    if (!route.steps.empty()) {
        const auto first = route.steps.front();
        direction = first.x > field.start.x   ? 1
                    : first.x < field.start.x ? 3
                    : first.y > field.start.y ? 0
                                              : 2;
    }
    return {FacilityDepartureError::none, MapAccessError::none,
            FacilityDeparture{category, target, binding, std::move(route), direction}};
}
} // namespace ark::simulation::rules
