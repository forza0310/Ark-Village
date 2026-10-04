#include "dungeon_village_reference/ai_rewards.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
AiRewardResult fail(AiRewardError e) { return {e, std::nullopt}; }
BattleActorRecord *resolve(AiRewardState &s, CharacterId id) {
    const auto live = s.battle.actors.find(id);
    if (live != s.battle.actors.end())
        return &live->second;
    const auto retired = s.retired_actors.find(id);
    return retired == s.retired_actors.end() ? nullptr : &retired->second;
}
void collect_retired(AiRewardState &s) {
    std::set<CharacterId> references;
    for (const auto &[id, encounter] : s.encounters) {
        (void)id;
        references.insert(encounter.members.begin(), encounter.members.end());
        for (const auto *roster : {&encounter.group.humans, &encounter.group.monsters})
            for (const auto &member : *roster)
                references.insert(member.id);
    }
    for (auto it = s.retired_actors.begin(); it != s.retired_actors.end();)
        if (references.count(it->first) == 0)
            it = s.retired_actors.erase(it);
        else
            ++it;
}
bool add(int &value, int amount) {
    const auto sum = static_cast<std::int64_t>(value) + amount;
    if (sum < 0 || sum > std::numeric_limits<int>::max())
        return false;
    value = static_cast<int>(sum);
    return true;
}
bool transition(BattleActorRecord &a, int state, std::optional<int> category = {}) {
    ActorStateTransitionInput input;
    input.control = a.control;
    input.human = a.kind == ActorKind::human;
    input.baseline = a.baseline;
    input.next_state = state;
    input.current_facility_category = category;
    const auto t = prepare_actor_state_transition(input);
    if (!t || t->request_cleanup)
        return false; // A bound inn requires the facility owner's r(), never silently skip it.
    a.control = t->control;
    a.baseline = t->baseline;
    a.state_counter = a.state_parameter = 0;
    if (t->clear_encounter)
        a.encounter.reset();
    if (t->reset_attack_count)
        a.attack_count = 0;
    if (t->copy_attack_position)
        a.position = a.attack_position;
    return true;
}
} // namespace
AiRewardResult prepare_monster_death_commit(const AiRewardState &s, CharacterId id) {
    const auto a = s.battle.actors.find(id);
    if (a == s.battle.actors.end() || a->second.kind != ActorKind::monster || !(a->second.id == id))
        return fail(AiRewardError::stale_actor);
    const auto &actor = a->second;
    if (actor.control.state != 3 || actor.state_counter < 0 ||
        s.battle.monsters.count(actor.definition) == 0 ||
        s.monster_growth.count(actor.definition) == 0 || s.contexts.count(id) == 0)
        return fail(AiRewardError::invalid_input);
    TimedLifecycleInput input;
    input.kind = ActorKind::monster;
    input.state = 3;
    input.old_counter = actor.state_counter;
    input.death_parameter = actor.state_parameter;
    input.position = {actor.position.x, actor.position.z};
    const auto step = prepare_timed_lifecycle(input);
    if (!step.candidate)
        return fail(AiRewardError::preparation_failed);
    AiRewardCandidate c{s, step.candidate->delete_instance, step.candidate->requests, {}, {}};
    if (!c.removed)
        return {AiRewardError::none, c};
    if (actor.encounter) {
        const auto encounter = c.state.encounters.find(*actor.encounter);
        if (encounter == c.state.encounters.end())
            return fail(AiRewardError::stale_encounter);
        auto &members = encounter->second.members;
        const auto member = std::find(members.begin(), members.end(), id);
        if (member != members.end())
            members.erase(member); // Vector.removeElement removes first matching reference only.
    }
    if (actor.state_parameter == 0) {
        auto &m = c.state.monster_growth.at(actor.definition);
        auto &battle = c.state.battle.monsters.at(actor.definition);
        const bool boss = (battle.flags & 4U) != 0;
        const auto old_reward = prepare_monster_growth(m.base_death_reward, m.growth, 1, boss);
        if (!old_reward ||
            (actor.encounter &&
             !add(c.state.encounters.at(*actor.encounter).runtime.reward, *old_reward)) ||
            !add(m.defeats, 1) || !add(m.growth, 1))
            return fail(AiRewardError::preparation_failed);
        const auto cash = prepare_monster_growth(m.base_cash_reward, m.growth, 1, boss);
        const auto reward = prepare_monster_growth(m.base_death_reward, m.growth, 1, boss);
        const auto capacity = prepare_monster_growth(m.base_hp, m.growth, 0, boss);
        if (!cash || !reward || !capacity || c.state.next_cash_id == 0 ||
            c.state.accounting.entries().count(c.state.next_cash_id) ||
            c.state.next_cash_id == std::numeric_limits<std::uint64_t>::max())
            return fail(AiRewardError::preparation_failed);
        battle.death_reward = *reward;
        battle.statF = *cash;
        for (auto &[other_id, other] : c.state.battle.actors) {
            (void)other_id;
            if (other.kind == ActorKind::monster && other.definition == actor.definition)
                other.capacity = *capacity;
        }
        if (c.state.accounting.post_cash({c.state.next_cash_id++, c.state.period,
                                          CashCategory::monsters, CashDirection::income, *cash}) !=
            AccountingError::none)
            return fail(AiRewardError::preparation_failed);
    }
    const auto member = std::find(c.state.monster_order.begin(), c.state.monster_order.end(), id);
    if (member == c.state.monster_order.end())
        return fail(AiRewardError::stale_actor);
    c.state.monster_order.erase(member);
    if (c.state.retired_actors.count(id))
        return fail(AiRewardError::invalid_input);
    c.state.retired_actors.emplace(id, c.state.battle.actors.at(id));
    c.state.battle.actors.erase(id);
    c.state.contexts.erase(id);
    collect_retired(c.state);
    return {AiRewardError::none, c};
}
AiRewardResult prepare_encounter_reward_commit(const AiRewardState &s,
                                               const EncounterCommitInput &i) {
    const auto encounter = s.encounters.find(i.encounter);
    if (encounter == s.encounters.end() || encounter->second.runtime.id != i.encounter)
        return fail(AiRewardError::stale_encounter);
    EncounterStepInput input;
    input.state = encounter->second.runtime;
    input.group_exists = encounter->second.group_exists;
    input.task_exists = s.task_active;
    input.quest = i.quest;
    input.quest.participants = s.battle.participants;
    input.quest_cells = i.cells;
    input.tickets = i.tickets;
    input.town_overlap = i.town_overlap;
    input.event91_present = s.battle.events.count(91);
    input.event128_present = s.battle.events.count(128);
    input.event205_present = s.battle.events.count(205);
    input.feature16 = s.feature16;
    std::set<CharacterId> seen;
    for (const auto id : s.human_order) {
        const auto actor = s.battle.actors.find(id);
        const auto context = s.contexts.find(id);
        if (!seen.insert(id).second || actor == s.battle.actors.end() ||
            context == s.contexts.end() || actor->second.kind != ActorKind::human ||
            !(actor->second.id == id) || !s.battle.humans.count(actor->second.definition))
            return fail(AiRewardError::stale_actor);
        const auto &a = actor->second;
        const auto &shared = s.battle.humans.at(a.definition);
        input.humans.push_back({id, a.definition, context->second.cell, context->second.inside_town,
                                a.control.state, a.attack_count, a.control.flags,
                                shared.recent_reward, shared.recent_kills});
    }
    seen.clear();
    for (const auto id : s.monster_order) {
        const auto actor = s.battle.actors.find(id);
        if (!seen.insert(id).second || actor == s.battle.actors.end() ||
            actor->second.kind != ActorKind::monster || !(actor->second.id == id))
            return fail(AiRewardError::stale_actor);
        input.monsters.push_back({id, actor->second.encounter});
    }
    const auto step = prepare_encounter_step(input);
    if (!step.candidate)
        return fail(AiRewardError::preparation_failed);
    AiRewardCandidate c{s, step.candidate->remove, {}, step.candidate->requests, {}};
    c.state.encounters.at(i.encounter).runtime = step.candidate->state;
    bool spawned = !step.candidate->spawn;
    for (const auto &r : c.encounter_requests) {
        BattleActorRecord *a{};
        RewardActorContext *ctx{};
        if (r.actor) {
            const auto actor = c.state.battle.actors.find(*r.actor);
            const auto context = c.state.contexts.find(*r.actor);
            if (actor == c.state.battle.actors.end() || context == c.state.contexts.end())
                return fail(AiRewardError::stale_actor);
            a = &actor->second;
            ctx = &context->second;
        }
        switch (r.kind) {
        case EncounterRequestKind::update_group: {
            const auto group = prepare_battle_group_commit(c.state, i.encounter, i.posture_tickets);
            if (!group.candidate)
                return fail(group.error);
            c.state = group.candidate->state;
            break;
        }
        case EncounterRequestKind::snapshot_influence:
            // Group update precedes spawning; spawning precedes influence snapshot.
            if (!spawned) {
                if (!i.spawn_offset_tickets)
                    return fail(AiRewardError::preparation_failed);
                c.state.encounters.at(i.encounter).runtime.spawned = input.state.spawned;
                const auto spawn = prepare_encounter_monster_spawn(
                    c.state, i.encounter, *step.candidate->spawn, *i.spawn_offset_tickets);
                if (!spawn.candidate)
                    return fail(spawn.error);
                c.state = spawn.candidate->state;
                spawned = true; // Constructor commits the single planned count increment.
            }
            break;
        case EncounterRequestKind::cancel_monster:
            if (!a || !transition(*a, 3))
                return fail(AiRewardError::preparation_failed);
            a->state_parameter = 1;
            a->attack_position = a->position;
            break;
        case EncounterRequestKind::state10:
            if (!a || !transition(*a, 10, ctx->facility_category))
                return fail(AiRewardError::preparation_failed);
            break;
        case EncounterRequestKind::reset_attack_count:
            if (!a)
                return fail(AiRewardError::invalid_input);
            a->attack_count = 0;
            break;
        case EncounterRequestKind::reset_hp: {
            if (!a)
                return fail(AiRewardError::invalid_input);
            const auto hp = prepare_hp_assignment(a->hp, a->capacity);
            if (!hp.candidate)
                return fail(AiRewardError::preparation_failed);
            a->hp = *hp.candidate;
            break;
        }
        case EncounterRequestKind::reset_recent_reward_and_kills:
            if (!a || !c.state.battle.humans.count(a->definition))
                return fail(AiRewardError::invalid_input);
            c.state.battle.humans.at(a->definition).recent_reward = 0;
            c.state.battle.humans.at(a->definition).recent_kills = 0;
            break;
        case EncounterRequestKind::reward_display:
            if (!ctx)
                return fail(AiRewardError::invalid_input);
            ctx->effects.display.push_back({24, -r.delay, 0, r.value, 0, 0});
            break;
        case EncounterRequestKind::reward_accumulation: {
            if (!a || !c.state.growth.count(a->definition))
                return fail(AiRewardError::invalid_input);
            auto &pending = c.state.growth.at(a->definition).pending;
            const auto reward = prepare_delayed_reward(pending, r.value, r.delay);
            if (!reward)
                return fail(AiRewardError::preparation_failed);
            pending = *reward;
            c.state.growth.at(a->definition).notice_pending = false; // e.c(delay,amount) clears P.
            break;
        }
        case EncounterRequestKind::clear_boost2048:
            if (!a)
                return fail(AiRewardError::invalid_input);
            a->control.flags &= ~2048U;
            break;
        case EncounterRequestKind::event:
            c.state.battle.events.insert(r.parameter);
            break;
        case EncounterRequestKind::completion_delta:
            if (!add(c.state.pending_completion, r.parameter))
                return fail(AiRewardError::preparation_failed);
            break;
        case EncounterRequestKind::mark_task_complete:
            c.state.task_completed = true;
            break;
        case EncounterRequestKind::clear_task:
            c.state.task_active = false;
            break;
        case EncounterRequestKind::set_feature16:
            c.state.feature16 = true;
            break;
        default:
            break; // Map, battle group, expressions and UI remain typed, ordered requests.
        }
    }
    if (c.removed) {
        c.state.encounters.erase(i.encounter);
        c.state.battle.quest_encounters.erase(i.encounter);
    }
    collect_retired(c.state);
    return {AiRewardError::none, c};
}
AiRewardResult prepare_actor_growth_commit(const AiRewardState &s, CharacterId id) {
    const auto actor = s.battle.actors.find(id);
    const auto context = s.contexts.find(id);
    if (actor == s.battle.actors.end() || context == s.contexts.end() ||
        actor->second.kind != ActorKind::human || !(actor->second.id == id))
        return fail(AiRewardError::stale_actor);
    const auto shared = s.growth.find(actor->second.definition);
    if (shared == s.growth.end())
        return fail(AiRewardError::invalid_input);
    HumanGrowthInput input;
    input.definition = shared->second.definition;
    input.professions = s.professions;
    input.experience = shared->second.experience;
    input.pending = shared->second.pending;
    input.notice_pending = shared->second.notice_pending;
    input.notice_attributes = shared->second.notice_attributes;
    input.effects = context->second.effects;
    input.event109_seen = s.battle.events.count(109);
    input.event113_seen = s.battle.events.count(113);
    const auto step = prepare_human_growth(input);
    if (!step.candidate)
        return fail(AiRewardError::preparation_failed);
    AiRewardCandidate c{s, false, {}, {}, step.candidate->requests};
    auto &g = c.state.growth.at(actor->second.definition);
    g.definition = step.candidate->definition;
    g.experience = step.candidate->experience;
    g.pending = step.candidate->pending;
    g.notice_pending = step.candidate->notice_pending;
    g.notice_attributes = step.candidate->notice_attributes;
    if (step.candidate->stats) {
        g.derived = *step.candidate->stats;
        if (!c.state.battle.humans.count(actor->second.definition))
            return fail(AiRewardError::invalid_input);
        c.state.battle.humans.at(actor->second.definition).luck = g.derived.attributes[5];
        for (auto &[other_id, other] : c.state.battle.actors) {
            (void)other_id;
            if (other.kind == ActorKind::human && other.definition == actor->second.definition)
                other.capacity = g.derived.combat[0]; // h() reads shared w0; current HP unchanged.
        }
    }
    c.state.contexts.at(id).effects = step.candidate->effects;
    for (const auto &r : c.growth_requests) {
        if (r.kind == HumanGrowthRequestKind::event109)
            c.state.battle.events.insert(109);
        else if (r.kind == HumanGrowthRequestKind::event113)
            c.state.battle.events.insert(113);
        else if (r.kind == HumanGrowthRequestKind::unlock_profession)
            c.state.professions.at(static_cast<std::size_t>(r.profession)).unlocked = true;
    }
    return {AiRewardError::none, c};
}
AiRewardResult prepare_battle_group_join(const AiRewardState &s, std::uint64_t encounter,
                                         CharacterId caller, CharacterId opponent) {
    const auto e = s.encounters.find(encounter);
    if (e == s.encounters.end() || !e->second.group_exists)
        return fail(AiRewardError::stale_encounter);
    const auto a = s.battle.actors.find(caller), b = s.battle.actors.find(opponent);
    if (a == s.battle.actors.end() || b == s.battle.actors.end() || !(a->second.id == caller) ||
        !(b->second.id == opponent) || a->second.kind == b->second.kind)
        return fail(AiRewardError::stale_actor);
    AiRewardCandidate c{s, false, {}, {}, {}};
    auto &actor = c.state.battle.actors.at(caller);
    actor.control.flags |= 128U;
    actor.attack_slot = 0;
    actor.group = encounter;
    auto &group = c.state.encounters.at(encounter).group;
    const auto append = [&group](const BattleActorRecord &record) {
        auto &roster = record.kind == ActorKind::human ? group.humans : group.monsters;
        roster.push_back({record.id, record.control.flags});
    };
    append(actor);
    append(b->second);
    return {AiRewardError::none, c};
}
AiRewardResult prepare_battle_group_commit(const AiRewardState &s, std::uint64_t encounter,
                                           const std::vector<int> &tickets) {
    const auto e = s.encounters.find(encounter);
    if (e == s.encounters.end() || !e->second.group_exists)
        return fail(AiRewardError::stale_encounter);
    AiRewardCandidate c{s, false, {}, {}, {}};
    auto group = e->second.group;
    for (auto *roster : {&group.humans, &group.monsters})
        for (auto &member : *roster) {
            const auto actor = resolve(c.state, member.id);
            if (!actor || !(actor->id == member.id) ||
                actor->kind != (roster == &group.humans ? ActorKind::human : ActorKind::monster))
                return fail(AiRewardError::stale_actor);
            member.flags = actor->control.flags;
        }
    const auto step = prepare_battle_group_step(group, tickets);
    if (!step.candidate)
        return fail(AiRewardError::preparation_failed);
    c.state.encounters.at(encounter).group = step.candidate->state;
    // c.f.b ignores g.a's return: disband empties vectors, not event.i or actor.dc.
    for (const auto &slot : step.candidate->assignments) {
        auto &actor = *resolve(c.state, slot.id);
        actor.attack_slot = slot.slot;
        if (slot.monster_posture)
            actor.monster_posture = *slot.monster_posture;
    }
    for (const auto id : step.candidate->release_group_flag)
        resolve(c.state, id)->control.flags &= ~128U;
    collect_retired(c.state);
    return {AiRewardError::none, c};
}
AiRewardResult prepare_encounter_monster_spawn(const AiRewardState &s, std::uint64_t encounter,
                                               const EncounterSpawnCandidate &spawn,
                                               const std::array<int, 2> &tickets) {
    if (!s.encounters.count(encounter))
        return fail(AiRewardError::stale_encounter);
    const auto definition = s.monster_growth.find(spawn.definition);
    const auto shared = s.battle.monsters.find(spawn.definition);
    if (definition == s.monster_growth.end() || shared == s.battle.monsters.end() ||
        s.next_actor_id == 0 || s.next_actor_id == std::numeric_limits<std::uint64_t>::max() ||
        s.battle.actors.count({s.next_actor_id}) || s.retired_actors.count({s.next_actor_id}) ||
        s.contexts.count({s.next_actor_id}))
        return fail(AiRewardError::invalid_input);
    const auto &d = definition->second;
    if (d.body < 0 || d.body > 3 || d.sprite_variant < 0 || d.sprite_variant >= 30 ||
        spawn.cell.x < 0 || spawn.cell.y < 0 || spawn.cell.x > 9998 || spawn.cell.y > 9998 ||
        tickets[0] < 0 || tickets[0] >= 100 || tickets[1] < 0 || tickets[1] >= 100)
        return fail(AiRewardError::invalid_input);
    const bool boss = (shared->second.flags & 4U) != 0;
    const auto capacity = prepare_monster_growth(d.base_hp, d.growth, 0, boss);
    if (!capacity || *capacity <= 0)
        return fail(AiRewardError::preparation_failed);
    std::set<int> legacy;
    for (const auto id : s.monster_order) {
        const auto actor = s.battle.actors.find(id);
        if (actor == s.battle.actors.end() || actor->second.kind != ActorKind::monster ||
            actor->second.legacy_id < 0)
            return fail(AiRewardError::stale_actor);
        legacy.insert(actor->second.legacy_id);
    }
    int first_free{};
    while (legacy.count(first_free)) {
        if (first_free == std::numeric_limits<int>::max())
            return fail(AiRewardError::invalid_input);
        ++first_free;
    }
    BattleActorRecord actor;
    actor.id = {s.next_actor_id};
    actor.legacy_id = first_free;
    actor.kind = ActorKind::monster;
    actor.definition = spawn.definition;
    actor.body = d.body;
    actor.sprite = d.body * 30 + d.sprite_variant;
    actor.capacity = *capacity;
    actor.hp = *prepare_hp_assignment(actor.hp, *capacity).candidate;
    actor.control.flags =
        2U | (boss && d.growth > 0 ? 4096U : 0U) | (spawn.boss_instance ? 16384U : 0U);
    actor.position = {spawn.cell.x * 100.0F + 50.0F + (-40.0F + tickets[0] * 80.0F / 99.0F), 0,
                      spawn.cell.y * 100.0F + 50.0F + (-40.0F + tickets[1] * 80.0F / 99.0F)};
    if (!transition(actor, 8))
        return fail(AiRewardError::preparation_failed);
    actor.encounter = encounter;
    AiRewardCandidate c{s, false, {}, {}, {}};
    c.state.battle.actors.emplace(actor.id, actor);
    c.state.contexts.emplace(actor.id, RewardActorContext{spawn.cell, false, {}, {}});
    c.state.monster_order.push_back(actor.id);
    c.state.encounters.at(encounter).members.push_back(actor.id);
    if (!add(c.state.encounters.at(encounter).runtime.spawned, 1))
        return fail(AiRewardError::preparation_failed);
    ++c.state.next_actor_id;
    return {AiRewardError::none, c};
}
} // namespace dungeon_village_reference
