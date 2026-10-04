#include "ark/simulation/rules/ai_rewards.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation::rules {
namespace {
AiRewardResult fail(AiRewardError e) { return {e, std::nullopt}; }
BattleActorRecord *resolve(AiRewardState &s, CharacterId id) {
    const auto live = s.battle.actors.find(id);
    if (live != s.battle.actors.end())
        return &live->second;
    const auto retired = s.retired_actors.find(id);
    return retired == s.retired_actors.end() ? nullptr : &retired->second;
}
RewardEncounter *resolve_event(AiRewardState &s, std::uint64_t id) {
    const auto live = s.encounters.find(id);
    if (live != s.encounters.end())
        return &live->second;
    const auto retired = s.retired_encounters.find(id);
    return retired != s.retired_encounters.end() ? &retired->second : nullptr;
}
void collect_retired(AiRewardState &s) {
    std::set<CharacterId> references;
    std::set<std::uint64_t> encounters;
    std::vector<CharacterId> actors_to_visit;
    std::vector<std::uint64_t> events_to_visit;
    const auto actor = [&](CharacterId id) {
        if (references.insert(id).second)
            actors_to_visit.push_back(id);
    };
    const auto event = [&](std::uint64_t id) {
        if (encounters.insert(id).second)
            events_to_visit.push_back(id);
    };
    for (const auto id : s.external_actor_roots)
        actor(id);
    for (const auto id : s.facility_actor_roots)
        actor(id);
    for (const auto id : s.external_encounter_roots)
        event(id);
    for (const auto &[id, a] : s.battle.actors) {
        (void)a;
        actor(id);
    }
    for (const auto &[id, e] : s.encounters) {
        (void)e;
        event(id);
    }
    for (const auto &[id, projectile] : s.projectiles) {
        (void)id;
        actor(projectile.caster);
        actor(projectile.original_target);
    }
    // Actor<->event and rescue cycles are traced only when reachable from a live root.
    while (!actors_to_visit.empty() || !events_to_visit.empty()) {
        if (!actors_to_visit.empty()) {
            const auto id = actors_to_visit.back();
            actors_to_visit.pop_back();
            if (const auto *a = resolve(s, id)) {
                if (a->rescue)
                    actor(*a->rescue);
                if (a->follow)
                    actor(*a->follow);
                if (a->perceived_enemy)
                    actor(*a->perceived_enemy);
                if (a->encounter)
                    event(*a->encounter);
                if (a->group)
                    event(*a->group);
            }
        } else {
            const auto id = events_to_visit.back();
            events_to_visit.pop_back();
            const auto live = s.encounters.find(id);
            const auto retired = s.retired_encounters.find(id);
            const auto *e = live != s.encounters.end()              ? &live->second
                            : retired != s.retired_encounters.end() ? &retired->second
                                                                    : nullptr;
            if (!e)
                continue;
            for (const auto member : e->members)
                actor(member);
            for (const auto *roster : {&e->group.humans, &e->group.monsters})
                for (const auto &member : *roster)
                    actor(member.id);
        }
    }
    for (auto it = s.retired_actors.begin(); it != s.retired_actors.end();)
        if (references.count(it->first) == 0) {
            s.contexts.erase(it->first);
            it = s.retired_actors.erase(it);
        } else
            ++it;
    for (auto it = s.retired_encounters.begin(); it != s.retired_encounters.end();)
        if (encounters.count(it->first) == 0)
            it = s.retired_encounters.erase(it);
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
    auto *bound = actor.encounter ? resolve_event(c.state, *actor.encounter) : nullptr;
    if (actor.encounter) {
        if (!bound)
            return fail(AiRewardError::stale_encounter);
        auto &members = bound->members;
        const auto member = std::find(members.begin(), members.end(), id);
        if (member != members.end())
            members.erase(member); // Vector.removeElement removes first matching reference only.
    }
    if (actor.state_parameter == 0) {
        auto &m = c.state.monster_growth.at(actor.definition);
        auto &battle = c.state.battle.monsters.at(actor.definition);
        const bool boss = (battle.flags & 4U) != 0;
        const auto old_reward = prepare_monster_growth(m.base_death_reward, m.growth, 1, boss);
        if (!old_reward || (bound && !add(bound->runtime.reward, *old_reward)) ||
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
        for (auto &[other_id, other] : c.state.retired_actors) {
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
    collect_retired(c.state);
    return {AiRewardError::none, c};
}
EncounterExternalWriteback encounter_external_writeback(const AiRewardState &s) {
    return {s.battle.events,           s.pending_completion, s.task_active,
            s.task_completed,          s.feature16,          s.external_actor_roots,
            s.external_encounter_roots};
}
std::optional<std::set<int>>
consume_world_combat_event(AiRewardState &owner, const BattleCommitState &battle, int event,
                           const WorldCombatEventConsumer &consumer,
                           std::optional<std::vector<std::array<int, 3>>> &popularity_queue,
                           bool required) {
    if (!consumer)
        return required ? std::nullopt : std::optional<std::set<int>>(battle.events);
    auto source = owner;
    source.battle = battle;
    const auto fields = consumer(source, event);
    if (!fields || fields->globals.pending_completion < 0 || fields->globals.monster_availability)
        return {};
    owner.pending_completion = fields->globals.pending_completion;
    owner.task_active = fields->globals.task_active;
    owner.task_completed = fields->globals.task_completed;
    owner.feature16 = fields->globals.feature16;
    owner.external_actor_roots = fields->globals.external_actor_roots;
    owner.external_encounter_roots = fields->globals.external_encounter_roots;
    if (fields->popularity_queue)
        popularity_queue = fields->popularity_queue;
    auto events = battle.events;
    events.insert(fields->globals.events.begin(), fields->globals.events.end());
    return events;
}
bool encounter_request_needs_external(EncounterRequestKind kind) {
    switch (kind) {
    case EncounterRequestKind::clear_task:
    case EncounterRequestKind::refresh_map:
    case EncounterRequestKind::event:
    case EncounterRequestKind::page30:
    case EncounterRequestKind::page31:
    case EncounterRequestKind::completion_delta:
    case EncounterRequestKind::mark_task_complete:
    case EncounterRequestKind::refresh_global:
    case EncounterRequestKind::refresh_task_catalog:
    case EncounterRequestKind::set_feature16:
        return true;
    default:
        return false;
    }
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
    input.draw = i.draw;
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
        auto event_id = actor->second.encounter;
        if (event_id) {
            const auto live = s.encounters.find(*event_id);
            const auto retired = s.retired_encounters.find(*event_id);
            const auto *bound = live != s.encounters.end()              ? &live->second
                                : retired != s.retired_encounters.end() ? &retired->second
                                                                        : nullptr;
            if (!bound)
                return fail(AiRewardError::stale_encounter);
            if (bound->legacy_id == encounter->second.legacy_id)
                event_id = i.encounter; // Source count/cancel compares f164b, not db pointer.
        }
        input.monsters.push_back({id, event_id});
    }
    AiRewardCandidate c{s, false, {}, {}, {}};
    std::optional<EncounterSpawnCandidate> planned_spawn;
    bool spawned = true;
    auto request_error = AiRewardError::none;
    const auto reject_request = [&](AiRewardError error) {
        request_error = error;
        return false;
    };
    const auto consume_request = [&](const EncounterRequest &r) -> bool {
        BattleActorRecord *a{};
        RewardActorContext *ctx{};
        if (r.actor) {
            const auto actor = c.state.battle.actors.find(*r.actor);
            const auto context = c.state.contexts.find(*r.actor);
            if (actor == c.state.battle.actors.end() || context == c.state.contexts.end())
                return reject_request(AiRewardError::stale_actor);
            a = &actor->second;
            ctx = &context->second;
        }
        switch (r.kind) {
        case EncounterRequestKind::update_group: {
            const auto group =
                prepare_battle_group_commit(c.state, i.encounter, i.posture_tickets, i.draw);
            if (!group.candidate)
                return reject_request(group.error);
            c.state = group.candidate->state;
            break;
        }
        case EncounterRequestKind::snapshot_influence:
            // Group update precedes spawning; spawning precedes influence snapshot.
            if (!spawned) {
                if (!i.spawn_offset_tickets && !i.draw)
                    return reject_request(AiRewardError::preparation_failed);
                c.state.encounters.at(i.encounter).runtime.spawned = input.state.spawned;
                const auto spawn = prepare_encounter_monster_spawn(
                    c.state, i.encounter, *planned_spawn,
                    i.spawn_offset_tickets.value_or(std::array<int, 2>{}),
                    i.spawn_offset_tickets ? CombatRandomDraw{} : i.draw);
                if (!spawn.candidate)
                    return reject_request(spawn.error);
                c.state = spawn.candidate->state;
                spawned = true; // Constructor commits the single planned count increment.
            }
            if (i.snapshot_field) {
                if (!valid_combat_influence_field(*i.snapshot_field))
                    return reject_request(AiRewardError::invalid_input);
                auto &e = c.state.encounters.at(i.encounter);
                e.influence = i.snapshot_field;
                e.human_scratch = i.snapshot_field->human_field;
                e.monster_scratch = i.snapshot_field->monster_field;
            }
            break;
        case EncounterRequestKind::cancel_monster:
            if (!a || !transition(*a, 3))
                return reject_request(AiRewardError::preparation_failed);
            a->state_parameter = 1;
            a->attack_position = a->position;
            break;
        case EncounterRequestKind::state10:
            if (!a || !transition(*a, 10, ctx->facility_category))
                return reject_request(AiRewardError::preparation_failed);
            break;
        case EncounterRequestKind::reset_attack_count:
            if (!a)
                return reject_request(AiRewardError::invalid_input);
            a->attack_count = 0;
            break;
        case EncounterRequestKind::reset_hp: {
            if (!a)
                return reject_request(AiRewardError::invalid_input);
            const auto hp = prepare_hp_assignment(a->hp, a->capacity);
            if (!hp.candidate)
                return reject_request(AiRewardError::preparation_failed);
            a->hp = *hp.candidate;
            break;
        }
        case EncounterRequestKind::reset_recent_reward_and_kills:
            if (!a || !c.state.battle.humans.count(a->definition))
                return reject_request(AiRewardError::invalid_input);
            c.state.battle.humans.at(a->definition).recent_reward = 0;
            c.state.battle.humans.at(a->definition).recent_kills = 0;
            break;
        case EncounterRequestKind::reward_display:
            if (!ctx)
                return reject_request(AiRewardError::invalid_input);
            ctx->effects.display.push_back({24, -r.delay, 0, r.value, 0, 0});
            break;
        case EncounterRequestKind::reward_accumulation: {
            if (!a || !c.state.growth.count(a->definition))
                return reject_request(AiRewardError::invalid_input);
            auto &pending = c.state.growth.at(a->definition).pending;
            const auto reward = prepare_delayed_reward(pending, r.value, r.delay);
            if (!reward)
                return reject_request(AiRewardError::preparation_failed);
            pending = *reward;
            c.state.growth.at(a->definition).notice_pending = false; // e.c(delay,amount) clears P.
            break;
        }
        case EncounterRequestKind::clear_boost2048:
            if (!a)
                return reject_request(AiRewardError::invalid_input);
            a->control.flags &= ~2048U;
            break;
        case EncounterRequestKind::event:
            c.state.battle.events.insert(r.parameter);
            break;
        case EncounterRequestKind::completion_delta:
            if (!add(c.state.pending_completion, r.parameter))
                return reject_request(AiRewardError::preparation_failed);
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
        case EncounterRequestKind::expression5:
        case EncounterRequestKind::task_victory_expression:
            if (i.random_request) {
                auto next = i.random_request(c.state, r);
                if (!next)
                    return reject_request(AiRewardError::preparation_failed);
                c.state = std::move(*next);
            } else if (i.draw)
                return reject_request(AiRewardError::preparation_failed);
            break;
        default:
            break; // External map/UI consumers remain typed, ordered requests.
        }
        return true;
    };
    // 在事件原req位置同步处理group和逐人物表现/领域写回，避免共享随机游标反序。
    input.synchronous_request = [&](const EncounterRuntimeState &runtime,
                                    const EncounterRequest &request) {
        c.state.encounters.at(i.encounter).runtime = runtime;
        // 实际spawn计划尚未返回，生成当轮随后无事件抽取；无spawn先同步真实field。
        if (request.kind == EncounterRequestKind::snapshot_influence)
            return runtime.spawned != input.state.spawned || consume_request(request);
        if (!consume_request(request))
            return false;
        if (i.external_request && encounter_request_needs_external(request.kind)) {
            const auto fields = i.external_request(c.state, request);
            if (!fields || fields->pending_completion < 0)
                return reject_request(AiRewardError::preparation_failed);
            c.state.battle.events.insert(fields->events.begin(), fields->events.end());
            c.state.pending_completion = fields->pending_completion;
            c.state.task_active = fields->task_active;
            c.state.task_completed = fields->task_completed;
            c.state.feature16 = fields->feature16;
            c.state.external_actor_roots = fields->external_actor_roots;
            c.state.external_encounter_roots = fields->external_encounter_roots;
            if (fields->monster_availability) {
                if (request.kind != EncounterRequestKind::mark_task_complete)
                    return reject_request(AiRewardError::preparation_failed);
                for (const auto &entry : *fields->monster_availability) {
                    const auto current = c.state.monster_growth.find(entry.first);
                    if (current == c.state.monster_growth.end() || entry.second[0] < 0 ||
                        entry.second[0] > 1 || entry.second[1] < 0 || entry.second[1] > 1)
                        return reject_request(AiRewardError::preparation_failed);
                    current->second.status = entry.second[0];
                    current->second.newly_unlocked = entry.second[1] != 0;
                }
            }
        }
        return true;
    };
    const auto step = prepare_encounter_step(input);
    if (!step.candidate)
        return fail(request_error != AiRewardError::none ? request_error
                                                         : AiRewardError::preparation_failed);
    c.removed = step.candidate->remove;
    c.encounter_requests = step.candidate->requests;
    c.state.encounters.at(i.encounter).runtime = step.candidate->state;
    planned_spawn = step.candidate->spawn;
    spawned = !planned_spawn;
    for (const auto &request : c.encounter_requests)
        if (request.kind == EncounterRequestKind::snapshot_influence && !consume_request(request))
            return fail(request_error);
    if (c.removed) {
        if (c.state.retired_encounters.count(i.encounter))
            return fail(AiRewardError::invalid_input);
        c.state.retired_encounters.emplace(i.encounter, c.state.encounters.at(i.encounter));
        c.state.encounters.erase(i.encounter);
        const auto order =
            std::find(c.state.encounter_order.begin(), c.state.encounter_order.end(), i.encounter);
        if (order != c.state.encounter_order.end())
            c.state.encounter_order.erase(order);
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
        for (auto &[other_id, other] : c.state.retired_actors) {
            (void)other_id;
            if (other.kind == ActorKind::human && other.definition == actor->second.definition)
                other.capacity = g.derived.combat[0];
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
    const auto retired = s.retired_encounters.find(encounter);
    const auto *bound = e != s.encounters.end()                 ? &e->second
                        : retired != s.retired_encounters.end() ? &retired->second
                                                                : nullptr;
    if (!bound || !bound->group_exists)
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
    auto &group = (e != s.encounters.end() ? c.state.encounters.at(encounter)
                                           : c.state.retired_encounters.at(encounter))
                      .group;
    const auto append = [&group](const BattleActorRecord &record) {
        auto &roster = record.kind == ActorKind::human ? group.humans : group.monsters;
        roster.push_back({record.id, record.control.flags});
    };
    append(actor);
    append(b->second);
    return {AiRewardError::none, c};
}
AiRewardResult prepare_battle_group_commit(const AiRewardState &s, std::uint64_t encounter,
                                           const std::vector<int> &tickets,
                                           const CombatRandomDraw &draw) {
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
    const auto step = prepare_battle_group_step(group, tickets, draw);
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
                                               const std::array<int, 2> &explicit_tickets,
                                               const CombatRandomDraw &draw) {
    auto tickets = explicit_tickets;
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
        spawn.cell.x < 0 || spawn.cell.y < 0 || spawn.cell.x > 9998 || spawn.cell.y > 9998)
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
    if (draw) {
        for (auto &ticket : tickets) {
            const auto drawn = draw(100);
            if (!drawn || *drawn < 0 || *drawn >= 100)
                return fail(AiRewardError::preparation_failed);
            ticket = *drawn;
        }
    }
    if (tickets[0] < 0 || tickets[0] >= 100 || tickets[1] < 0 || tickets[1] >= 100)
        return fail(AiRewardError::invalid_input);
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
    c.state.contexts.at(actor.id).half_cell = {spawn.cell.x * 2 + 1, spawn.cell.y * 2 + 1};
    c.state.monster_order.push_back(actor.id);
    c.state.encounters.at(encounter).members.push_back(actor.id);
    if (!add(c.state.encounters.at(encounter).runtime.spawned, 1))
        return fail(AiRewardError::preparation_failed);
    ++c.state.next_actor_id;
    return {AiRewardError::none, c};
}
WorldProjectileResult prepare_world_projectile(const AiRewardState &s,
                                               const WorldProjectileInput &i) {
    const auto fail = [](AiRewardError error) {
        return WorldProjectileResult{error, std::nullopt};
    };
    const auto projectile = s.projectiles.find(i.projectile);
    if (projectile == s.projectiles.end() ||
        std::count(s.projectile_order.begin(), s.projectile_order.end(), i.projectile) != 1)
        return fail(AiRewardError::invalid_input);
    WorldProjectileCandidate c{s, {}, {}, {}, {}, {}};
    const auto &p = projectile->second;
    ProjectileContext context;
    const auto caster = resolve(c.state, p.caster), original = resolve(c.state, p.original_target);
    context.caster_reference = caster != nullptr;
    context.original_target_reference = original != nullptr;
    if (caster && original && p.kind != ProjectileKind::delayed_damage) {
        if (!i.box)
            return fail(AiRewardError::invalid_input);
        context.box = *i.box;
        std::set<CharacterId> seen;
        for (const auto id : s.monster_order) {
            const auto actor = s.battle.actors.find(id);
            if (actor == s.battle.actors.end() || !seen.insert(id).second ||
                !(actor->second.id == id) || actor->second.kind != ActorKind::monster ||
                actor->second.body < 0 || actor->second.body > 3 ||
                !i.monster_boxes[actor->second.body])
                return fail(AiRewardError::stale_actor);
            context.monsters.push_back(
                {id, actor->second.position, *i.monster_boxes[actor->second.body]});
        }
    }
    const auto step = advance_projectile(p, context);
    if (!step.candidate)
        return fail(AiRewardError::preparation_failed);
    c.step = *step.candidate;
    if (c.step.damage_target) {
        const auto victim = resolve(c.state, *c.step.damage_target);
        if (!caster || !victim || caster->kind != ActorKind::human ||
            victim->kind != ActorKind::monster)
            return fail(AiRewardError::stale_actor);
        int damage = p.damage;
        if (c.step.physical_damage) {
            const auto physical = prepare_actor_physical_damage(s, p.caster, *c.step.damage_target,
                                                                i.physical_jitter, i.draw);
            if (!physical.candidate)
                return fail(AiRewardError::preparation_failed);
            c.physical_damage = physical.candidate;
            damage = physical.candidate->value;
        }
        // Temporarily project retained actors for the shared hit consumer, then restore ownership.
        std::vector<CharacterId> retained;
        for (const auto id : {p.caster, *c.step.damage_target})
            if (!c.state.battle.actors.count(id)) {
                c.state.battle.actors.emplace(id, c.state.retired_actors.at(id));
                retained.push_back(id);
            }
        const auto old_next_object = c.state.battle.next_object_id;
        const auto hit = prepare_battle_commit(
            c.state.battle,
            {p.caster, *c.step.damage_target, damage, i.current_weapon_kind, i.caster_visible,
             i.drop_ticket, i.drop_selection, i.draw,
             [&](const BattleCommitState &, CharacterId actor, int kind, int delay) {
                 const auto found = c.state.contexts.find(actor);
                 if (!i.expression)
                     return !i.draw;
                 if (found == c.state.contexts.end())
                     return false;
                 auto effects = i.expression(actor, found->second.effects, kind, delay);
                 if (!effects || !valid_actor_effect_state(*effects))
                     return false;
                 found->second.effects = std::move(*effects);
                 return true;
             },
             [&](const BattleCommitState &battle, int event) {
                 return consume_world_combat_event(c.state, battle, event, i.event,
                                                   c.popularity_queue, static_cast<bool>(i.draw));
             }});
        if (!hit.candidate)
            return fail(AiRewardError::preparation_failed);
        c.hit = hit.candidate->hit;
        c.state.battle = hit.candidate->state;
        if (c.state.battle.next_object_id != old_next_object)
            c.spawned_objects.push_back(old_next_object);
        for (const auto id : retained) {
            c.state.retired_actors.at(id) = c.state.battle.actors.at(id);
            c.state.battle.actors.erase(id);
        }
    }
    if (c.step.spawned) {
        if (c.state.next_projectile_id == 0 ||
            c.state.next_projectile_id == std::numeric_limits<std::uint64_t>::max() ||
            c.state.projectiles.count(c.state.next_projectile_id))
            return fail(AiRewardError::invalid_input);
        c.spawned_projectile = c.state.next_projectile_id++;
        c.state.projectiles.emplace(*c.spawned_projectile, *c.step.spawned);
        c.state.projectile_order.push_back(*c.spawned_projectile);
    }
    if (c.step.remove) {
        c.state.projectiles.erase(i.projectile);
        const auto id = std::find(c.state.projectile_order.begin(), c.state.projectile_order.end(),
                                  i.projectile);
        c.state.projectile_order.erase(id);
    } else
        c.state.projectiles.at(i.projectile) = c.step.state;
    collect_retired(c.state);
    return {AiRewardError::none, c};
}
AiRewardState collect_ai_references(AiRewardState state) {
    collect_retired(state);
    return state;
}
DamageResult prepare_actor_physical_damage(const AiRewardState &s, CharacterId attacker,
                                           CharacterId target, std::optional<int> jitter,
                                           const CombatRandomDraw &draw) {
    const auto find = [&](CharacterId id) -> const BattleActorRecord * {
        const auto live = s.battle.actors.find(id);
        if (live != s.battle.actors.end())
            return &live->second;
        const auto retired = s.retired_actors.find(id);
        return retired != s.retired_actors.end() ? &retired->second : nullptr;
    };
    const auto *a = find(attacker), *b = find(target);
    if (!a || !b || a->kind == b->kind)
        return {CombatAiError::invalid_input, {}};
    const auto *human = a->kind == ActorKind::human ? a : b;
    const auto *monster = a->kind == ActorKind::monster ? a : b;
    const auto growth = s.growth.find(human->definition);
    const auto definition = s.monster_growth.find(monster->definition);
    const auto shared = s.battle.monsters.find(monster->definition);
    if (growth == s.growth.end() || definition == s.monster_growth.end() ||
        shared == s.battle.monsters.end())
        return {CombatAiError::invalid_input, {}};
    const bool attacking = a->kind == ActorKind::monster;
    const auto effective = prepare_monster_growth(
        attacking ? definition->second.base_attack : definition->second.base_defense,
        definition->second.growth, 0, (shared->second.flags & 4U) != 0);
    if (!effective)
        return {CombatAiError::invalid_input, {}};
    return prepare_physical_damage(
        {a->kind, attacking ? *effective : growth->second.derived.combat[1],
         attacking ? growth->second.derived.combat[2] : *effective,
         (human->control.flags & 2048U) != 0, (monster->control.flags & 4096U) != 0, jitter, draw});
}
} // namespace ark::simulation::rules
