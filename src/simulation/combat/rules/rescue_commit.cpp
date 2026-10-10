#include "ark/simulation/combat/rules/rescue_commit.hpp"
#include "ark/simulation/actors/rules/world_detached_actor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace ark::simulation::rules {
namespace {
RescueWorldResult fail(RescueWorldError e) { return {e, std::nullopt}; }
bool human(const RescueWorldState &s, CharacterId id) {
    const auto a = s.ai.battle.actors.find(id);
    return id.value && a != s.ai.battle.actors.end() && a->second.id == id &&
           a->second.kind == ActorKind::human && s.actors.count(id) && s.ai.contexts.count(id) &&
           std::find(s.ai.human_order.begin(), s.ai.human_order.end(), id) !=
               s.ai.human_order.end();
}
std::optional<std::vector<RescueTargetSnapshot>> rescue_targets(const RescueWorldState &s) {
    std::vector<RescueTargetSnapshot> people;
    for (const auto id : s.ai.human_order) {
        if (!human(s, id))
            return std::nullopt;
        const auto &a = s.ai.battle.actors.at(id);
        const auto &ctx = s.ai.contexts.at(id);
        people.push_back({id,
                          a.control.state,
                          {a.position.x, a.position.z},
                          ctx.cell,
                          {},
                          ctx.inside_town,
                          s.actors.at(id).on_event_cell,
                          a.hp.target < a.capacity / 2});
    }
    return people;
}
bool valid_box(CollisionBox b) {
    return std::isfinite(b.x_offset) && std::isfinite(b.z_offset) && std::isfinite(b.width) &&
           std::isfinite(b.depth) && b.width > 0 && b.depth > 0 && std::abs(b.x_offset) < 1000000 &&
           std::abs(b.z_offset) < 1000000 && b.width < 1000000 && b.depth < 1000000;
}
bool touching(CombatPoint a, CollisionBox ab, CombatPoint b, CollisionBox bb) {
    const float ax = a.x + ab.x_offset, az = a.z + ab.z_offset;
    const float bx = b.x + bb.x_offset, bz = b.z + bb.z_offset;
    return ax <= bx + bb.width && bx <= ax + ab.width && az - ab.depth <= bz && bz - bb.depth <= az;
}
bool transition(BattleActorRecord &a, int state) {
    ActorStateTransitionInput i;
    i.control = a.control;
    i.human = a.kind == ActorKind::human;
    i.baseline = a.baseline;
    i.next_state = state;
    const auto c = prepare_actor_state_transition(i);
    if (!c || c->request_cleanup)
        return false;
    a.control = c->control;
    a.baseline = c->baseline;
    a.state_counter = a.state_parameter = 0;
    if (c->clear_encounter)
        a.encounter.reset();
    if (c->reset_attack_count)
        a.attack_count = 0;
    return true;
}
RescueFacility *bound(RescueWorldState &s, CharacterId id) {
    const auto &binding = s.actors.at(id).binding;
    if (!binding)
        return nullptr;
    const auto f = s.facilities.find(binding->instance_id.value);
    if (f == s.facilities.end() || f->second.placement.definition_id != binding->definition_id ||
        !(f->second.placement.instance_id == binding->instance_id) ||
        !arrival_binding_matches(s.map, *binding, s.ai.contexts.at(id).cell))
        return nullptr;
    return &f->second;
}
void release_first(RescueFacility &f, CharacterId id) {
    const auto it = std::find(f.occupants.begin(), f.occupants.end(), id);
    if (it != f.occupants.end())
        f.occupants.erase(it);
}
RescueFacility *occupation_at_goal(RescueWorldState &s, CharacterId id) {
    const auto &binding = s.actors.at(id).binding;
    if (!binding || binding->goal.x < 0 || binding->goal.x >= s.map.width || binding->goal.y < 0 ||
        binding->goal.y >= s.map.height || !valid_legacy_map(s.map))
        return nullptr;
    const auto &cell =
        s.map.cells[static_cast<std::size_t>(binding->goal.y * s.map.width + binding->goal.x)];
    if (!cell.facility)
        return nullptr;
    const auto f = s.facilities.find(cell.facility->instance_id.value);
    return f == s.facilities.end() ? nullptr : &f->second;
}
bool cleanup(RescueWorldState &s, CharacterId id) {
    auto &a = s.ai.battle.actors.at(id);
    const auto c = prepare_actor_cleanup(a.kind, a.control.flags);
    if (!c)
        return false;
    if (auto *f = bound(s, id))
        release_first(*f, id);
    a.rescue.reset();
    a.encounter.reset();
    a.group.reset();
    a.position.height = 0;
    a.control.flags = c->flags;
    a.control.queue.clear();
    if (!transition(a, c->state))
        return false;
    // r invokes c19 (preserves k/l), not n0. Monster c0 already resets k/l in transition.
    if (c->waiting_updates)
        a.control.queue.push_back({1, c->waiting_updates, 0});
    a.control.queue.push_back({8, c->activity});
    return true;
}
FacilityArrivalState statistics(const RescueWorldState &s, CharacterId id,
                                const RescueFacility &f) {
    auto v = s.actors.at(id).visits;
    v.legacy_actor_total = s.human_spending.at(s.ai.battle.actors.at(id).definition);
    v.current_month_facility_sales = f.sales;
    return v;
}
void store_visits(RescueWorldState &s, CharacterId id, FacilityArrivalState visits) {
    // Drop owner projections so a later same-definition visitor cannot read a stale B2/sales.
    visits.legacy_actor_total = visits.current_month_facility_sales = 0;
    s.actors.at(id).visits = visits;
}
FacilityArrivalInput arrival(const RescueWorldState &s, CharacterId id, const RescueFacility &f) {
    const auto &a = s.ai.battle.actors.at(id);
    return {id,
            f.placement.instance_id,
            f.placement.definition_id,
            f.kind,
            f.category,
            f.detail,
            0,
            a.control.flags,
            a.object_slot,
            s.month_index,
            f.price};
}
bool use(RescueWorldState &s, CharacterId id, int mode, const RescueFacility &f,
         std::optional<Position> target = {}, std::optional<int> direction = {}) {
    auto &a = s.ai.battle.actors.at(id);
    FacilityUsePlanInput i;
    i.control = a.control;
    i.category = f.category;
    i.detail = f.detail;
    i.activity = mode;
    i.definition_wait = f.definition_wait;
    i.world_target = target;
    i.direction_ticket = direction;
    const auto plan = prepare_facility_use_plan(i);
    if (!plan.candidate || plan.candidate->cleanup)
        return false;
    a.control = plan.candidate->control;
    if (plan.candidate->reset_state_counter_and_parameter)
        a.state_counter = a.state_parameter = 0;
    if (plan.candidate->clear_encounter)
        a.encounter.reset();
    return true;
}
} // namespace
RescueWorldResult prepare_world_rescue_bind(const RescueWorldState &s, CharacterId id,
                                            std::optional<CharacterId> target, bool touching) {
    if (!human(s, id) || (target && (!human(s, *target) || *target == id)))
        return fail(RescueWorldError::stale_actor);
    bool available = false;
    for (const auto &[key, f] : s.facilities) {
        (void)key;
        available |= f.status == 1 && f.category == 2;
    }
    const auto &a = s.ai.battle.actors.at(id);
    RescueBindingInput i;
    i.rescuer = id;
    i.rescuer_state = a.control.state;
    i.object_slot = a.object_slot;
    i.rescue_enabled = available;
    i.target = target;
    i.touching = touching;
    if (target) {
        const auto &other = s.ai.battle.actors.at(*target);
        i.target_state = other.control.state;
        i.target_position = {other.position.x, other.position.z};
    }
    const auto r = prepare_rescue_binding(i);
    if (!r.candidate)
        return fail(RescueWorldError::preparation_failed);
    RescueWorldCandidate c;
    c.state = s;
    c.binding_action = r.candidate->action;
    c.requests = r.candidate->requests;
    auto &actor = c.state.ai.battle.actors.at(id);
    if (c.binding_action == RescueBindingAction::bind) {
        auto &other = c.state.ai.battle.actors.at(*target);
        if (!transition(other, 16) || !transition(actor, 0))
            return fail(RescueWorldError::preparation_failed);
        other.rescue = id;
        actor.rescue = *target;
        actor.object_slot = -2;
        c.state.actors.at(id).path_pending = false;
        c.state.actors.at(id).journey.reset();
        c.state.actors.at(id).unbound_route.reset();
        c.state.actors.at(id).waypoint = 0;
        actor.control.queue.push_back({8, 4});
    } else if (c.binding_action == RescueBindingAction::baseline) {
        const auto baseline =
            prepare_actor_baseline_restore(actor.control, actor.baseline, true, 0);
        if (!baseline)
            return fail(RescueWorldError::preparation_failed);
        actor.control = baseline->control;
        if (baseline->clear_encounter)
            actor.encounter.reset();
    }
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_rescue_seek(const RescueWorldState &s, CharacterId id,
                                            CollisionBox actor_box, CollisionBox rescue_box) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    if (!valid_box(actor_box) || !valid_box(rescue_box))
        return fail(RescueWorldError::invalid_input);
    const auto people = rescue_targets(s);
    if (!people)
        return fail(RescueWorldError::invalid_input);
    const auto &a = s.ai.battle.actors.at(id);
    const auto nearest = select_rescue_target({a.position.x, a.position.z}, *people);
    if (nearest.error != ActorAiError::none)
        return fail(RescueWorldError::preparation_failed);
    const auto target =
        nearest.candidate ? std::optional<CharacterId>(nearest.candidate->id) : std::nullopt;
    const auto r = prepare_world_rescue_bind(
        s, id, target,
        target &&
            touching(a.position, actor_box, s.ai.battle.actors.at(*target).position, rescue_box));
    if (!r.candidate || r.candidate->binding_action != RescueBindingAction::chase)
        return r;
    auto c = *r.candidate;
    const auto &other = s.ai.battle.actors.at(*target);
    const auto step = advance_character_motion(
        {a.position.x, a.position.z}, {other.position.x, other.position.z}, a.control.flags);
    if (!step.step)
        return fail(RescueWorldError::preparation_failed);
    auto &actor = c.state.ai.battle.actors.at(id);
    actor.position.x = step.step->position.x;
    actor.position.z = step.step->position.z;
    if (step.step->velocity)
        c.state.actors.at(id).horizontal_velocity = *step.step->velocity;
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_rescue_return(const RescueWorldState &s, CharacterId id,
                                              const ActivityCandidateInput &view) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    const auto &a = s.ai.battle.actors.at(id);
    if (a.object_slot != -2 || !a.rescue || !human(s, *a.rescue) ||
        !(s.ai.battle.actors.at(*a.rescue).rescue == id) || a.control.queue.empty() ||
        a.control.queue.front() != LegacyActorControl{8, 4})
        return fail(RescueWorldError::invalid_input);
    const auto cell = character_world_cell({a.position.x, a.position.z});
    if (!cell)
        return fail(RescueWorldError::invalid_input);
    const auto field = search_legacy_map(s.map, *cell);
    if (!field.field)
        return fail(RescueWorldError::preparation_failed);
    auto input = view;
    input.legacy_activity = 4;
    input.last_visited_instance = s.actors.at(id).visits.last_visited_instance;
    input.instances.clear();
    for (const auto &[key, f] : s.facilities) {
        if (key != f.placement.instance_id.value)
            return fail(RescueWorldError::invalid_input);
        input.instances.push_back({f.placement.instance_id, f.placement.definition_id, f.status});
    }
    const auto snapshot = collect_activity_candidates(*field.field, input);
    const auto people = rescue_targets(s);
    if (!snapshot.snapshot || !people)
        return fail(RescueWorldError::preparation_failed);
    DepartureOverrideInput priority;
    priority.self = id;
    priority.flags = a.control.flags;
    priority.activity = 4;
    priority.has_object = true;
    priority.active_task = s.ai.task_active;
    priority.definition_task_flag = s.actors.at(id).definition_task_flag;
    for (const auto &candidate : snapshot.snapshot->cells)
        priority.reachable.push_back(candidate.position);
    const auto down = select_rescue_target({a.position.x, a.position.z}, *people);
    if (down.error != ActorAiError::none)
        return fail(RescueWorldError::preparation_failed);
    if (down.candidate)
        priority.nearest_down = s.ai.contexts.at(down.candidate->id).cell;
    for (const auto person : s.ai.human_order) {
        const auto &binding = s.actors.at(person).binding;
        if (binding)
            priority.people.push_back({person, binding->goal});
    }
    if (s.object_order.size() != s.ai.battle.objects.size())
        return fail(RescueWorldError::invalid_input);
    std::set<std::uint64_t> seen;
    std::vector<ObjectProbe> objects;
    for (const auto key : s.object_order) {
        if (!seen.insert(key).second || !s.ai.battle.objects.count(key))
            return fail(RescueWorldError::invalid_input);
        const auto &o = s.ai.battle.objects.at(key);
        objects.push_back({o.id, o.state, o.position});
    }
    const auto object = select_ground_object(a.position, objects);
    if (object.error != ObjectError::none)
        return fail(RescueWorldError::preparation_failed);
    if (object.selected)
        priority.nearest_object = s.ai.battle.objects.at(object.selected->value).cached_cell;
    const auto override = prepare_departure_override(priority);
    if (!override)
        return fail(RescueWorldError::preparation_failed);
    RescueWorldCandidate c;
    c.state = s;
    c.departure_override = override;
    if (override->kind != DepartureOverrideKind::ordinary)
        return {RescueWorldError::none, c}; // Typed handoff, not invented inn fallback.
    const ActivityCandidateCell *best = nullptr;
    for (const auto &candidate : snapshot.snapshot->cells)
        if (candidate.definition.legacy_category == 2 && candidate.cost &&
            (!best || *candidate.cost < *best->cost))
            best = &candidate;
    if (!best || !best->instance)
        return fail(RescueWorldError::preparation_failed);
    const auto route = trace_legacy_path(*field.field, best->position);
    if (route.error != MapAccessError::none)
        return fail(RescueWorldError::preparation_failed);
    const auto index = static_cast<std::size_t>(best - snapshot.snapshot->cells.data());
    FacilityDeparture journey{
        2,
        {index, index, index, *best},
        {best->position, best->instance->instance_id, best->definition.definition_id},
        route,
        {}};
    if (!route.steps.empty()) {
        const auto first = route.steps.front();
        journey.legacy_direction = first.x > cell->x   ? 1
                                   : first.x < cell->x ? 3
                                   : first.y > cell->y ? 0
                                                       : 2;
    }
    auto &ctx = c.state.actors.at(id);
    ctx.journey = journey;
    ctx.unbound_route.reset();
    ctx.waypoint = 0;
    ctx.path_pending = true;
    ctx.binding = journey.binding;
    auto &actor = c.state.ai.battle.actors.at(id);
    const auto prefix = prepare_local_control_prefix(actor.control, {{}, true});
    if (!prefix.candidate)
        return fail(RescueWorldError::preparation_failed);
    actor.control = prefix.candidate->state;
    if (journey.legacy_direction)
        actor.control.facing = *journey.legacy_direction;
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_rescue_path_c(const RescueWorldState &s, CharacterId id) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    const auto &ctx = s.actors.at(id);
    const auto &a = s.ai.battle.actors.at(id);
    if (!ctx.journey || !ctx.path_pending || a.control.state != 0)
        return fail(RescueWorldError::invalid_input);
    const auto status =
        inspect_facility_entry(s.map, ctx.journey->binding, {a.position.x, a.position.z}, true);
    if (status == FacilityEntryStatus::ready) {
        const auto arrival = prepare_world_rescue_delivery(s, id);
        if (!arrival.candidate)
            return arrival;
        auto c = *arrival.candidate;
        c.state.actors.at(id).journey.reset();
        c.state.actors.at(id).path_pending = false;
        return {RescueWorldError::none, c};
    }
    if (status != FacilityEntryStatus::not_entered ||
        ctx.waypoint >= ctx.journey->route.steps.size())
        return fail(RescueWorldError::stale_binding);
    const auto cell = ctx.journey->route.steps[ctx.waypoint];
    const auto &tile = s.map.cells.at(static_cast<std::size_t>(cell.y * s.map.width + cell.x));
    // Intermediate6/7 entry offsets need source definition direction; this closed interval rejects
    // that unresolved metadata instead of inventing an entrance. Ordinary roads/ground use center.
    if (tile.legacy_state == 6 || tile.legacy_state == 7)
        return fail(RescueWorldError::invalid_input);
    const auto waypoint = character_waypoint(cell, tile.legacy_state, 0);
    const auto step = waypoint.target
                          ? advance_character_motion({a.position.x, a.position.z}, *waypoint.target,
                                                     a.control.flags)
                          : CharacterMotionResult{CharacterMotionError::invalid_input, {}};
    if (!step.step)
        return fail(RescueWorldError::preparation_failed);
    RescueWorldCandidate c;
    c.state = s;
    auto &actor = c.state.ai.battle.actors.at(id);
    actor.position.x = step.step->position.x;
    actor.position.z = step.step->position.z;
    c.state.ai.contexts.at(id).cell = step.step->logical_cell;
    if (step.step->velocity)
        c.state.actors.at(id).horizontal_velocity = *step.step->velocity;
    if (step.step->waypoint_overlap)
        ++c.state.actors.at(id).waypoint;
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_rescue_cleanup(const RescueWorldState &s, CharacterId id) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    return prepare_world_actor_cleanup(s, id);
}
RescueWorldResult prepare_world_actor_cleanup(const RescueWorldState &s, CharacterId id) {
    const auto actor = s.ai.battle.actors.find(id);
    if (!id.value || actor == s.ai.battle.actors.end() || !(actor->second.id == id) ||
        !s.actors.count(id) || !s.ai.contexts.count(id))
        return fail(RescueWorldError::stale_actor);
    if (actor->second.kind != ActorKind::human && actor->second.kind != ActorKind::monster)
        return fail(RescueWorldError::invalid_input);
    const auto &roster =
        actor->second.kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order;
    if (std::count(roster.begin(), roster.end(), id) != 1)
        return fail(RescueWorldError::stale_actor);
    RescueWorldCandidate c;
    c.state = s;
    if (!cleanup(c.state, id))
        return fail(RescueWorldError::preparation_failed);
    c.cleaned_up = true;
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_detached_actor_cleanup(const RescueWorldState &s, CharacterId id) {
    if (!valid_detached_human(s, id))
        return fail(RescueWorldError::stale_actor);
    RescueWorldCandidate c;
    c.state = s;
    if (!cleanup(c.state, id))
        return fail(RescueWorldError::preparation_failed);
    c.cleaned_up = true;
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_rescue_follow(const RescueWorldState &s, CharacterId id) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    RescueWorldCandidate c;
    c.state = s;
    auto &a = c.state.ai.battle.actors.at(id);
    const BattleActorRecord *other = nullptr;
    if (a.rescue) {
        const auto live = c.state.ai.battle.actors.find(*a.rescue);
        const auto retired = c.state.ai.retired_actors.find(*a.rescue);
        if (live != c.state.ai.battle.actors.end())
            other = &live->second;
        else if (retired != c.state.ai.retired_actors.end())
            other = &retired->second;
        else
            return fail(RescueWorldError::stale_actor);
    }
    const auto repair = prepare_carry_reference_repair(
        {a.control.flags, a.object_slot, a.rescue.has_value(), other && other->rescue.has_value()});
    if (!repair)
        return fail(RescueWorldError::invalid_input);
    a.object_slot = repair->object_slot;
    if (repair->clear_reference)
        a.rescue.reset();
    if (repair->reset_all_hp) {
        const auto hp = prepare_hp_assignment(a.hp, a.capacity);
        if (!hp.candidate)
            return fail(RescueWorldError::preparation_failed);
        a.hp = *hp.candidate;
    }
    if (repair->reset_action)
        a.control.action = a.control.action_counter = 0;
    if (a.control.state == 16) {
        const bool in_roster =
            a.rescue && std::find(s.ai.human_order.begin(), s.ai.human_order.end(), *a.rescue) !=
                            s.ai.human_order.end();
        const auto follow = prepare_rescued_follow(
            a.rescue.has_value(), in_roster,
            other ? WorldPosition{other->position.x, other->position.z} : WorldPosition{},
            other ? other->position.height : 0);
        if (!follow)
            return fail(RescueWorldError::preparation_failed);
        if (follow->cleanup) {
            if (!cleanup(c.state, id))
                return fail(RescueWorldError::preparation_failed);
            c.cleaned_up = true;
        } else
            a.position = {follow->position.x, follow->height, follow->position.z};
    }
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_rescue_delivery(const RescueWorldState &s, CharacterId id,
                                                const RescueDeliveryProjection &projection) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    const auto &original = s.ai.battle.actors.at(id);
    if (original.object_slot != -2 || !original.rescue || !human(s, *original.rescue))
        return fail(RescueWorldError::stale_actor);
    const auto rescued_id = *original.rescue;
    const auto &rescued = s.ai.battle.actors.at(rescued_id);
    if (!(rescued.rescue == id) || rescued.control.state != 16 || rescued.object_slot != -1 ||
        !s.human_spending.count(original.definition) || !s.human_spending.count(rescued.definition))
        return fail(RescueWorldError::invalid_input);
    RescueWorldCandidate c;
    c.state = s;
    auto *f = bound(c.state, id);
    if (!f || !((f->category == 2 && f->detail == 0) || (f->category == 8 && f->detail == 2)))
        return fail(RescueWorldError::stale_binding);
    // Preserve the recursive old flags/s input; applying use1 first would wrongly clear512.
    const auto rescued_arrival = prepare_facility_arrival(statistics(c.state, rescued_id, *f),
                                                          arrival(c.state, rescued_id, *f));
    if (!rescued_arrival.candidate)
        return fail(RescueWorldError::preparation_failed);
    auto &carried = c.state.ai.battle.actors.at(rescued_id);
    carried.rescue.reset();
    if (!transition(carried, 0) ||
        !use(c.state, rescued_id, 1, *f, projection.rescued_target, projection.rescued_direction))
        return fail(RescueWorldError::preparation_failed);
    store_visits(c.state, rescued_id, rescued_arrival.candidate->state);
    c.state.human_spending.at(carried.definition) =
        rescued_arrival.candidate->state.legacy_actor_total;
    f->sales = rescued_arrival.candidate->state.current_month_facility_sales;
    c.state.actors.at(rescued_id).binding = c.state.actors.at(id).binding;
    c.state.actors.at(rescued_id).destination = c.state.actors.at(id).destination;
    carried.position = original.position;
    c.state.ai.contexts.at(rescued_id).cell = s.ai.contexts.at(id).cell;
    auto &carrier = c.state.ai.battle.actors.at(id);
    carrier.rescue.reset();
    carrier.object_slot = -1;
    carrier.control.flags |= 256U;
    const auto carrier_arrival =
        prepare_facility_arrival(statistics(c.state, id, *f), arrival(c.state, id, *f));
    if (!carrier_arrival.candidate)
        return fail(RescueWorldError::preparation_failed);
    // The movement-arrival caller clears256 BEFORE selecting helper mode2.
    carrier.control.flags &= ~256U;
    if (!use(c.state, id, 2, *f, projection.carrier_target, projection.carrier_direction))
        return fail(RescueWorldError::preparation_failed);
    store_visits(c.state, id, carrier_arrival.candidate->state);
    c.state.human_spending.at(carrier.definition) =
        carrier_arrival.candidate->state.legacy_actor_total;
    f->sales = carrier_arrival.candidate->state.current_month_facility_sales;
    const auto income =
        rescued_arrival.candidate->cash_income + carrier_arrival.candidate->cash_income;
    if (income > 0) {
        auto &ai = c.state.ai;
        if (ai.next_cash_id == 0 || ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
            ai.accounting.post_cash({ai.next_cash_id, ai.period, CashCategory::facilities,
                                     CashDirection::income, income}) != AccountingError::none)
            return fail(RescueWorldError::preparation_failed);
        ++ai.next_cash_id;
    }
    c.arrived = true;
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_inn_c(const RescueWorldState &s, CharacterId id) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    RescueWorldCandidate c;
    c.state = s;
    auto &a = c.state.ai.battle.actors.at(id);
    if (a.control.state != 14 || a.state_counter < 0)
        return fail(RescueWorldError::invalid_input);
    auto *f = bound(c.state, id);
    if (!f) {
        if (!cleanup(c.state, id))
            return fail(RescueWorldError::preparation_failed);
        c.cleaned_up = true;
    } else if (f->category == 2 && a.state_counter == 170) {
        const auto hp = prepare_hp_change(a.hp, a.capacity, a.capacity);
        if (!hp.candidate)
            return fail(RescueWorldError::preparation_failed);
        a.hp = *hp.candidate;
        c.recovered = true;
    }
    return {RescueWorldError::none, c};
}
RescueWorldResult prepare_world_inn_d(const RescueWorldState &s, CharacterId id) {
    if (!human(s, id))
        return fail(RescueWorldError::stale_actor);
    RescueWorldCandidate c;
    c.state = s;
    auto &a = c.state.ai.battle.actors.at(id);
    if (a.control.state != 14 || a.state_counter < 0 || a.control.action_counter < 0 ||
        a.control.alternate_counter < 0 || a.state_counter == std::numeric_limits<int>::max() ||
        a.control.action_counter == std::numeric_limits<int>::max() ||
        a.control.alternate_counter == std::numeric_limits<int>::max())
        return fail(RescueWorldError::invalid_input);
    const auto counters = advance_actor_counters(
        {a.control.alternate_counter, a.control.action_counter, a.state_counter, a.hit_flash,
         a.label_timer, a.miss_label, a.damage_total, a.hit_count});
    const auto effects = advance_actor_effects(c.state.ai.contexts.at(id).effects);
    if (!counters || !effects.candidate)
        return fail(RescueWorldError::preparation_failed);
    const auto labels = expire_actor_hit_label(*counters);
    if (!labels)
        return fail(RescueWorldError::preparation_failed);
    a.state_counter = labels->state;
    a.control.action_counter = labels->action;
    a.control.alternate_counter = labels->alternate;
    a.hit_flash = labels->hit_flash;
    a.label_timer = labels->hit_label;
    a.miss_label = labels->miss_label;
    a.damage_total = labels->damage_total;
    a.hit_count = labels->hits;
    c.state.ai.contexts.at(id).effects = effects.candidate->state;
    const auto hp = advance_hp_animation(a.hp);
    if (!hp.candidate)
        return fail(RescueWorldError::preparation_failed);
    a.hp = *hp.candidate;
    return prepare_world_inn_control(c.state, id);
}
RescueWorldResult prepare_world_inn_control(const RescueWorldState &s, CharacterId id,
                                            std::size_t domain_limit) {
    const auto found = s.ai.battle.actors.find(id);
    if (found == s.ai.battle.actors.end() || !(found->second.id == id) || !s.actors.count(id) ||
        !s.ai.contexts.count(id))
        return fail(RescueWorldError::stale_actor);
    RescueWorldCandidate c;
    c.state = s;
    if (domain_limit == 0)
        return fail(RescueWorldError::invalid_input);
    std::size_t domains{};
    auto &a = c.state.ai.battle.actors.at(id);
    for (;;) {
        const auto local = prepare_local_control_prefix(a.control);
        if (!local.candidate)
            return fail(RescueWorldError::preparation_failed);
        a.control = local.candidate->state;
        c.flow = local.candidate->flow;
        if (local.candidate->flow == ActorControlFlow::waiting || a.control.queue.empty())
            break;
        auto *f = bound(c.state, id);
        const auto opcode = a.control.queue.front()[0];
        if (opcode == 21) {
            // 21 queries CURRENT instance at O, unlike q's old-s/definition/identity guard.
            f = occupation_at_goal(c.state, id);
            if (f && f->category == 5)
                break; // m.a category5 has actual actor counters/definition/page side effects.
            a.control.queue.erase(a.control.queue.begin());
            if (f) {
                f->occupants.push_back(id);
                c.occupied = true;
            } // Original q()==null still consumes21; state14 c() cleans up next time.
            if (++domains >= domain_limit)
                break;
        } else if (opcode == 24) {
            if (!f) {
                if (!cleanup(c.state, id))
                    return fail(RescueWorldError::preparation_failed);
                c.cleaned_up = true;
                break;
            }
            if (f->category == 1)
                break; // 商店满足度/装备有独立尾部；类别5的24只有共同退出，21另行交接。
            if (!c.state.facility_uses.count(f->placement.definition_id))
                return fail(RescueWorldError::invalid_input);
            FacilityServiceExitInput i;
            i.actor = id;
            i.control = a.control;
            i.binding_valid = true;
            i.map = c.state.map;
            i.facility = f->placement;
            i.position = {a.position.x, a.position.z};
            i.category = f->category;
            i.detail = f->detail;
            i.progress = c.state.facility_uses.at(f->placement.definition_id);
            i.upgrade_uses = f->upgrade_uses;
            i.occupants = f->occupants;
            const auto exit = prepare_facility_service_exit(i);
            if (!exit.candidate || !exit.candidate->position || !exit.candidate->shared_use)
                return fail(RescueWorldError::preparation_failed);
            const auto &e = *exit.candidate;
            a.control = e.control;
            a.position.x = e.position->position.x;
            a.position.z = e.position->position.z;
            c.state.ai.contexts.at(id).cell = e.position->logical_cell;
            f->occupants = e.occupants;
            c.state.facility_uses.at(f->placement.definition_id) = e.shared_use->progress;
            a.state_counter = a.state_parameter = 0;
            a.baseline = 0;
            a.encounter.reset();
            if (e.home_hp_and_visits) {
                const auto hp = prepare_hp_assignment(a.hp, a.capacity);
                if (!hp.candidate)
                    return fail(RescueWorldError::preparation_failed);
                a.hp = *hp.candidate;
                c.state.actors.at(id).visits.legacy_visit_counts.fill(0);
            }
            c.exited = true;
            c.flow = ActorControlFlow::delegated;
            break; // Activity8 needs fresh world selection; preserve deferred expression tail.
        } else
            break; // The world interpreter must continue this same d, without another prefix.
    }
    return {RescueWorldError::none, c};
}
} // namespace ark::simulation::rules
