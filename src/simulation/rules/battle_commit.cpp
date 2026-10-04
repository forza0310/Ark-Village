#include "ark/simulation/rules/battle_commit.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
namespace {
bool add(int &v, int amount) {
    const auto sum = static_cast<std::int64_t>(v) + amount;
    if (v < 0 || amount < 0 || sum > std::numeric_limits<int>::max())
        return false;
    v = static_cast<int>(sum);
    return true;
}
bool transition(BattleActorRecord &a, int state) {
    ActorStateTransitionInput i;
    i.control = a.control;
    i.human = a.kind == ActorKind::human;
    i.next_state = state;
    i.baseline = a.baseline;
    const auto p = prepare_actor_state_transition(i);
    if (!p)
        return false;
    a.control = p->control;
    a.baseline = p->baseline;
    a.state_counter = a.state_parameter = 0;
    if (p->copy_attack_position)
        a.position = a.attack_position;
    if (p->clear_encounter)
        a.encounter.reset();
    return true;
}
bool valid_actor(const BattleActorRecord &a) {
    return a.control.state >= 0 && a.control.state <= 20 && a.control.action >= 0 &&
           a.control.action <= 11 && a.control.action_counter >= 0 &&
           a.control.alternate_counter >= 0 && a.control.facing >= 0 && a.control.facing <= 3 &&
           std::all_of(a.control.queue.begin(), a.control.queue.end(), valid_actor_control) &&
           a.id.value != 0 && (a.kind == ActorKind::human || a.kind == ActorKind::monster) &&
           a.definition >= 0 && a.state_counter >= 0 && a.attack_count >= 0 && a.down_timer >= 0 &&
           a.object_slot >= -2 && (!a.rescue || a.rescue->value != 0);
}
} // namespace
BattleCommitResult prepare_battle_commit(const BattleCommitState &s, const BattleCommitInput &i) {
    const auto refuse = [](BattleCommitError e) { return BattleCommitResult{e, std::nullopt}; };
    const auto attacker_it = s.actors.find(i.attacker), target_it = s.actors.find(i.target);
    if (attacker_it == s.actors.end() || target_it == s.actors.end())
        return refuse(BattleCommitError::stale_actor);
    if (!valid_actor(attacker_it->second) || !valid_actor(target_it->second) ||
        i.attacker == i.target || attacker_it->second.kind == target_it->second.kind ||
        !(attacker_it->second.id == i.attacker) || !(target_it->second.id == i.target))
        return refuse(BattleCommitError::invalid_input);
    const auto &attacker = attacker_it->second;
    const auto &victim = target_it->second;
    const bool human_victim = victim.kind == ActorKind::human;
    const int human_id = human_victim ? victim.definition : attacker.definition;
    const int monster_id = human_victim ? attacker.definition : victim.definition;
    if (s.humans.count(human_id) == 0 || s.monsters.count(monster_id) == 0)
        return refuse(BattleCommitError::invalid_input);
    const auto &monster = s.monsters.at(monster_id);
    const auto matches = std::count(s.participants.begin(), s.participants.end(), human_id);
    if (matches > std::numeric_limits<int>::max())
        return refuse(BattleCommitError::numeric_overflow);
    HitContext context;
    context.attacker_miss = attacker.miss;
    context.attacker_kind = attacker.kind;
    context.weapon_kind = i.weapon_kind;
    context.attacker_visible = i.attacker_visible;
    context.attacker_first_visit = (attacker.control.flags & 8192U) != 0;
    context.target_carries_rescued_actor = human_victim && victim.object_slot == -2;
    context.rescue_reference = victim.rescue.has_value();
    context.target_attack_locked = victim.control.action == 3 || victim.control.action == 6 ||
                                   victim.control.action == 9 || victim.control.state == 8;
    context.victim_participant_matches = human_victim ? static_cast<int>(matches) : 0;
    context.killer_participant_matches = human_victim ? 0 : static_cast<int>(matches);
    context.task_encounter = victim.encounter && s.quest_encounters.count(*victim.encounter);
    context.global_down_count = s.global_downs;
    context.event131_present = s.events.count(131);
    context.event217_present = s.events.count(217);
    context.boss_flags4 = (monster.flags & 4U) != 0;
    context.monster_rank = monster.rank;
    context.monster_stat1 = monster.stat1;
    context.monster_statF = monster.statF;
    context.target_action = victim.control.action;
    context.drop_ticket = i.drop_ticket;
    context.draw = i.draw;
    const HitTargetState target{victim.kind,      victim.control.flags, victim.hp,
                                victim.capacity,  victim.damage_total,  victim.hit_count,
                                victim.hit_flash, victim.label_timer,   victim.miss_label};
    const auto hit = prepare_hit(target, i.damage, context);
    if (!hit.candidate)
        return refuse(BattleCommitError::preparation_failed);
    BattleCommitCandidate c{s, *hit.candidate, std::nullopt, {}};
    auto &a = c.state.actors.at(i.target);
    a.control.flags = c.hit.target.flags;
    a.hp = c.hit.target.hp;
    a.damage_total = c.hit.target.damage_total;
    a.hit_count = c.hit.target.hit_count;
    a.hit_flash = c.hit.target.hit_flash;
    a.label_timer = c.hit.target.label_timer;
    a.miss_label = c.hit.target.miss_label;
    auto &human = c.state.humans.at(human_id);
    auto &m = c.state.monsters.at(monster_id);
    for (const auto &r : c.hit.requests) {
        switch (r.kind) {
        case HitRequestKind::state:
            if (!transition(a, r.parameter))
                return refuse(BattleCommitError::preparation_failed);
            break;
        case HitRequestKind::reset_down_timer:
            a.down_timer = 0;
            break;
        case HitRequestKind::expression:
            if (i.expression) {
                if (!i.expression(c.state, i.target, r.parameter, 16))
                    return refuse(BattleCommitError::preparation_failed);
            } else if (i.draw) {
                return refuse(BattleCommitError::preparation_failed);
            } else {
                c.presentation.push_back(r);
            }
            break;
        case HitRequestKind::drop_rescued_actor: {
            if (!a.rescue)
                return refuse(BattleCommitError::stale_actor);
            const auto other = c.state.actors.find(*a.rescue);
            if (other == c.state.actors.end() || other->second.kind != ActorKind::human ||
                !valid_actor(other->second) || !transition(other->second, 2))
                return refuse(BattleCommitError::stale_actor);
            other->second.rescue.reset();
            break;
        }
        case HitRequestKind::clear_rescue_links:
            a.object_slot = -1;
            a.rescue.reset();
            break;
        case HitRequestKind::participant_down_count:
            if (!add(human.participant_downs, r.parameter))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        case HitRequestKind::global_down_count:
            if (!add(c.state.global_downs, r.parameter))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        case HitRequestKind::event131:
        case HitRequestKind::event217:
            c.state.events.insert(r.parameter);
            if (i.external_event) {
                const auto events = i.external_event(c.state, r.parameter);
                if (!events)
                    return refuse(BattleCommitError::preparation_failed);
                c.state.events.insert(events->begin(), events->end());
            } else if (i.draw) {
                return refuse(BattleCommitError::preparation_failed);
            }
            break;
        case HitRequestKind::clear_recent_reward_and_kills:
            human.recent_reward = human.recent_kills = 0;
            break;
        case HitRequestKind::monster_human_kills:
            if (!add(m.human_kills, r.parameter))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        case HitRequestKind::copy_attack_position:
            a.attack_position = a.position;
            break;
        case HitRequestKind::kill_count:
            if (!add(human.kills, r.parameter))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        case HitRequestKind::kill_stat1:
            if (!add(human.killed_stat1, r.parameter))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        case HitRequestKind::kill_statF:
            if (!add(human.battle_reward_stat, r.parameter))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        case HitRequestKind::record_monster: {
            const auto product = static_cast<std::int64_t>(m.death_reward) * 50;
            if (m.death_reward < 0 || product > std::numeric_limits<int>::max() ||
                !add(human.recent_reward, static_cast<int>(product / 100)) ||
                !add(human.recent_kills, 1))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        }
        case HitRequestKind::participant_task_kills:
            if (!add(human.task_kills, r.parameter))
                return refuse(BattleCommitError::numeric_overflow);
            break;
        case HitRequestKind::spawn_drop: {
            if (!i.drop_selection)
                return refuse(BattleCommitError::preparation_failed);
            auto input = *i.drop_selection;
            input.luck = human.luck;
            input.progress = s.drop_progress;
            if (!input.draw)
                input.draw = i.draw;
            const auto drop = prepare_drop_selection(input);
            if (!drop.candidate)
                return refuse(BattleCommitError::preparation_failed);
            c.drop = drop.candidate;
            if (c.drop->selected) {
                if (c.state.next_object_id == 0 ||
                    c.state.next_object_id == std::numeric_limits<std::uint64_t>::max() ||
                    c.state.objects.count(c.state.next_object_id))
                    return refuse(BattleCommitError::invalid_input);
                const auto object =
                    prepare_ground_drop({c.state.next_object_id}, a.position,
                                        c.drop->selected->kind, c.drop->selected->id);
                if (!object)
                    return refuse(BattleCommitError::preparation_failed);
                c.state.objects.emplace(c.state.next_object_id++, *object);
            }
            break;
        }
        case HitRequestKind::global_monster_record:
            c.state.defeated_definitions.push_back(monster_id);
            break;
        default:
            c.presentation.push_back(r);
            break;
        }
    }
    return {BattleCommitError::none, c};
}
} // namespace ark::simulation::rules
