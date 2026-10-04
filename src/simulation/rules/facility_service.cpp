#include "ark/simulation/rules/facility_service.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
namespace {
bool valid_control(const ActorControlState &c) {
    if (c.state < 0 || c.state > 20 || c.action < 0 || c.action > 11 || c.action_counter < 0 ||
        c.alternate_counter < 0 || c.facing < 0 || c.facing > 3)
        return false;
    return std::all_of(c.queue.begin(), c.queue.end(), valid_actor_control);
}
FacilityServiceError exit_error(FacilityExitError e) {
    if (e == FacilityExitError::numeric_overflow)
        return FacilityServiceError::numeric_overflow;
    if (e == FacilityExitError::missing_ticket)
        return FacilityServiceError::missing_ticket;
    if (e == FacilityExitError::invalid_ticket)
        return FacilityServiceError::invalid_ticket;
    return FacilityServiceError::invalid_input;
}
} // namespace
ResolvedArrivalResult prepare_resolved_arrival(const ResolvedArrivalInput &i) {
    auto a = i.arrival;
    if (a.legacy_selection < -2)
        return {FacilityServiceError::invalid_input, std::nullopt};
    if (a.legacy_selection == -2)
        return {FacilityServiceError::unresolved_selection, std::nullopt};
    const bool equipment = a.legacy_detail == 1 || a.legacy_detail == 4 || a.legacy_detail == 5;
    if (equipment && (!i.equipment_id || !i.equipment_price))
        return {FacilityServiceError::unresolved_selection, std::nullopt};
    if (equipment && (*i.equipment_id < 0 || *i.equipment_price < 0))
        return {FacilityServiceError::invalid_input, std::nullopt};
    if (!equipment && (i.equipment_id || i.equipment_price))
        return {FacilityServiceError::invalid_input, std::nullopt};
    const bool delivered =
        a.legacy_selection >= 0 && (a.legacy_category == 1 || a.legacy_category == 7);
    if (delivered && !i.carried_object_exists)
        return {FacilityServiceError::unresolved_selection, std::nullopt};
    ResolvedArrivalCandidate c;
    c.object_slot = delivered ? -1 : a.legacy_selection;
    if (delivered)
        c.delivered_object = a.legacy_selection;
    if (equipment) {
        c.selected_equipment = i.equipment_id;
        c.selected_detail = a.legacy_detail;
        a.resolved_instance_price = *i.equipment_price;
    }
    const int price = a.resolved_instance_price;
    // Reuse ordinary counters/guards, only AFTER explicitly resolving special preconditions.
    a.legacy_selection = -1;
    a.legacy_detail = equipment ? 0 : a.legacy_detail;
    if (delivered && price > 0 && (a.legacy_flags & (512U | 256U)) == 0 &&
        a.legacy_actor_kind == 0) {
        const auto total = static_cast<std::int64_t>(price) + 5000;
        if (total > std::numeric_limits<int>::max())
            return {FacilityServiceError::numeric_overflow, std::nullopt};
        a.resolved_instance_price = static_cast<int>(total);
    }
    const auto arrival = prepare_facility_arrival(i.statistics, a);
    if (!arrival.candidate)
        return {arrival.error == FacilityArrivalError::numeric_overflow
                    ? FacilityServiceError::numeric_overflow
                    : FacilityServiceError::invalid_input,
                std::nullopt};
    c.arrival = *arrival.candidate;
    return {FacilityServiceError::none, c};
}
FacilityUsePlanResult prepare_facility_use_plan(const FacilityUsePlanInput &i) {
    if (!valid_control(i.control) || i.category < 0 || i.category > 10 || i.detail < 0 ||
        i.activity < 0 || i.activity > 8 || i.definition_wait < 0 || i.category_six_counter < 0)
        return {FacilityServiceError::invalid_input, std::nullopt};
    FacilityUsePlan c{i.control, i.category_six_counter};
    c.control.queue.clear();
    const auto state = [&](int v) {
        c.control.state = v;
        c.control.flags &= ~16U;
        c.control.alternate_counter = 0;
        c.reset_state_counter_and_parameter = true;
        c.clear_encounter = true;
        if (v == 0) {
            c.control.action = 0;
            c.control.action_counter = 0;
        }
    };
    const auto append = [&](LegacyActorControl v) { c.control.queue.push_back(std::move(v)); };
    const auto ordinary = [&] {
        state(14);
        append({6, 1});
        append({21});
    };
    switch (i.category) {
    case 1:
    case 7:
    case 9:
        ordinary();
        append({1, i.definition_wait, 0});
        append({24});
        if (i.category == 9)
            c.control.flags &= ~(512U | 1024U | 32768U);
        break;
    case 2:
        ordinary();
        if (i.activity <= 1) {
            append({6, 32});
            append({1, 200, 0});
        }
        append({24});
        c.control.flags &= ~(512U | 1024U | 32768U);
        break;
    case 4:
        c.category_six_counter = 0;
        state(0);
        append({8, 6});
        break;
    case 5:
        ordinary();
        break;
    case 6:
        if (i.detail != 3)
            break;
        if (!i.world_target)
            return {FacilityServiceError::invalid_input, std::nullopt};
        append({6, 1});
        append({0, i.world_target->x, i.world_target->y});
        append({1, 20, 0});
        c.ground_effect20 = true;
        append({7, 1});
        append({23});
        append({2, 15});
        break;
    case 8:
        if (i.detail != 2)
            break;
        if (!i.direction_ticket)
            return {FacilityServiceError::missing_ticket, std::nullopt};
        if (*i.direction_ticket < 0 || *i.direction_ticket >= 4)
            return {FacilityServiceError::invalid_ticket, std::nullopt};
        if (!i.world_target)
            return {FacilityServiceError::invalid_input, std::nullopt};
        state(14);
        append({0, i.world_target->x, i.world_target->y});
        for (const auto &v : std::vector<LegacyActorControl>{{6, 1},
                                                             {7, 2},
                                                             {22, 30, 110},
                                                             {4, *i.direction_ticket},
                                                             {1, 12, 0},
                                                             {7, 1},
                                                             {1, 6, 0},
                                                             {18, 15, 20},
                                                             {3, 11},
                                                             {1, 70, 0},
                                                             {3, 0},
                                                             {1, 10, 0},
                                                             {6, 1},
                                                             {22, 0, 0},
                                                             {1, 12, 0},
                                                             {7, 1},
                                                             {6, 2},
                                                             {24}})
            append(v);
        break;
    case 10:
        c.cleanup = true;
        break;
    default:
        break; // Categories0/3 do nothing after w; duplicated category4 tail is unreachable.
    }
    return {FacilityServiceError::none, c};
}
FacilityServiceExitResult prepare_facility_service_exit(const FacilityServiceExitInput &i) {
    if (i.actor.value == 0 || !valid_control(i.control) || i.control.queue.empty() ||
        i.control.queue.front()[0] != 24 || i.category < 0 || i.category > 10 || i.detail < 0)
        return {FacilityServiceError::invalid_input, std::nullopt};
    FacilityServiceExitCandidate c;
    c.control = i.control;
    c.control.queue.erase(c.control.queue.begin());
    c.occupants = i.occupants;
    if (!i.binding_valid) {
        c.cleanup = prepare_actor_cleanup(ActorKind::human, i.control.flags);
        if (!c.cleanup)
            return {FacilityServiceError::invalid_input, std::nullopt};
        c.control.queue.clear();
        return {FacilityServiceError::none, c};
    }
    const auto position = prepare_facility_exit_position(i.map, i.facility, i.position);
    if (!position.candidate)
        return {exit_error(position.error), std::nullopt};
    c.position = position.candidate;
    const auto use =
        prepare_facility_use_completion(i.facility.definition_id, i.upgrade_uses, i.progress);
    if (!use.candidate)
        return {exit_error(use.error), std::nullopt};
    c.shared_use = use.candidate;
    c.control.flags &= ~(33U | 16U);
    const auto occupant = std::find(c.occupants.begin(), c.occupants.end(), i.actor);
    if (occupant != c.occupants.end())
        c.occupants.erase(occupant);
    c.control.state = c.control.action = c.control.action_counter = c.control.alternate_counter = 0;
    c.control.queue = {{8, 0}};
    c.order = {FacilityExitCommit::position,      FacilityExitCommit::shared_use,
               FacilityExitCommit::clear_flags33, FacilityExitCommit::release_occupation,
               FacilityExitCommit::reset_control, FacilityExitCommit::enqueue_activity};
    if (i.category == 2)
        c.control.queue.push_back({18, 9, 0});
    else if (i.category == 1) {
        if (i.detail == 1 || i.detail == 4 || i.detail == 5) {
            auto e = i.equipment;
            e.category = i.category;
            e.detail = i.detail;
            const auto queue = prepare_equipment_exit_tail(e);
            if (!queue)
                return {FacilityServiceError::unresolved_selection, std::nullopt};
            c.control.queue = *queue;
        } else {
            const auto &s = i.satisfaction;
            if (!(s.character_id == i.actor) || !(s.instance_id == i.facility.instance_id) ||
                s.definition_id != i.facility.definition_id)
                return {FacilityServiceError::invalid_input, std::nullopt};
            const auto satisfaction = prepare_facility_satisfaction(s);
            if (!satisfaction.candidate)
                return {exit_error(satisfaction.error), std::nullopt};
            c.satisfaction = satisfaction.candidate;
            c.order.push_back(FacilityExitCommit::satisfaction);
            c.control.queue.push_back({20, 1});
            if (!i.effects.empty()) {
                if (!i.effect_ticket)
                    return {FacilityServiceError::missing_ticket, std::nullopt};
                if (*i.effect_ticket < 0 ||
                    static_cast<std::size_t>(*i.effect_ticket) >= i.effects.size())
                    return {FacilityServiceError::invalid_ticket, std::nullopt};
                for (const auto &e : i.effects)
                    if (e.attribute_index < 0 || e.attribute_index >= 6)
                        return {FacilityServiceError::invalid_input, std::nullopt};
                const auto &e = i.effects[*i.effect_ticket];
                c.control.queue.push_back({19, 6, e.attribute_index, e.delta});
            }
        }
    } else if (i.category == 9) {
        c.home_hp_and_visits = true;
        c.order.push_back(FacilityExitCommit::home_hp_and_visits);
    }
    return {FacilityServiceError::none, c};
}
} // namespace ark::simulation::rules
