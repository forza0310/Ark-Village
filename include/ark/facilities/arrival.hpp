// Adapted from research d7ca763 example/include/dungeon_village_reference/facility_arrival.hpp;
// independent product build.
#pragma once

// Ordinary arrival candidate separates visit counters, actor statistics, instance sales and cash
// income.

#include "ark/facilities/facility.hpp"
#include "ark/people/actor_id.hpp"

#include <array>

namespace ark::facilities {

struct FacilityArrivalState {
    std::array<std::int32_t, 6> legacy_visit_counts{};
    std::int32_t legacy_category_one_count{};
    std::int32_t legacy_category_six_counter{};
    std::optional<InstanceId> last_visited_instance;
    std::int32_t legacy_actor_total{};
    std::int32_t current_month_facility_sales{};
};

struct FacilityArrivalInput {
    people::ActorId character_id;
    InstanceId instance_id;
    std::int32_t definition_id{};
    std::int32_t legacy_kind{};
    std::int32_t legacy_category{};
    std::int32_t legacy_detail{};
    std::int32_t legacy_actor_kind{};
    std::uint32_t legacy_flags{};
    std::int32_t legacy_selection{-1};
    std::int32_t legacy_month_index{};
    std::int32_t resolved_instance_price{};
};

struct FacilityArrivalCandidate {
    people::ActorId character_id;
    InstanceId instance_id;
    std::int32_t definition_id{};
    std::int32_t legacy_month_index{};
    FacilityArrivalState state;
    std::int64_t cash_income{};
};

enum class FacilityArrivalError { none, invalid_input, unsupported_branch, numeric_overflow };

struct FacilityArrivalResult {
    FacilityArrivalError error{FacilityArrivalError::none};
    std::optional<FacilityArrivalCandidate> candidate;
};

// Return a candidate only; the owner must atomically commit all counters and cash, with arrival
// deduplication.
FacilityArrivalResult prepare_facility_arrival(const FacilityArrivalState &state,
                                               const FacilityArrivalInput &input);

} // namespace ark::facilities
