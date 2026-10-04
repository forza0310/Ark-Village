#include "dungeon_village_reference/world_actor_tail.hpp"
#include "dungeon_village_reference/world_detached_actor.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
bool live(const RescueWorldState &s, CharacterId id) {
    const auto a = s.ai.battle.actors.find(id);
    if (!id.value || a == s.ai.battle.actors.end() || !(a->second.id == id) ||
        !s.actors.count(id) || !s.ai.contexts.count(id))
        return false;
    const auto &roster = a->second.kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order;
    return std::count(roster.begin(), roster.end(), id) == 1;
}
void release(RescueWorldState &s, CharacterId id) {
    const auto &b = s.actors.at(id).binding;
    if (!b || !arrival_binding_matches(s.map, *b, s.ai.contexts.at(id).cell))
        return;
    const auto f = s.facilities.find(b->instance_id.value);
    if (f == s.facilities.end() || !(f->second.placement.instance_id == b->instance_id) ||
        f->second.placement.definition_id != b->definition_id)
        return;
    auto &occupants = f->second.occupants;
    const auto occupant = std::find(occupants.begin(), occupants.end(), id);
    if (occupant != occupants.end())
        occupants.erase(occupant);
}
bool cleanup(RescueWorldState &s, CharacterId id, bool detached = false) {
    if (s.ai.battle.actors.at(id).kind == ActorKind::human) {
        const auto c = detached ? prepare_world_detached_actor_cleanup(s, id)
                                : prepare_world_rescue_cleanup(s, id);
        if (!c.candidate)
            return false;
        s = c.candidate->state;
        return true;
    }
    auto &a = s.ai.battle.actors.at(id);
    const auto r = prepare_actor_cleanup(a.kind, a.control.flags);
    if (!r)
        return false;
    release(s, id);
    a.rescue.reset();
    a.encounter.reset();
    a.group.reset();
    a.position.height = 0;
    a.control.flags = r->flags;
    ActorStateTransitionInput input;
    input.control = a.control;
    input.human = false;
    input.baseline = a.baseline;
    input.next_state = r->state;
    const auto transition = prepare_actor_state_transition(input);
    if (!transition)
        return false;
    a.control = transition->control;
    a.baseline = transition->baseline;
    a.state_counter = a.state_parameter = 0;
    a.control.queue.push_back({8, r->activity});
    return true;
}
} // namespace
static WorldActorTailResult actor_tail(const RescueWorldState &s, const WorldActorTailInput &i,
                                       bool detached) {
    const auto failed = [](RescueWorldError e) -> WorldActorTailResult { return {e, {}}; };
    if (!(detached ? valid_detached_human(s, i.actor) : live(s, i.actor)))
        return failed(RescueWorldError::stale_actor);
    if (!valid_world_map_facts(i.facts) || !valid_legacy_map(s.map) ||
        s.map.width != i.facts.map.width || s.map.height != i.facts.map.height)
        return failed(RescueWorldError::invalid_input);
    // Facts own the same current logical map; two disagreeing map projections are invalid.
    for (std::size_t n = 0; n < s.map.cells.size(); ++n)
        if (s.map.cells[n].legacy_state != i.facts.map.cells[n].legacy_state)
            return failed(RescueWorldError::invalid_input);
    WorldActorTailCandidate c;
    c.state = s;
    const auto &original = s.ai.battle.actors.at(i.actor);
    auto &initial = c.state.actors.at(i.actor);
    const auto old_cell = s.ai.contexts.at(i.actor).cell;
    const bool old_inside = old_cell.x > i.facts.town.left && old_cell.x < i.facts.town.right &&
                            old_cell.y > i.facts.town.top && old_cell.y < i.facts.town.bottom;
    if (initial.town_updates < 0 || initial.town_updates >= std::numeric_limits<int>::max() ||
        initial.outside_updates < 0 ||
        initial.outside_updates >= std::numeric_limits<int>::max() - 1)
        return failed(RescueWorldError::invalid_input);
    if (old_inside)
        initial.town_updates = (initial.town_updates + 1) % std::numeric_limits<int>::max();
    else if (original.control.state != 0 && !(original.control.flags & 16U))
        ++initial.outside_updates;
    const auto physics = prepare_world_physics_projection(c.state.ai, i.actor, i.facts);
    if (!physics.candidate)
        return failed(RescueWorldError::preparation_failed);
    c.state.ai = physics.candidate->state;
    c.queried_area = physics.candidate->queried_area;
    auto &projected = c.state.ai.battle.actors.at(i.actor);
    if (projected.control.state != 4 && projected.control.state != 20 &&
        i.facing_after_projection) {
        std::optional<int> facing;
        try {
            facing = i.facing_after_projection(projected);
        } catch (...) {
            return failed(RescueWorldError::preparation_failed);
        }
        if (!facing || *facing < 0 || *facing > 3)
            return failed(RescueWorldError::invalid_input);
        projected.control.facing = *facing;
    }
    c.projected_actor = projected;
    const auto &a = c.state.ai.battle.actors.at(i.actor);
    const auto &ctx = c.state.actors.at(i.actor);
    const auto &perception = c.state.ai.contexts.at(i.actor);
    ActorRetentionInput input;
    input.state = {a.kind,
                   a.control.state,
                   a.control.flags,
                   ctx.town_updates,
                   ctx.outside_updates,
                   ctx.blocked_updates,
                   ctx.spawn_updates,
                   ctx.no_path_updates,
                   ctx.short_exit_updates,
                   ctx.bad_area_updates};
    input.location_counters_already_advanced = true;
    input.at_spawn_after_projection = std::find(i.spawn_cells.begin(), i.spawn_cells.end(),
                                                perception.cell) != i.spawn_cells.end();
    input.area_before = perception.move_area;
    input.route_cells = ctx.journey         ? ctx.journey->route.steps.size()
                        : ctx.unbound_route ? ctx.unbound_route->steps.size()
                                            : 0;
    input.has_encounter = a.encounter.has_value();
    input.reported_hp = a.control.action == 7 ? 0 : a.hp.target;
    const auto retention = prepare_actor_retention(input);
    if (!retention)
        return failed(RescueWorldError::preparation_failed);
    for (const auto request : retention->requests) {
        if (request == ActorRetentionRequest::cleanup) {
            if (!cleanup(c.state, i.actor, detached))
                return failed(RescueWorldError::preparation_failed);
            c.cleaned_up = true;
        } else if (request == ActorRetentionRequest::clear_path) {
            auto &path = c.state.actors.at(i.actor);
            path.path_pending = false;
            path.journey.reset();
            path.unbound_route.reset();
            path.waypoint = 0;
        } else if (request == ActorRetentionRequest::mark_escape32768)
            c.state.ai.battle.actors.at(i.actor).control.flags |= 32768U;
        else if (request == ActorRetentionRequest::assign_hp1) {
            auto &actor = c.state.ai.battle.actors.at(i.actor);
            const auto hp = prepare_hp_assignment(actor.hp, 1);
            if (!hp.candidate)
                return failed(RescueWorldError::preparation_failed);
            actor.hp = *hp.candidate;
        } else if (request == ActorRetentionRequest::reset_action) {
            auto &control = c.state.ai.battle.actors.at(i.actor).control;
            control.action = control.action_counter = 0; // n0 preserves i, including r's cleared i.
        }
    }
    auto &counts = c.state.actors.at(i.actor);
    counts.town_updates = retention->state.town_updates;
    counts.outside_updates = retention->state.outside_updates;
    counts.blocked_updates = retention->state.blocked_updates;
    counts.spawn_updates = retention->state.spawn_updates;
    counts.no_path_updates = retention->state.no_path_updates;
    counts.short_exit_updates = retention->state.short_exit_updates;
    counts.bad_area_updates = retention->state.bad_area_updates;
    c.delete_instance = retention->delete_instance;
    c.deletion_reason = retention->reason;
    return {RescueWorldError::none, c};
}
WorldActorTailResult prepare_world_actor_tail(const RescueWorldState &s,
                                              const WorldActorTailInput &i) {
    return actor_tail(s, i, false);
}
WorldActorTailResult prepare_world_detached_actor_tail(const RescueWorldState &s,
                                                       const WorldActorTailInput &i) {
    return actor_tail(s, i, true);
}
WorldActorTailResult prepare_world_actor_remove(const RescueWorldState &s, CharacterId id,
                                                bool from_execution) {
    if (!live(s, id) || s.ai.retired_actors.count(id))
        return {RescueWorldError::stale_actor, {}};
    WorldActorTailCandidate c;
    c.state = s;
    const auto &a = c.state.ai.battle.actors.at(id);
    if (from_execution && a.kind == ActorKind::human)
        release(c.state, id);
    auto &roster = a.kind == ActorKind::human ? c.state.ai.human_order : c.state.ai.monster_order;
    roster.erase(std::find(roster.begin(), roster.end(), id));
    c.state.ai.retired_actors.emplace(id, a);
    c.state.ai.battle.actors.erase(id);
    c.delete_instance = true;
    // Keep contextual records until the world collector traces external task/page roots too.
    return {RescueWorldError::none, c};
}
} // namespace dungeon_village_reference
