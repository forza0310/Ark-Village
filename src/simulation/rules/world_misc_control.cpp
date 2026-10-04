#include "ark/simulation/rules/world_misc_control.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ark::simulation::rules {
namespace {
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
WorldMiscControlError actor_error(const RescueWorldState &s, CharacterId id) {
    const auto a = s.ai.battle.actors.find(id);
    if (!id.value || a == s.ai.battle.actors.end() || !(a->second.id == id) ||
        !s.ai.contexts.count(id) || !s.actors.count(id))
        return WorldMiscControlError::stale_actor;
    if (a->second.kind != ActorKind::human && a->second.kind != ActorKind::monster)
        return WorldMiscControlError::invalid_input;
    const auto &roster = a->second.kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order;
    if (std::count(roster.begin(), roster.end(), id) != 1)
        return WorldMiscControlError::stale_actor;
    return WorldMiscControlError::none;
}
} // namespace
WorldMiscControlResult prepare_world_misc_control(const WorldMiscControlState &s,
                                                  const WorldMiscControlInput &i) {
    const auto fail = [](WorldMiscControlError e) -> WorldMiscControlResult { return {e, {}}; };
    const auto error = actor_error(s.world, i.actor);
    if (error != WorldMiscControlError::none)
        return fail(error);
    const auto &old = s.world.ai.battle.actors.at(i.actor);
    if (!world_control_detail::valid_control(old.control) || old.control.queue.empty())
        return fail(WorldMiscControlError::invalid_input);
    const auto command = old.control.queue.front();
    if (command[0] != 25 && command[0] != 26 && command[0] != 32 && command[0] != 33)
        return fail(WorldMiscControlError::invalid_input);
    WorldMiscControlCandidate c{s};
    auto &a = c.state.world.ai.battle.actors.at(i.actor);
    a.control.queue.erase(a.control.queue.begin());
    if (command[0] == 25) {
        if (!i.cached_view)
            return fail(WorldMiscControlError::missing_projection);
        if (a.state_counter < 0 || a.capacity < 0)
            return fail(WorldMiscControlError::invalid_input);
        const auto hp = prepare_hp_change(a.hp, command[1], a.capacity);
        if (!hp.candidate)
            return fail(WorldMiscControlError::preparation_failed);
        auto &display = c.state.world.ai.contexts.at(i.actor).effects.display;
        // ci={32,36},cj={6,10}: a(400,6)=160，b(400,6)向零截整为-26。
        display.push_back({2, 0, command[1], 160, -26, 2});
        a.hp = *hp.candidate;
        if (a.control.state == 2) {
            const int progress =
                a.capacity == 0 ? 0
                                : static_cast<int>(static_cast<std::int64_t>(
                                                       std::clamp(a.hp.target, 0, a.capacity)) *
                                                   900 / a.capacity);
            a.state_counter = std::max(a.state_counter, progress);
        }
        display.push_back({6, 0, i.cached_view->x, i.cached_view->y});
    } else if (command[0] == 26) {
        // 原P地面出口对双方均排26；n()仍取共享bv，不改怪物定义p/r。
        if (a.definition < 0)
            return fail(WorldMiscControlError::invalid_input);
        const auto definition = c.state.human_definition_state.find(a.definition);
        if (definition == c.state.human_definition_state.end())
            return fail(WorldMiscControlError::missing_definition);
        definition->second = 1; // n.j()固定输入为空；不额外清理/重置人物状态或队列。
        c.action = WorldControlAction::delete_true;
    } else if (command[0] == 32) {
        a.vertical_velocity = 28.0F / 6.0F; // n初始化 aN=d.a(14,7)，不是aO或重力aM。
    } else {
        if (!i.cached_view || !i.sound_projection)
            return fail(WorldMiscControlError::missing_projection);
        const auto sound_position = i.sound_projection(*i.cached_view);
        if (!sound_position)
            return fail(WorldMiscControlError::preparation_failed);
        c.sounds.push_back({i.actor, 8, *sound_position});
    }
    return {WorldMiscControlError::none, std::move(c)};
}
WorldStateCommandResult prepare_world_state_transition(const RescueWorldState &s,
                                                       const WorldStateTransitionInput &i) {
    const auto fail = [](WorldMiscControlError e) -> WorldStateCommandResult { return {e, {}}; };
    const auto error = actor_error(s, i.actor);
    if (error != WorldMiscControlError::none)
        return fail(error);
    const auto &old = s.ai.battle.actors.at(i.actor);
    if (!world_control_detail::valid_control(old.control))
        return fail(WorldMiscControlError::invalid_input);
    const int next_state = i.next_state;
    ActorStateTransitionInput transition;
    transition.control = old.control;
    transition.human = old.kind == ActorKind::human;
    transition.next_state = next_state;
    transition.baseline = old.baseline;
    transition.boost_ticket = i.boost_ticket;
    transition.boost_event116_seen = s.ai.battle.events.count(116) != 0;
    if (next_state == 18 && !(old.control.flags & 2048U)) {
        // 原a.e.a(1,this)没有human守卫；即使是怪物，也按n()的共享bv索引读取。
        const auto definition = s.ai.growth.find(old.definition);
        if (definition == s.ai.growth.end())
            return fail(WorldMiscControlError::missing_definition);
        transition.legacy_u = definition->second.definition.legacy_u;
    }
    const RescueFacility *q = nullptr;
    if (next_state == 10) {
        if (!valid_legacy_map(s.map))
            return fail(WorldMiscControlError::invalid_input);
        q = current_q(s, i.actor);
        if (q)
            transition.current_facility_category = q->category;
    }
    const auto prepared = prepare_actor_state_transition(transition);
    if (!prepared)
        return fail(WorldMiscControlError::preparation_failed);
    if (prepared->copy_attack_position &&
        (!std::isfinite(old.attack_position.x) || !std::isfinite(old.attack_position.z) ||
         !std::isfinite(old.attack_position.height)))
        return fail(WorldMiscControlError::invalid_input);
    WorldStateCommandCandidate c{s};
    auto &a = c.state.ai.battle.actors.at(i.actor);
    a.control = prepared->control;
    a.baseline = prepared->baseline;
    a.state_counter = a.state_parameter = 0;
    if (prepared->clear_encounter)
        a.encounter.reset();
    if (prepared->copy_attack_position)
        a.position = a.attack_position;
    if (prepared->reset_attack_count)
        a.attack_count = 0;
    c.consumed_boost_ticket = prepared->consumed_boost_ticket;
    if (prepared->request_boost_event116) {
        c.state.ai.battle.events.insert(116);
        c.event_requests.push_back(116);
    }
    if (prepared->request_cleanup) {
        if (a.kind == ActorKind::human) {
            const auto cleanup = prepare_world_rescue_cleanup(c.state, i.actor);
            if (!cleanup.candidate)
                return fail(WorldMiscControlError::preparation_failed);
            c.state = cleanup.candidate->state;
        } else {
            const auto cleanup = prepare_actor_cleanup(a.kind, a.control.flags);
            if (!cleanup || !q)
                return fail(WorldMiscControlError::preparation_failed);
            auto &occupants = c.state.facilities.at(q->placement.instance_id.value).occupants;
            const auto occupant = std::find(occupants.begin(), occupants.end(), i.actor);
            if (occupant != occupants.end())
                occupants.erase(occupant);
            a.rescue.reset();
            a.encounter.reset();
            a.group.reset();
            a.position.height = 0;
            a.control.flags = cleanup->flags;
            transition.control = a.control;
            transition.human = false;
            transition.next_state = cleanup->state;
            transition.baseline = a.baseline;
            transition.current_facility_category.reset();
            const auto reset = prepare_actor_state_transition(transition);
            if (!reset)
                return fail(WorldMiscControlError::preparation_failed);
            a.control = reset->control;
            a.baseline = reset->baseline;
            a.state_counter = a.state_parameter = 0;
            a.control.queue.push_back({8, cleanup->activity});
        }
        c.cleaned_up = true;
    }
    return {WorldMiscControlError::none, std::move(c)};
}
WorldStateCommandResult prepare_world_state_command(const RescueWorldState &s,
                                                    const WorldStateCommandInput &i) {
    const auto error = actor_error(s, i.actor);
    if (error != WorldMiscControlError::none)
        return {error, {}};
    const auto &control = s.ai.battle.actors.at(i.actor).control;
    if (!world_control_detail::valid_control(control) || control.queue.empty() ||
        control.queue.front()[0] != 2)
        return {WorldMiscControlError::invalid_input, {}};
    auto next = s;
    next.ai.battle.actors.at(i.actor).control.queue.erase(
        next.ai.battle.actors.at(i.actor).control.queue.begin());
    return prepare_world_state_transition(next,
                                          {i.actor, control.queue.front()[1], i.boost_ticket});
}
} // namespace ark::simulation::rules
