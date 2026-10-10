#include "ark/simulation/ai/rules/world_perception.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace ark::simulation::rules {
namespace {
const BattleActorRecord *actor(const AiRewardState &s, CharacterId id) {
    const auto live = s.battle.actors.find(id);
    if (live != s.battle.actors.end() && id.value && live->second.id == id)
        return &live->second;
    const auto retired = s.retired_actors.find(id);
    return retired != s.retired_actors.end() && id.value && retired->second.id == id
               ? &retired->second
               : nullptr;
}
const RewardEncounter *event(const AiRewardState &s, std::uint64_t id) {
    const auto live = s.encounters.find(id);
    if (live != s.encounters.end())
        return &live->second;
    const auto retired = s.retired_encounters.find(id);
    return retired != s.retired_encounters.end() ? &retired->second : nullptr;
}
bool live(const AiRewardState &s, CharacterId id) {
    const auto *a = actor(s, id);
    return a && s.battle.actors.count(id) && s.contexts.count(id) &&
           (a->kind == ActorKind::human || a->kind == ActorKind::monster);
}
bool in_map(Position p, const WorldMapFacts &f) {
    return p.x >= 0 && p.x < f.map.width && p.y >= 0 && p.y < f.map.height;
}
std::optional<Position> half_cell(CombatPoint p) {
    if (!character_world_cell({p.x, p.z}))
        return {};
    return Position{static_cast<int>(p.x / 50.0F), static_cast<int>(p.z / 50.0F)};
}
WorldPerceptionResult fail(AiRewardError e) { return {e, {}}; }
} // namespace
bool valid_world_map_facts(const WorldMapFacts &f) {
    return valid_world_map_facts(f.map, f.surface, f.flags, f.town);
}
bool valid_world_map_facts(const LegacyMap &map, const std::vector<int> &surface,
                           const std::vector<std::uint32_t> &flags, const TownBounds &town) {
    return valid_legacy_map(map) && surface.size() == map.cells.size() &&
           flags.size() == map.cells.size() && town.left < town.right &&
           town.top < town.bottom &&
           std::none_of(surface.begin(), surface.end(), [](int v) { return v < 0; });
}
std::optional<MoveAreaCandidate> query_world_move_area(const AiRewardState &s, CharacterId id,
                                                       const WorldMapFacts &f) {
    if (!live(s, id) || !valid_world_map_facts(f))
        return {};
    const auto &a = s.battle.actors.at(id);
    const auto *e = a.encounter ? event(s, *a.encounter) : nullptr;
    if (a.encounter && !e)
        return {};
    constexpr Position offsets[]{{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    std::array<MoveAreaProbe, 4> probes;
    for (std::size_t n = 0; n < probes.size(); ++n) {
        const auto half = half_cell(
            {a.position.x + offsets[n].x, a.position.height, a.position.z + offsets[n].y});
        if (!half)
            return {};
        const Position cell{half->x / 2, half->y / 2};
        auto &probe = probes[n];
        probe.cell_in_map = in_map(cell, f);
        if (probe.cell_in_map) {
            const auto index = static_cast<std::size_t>(cell.y * f.map.width + cell.x);
            probe.legacy_surface = f.surface[index];
            probe.logical_state = f.map.cells[index].legacy_state;
        }
        probe.in_encounter_square =
            !e || (std::abs(static_cast<std::int64_t>(cell.x) - e->runtime.center.x) <= 1 &&
                   std::abs(static_cast<std::int64_t>(cell.y) - e->runtime.center.y) <= 1);
    }
    return prepare_move_area(a.control.state, a.encounter.has_value(), probes);
}
EnemySelectionResult query_current_combat_enemy(const AiRewardState &s, CharacterId id) {
    if (!live(s, id))
        return {ActorAiError::invalid_input, {}};
    const auto &a = s.battle.actors.at(id);
    EnemySelectionInput input;
    input.position = {a.position.x, a.position.z};
    input.active_battle_group = (a.control.flags & 128U) != 0;
    std::vector<CharacterId> roster;
    if (input.active_battle_group) {
        if (!a.group)
            return {}; // Source dc==null produces an empty e() list; own c later clears128.
        const auto *e = event(s, *a.group);
        if (!e)
            return {ActorAiError::invalid_input, {}};
        for (const auto &member : a.kind == ActorKind::human ? e->group.monsters : e->group.humans)
            roster.push_back(member.id);
    } else {
        if (!a.encounter)
            return {}; // No db means no scan, including no unrelated roster validation.
        const auto *bound = event(s, *a.encounter);
        if (!bound || bound->legacy_id < 0)
            return {ActorAiError::invalid_input, {}};
        input.encounter_id = static_cast<std::uint64_t>(bound->legacy_id);
        roster = a.kind == ActorKind::human ? s.monster_order : s.human_order;
    }
    for (const auto other : roster) {
        const auto *b = actor(s, other);
        const auto ctx = s.contexts.find(other);
        if (!b || ctx == s.contexts.end() || b->kind == a.kind)
            return {ActorAiError::invalid_input, {}};
        std::optional<std::uint64_t> legacy_event;
        if (!input.active_battle_group && b->encounter) {
            const auto *bound = event(s, *b->encounter);
            if (!bound || bound->legacy_id < 0)
                return {ActorAiError::invalid_input, {}};
            legacy_event = static_cast<std::uint64_t>(bound->legacy_id);
        }
        input.opposite_roster.push_back({other,
                                         b->control.state,
                                         ctx->second.move_area,
                                         {b->position.x, b->position.z},
                                         legacy_event});
    }
    return select_combat_enemy(input);
}
template <class State>
static WorldPerceptionResult perception_prefix(State &&s, CharacterId id, const WorldMapFacts &f,
                                               int mode) {
    if (!live(s, id))
        return fail(AiRewardError::stale_actor);
    if (!valid_world_map_facts(f))
        return fail(AiRewardError::invalid_input);
    const auto &original = s.battle.actors.at(id);
    const auto prefix = prepare_actor_decision_prefix(
        original.control.flags, original.blocked_battle_steps, original.attack_cooldown,
        original.hp.target, original.capacity);
    const auto area = query_world_move_area(s, id, f);
    if (!prefix || !area || mode < 0 || mode > 4)
        return fail(AiRewardError::preparation_failed);
    WorldPerceptionCandidate c;
    c.state = std::forward<State>(s);
    c.area = *area;
    auto &a = c.state.battle.actors.at(id);
    auto &ctx = c.state.contexts.at(id);
    a.control.flags = prefix->flags;
    a.decision_start = a.position;
    ctx.low_hp = prefix->low_hp;
    ctx.inside_town = inside_town(ctx.cell, f.town);
    ctx.move_area = area->allowed;
    if (prefix->restore_baseline) {
        const auto baseline =
            prepare_actor_baseline_restore(a.control, a.baseline, a.kind == ActorKind::human, mode);
        if (!baseline)
            return fail(AiRewardError::preparation_failed);
        a.control = baseline->control;
        if (baseline->clear_encounter)
            a.encounter.reset();
        c.restored_baseline = true;
    }
    a.blocked_battle_steps = prefix->blocked_battle_steps;
    a.attack_cooldown = prefix->attack_cooldown;
    const auto selected = query_current_combat_enemy(c.state, id);
    if (selected.error != ActorAiError::none)
        return fail(AiRewardError::preparation_failed);
    c.enemy = selected.candidate;
    a.perceived_enemy = c.enemy ? std::optional<CharacterId>(c.enemy->id) : std::nullopt;
    c.sensed_distance = c.enemy ? c.enemy->world_distance : std::numeric_limits<float>::max();
    a.perceived_distance = c.sensed_distance;
    if (!c.enemy)
        a.control.flags &= ~128U;
    return {AiRewardError::none, std::move(c)};
}
template <class State>
static WorldPerceptionResult reference_preemption(State &&s, CharacterId id, const WorldMapFacts &f,
                                                  bool rescue_enabled, bool task_flag,
                                                  const std::vector<std::uint64_t> &object_order) {
    if (!live(s, id) || !valid_world_map_facts(f))
        return fail(AiRewardError::invalid_input);
    const auto &original = s.battle.actors.at(id);
    const auto *other = original.rescue ? actor(s, *original.rescue) : nullptr;
    if (original.rescue && !other)
        return fail(AiRewardError::stale_actor);
    const auto repair = prepare_carry_reference_repair(
        {original.control.flags, original.object_slot, original.rescue.has_value(),
         other && other->rescue.has_value()});
    if (!repair)
        return fail(AiRewardError::preparation_failed);
    WorldPerceptionCandidate c;
    c.state = std::forward<State>(s);
    auto &a = c.state.battle.actors.at(id);
    a.object_slot = repair->object_slot;
    if (repair->clear_reference)
        a.rescue.reset();
    if (repair->reset_all_hp) {
        const auto hp = prepare_hp_assignment(a.hp, a.capacity);
        if (!hp.candidate)
            return fail(AiRewardError::preparation_failed);
        a.hp = *hp.candidate;
    }
    if (repair->reset_action)
        a.control.action = a.control.action_counter = 0;
    // Do not touch irrelevant rosters when this actor cannot execute the idle scans.
    if (a.kind != ActorKind::human || (a.control.state != 0 && a.control.state != 5) ||
        a.object_slot != -1)
        return {AiRewardError::none, std::move(c)};
    const auto &ctx = c.state.contexts.at(id);
    PreemptionInput input;
    input.kind = a.kind;
    input.state = a.control.state;
    input.rescue_enabled = rescue_enabled;
    input.definition_task_flag = task_flag;
    input.inside_town = ctx.inside_town;
    input.cell = ctx.cell;
    std::set<CharacterId> people;
    if (rescue_enabled && !task_flag) {
        for (const auto other_id : c.state.human_order) {
            if (!live(c.state, other_id) || !people.insert(other_id).second ||
                c.state.battle.actors.at(other_id).kind != ActorKind::human)
                return fail(AiRewardError::invalid_input);
            const auto &b = c.state.battle.actors.at(other_id);
            const auto &cached = c.state.contexts.at(other_id);
            const bool event_cell =
                in_map(cached.cell, f) &&
                (f.flags[static_cast<std::size_t>(cached.cell.y * f.map.width + cached.cell.x)] &
                 2U);
            input.people.push_back({other_id,
                                    b.control.state,
                                    {b.position.x, b.position.z},
                                    cached.cell,
                                    cached.half_cell,
                                    cached.inside_town,
                                    event_cell,
                                    cached.low_hp});
        }
    }
    // A successful rescue scan prevents the subsequent object scan, including its validation.
    auto next_state = select_idle_preemption(input);
    if (!next_state && !ctx.inside_town) {
        if (object_order.size() != c.state.battle.objects.size())
            return fail(AiRewardError::invalid_input);
        std::set<std::uint64_t> objects;
        for (const auto key : object_order) {
            const auto found = c.state.battle.objects.find(key);
            if (!objects.insert(key).second || found == c.state.battle.objects.end() ||
                found->second.id.value != key)
                return fail(AiRewardError::invalid_input);
            const auto &o = found->second;
            input.objects.push_back({key, o.state, {o.position.x, o.position.z}, o.cached_cell});
        }
        next_state = select_idle_preemption(input);
    }
    if (next_state) {
        ActorStateTransitionInput transition;
        transition.control = a.control;
        transition.human = true;
        transition.baseline = a.baseline;
        transition.next_state = *next_state;
        const auto prepared = prepare_actor_state_transition(transition);
        if (!prepared || prepared->request_cleanup)
            return fail(AiRewardError::preparation_failed);
        a.control = prepared->control;
        a.baseline = prepared->baseline;
        a.state_counter = a.state_parameter = 0;
        if (prepared->clear_encounter)
            a.encounter.reset();
    }
    return {AiRewardError::none, std::move(c)};
}
WorldPerceptionResult prepare_world_perception_prefix(const AiRewardState &s, CharacterId id,
                                                      const WorldMapFacts &f, int mode) {
    return perception_prefix(s, id, f, mode);
}
WorldPerceptionResult prepare_world_perception_prefix_consuming(AiRewardState &&s, CharacterId id,
                                                                const WorldMapFacts &f, int mode) {
    return perception_prefix(std::move(s), id, f, mode);
}
WorldPerceptionResult
prepare_world_reference_preemption(const AiRewardState &s, CharacterId id, const WorldMapFacts &f,
                                   bool rescue_enabled, bool task_flag,
                                   const std::vector<std::uint64_t> &object_order) {
    return reference_preemption(s, id, f, rescue_enabled, task_flag, object_order);
}
WorldPerceptionResult prepare_world_reference_preemption_consuming(
    AiRewardState &&s, CharacterId id, const WorldMapFacts &f, bool rescue_enabled, bool task_flag,
    const std::vector<std::uint64_t> &object_order) {
    return reference_preemption(std::move(s), id, f, rescue_enabled, task_flag, object_order);
}
CombatInfluenceResult prepare_world_influence(const AiRewardState &s, const WorldMapFacts &f) {
    if (!valid_world_map_facts(f))
        return {CombatAiError::invalid_input, {}};
    CombatInfluenceInput input;
    input.map_width = f.map.width;
    input.map_height = f.map.height;
    input.legacy_surface = f.surface;
    std::set<CharacterId> seen;
    for (const auto kind : {ActorKind::human, ActorKind::monster})
        for (const auto id : kind == ActorKind::human ? s.human_order : s.monster_order) {
            if (!live(s, id) || !seen.insert(id).second || s.battle.actors.at(id).kind != kind)
                return {CombatAiError::invalid_input, {}};
            const auto &ctx = s.contexts.at(id);
            (kind == ActorKind::human ? input.humans : input.monsters)
                .push_back({s.battle.actors.at(id).control.state, ctx.move_area, ctx.half_cell});
        }
    return prepare_combat_influence(input);
}
WorldHealingTargetResult query_world_healing_target(const AiRewardState &s, CharacterId id) {
    if (!live(s, id))
        return {AiRewardError::stale_actor, {}};
    std::vector<RescueTargetSnapshot> people;
    std::set<CharacterId> seen;
    for (const auto other : s.human_order) {
        if (!live(s, other) || !seen.insert(other).second ||
            s.battle.actors.at(other).kind != ActorKind::human)
            return {AiRewardError::invalid_input, {}};
        const auto &a = s.battle.actors.at(other);
        const auto &ctx = s.contexts.at(other);
        people.push_back({other,
                          a.control.state,
                          {a.position.x, a.position.z},
                          ctx.cell,
                          ctx.half_cell,
                          ctx.inside_town,
                          false,
                          ctx.low_hp});
    }
    return {AiRewardError::none, select_healing_target(s.contexts.at(id).half_cell, people)};
}
template <class State> static WorldPerceptionResult actor_projection(State &&s, CharacterId id) {
    if (!live(s, id))
        return fail(AiRewardError::stale_actor);
    const auto &a = s.battle.actors.at(id);
    const auto whole = character_world_cell({a.position.x, a.position.z});
    const auto half = half_cell(a.position);
    if (!whole || !half)
        return fail(AiRewardError::invalid_input);
    WorldPerceptionCandidate c;
    c.state = std::forward<State>(s);
    c.state.contexts.at(id).cell = *whole;
    c.state.contexts.at(id).half_cell = *half;
    return {AiRewardError::none, std::move(c)};
}
WorldPerceptionResult prepare_world_actor_projection(const AiRewardState &s, CharacterId id) {
    return actor_projection(s, id);
}
WorldPerceptionResult prepare_world_actor_projection_consuming(AiRewardState &&s, CharacterId id) {
    return actor_projection(std::move(s), id);
}
template <class State>
static WorldEventGateResult event_gate(State &&s, CharacterId id, const WorldMapFacts &f,
                                       const WorldEventTask &task) {
    if (!live(s, id) || !valid_world_map_facts(f))
        return {AiRewardError::invalid_input, {}};
    const auto &a = s.battle.actors.at(id);
    const auto &ctx = s.contexts.at(id);
    EventGateInput input;
    input.actor.kind = a.kind;
    input.actor.flags = a.control.flags;
    input.actor.state_counter = a.state_counter;
    input.actor.in_move_area = ctx.move_area;
    input.actor.has_object = a.object_slot != -1;
    input.actor.inside_town = ctx.inside_town;
    if (a.perceived_enemy) {
        const auto other = s.contexts.find(*a.perceived_enemy);
        if (!actor(s, *a.perceived_enemy) || other == s.contexts.end())
            return {AiRewardError::stale_actor, {}};
        input.actor.has_enemy = true;
        input.actor.enemy_inside_town = other->second.inside_town;
    }
    input.cell = ctx.cell;
    input.cell_in_map = in_map(ctx.cell, f);
    if (input.cell_in_map)
        input.cell_event_flag =
            (f.flags[static_cast<std::size_t>(ctx.cell.y * f.map.width + ctx.cell.x)] & 2U) != 0;
    input.previous_encounter = a.encounter;
    input.active_task = s.task_active;
    input.definition_task_flag = task.definition_task_flag;
    input.task_kind = task.kind;
    input.task_center = task.center;
    input.task_encounter = task.encounter;
    if (a.kind == ActorKind::human && input.cell_in_map && !input.actor.has_object &&
        a.state_counter >= 5 && !(a.control.flags & 512U) && ctx.move_area &&
        input.cell_event_flag) {
        if (s.encounter_order.size() != s.encounters.size())
            return {AiRewardError::invalid_input, {}};
        if (s.task_active && task.definition_task_flag && task.encounter) {
            const auto *bound = event(s, *task.encounter);
            if (!bound)
                return {AiRewardError::stale_encounter, {}};
            input.task_original_id = bound->legacy_id;
        }
        std::set<std::uint64_t> seen;
        for (const auto key : s.encounter_order) {
            if (!seen.insert(key).second || !s.encounters.count(key))
                return {AiRewardError::stale_encounter, {}};
            input.encounters.push_back(
                {key, s.encounters.at(key).runtime.center, s.encounters.at(key).legacy_id});
        }
    }
    const auto result = prepare_event_gate(input);
    if (!result.candidate)
        return {AiRewardError::preparation_failed, {}};
    WorldEventGateCandidate c{std::forward<State>(s), *result.candidate};
    if (c.gate.bind_encounter) {
        if (!event(c.state, *c.gate.bind_encounter))
            return {AiRewardError::stale_encounter, {}};
        c.state.battle.actors.at(id).encounter = c.gate.bind_encounter;
    }
    return {AiRewardError::none, std::move(c)};
}
WorldEventGateResult prepare_world_event_gate(const AiRewardState &s, CharacterId id,
                                              const WorldMapFacts &f, const WorldEventTask &task) {
    return event_gate(s, id, f, task);
}
WorldEventGateResult prepare_world_event_gate_consuming(AiRewardState &&s, CharacterId id,
                                                        const WorldMapFacts &f,
                                                        const WorldEventTask &task) {
    return event_gate(std::move(s), id, f, task);
}
template <class State>
static WorldPhysicsResult physics_projection(State &&s, CharacterId id, const WorldMapFacts &f) {
    if (!live(s, id) || !valid_world_map_facts(f))
        return {AiRewardError::invalid_input, {}};
    const auto &a = s.battle.actors.at(id);
    ActorPhysicsInput input;
    input.state = a.control.state;
    input.flags = a.control.flags;
    input.pause = a.physics_pause;
    input.height = a.position.height;
    input.vertical_velocity = a.vertical_velocity;
    input.position = {a.position.x, a.position.z};
    input.decision_start = {a.decision_start.x, a.decision_start.z};
    input.previous_area_after = a.area_after;
    input.blocked_battle_steps = a.blocked_battle_steps;
    // K ignores height; query only if original P/state guards admit it.
    if (a.physics_pause == 0 && a.control.state != 16) {
        const auto area = query_world_move_area(s, id, f);
        if (!area)
            return {AiRewardError::preparation_failed, {}};
        input.area_after = area->allowed;
    }
    const auto physics = prepare_actor_physics(input);
    if (!physics)
        return {AiRewardError::preparation_failed, {}};
    WorldPhysicsCandidate c{std::forward<State>(s), physics->query_area_after, physics->diagnostic};
    auto &next = c.state.battle.actors.at(id);
    next.physics_pause = physics->state.pause;
    next.vertical_velocity = physics->state.vertical_velocity;
    next.position = {physics->state.position.x, physics->state.height, physics->state.position.z};
    next.area_after = physics->state.previous_area_after;
    next.blocked_battle_steps = physics->state.blocked_battle_steps;
    auto projection = prepare_world_actor_projection_consuming(std::move(c.state), id);
    if (!projection.candidate)
        return {AiRewardError::preparation_failed, {}};
    c.state = std::move(projection.candidate->state);
    return {AiRewardError::none, std::move(c)};
}
WorldPhysicsResult prepare_world_physics_projection(const AiRewardState &s, CharacterId id,
                                                    const WorldMapFacts &f) {
    return physics_projection(s, id, f);
}
WorldPhysicsResult prepare_world_physics_projection_consuming(AiRewardState &&s, CharacterId id,
                                                              const WorldMapFacts &f) {
    return physics_projection(std::move(s), id, f);
}
WorldPerceptionResult prepare_world_encounter_influence(const AiRewardState &s, std::uint64_t id,
                                                        const CombatInfluenceCandidate &global) {
    if (!s.encounters.count(id) || !valid_combat_influence_field(global))
        return fail(AiRewardError::invalid_input);
    WorldPerceptionCandidate c;
    c.state = s;
    auto &e = c.state.encounters.at(id);
    e.influence = global;
    e.human_scratch = global.human_field;
    e.monster_scratch = global.monster_field;
    return {AiRewardError::none, std::move(c)};
}
WorldCombatMoveResult prepare_world_combat_move(const AiRewardState &s, CharacterId id,
                                                CharacterId enemy_id, const WorldMapFacts &f,
                                                bool low) {
    if (!live(s, id) || !actor(s, enemy_id) || !valid_world_map_facts(f))
        return {AiRewardError::invalid_input, {}};
    WorldCombatMoveCandidate c;
    c.state = s;
    const auto &a = s.battle.actors.at(id);
    const auto &target = *actor(s, enemy_id);
    if (!a.encounter) {
        c.diagnostic = 4;
        return {AiRewardError::none, std::move(c)};
    }
    RewardEncounter *e = nullptr;
    if (c.state.encounters.count(*a.encounter))
        e = &c.state.encounters.at(*a.encounter);
    else if (c.state.retired_encounters.count(*a.encounter))
        e = &c.state.retired_encounters.at(*a.encounter);
    if (!e)
        return {AiRewardError::stale_encounter, {}};
    if (!e->influence) {
        // New f construction allocates zero p/q/r/s with original half-map dimensions.
        CombatInfluenceCandidate empty;
        empty.width = f.map.width * 2;
        empty.height = f.map.height * 2;
        if (static_cast<std::uint64_t>(empty.width) * empty.height > 4000000)
            return {AiRewardError::invalid_input, {}};
        empty.human_field.resize(static_cast<std::size_t>(empty.width) * empty.height);
        empty.monster_field = empty.human_field;
        e->influence = empty;
    }
    const auto &field = *e->influence;
    if (!valid_combat_influence_field(field) || field.width != f.map.width * 2 ||
        field.height != f.map.height * 2 ||
        !character_world_cell({target.position.x, target.position.z}))
        return {AiRewardError::invalid_input, {}};
    auto &scratch = a.kind == ActorKind::human ? e->human_scratch : e->monster_scratch;
    scratch = a.kind == ActorKind::human ? field.human_field : field.monster_field;
    constexpr Position offsets[]{{-1, 1}, {0, 1},   {1, 1},  {-1, 0}, {0, 0},
                                 {1, 0},  {-1, -1}, {0, -1}, {1, -1}};
    const auto half = s.contexts.at(id).half_cell;
    std::array<CombatMoveSample, 9> samples;
    for (std::size_t n = 0; n < samples.size(); ++n) {
        const std::int64_t x = static_cast<std::int64_t>(half.x) + offsets[n].x;
        const std::int64_t y = static_cast<std::int64_t>(half.y) + offsets[n].y;
        auto &sample = samples[n];
        sample.in_half_grid = x >= 0 && x < field.width && y >= 0 && y < field.height;
        if (!sample.in_half_grid)
            continue;
        sample.in_encounter_square = std::abs(x / 2 - e->runtime.center.x) <= 1 &&
                                     std::abs(y / 2 - e->runtime.center.y) <= 1;
        sample.influence = scratch[static_cast<std::size_t>(y * field.width + x)];
        const float dx = target.position.x - (static_cast<float>(x) * 50.0F + 25.0F);
        const float dz = target.position.z - (static_cast<float>(y) * 50.0F + 25.0F);
        sample.enemy_distance = std::sqrt(dx * dx + dz * dz);
    }
    const auto scores = prepare_combat_step(samples, low);
    if (!scores.candidate)
        return {AiRewardError::preparation_failed, {}};
    c.scores = *scores.candidate;
    if (c.scores.selected) {
        const auto offset = offsets[*c.scores.selected];
        c.target =
            WorldPosition{(half.x + offset.x) * 50.0F + 25.0F, (half.y + offset.y) * 50.0F + 25.0F};
        const auto motion =
            advance_character_motion({a.position.x, a.position.z}, *c.target, a.control.flags);
        if (!motion.step)
            return {AiRewardError::preparation_failed, {}};
        auto &next = c.state.battle.actors.at(id);
        next.position.x = motion.step->position.x;
        next.position.z = motion.step->position.z;
        c.diagnostic = 6;
    }
    return {AiRewardError::none, std::move(c)};
}
template <class State>
static WorldExecutionPrefixResult execution_prefix(State &&s, CharacterId id) {
    const auto failed = [](AiRewardError error) -> WorldExecutionPrefixResult {
        return {error, {}};
    };
    if (!live(s, id))
        return failed(AiRewardError::stale_actor);
    const auto &original = s.battle.actors.at(id);
    const auto counters =
        advance_actor_counters({original.control.alternate_counter, original.control.action_counter,
                                original.state_counter, original.hit_flash, original.label_timer,
                                original.miss_label, original.damage_total, original.hit_count});
    const auto effects = advance_actor_effects(s.contexts.at(id).effects);
    if (!counters || !effects.candidate)
        return failed(AiRewardError::preparation_failed);
    const auto labels = expire_actor_hit_label(*counters);
    const auto hp = advance_hp_animation(original.hp);
    if (!labels || !hp.candidate)
        return failed(AiRewardError::preparation_failed);
    const bool carry_expression = original.object_slot == -2;
    WorldExecutionPrefixCandidate c{
        std::forward<State>(s), effects.candidate->sounds, {}, carry_expression};
    auto &a = c.state.battle.actors.at(id);
    a.control.alternate_counter = labels->alternate;
    a.control.action_counter = labels->action;
    a.state_counter = labels->state;
    a.hit_flash = labels->hit_flash;
    a.label_timer = labels->hit_label;
    a.miss_label = labels->miss_label;
    a.damage_total = labels->damage_total;
    a.hit_count = labels->hits;
    a.hp = *hp.candidate;
    c.state.contexts.at(id).effects = effects.candidate->state;
    if (a.kind == ActorKind::human) {
        auto growth = prepare_actor_growth_commit_consuming(std::move(c.state), id);
        if (!growth.candidate)
            return failed(growth.error);
        c.state = std::move(growth.candidate->state);
        c.growth_requests = std::move(growth.candidate->growth_requests);
    }
    return {AiRewardError::none, std::move(c)};
}
WorldExecutionPrefixResult prepare_world_execution_prefix(const AiRewardState &s, CharacterId id) {
    return execution_prefix(s, id);
}
WorldExecutionPrefixResult prepare_world_execution_prefix_consuming(AiRewardState &&s,
                                                                    CharacterId id) {
    return execution_prefix(std::move(s), id);
}
WorldBattlePreparationResult prepare_world_battle_preparation(const AiRewardState &s,
                                                              CharacterId id,
                                                              const WorldMapFacts &f, int mode) {
    const auto failed = [](AiRewardError error) -> WorldBattlePreparationResult {
        return {error, {}, {}};
    };
    if (!live(s, id) || s.battle.actors.at(id).control.state != 18 || !valid_world_map_facts(f))
        return failed(AiRewardError::invalid_input);
    const auto &a = s.battle.actors.at(id);
    const auto &ctx = s.contexts.at(id);
    BattleGateInput gate;
    gate.kind = a.kind;
    gate.flags = a.control.flags;
    gate.state_counter = a.state_counter;
    gate.in_move_area = ctx.move_area;
    gate.has_object = a.object_slot != -1;
    gate.has_enemy = a.perceived_enemy.has_value();
    gate.inside_town = ctx.inside_town;
    if (a.perceived_enemy) {
        if (!actor(s, *a.perceived_enemy) || !s.contexts.count(*a.perceived_enemy))
            return failed(AiRewardError::stale_actor);
        gate.enemy_inside_town = s.contexts.at(*a.perceived_enemy).inside_town;
    }
    const auto checked = prepare_battle_gate(gate);
    if (!checked.candidate)
        return failed(AiRewardError::preparation_failed);
    BattlePreparationInput input;
    input.battle_gate = checked.candidate->allowed;
    input.flags = a.control.flags;
    input.cell_in_map = in_map(ctx.cell, f);
    if (input.cell_in_map)
        input.cell_event_flag =
            (f.flags[static_cast<std::size_t>(ctx.cell.y * f.map.width + ctx.cell.x)] & 2U) != 0;
    // G/map early branches do not dereference the old db or scan bn.
    if (!input.battle_gate && !(input.cell_in_map && !input.cell_event_flag) && a.encounter) {
        const auto *bound = event(s, *a.encounter);
        if (!bound)
            return failed(AiRewardError::stale_encounter);
        input.encounter = BattlePreparationEncounter{bound->legacy_id, bound->runtime.center};
        if (s.encounter_order.size() != s.encounters.size())
            return failed(AiRewardError::invalid_input);
        std::set<std::uint64_t> seen;
        for (const auto key : s.encounter_order) {
            if (!seen.insert(key).second || !s.encounters.count(key))
                return failed(AiRewardError::invalid_input);
            const auto &e = s.encounters.at(key);
            input.live_encounters.push_back({e.legacy_id, e.runtime.center});
        }
    }
    const auto prepared = prepare_battle_preparation(input);
    auto state = s;
    auto &next = state.battle.actors.at(id);
    if (prepared.action == BattlePreparationAction::battle) {
        ActorStateTransitionInput transition;
        transition.control = next.control;
        transition.human = next.kind == ActorKind::human;
        transition.baseline = next.baseline;
        transition.next_state = 1;
        const auto changed = prepare_actor_state_transition(transition);
        if (!changed)
            return failed(AiRewardError::preparation_failed);
        next.control = changed->control;
        next.state_counter = next.state_parameter = 0;
    } else if (prepared.action == BattlePreparationAction::restore_baseline) {
        const auto restored = prepare_actor_baseline_restore(next.control, next.baseline,
                                                             next.kind == ActorKind::human, mode);
        if (!restored)
            return failed(AiRewardError::preparation_failed);
        next.control = restored->control;
        if (prepared.clear_encounter || restored->clear_encounter)
            next.encounter.reset();
    }
    return {AiRewardError::none, state, prepared};
}
} // namespace ark::simulation::rules
