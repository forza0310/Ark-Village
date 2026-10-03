// Adapted from maintained research/example/facility_departure.hpp; no runtime research dependency.
#pragma once

// Ordinary facility branch only: callers must resolve task/rescue/object priority first.
// Tickets are injected, and a successful candidate does not move, reserve or charge an actor.
#include "ark/people/activity_choice.hpp"
#include "ark/people/facility_choice.hpp"

namespace ark::people {

struct FacilityDepartureInput {
    std::int32_t legacy_activity{};
    std::array<std::int64_t, 6> legacy_visit_counts{};
    std::uint32_t legacy_flags{};
    std::int64_t category_ticket{};
    std::int64_t facility_ticket{};
};

struct FacilityDeparture {
    std::int32_t category{};
    SnapshotFacilityTarget selection;
    world::ArrivalTarget binding;
    world::Route route;
    // Empty route preserves existing facing; numbers are legacy grid directions, not screen axes.
    std::optional<std::int32_t> legacy_direction;
};

enum class FacilityDepartureError {
    none,
    invalid_field,
    invalid_snapshot,
    invalid_input,
    unsupported_activity,
    no_category,
    unsupported_category,
    invalid_ticket,
    numeric_overflow,
    no_facility_weight,
    route_failure,
    binding_mismatch
};

struct FacilityDepartureResult {
    FacilityDepartureError error{FacilityDepartureError::none};
    world::RouteError route_error{world::RouteError::none};
    std::optional<FacilityDeparture> departure;
};

// Counts come from the snapshot, not an independently supplied count array. Preserve duplicate
// weights and the full-snapshot first goal. Failures never expose a partly prepared departure.
FacilityDepartureResult prepare_facility_departure(const world::DistanceField &field,
                                                   const ActivityCandidateSnapshot &snapshot,
                                                   const FacilityDepartureInput &input);

} // namespace ark::people
