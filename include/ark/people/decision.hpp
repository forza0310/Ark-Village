#pragma once
// Compose the researched priority gate with ordinary selection; no world or queue mutation.
#include "ark/people/ai_perception.hpp"
#include "ark/people/departure.hpp"
namespace ark::people {
struct DecisionCandidate {
    DepartureOverrideCandidate priority;
    std::optional<FacilityDeparture> facility; // Present only for successful ordinary selection.
};
enum class DecisionError { none, invalid_input, invalid_field, facility_refused };
struct DecisionResult {
    DecisionError error{DecisionError::none};
    FacilityDepartureError facility_error{FacilityDepartureError::none};
    world::RouteError route_error{world::RouteError::none};
    std::optional<DecisionCandidate> candidate;
};
// Snapshot must be collected from this field/activity. Reachable override entries come from the
// same ordered candidate cells, not every road/ground cell in the distance field. All nearest
// targets and ownership observations are caller-resolved; a special target is only a handoff.
DecisionResult prepare_decision(const world::DistanceField &field,
                                const ActivityCandidateSnapshot &snapshot,
                                const DepartureOverrideInput &priority,
                                const FacilityDepartureInput &ordinary);
} // namespace ark::people
