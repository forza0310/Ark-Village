#include "dungeon_village_reference/combat_commit.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dungeon_village_reference {
namespace {
WorldAttackResult fail(AiRewardError e) { return {e, std::nullopt}; }
const BattleActorRecord *resolve(const AiRewardState &s, CharacterId id) {
    if (!id.value)
        return nullptr;
    const auto live = s.battle.actors.find(id);
    if (live != s.battle.actors.end() && live->second.id == id)
        return &live->second;
    const auto retired = s.retired_actors.find(id);
    return retired != s.retired_actors.end() && retired->second.id == id ? &retired->second
                                                                         : nullptr;
}
const RewardEncounter *encounter(const AiRewardState &s, std::uint64_t id) {
    const auto live = s.encounters.find(id);
    if (live != s.encounters.end())
        return &live->second;
    const auto retired = s.retired_encounters.find(id);
    return retired != s.retired_encounters.end() ? &retired->second : nullptr;
}
bool live(const AiRewardState &s, CharacterId id) {
    const auto a = s.battle.actors.find(id);
    return id.value && a != s.battle.actors.end() && a->second.id == id && s.contexts.count(id) &&
           (a->second.kind == ActorKind::human || a->second.kind == ActorKind::monster);
}
bool weapon_valid(const CombatWeaponRule &w) {
    return w.kind >= 0 && w.kind <= 3 && w.range >= 0 && w.combo >= 1 && w.miss_low >= 0 &&
           w.miss_low <= 100 && w.miss_high >= 0 && w.miss_high <= 100;
}
std::optional<EnemySelectionResult> enemy(const AiRewardState &s, CharacterId id) {
    const auto &a = s.battle.actors.at(id);
    EnemySelectionInput input;
    input.position = {a.position.x, a.position.z};
    input.active_battle_group = (a.control.flags & 128U) != 0;
    input.encounter_id = a.encounter;
    std::vector<CharacterId> roster;
    if (input.active_battle_group) {
        if (!a.group)
            return std::nullopt;
        const auto *e = encounter(s, *a.group);
        if (!e)
            return std::nullopt;
        const auto &opponents = a.kind == ActorKind::human ? e->group.monsters : e->group.humans;
        for (const auto &member : opponents)
            roster.push_back(member.id);
    } else
        roster = a.kind == ActorKind::human ? s.monster_order : s.human_order;
    for (const auto other : roster) {
        const auto *b = resolve(s, other);
        const auto ctx = s.contexts.find(other);
        if (!b || ctx == s.contexts.end())
            return std::nullopt;
        input.opposite_roster.push_back({other,
                                         b->control.state,
                                         ctx->second.move_area,
                                         {b->position.x, b->position.z},
                                         b->encounter});
    }
    return select_combat_enemy(input);
}
std::optional<CharacterId> healing_target(const AiRewardState &s, CharacterId id, bool &valid) {
    std::vector<RescueTargetSnapshot> people;
    for (const auto other : s.human_order) {
        if (!live(s, other) || s.battle.actors.at(other).kind != ActorKind::human) {
            valid = false;
            return {};
        }
        const auto &a = s.battle.actors.at(other);
        const auto &ctx = s.contexts.at(other);
        people.push_back({other,
                          a.control.state,
                          {a.position.x, a.position.z},
                          ctx.cell,
                          ctx.half_cell,
                          ctx.inside_town,
                          false,
                          a.hp.target < a.capacity / 2});
    }
    valid = true;
    return select_healing_target(s.contexts.at(id).half_cell, people);
}
bool facing(std::optional<int> value) { return value && *value >= 0 && *value <= 3; }
bool launch(WorldAttackCandidate &c, CharacterId actor, CharacterId target, ProjectileKind kind,
            std::optional<int> direction, int effect = 0, int damage = 0) {
    if (!facing(direction) || c.state.next_projectile_id == 0 ||
        c.state.next_projectile_id == std::numeric_limits<std::uint64_t>::max() ||
        c.state.projectiles.count(c.state.next_projectile_id))
        return false;
    const auto projectile =
        prepare_projectile(kind, actor, target, c.state.battle.actors.at(actor).position,
                           resolve(c.state, target)->position, *direction, effect, damage);
    if (!projectile.candidate)
        return false;
    c.state.battle.actors.at(actor).control.facing = *direction;
    c.projectile = c.state.next_projectile_id++;
    c.state.projectiles.emplace(*c.projectile, *projectile.candidate);
    c.state.projectile_order.push_back(*c.projectile);
    return true;
}
bool hit(WorldAttackCandidate &c, const WorldAttackInput &i, CharacterId target) {
    const auto damage = prepare_actor_physical_damage(c.state, i.actor, target, i.physical_jitter);
    if (!damage.candidate)
        return false;
    c.physical_damage = damage.candidate;
    const auto previous_object = c.state.battle.next_object_id;
    const bool retired = !c.state.battle.actors.count(target);
    if (retired)
        c.state.battle.actors.emplace(target, c.state.retired_actors.at(target));
    const auto result = prepare_battle_commit(
        c.state.battle, {i.actor, target, damage.candidate->value, i.weapon.kind, i.actor_visible,
                         i.drop_ticket, i.drop_selection});
    if (!result.candidate)
        return false;
    c.hit = result.candidate->hit;
    c.state.battle = result.candidate->state;
    if (retired) {
        c.state.retired_actors.at(target) = c.state.battle.actors.at(target);
        c.state.battle.actors.erase(target);
    }
    if (previous_object != c.state.battle.next_object_id)
        c.objects.push_back(previous_object);
    if (c.hit->landed)
        c.requests.push_back({WorldAttackVisual::contact, i.actor, target, 0});
    return true;
}
} // namespace
WorldAttackResult prepare_world_attack_setup(const AiRewardState &s,
                                             const WorldAttackSetupInput &i) {
    if (!live(s, i.actor) || !resolve(s, i.target) || i.actor == i.target)
        return fail(AiRewardError::stale_actor);
    if (!weapon_valid(i.weapon) || !facing(i.facing))
        return fail(AiRewardError::invalid_input);
    WorldAttackCandidate c;
    c.state = s;
    auto &a = c.state.battle.actors.at(i.actor);
    const auto &target = *resolve(s, i.target);
    if (a.kind == target.kind || a.attack_count < 0 ||
        a.attack_count == std::numeric_limits<int>::max())
        return fail(AiRewardError::invalid_input);
    ++a.attack_count;
    a.control.queue.clear();
    a.attack_armed = true;
    a.control.flags |= 4U;
    a.control.facing = *i.facing;
    c.target = i.target;
    if (a.kind == ActorKind::human) {
        const auto growth = s.growth.find(a.definition);
        if (growth == s.growth.end())
            return fail(AiRewardError::invalid_input);
        const auto setup = prepare_human_attack(
            {growth->second.derived.attributes[2], i.weapon.kind, i.weapon.combo, i.weapon.miss_low,
             i.weapon.miss_high, (a.control.flags & 2048U) != 0, i.human_tickets});
        if (!setup.candidate)
            return fail(AiRewardError::preparation_failed);
        a.miss = setup.candidate->miss;
        a.combo_index = 0;
        a.combo_count = setup.candidate->combo_count;
        a.control.queue = setup.candidate->queue;
        c.requests.push_back({WorldAttackVisual::expression, i.actor, {}, 1});
    } else {
        if (!i.monster_miss_ticket || *i.monster_miss_ticket < 0 || *i.monster_miss_ticket >= 100)
            return fail(AiRewardError::invalid_input);
        a.miss = *i.monster_miss_ticket < 12;
        const float dx = target.position.x - a.position.x, dz = target.position.z - a.position.z;
        const float distance = std::sqrt(dx * dx + dz * dz);
        if (!std::isfinite(distance) || distance == 0)
            return fail(AiRewardError::invalid_input);
        const float span = std::max(distance - 70.0F, distance / 4.0F);
        a.attack_destination = {a.position.x + dx * span / distance, 0,
                                a.position.z + dz * span / distance};
        a.control.queue = {{3, 6}, {17}, {3, 0}, {1, 10, 0}, {7, 4}};
    }
    return {AiRewardError::none, c};
}
WorldAttackResult prepare_world_attack_control(const AiRewardState &s, const WorldAttackInput &i) {
    if (!live(s, i.actor))
        return fail(AiRewardError::stale_actor);
    const auto &original = s.battle.actors.at(i.actor);
    if (!weapon_valid(i.weapon) || original.control.queue.empty() ||
        !valid_actor_control(original.control.queue.front()))
        return fail(AiRewardError::invalid_input);
    const int opcode = original.control.queue.front()[0];
    if (opcode < 14 || opcode > 17)
        return fail(AiRewardError::invalid_input);
    WorldAttackCandidate c;
    c.state = s;
    if (opcode == 14) {
        if (original.kind != ActorKind::human || original.control.action_counter < 0 ||
            original.control.action_counter > 1000000 || original.combo_index < 0 ||
            original.combo_count < 1 || original.combo_index >= original.combo_count)
            return fail(AiRewardError::invalid_input);
        HumanAttackFrame window{i.weapon.kind,
                                original.control.action_counter,
                                original.combo_index,
                                original.combo_count,
                                original.attack_armed,
                                false,
                                0,
                                i.weapon.range,
                                original.control.action};
        const auto probe = prepare_human_attack_frame(window);
        if (!probe.candidate)
            return fail(AiRewardError::preparation_failed);
        const auto selected = probe.candidate->query_enemy
                                  ? enemy(s, i.actor)
                                  : std::optional<EnemySelectionResult>(EnemySelectionResult{});
        if (!selected || selected->error != ActorAiError::none)
            return fail(AiRewardError::preparation_failed);
        c.target = selected->candidate ? std::optional<CharacterId>(selected->candidate->id)
                                       : std::nullopt;
        window.enemy_found = c.target.has_value();
        window.enemy_distance = selected->candidate ? selected->candidate->world_distance : 0;
        const auto frame = prepare_human_attack_frame(window);
        if (!frame.candidate)
            return fail(AiRewardError::preparation_failed);
        auto &a = c.state.battle.actors.at(i.actor);
        a.control.action_counter = frame.candidate->counter;
        a.combo_index = frame.candidate->combo_index;
        a.attack_armed = frame.candidate->armed;
        c.completed = frame.candidate->completed;
        if (frame.candidate->request_damage && !hit(c, i, *c.target))
            return fail(AiRewardError::preparation_failed);
        if (frame.candidate->request_arrow &&
            !launch(c, i.actor, *c.target, ProjectileKind::arrow, i.facing))
            return fail(AiRewardError::preparation_failed);
    } else if (opcode == 15 || opcode == 16) {
        if (original.kind != ActorKind::human || !s.growth.count(original.definition))
            return fail(AiRewardError::invalid_input);
        const auto &growth = s.growth.at(original.definition);
        auto frame =
            prepare_spell_frame(original.control.action_counter, false, original.control.action);
        if (!frame)
            return fail(AiRewardError::preparation_failed);
        if (frame->prepare_visual)
            c.requests.push_back(
                {opcode == 15 ? WorldAttackVisual::spell_source : WorldAttackVisual::healing_source,
                 i.actor,
                 {},
                 0});
        if (frame->query_target) {
            if (opcode == 15) {
                const auto selected = enemy(s, i.actor);
                if (!selected || selected->error != ActorAiError::none)
                    return fail(AiRewardError::preparation_failed);
                if (selected->candidate)
                    c.target = selected->candidate->id;
            } else {
                bool valid{};
                c.target = healing_target(s, i.actor, valid);
                if (!valid)
                    return fail(AiRewardError::preparation_failed);
            }
            if (c.target) {
                if (opcode == 15) {
                    const auto damage = prepare_spell_damage(
                        {growth.derived.combat[3],
                         {growth.derived.available_spells[0], growth.derived.available_spells[1],
                          growth.derived.available_spells[2]},
                         (original.control.flags & 2048U) != 0,
                         i.spell_ticket,
                         i.enhancement_ticket,
                         i.magic_jitter});
                    if (!damage.candidate ||
                        !launch(c, i.actor, *c.target, ProjectileKind::spell, i.facing,
                                damage.candidate->effect, damage.candidate->damage.value))
                        return fail(AiRewardError::preparation_failed);
                } else {
                    const auto amount =
                        prepare_healing_amount(growth.derived.combat[3], i.magic_jitter);
                    auto &target = c.state.battle.actors.at(*c.target);
                    if (!amount.candidate || target.capacity <= 0 || target.state_counter < 0)
                        return fail(AiRewardError::preparation_failed);
                    const auto hp =
                        prepare_hp_change(target.hp, amount.candidate->value, target.capacity);
                    if (!hp.candidate)
                        return fail(AiRewardError::preparation_failed);
                    target.hp = *hp.candidate;
                    // a(amount,target) also advances down recovery proportionally to the new HP.
                    if (target.control.state == 2) {
                        const int counter =
                            static_cast<int>(static_cast<std::int64_t>(
                                                 std::clamp(target.hp.target, 0, target.capacity)) *
                                             900 / target.capacity);
                        target.state_counter = std::max(target.state_counter, counter);
                    }
                    c.state.contexts.at(*c.target).effects.display.push_back(
                        {2, 0, amount.candidate->value, 160, -26, 2});
                    c.requests.push_back(
                        {WorldAttackVisual::healing_target, i.actor, *c.target, 0});
                }
            }
        }
        frame = prepare_spell_frame(original.control.action_counter, c.target.has_value(),
                                    original.control.action);
        c.completed = frame->completed;
        if (c.completed)
            c.state.battle.actors.at(i.actor).attack_idle = 0;
    } else {
        if (original.kind != ActorKind::monster || original.body < 0 || original.body > 3)
            return fail(AiRewardError::invalid_input);
        MonsterAttackFrame input;
        input.action = original.control.action;
        input.counter = original.control.action_counter;
        input.armed = original.attack_armed;
        input.origin = original.position;
        input.destination = original.attack_destination;
        input.previous_position = original.attack_position;
        input.half_cell = s.contexts.at(i.actor).half_cell;
        constexpr int ranges[]{100, 130, 130, 130};
        input.range = ranges[original.body];
        input.preferred = original.perceived_enemy;
        for (const auto id : s.human_order) {
            if (!live(s, id))
                return fail(AiRewardError::stale_actor);
            const auto &a = s.battle.actors.at(id);
            const auto &ctx = s.contexts.at(id);
            const int state = a.control.state;
            const bool eligible = ctx.move_area && state != 2 && state != 3 && state != 8 &&
                                  state != 9 && state != 14 && state != 15 && state != 16;
            input.humans.push_back({id, a.position, ctx.half_cell, eligible});
        }
        const auto frame = prepare_monster_attack_frame(input);
        if (!frame.candidate)
            return fail(AiRewardError::preparation_failed);
        c.target = frame.candidate->damage_target;
        c.state.battle.actors.at(i.actor).attack_armed = frame.candidate->armed;
        if (c.target && !hit(c, i, *c.target))
            return fail(AiRewardError::preparation_failed);
        c.state.battle.actors.at(i.actor).attack_position = frame.candidate->position;
        if (c.hit && c.hit->landed)
            c.requests.push_back({WorldAttackVisual::expression, *c.target, {}, 0});
        c.completed = frame.candidate->completed;
    }
    if (c.completed)
        c.state.battle.actors.at(i.actor).control.queue.erase(
            c.state.battle.actors.at(i.actor).control.queue.begin());
    c.early_stop = !c.completed;
    return {AiRewardError::none, c};
}
} // namespace dungeon_village_reference
