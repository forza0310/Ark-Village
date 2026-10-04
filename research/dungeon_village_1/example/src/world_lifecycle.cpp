#include "dungeon_village_reference/world_lifecycle.hpp"

#include <algorithm>
#include <cmath>

namespace dungeon_village_reference {
namespace {
bool restore(RescueWorldState &s, CharacterId id) {
    auto &a = s.ai.battle.actors.at(id);
    const auto restored = prepare_actor_baseline_restore(
        a.control, a.baseline, a.kind == ActorKind::human, s.actors.at(id).monster_mode);
    if (!restored)
        return false;
    a.control = restored->control;
    if (restored->clear_encounter)
        a.encounter.reset();
    // b()不调用c(D)：原B/C/i、路径、HP和dc在此保持不变。
    return true;
}
const RescueFacility *current_q(const RescueWorldState &s, CharacterId id) {
    const auto &binding = s.actors.at(id).binding;
    if (!binding || !arrival_binding_matches(s.map, *binding, s.ai.contexts.at(id).cell))
        return nullptr;
    const auto f = s.facilities.find(binding->instance_id.value);
    if (f == s.facilities.end() || !(f->second.placement.instance_id == binding->instance_id) ||
        f->second.placement.definition_id != binding->definition_id)
        return nullptr;
    return &f->second;
}
} // namespace
WorldLifecycleResult prepare_world_lifecycle_c(const RescueWorldState &s,
                                               const WorldLifecycleInput &i) {
    const auto fail = [](WorldLifecycleError e) -> WorldLifecycleResult { return {e, {}}; };
    const auto found = s.ai.battle.actors.find(i.actor);
    if (!i.actor.value || found == s.ai.battle.actors.end() || !(found->second.id == i.actor) ||
        !s.actors.count(i.actor) || !s.ai.contexts.count(i.actor))
        return fail(WorldLifecycleError::stale_actor);
    const auto &old = found->second;
    if (old.kind != ActorKind::human && old.kind != ActorKind::monster)
        return fail(WorldLifecycleError::invalid_input);
    const auto &roster = old.kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order;
    if (std::count(roster.begin(), roster.end(), i.actor) != 1)
        return fail(WorldLifecycleError::stale_actor);
    if (!world_control_detail::valid_control(old.control) || old.state_counter < 0 ||
        old.object_slot < -2 || !valid_actor_effect_state(s.ai.contexts.at(i.actor).effects))
        return fail(WorldLifecycleError::invalid_input);
    WorldLifecycleCandidate c{s};
    const auto cleanup = [&] {
        const auto result = prepare_world_actor_cleanup(c.state, i.actor);
        if (!result.candidate)
            return false;
        c.state = result.candidate->state;
        c.cleaned_up = true;
        return true;
    };
    switch (old.control.state) {
    case 6:
    case 7:
    case 19:
        return {WorldLifecycleError::none, std::move(c)};
    case 14: {
        if (!valid_legacy_map(s.map))
            return fail(WorldLifecycleError::invalid_input);
        const auto *facility = current_q(s, i.actor);
        if (!facility) {
            if (!cleanup())
                return fail(WorldLifecycleError::preparation_failed);
        } else if (facility->category == 2 && old.state_counter == 170) {
            const auto hp = prepare_hp_change(old.hp, old.capacity, old.capacity);
            if (!hp.candidate)
                return fail(WorldLifecycleError::preparation_failed);
            c.state.ai.battle.actors.at(i.actor).hp = *hp.candidate;
        }
        return {WorldLifecycleError::none, std::move(c)};
    }
    case 16: {
        // 共同前段已经修复R；这里不能再以R.R/512重跑一次修复。
        const bool in_roster =
            old.rescue && std::find(s.ai.human_order.begin(), s.ai.human_order.end(),
                                    *old.rescue) != s.ai.human_order.end();
        const auto carrier =
            old.rescue ? s.ai.battle.actors.find(*old.rescue) : s.ai.battle.actors.end();
        if (in_roster &&
            (carrier == s.ai.battle.actors.end() || !(carrier->second.id == *old.rescue) ||
             carrier->second.kind != ActorKind::human))
            return fail(WorldLifecycleError::stale_actor);
        const auto follow = prepare_rescued_follow(
            old.rescue.has_value(), in_roster,
            in_roster ? WorldPosition{carrier->second.position.x, carrier->second.position.z}
                      : WorldPosition{},
            in_roster ? carrier->second.position.height : 0);
        if (!follow)
            return fail(WorldLifecycleError::invalid_input);
        if (follow->cleanup) {
            if (!cleanup())
                return fail(WorldLifecycleError::preparation_failed);
        } else {
            auto &actor = c.state.ai.battle.actors.at(i.actor);
            actor.position = {follow->position.x, follow->height, follow->position.z};
        }
        return {WorldLifecycleError::none, std::move(c)};
    }
    case 2:
    case 4:
    case 10:
    case 12:
        break;
    default:
        return fail(WorldLifecycleError::unsupported_state);
    }
    const auto lifecycle = prepare_timed_lifecycle({old.kind,
                                                    old.control.state,
                                                    old.state_counter,
                                                    s.actors.at(i.actor).monster_mode,
                                                    old.state_parameter,
                                                    old.control.flags,
                                                    old.object_slot != -1,
                                                    s.ai.contexts.at(i.actor).inside_town,
                                                    s.ai.battle.events.count(90) != 0,
                                                    {old.position.x, old.position.z},
                                                    s.actors.at(i.actor).horizontal_velocity,
                                                    old.position.height,
                                                    old.hp.displayed,
                                                    old.capacity});
    if (!lifecycle.candidate)
        return fail(WorldLifecycleError::preparation_failed);
    // 原状态2先表情3，再写am1/3，旧B900才表情4、d(h)、b；请求逐条消费。
    auto &actor = c.state.ai.battle.actors.at(i.actor);
    actor.control.flags = lifecycle.candidate->flags;
    actor.position.x = lifecycle.candidate->position.x;
    actor.position.z = lifecycle.candidate->position.z;
    c.state.actors.at(i.actor).horizontal_velocity = lifecycle.candidate->horizontal_velocity;
    bool wrote_recovery = false;
    for (const auto &request : lifecycle.candidate->requests) {
        if (request.kind == LifecycleRequestKind::expression) {
            if (c.consumed_expressions >= i.expressions.size())
                return fail(WorldLifecycleError::missing_ticket);
            const auto &ticket = i.expressions[c.consumed_expressions];
            auto &effects = c.state.ai.contexts.at(i.actor).effects;
            const auto expression =
                prepare_actor_expression({effects, request.parameter, 0, ticket.probability,
                                          ticket.variant_count, ticket.variant});
            if (!expression.candidate)
                return fail(WorldLifecycleError::preparation_failed);
            effects = expression.candidate->state;
            ++c.consumed_expressions;
            c.consumed_variants += expression.candidate->consumed_variant ? 1 : 0;
            if (!wrote_recovery && lifecycle.candidate->write_hp_slot1_and3) {
                actor.hp.displayed = actor.hp.target = *lifecycle.candidate->write_hp_slot1_and3;
                wrote_recovery = true;
            }
            if (request.parameter == 4 && lifecycle.candidate->reset_all_hp_to_capacity) {
                const auto hp = prepare_hp_assignment(actor.hp, old.capacity);
                if (!hp.candidate)
                    return fail(WorldLifecycleError::preparation_failed);
                actor.hp = *hp.candidate;
            }
        } else if (request.kind == LifecycleRequestKind::restore_baseline) {
            if (!restore(c.state, i.actor))
                return fail(WorldLifecycleError::preparation_failed);
            c.restored_baseline = true;
        } else if (request.kind == LifecycleRequestKind::state) {
            const auto transition =
                prepare_world_state_transition(c.state, {i.actor, request.parameter, {}});
            if (!transition.candidate)
                return fail(WorldLifecycleError::preparation_failed);
            c.state = transition.candidate->state;
            c.transitioned = true;
        } else if (request.kind == LifecycleRequestKind::clear_path) {
            auto &path = c.state.actors.at(i.actor);
            path.journey.reset();
            path.unbound_route.reset();
            path.path_pending = false;
            path.waypoint = 0;
        } else if (request.kind == LifecycleRequestKind::activity) {
            c.state.ai.battle.actors.at(i.actor).control.queue.push_back({8, request.parameter});
        } else {
            return fail(WorldLifecycleError::preparation_failed);
        }
    }
    return {WorldLifecycleError::none, std::move(c)};
}
WorldMonsterActResult prepare_world_monster_act_c(const RescueWorldState &s,
                                                  const WorldMonsterActInput &i) {
    const auto fail = [](WorldLifecycleError error) -> WorldMonsterActResult {
        return {error, {}};
    };
    const auto found = s.ai.battle.actors.find(i.actor);
    if (!i.actor.value || found == s.ai.battle.actors.end() || !(found->second.id == i.actor) ||
        !s.actors.count(i.actor) || !s.ai.contexts.count(i.actor) ||
        std::count(s.ai.monster_order.begin(), s.ai.monster_order.end(), i.actor) != 1)
        return fail(WorldLifecycleError::stale_actor);
    const auto &old = found->second;
    const auto &context = s.ai.contexts.at(i.actor);
    const auto &runtime = s.actors.at(i.actor);
    if (old.kind != ActorKind::monster || old.control.state != 17 || old.state_counter < 0 ||
        !world_control_detail::valid_control(old.control) || old.object_slot < -2 ||
        runtime.monster_mode < 0 || runtime.monster_mode > 4 || runtime.town_updates < 0)
        return fail(WorldLifecycleError::invalid_input);
    WorldMonsterActCandidate c{s, {}, false, false, false};
    if (runtime.monster_mode == 1 || runtime.monster_mode == 4) {
        if (!i.path || !(i.path->actor == i.actor))
            return fail(WorldLifecycleError::invalid_input);
        const auto path = prepare_world_path_c(c.state, *i.path);
        if (!path.candidate)
            return fail(WorldLifecycleError::preparation_failed);
        c.state = path.candidate->state;
        c.path = path.candidate;
        const auto idle =
            prepare_monster_idle({runtime.monster_mode, false, path.candidate->path_returned_true,
                                  runtime.town_updates});
        if (!idle.candidate)
            return fail(WorldLifecycleError::preparation_failed);
        // P可能已修改A/队列/引用；原T1仍在P返回false后读L并执行r，不重查A。
        if (idle.candidate->action == MonsterIdleAction::cleanup) {
            const auto cleanup = prepare_world_actor_cleanup(c.state, i.actor);
            if (!cleanup.candidate)
                return fail(WorldLifecycleError::preparation_failed);
            c.state = cleanup.candidate->state;
            c.cleaned_up = true;
        }
    } else {
        bool enemy_inside{};
        if (!(old.control.flags & (512U | 1024U)) && old.state_counter >= 5 && context.move_area &&
            old.perceived_enemy) {
            const auto enemy = s.ai.contexts.find(*old.perceived_enemy);
            if (enemy == s.ai.contexts.end() || (!s.ai.battle.actors.count(*old.perceived_enemy) &&
                                                 !s.ai.retired_actors.count(*old.perceived_enemy)))
                return fail(WorldLifecycleError::stale_actor);
            enemy_inside = enemy->second.inside_town; // 原G读az.ax，不在此重新e或投影位置。
        }
        const auto gate =
            prepare_battle_gate({old.kind, old.control.flags, old.state_counter, context.move_area,
                                 old.object_slot != -1, old.perceived_enemy.has_value(),
                                 context.inside_town, enemy_inside});
        if (!gate.candidate)
            return fail(WorldLifecycleError::preparation_failed);
        c.called_battle_gate = true;
        if (gate.candidate->allowed) {
            const auto transition = prepare_world_state_transition(c.state, {i.actor, 1, {}});
            if (!transition.candidate)
                return fail(WorldLifecycleError::preparation_failed);
            c.state = transition.candidate->state;
            c.entered_battle = true;
        }
    }
    return {WorldLifecycleError::none, std::move(c)};
}
} // namespace dungeon_village_reference
