#include "ark/simulation/facilities/rules/world_facilities.hpp"

#include <algorithm>
#include <cmath>

namespace ark::simulation::rules {
namespace {
bool live(const RescueWorldState &s, CharacterId id) {
    const auto a = s.ai.battle.actors.find(id);
    if (!id.value || a == s.ai.battle.actors.end() || !(a->second.id == id) ||
        !s.actors.count(id) || !s.ai.contexts.count(id))
        return false;
    const auto &roster = a->second.kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order;
    return std::count(roster.begin(), roster.end(), id) == 1;
}
const RescueFacility *bound(const RescueWorldState &s, CharacterId id) {
    const auto &b = s.actors.at(id).binding;
    if (!b)
        return nullptr;
    const auto f = s.facilities.find(b->instance_id.value);
    if (f == s.facilities.end() || !(f->second.placement.instance_id == b->instance_id) ||
        f->second.placement.definition_id != b->definition_id ||
        !arrival_binding_matches(s.map, *b, s.ai.contexts.at(id).cell))
        return nullptr;
    return &f->second;
}
bool expression(RescueWorldState &s, CharacterId id, int type, int delay,
                const WorldExpressionTicket &t) {
    auto &effects = s.ai.contexts.at(id).effects;
    const auto r =
        prepare_actor_expression({effects, type, delay, t.probability, t.variant_count, t.variant});
    if (!r.candidate)
        return false;
    effects = r.candidate->state;
    return true;
}
} // namespace
WorldFacilityUseResult prepare_world_facility_use(const RescueWorldState &s,
                                                  const WorldFacilityUseInput &i) {
    if (!live(s, i.actor))
        return {RescueWorldError::stale_actor, {}, false};
    const auto *f = bound(s, i.actor);
    if (!f)
        return {RescueWorldError::stale_binding, {}, false};
    const auto &a = s.ai.battle.actors.at(i.actor);
    if (a.kind == ActorKind::monster && !(f->category == 6 && f->detail == 3))
        return {RescueWorldError::invalid_input, {}, false}; // P() has different other categories.
    auto direction = i.direction_ticket;
    if (f->category == 8 && f->detail == 2 && !direction && i.draw) {
        try {
            direction = i.draw(4);
        } catch (...) {
            return {RescueWorldError::preparation_failed, {}, false};
        }
    }
    auto target = i.world_target;
    if (f->category == 8 && f->detail == 2 && !target && direction && i.direction_target) {
        try {
            target = i.direction_target(*direction);
        } catch (...) {
            return {RescueWorldError::preparation_failed, {}, false};
        }
    }
    const auto plan = prepare_facility_use_plan(
        {a.control, f->category, f->detail, i.mode, f->definition_wait,
         s.actors.at(i.actor).visits.legacy_category_six_counter, target, direction});
    if (!plan.candidate)
        return {RescueWorldError::preparation_failed, {}, false};
    if (plan.candidate->cleanup) {
        const auto cleanup = prepare_world_rescue_cleanup(s, i.actor);
        if (!cleanup.candidate)
            return {cleanup.error, {}, false};
        return {RescueWorldError::none, cleanup.candidate->state, false};
    }
    auto next = s;
    auto &actor = next.ai.battle.actors.at(i.actor);
    actor.control = plan.candidate->control;
    if (plan.candidate->reset_state_counter_and_parameter)
        actor.state_counter = actor.state_parameter = 0;
    if (plan.candidate->clear_encounter)
        actor.encounter.reset();
    if (actor.control.state == 0 && plan.candidate->reset_state_counter_and_parameter)
        actor.baseline = 0;
    next.actors.at(i.actor).visits.legacy_category_six_counter =
        plan.candidate->category_six_counter;
    return {RescueWorldError::none, next, plan.candidate->ground_effect20};
}
WorldFacilityControlResult prepare_world_facility_control(const RescueWorldState &s,
                                                          const WorldFacilityControlInput &i) {
    const auto failed = [](RescueWorldError e) -> WorldFacilityControlResult { return {e, {}}; };
    if (!live(s, i.actor))
        return failed(RescueWorldError::stale_actor);
    if (i.domain_limit == 0)
        return failed(RescueWorldError::invalid_input);
    WorldFacilityControlCandidate c;
    c.state = s;
    std::size_t domains{};
    for (;;) {
        const auto before = c.state.ai.battle.actors.at(i.actor).control.queue;
        const auto simple = prepare_world_inn_control(c.state, i.actor, i.domain_limit - domains);
        if (!simple.candidate)
            return failed(simple.error);
        c.state = simple.candidate->state;
        c.occupied |= simple.candidate->occupied;
        c.exited |= simple.candidate->exited;
        c.cleaned_up |= simple.candidate->cleaned_up;
        c.flow = simple.candidate->flow;
        auto &a = c.state.ai.battle.actors.at(i.actor);
        if (!before.empty() && before.front()[0] == 21 && before != a.control.queue &&
            ++domains >= i.domain_limit)
            break;
        if (a.control.queue.empty() || c.flow == ActorControlFlow::waiting || c.exited ||
            c.cleaned_up)
            break;
        const auto command = a.control.queue.front();
        switch (command[0]) {
        case 0: {
            const auto step = advance_character_motion(
                {a.position.x, a.position.z},
                {static_cast<float>(command[1]), static_cast<float>(command[2])}, a.control.flags);
            if (!step.step)
                return failed(RescueWorldError::invalid_input);
            a.position.x = step.step->position.x;
            a.position.z = step.step->position.z;
            if (step.step->velocity)
                c.state.actors.at(i.actor).horizontal_velocity = *step.step->velocity;
            // v moves n but leaves s/t/u to the later d projection, including on arrival.
            if (!step.step->waypoint_overlap) {
                c.flow = ActorControlFlow::moving;
                return {RescueWorldError::none, c};
            }
            break;
        }
        case 2: {
            if (command[1] != 15)
                return {RescueWorldError::none, c};
            ActorStateTransitionInput transition;
            transition.control = a.control;
            transition.human = a.kind == ActorKind::human;
            transition.baseline = a.baseline;
            transition.next_state = 15;
            const auto r = prepare_actor_state_transition(transition);
            if (!r)
                return failed(RescueWorldError::preparation_failed);
            a.control = r->control;
            a.state_counter = a.state_parameter = 0;
            if (r->clear_encounter)
                a.encounter.reset();
            continue;
        }
        case 18:
            if (c.consumed_expressions >= i.expressions.size() ||
                !expression(c.state, i.actor, command[1], command[2],
                            i.expressions[c.consumed_expressions]))
                return failed(RescueWorldError::preparation_failed);
            ++c.consumed_expressions;
            break;
        case 22:
            if (command[2] < 0 || command[2] >= 2147483647)
                return failed(RescueWorldError::invalid_input);
            a.position.height = static_cast<float>(command[1]);
            a.physics_pause = command[2];
            break;
        case 23: {
            if (c.consumed_launch_tickets >= i.launch_tickets.size())
                return failed(RescueWorldError::preparation_failed);
            const int ticket = i.launch_tickets[c.consumed_launch_tickets++];
            if (ticket < 0 || ticket >= 4)
                return failed(RescueWorldError::invalid_input);
            const auto old_cell = c.state.ai.contexts.at(i.actor).cell;
            const auto target = character_waypoint({old_cell.x, old_cell.y - ticket - 7}, 4, 0);
            if (!target.target || !std::isfinite(a.position.x) || !std::isfinite(a.position.z))
                return failed(RescueWorldError::invalid_input);
            a.attack_destination.x = target.target->x;
            a.attack_destination.z = target.target->z;
            c.state.actors.at(i.actor).horizontal_velocity = {
                (target.target->x - a.position.x) / 60.0F,
                (target.target->z - a.position.z) / 60.0F};
            a.vertical_velocity = 240.0F / 29.0F; // d.a(120,30), not a physical-time launch.
            break;
        }
        default:
            return {RescueWorldError::none, c};
        }
        a.control.queue.erase(a.control.queue.begin());
        if (++domains >= i.domain_limit)
            break;
    }
    return {RescueWorldError::none, c};
}
WorldFacilityExecutionResult
prepare_world_facility_execution(const RescueWorldState &s, const WorldFacilityExecutionInput &i) {
    const auto prefix = prepare_world_execution_prefix(s.ai, i.control.actor);
    if (!prefix.candidate)
        return {RescueWorldError::preparation_failed, {}, {}, {}};
    auto next = s;
    next.ai = prefix.candidate->state;
    if (prefix.candidate->request_carry_expression &&
        (!i.carry_expression || !expression(next, i.control.actor, 17, 0, *i.carry_expression)))
        return {RescueWorldError::preparation_failed, {}, {}, {}};
    const auto control = prepare_world_facility_control(next, i.control);
    if (!control.candidate)
        return {control.error, {}, {}, {}};
    return {RescueWorldError::none, control.candidate, prefix.candidate->sounds,
            prefix.candidate->growth_requests};
}
WorldSpecialEntryResult prepare_world_special_entry_c(const RescueWorldState &s, CharacterId id,
                                                      TownBounds town,
                                                      std::optional<WorldExpressionTicket> ticket) {
    if (!live(s, id))
        return {RescueWorldError::stale_actor, {}};
    const auto &a = s.ai.battle.actors.at(id);
    if (a.control.state != 15 || a.state_counter < 0 || town.left >= town.right ||
        town.top >= town.bottom)
        return {RescueWorldError::invalid_input, {}};
    const auto cell = s.ai.contexts.at(id).cell;
    const bool inside =
        cell.x > town.left && cell.x < town.right && cell.y > town.top && cell.y < town.bottom;
    const auto velocity = s.actors.at(id).horizontal_velocity;
    const auto lifecycle = prepare_timed_lifecycle({a.kind,
                                                    15,
                                                    a.state_counter,
                                                    s.actors.at(id).monster_mode,
                                                    0,
                                                    a.control.flags,
                                                    a.object_slot != -1,
                                                    inside,
                                                    s.ai.battle.events.count(90) != 0,
                                                    {a.position.x, a.position.z},
                                                    velocity,
                                                    a.position.height,
                                                    a.hp.displayed,
                                                    a.capacity});
    if (!lifecycle.candidate)
        return {RescueWorldError::preparation_failed, {}};
    WorldSpecialEntryCandidate c{s, a.state_counter == 60, a.state_counter >= 70};
    auto &actor = c.state.ai.battle.actors.at(id);
    actor.position.x = lifecycle.candidate->position.x;
    actor.position.z = lifecycle.candidate->position.z;
    actor.position.height = lifecycle.candidate->height;
    c.state.actors.at(id).horizontal_velocity = lifecycle.candidate->horizontal_velocity;
    if (lifecycle.candidate->zero_vertical_velocity)
        actor.vertical_velocity = 0;
    if (lifecycle.candidate->monster_mode)
        c.state.actors.at(id).monster_mode = *lifecycle.candidate->monster_mode;
    for (const auto &request : lifecycle.candidate->requests) {
        if (request.kind == LifecycleRequestKind::expression) {
            if (!ticket || !expression(c.state, id, request.parameter, 0, *ticket))
                return {RescueWorldError::preparation_failed, {}};
        } else if (request.kind == LifecycleRequestKind::state) {
            ActorStateTransitionInput input;
            input.control = actor.control;
            input.human = actor.kind == ActorKind::human;
            input.baseline = actor.baseline;
            input.next_state = request.parameter;
            const auto state = prepare_actor_state_transition(input);
            if (!state)
                return {RescueWorldError::preparation_failed, {}};
            actor.control = state->control;
            actor.baseline = state->baseline;
            actor.state_counter = actor.state_parameter = 0;
            if (state->clear_encounter)
                actor.encounter.reset();
        } else if (request.kind == LifecycleRequestKind::activity)
            actor.control.queue.push_back({8, request.parameter});
        else
            return {RescueWorldError::preparation_failed, {}};
    }
    return {RescueWorldError::none, c};
}
} // namespace ark::simulation::rules
