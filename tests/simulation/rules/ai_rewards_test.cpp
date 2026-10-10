#include "ark/simulation/ai/rules/ai_rewards.hpp"
#include "ark/simulation/ai/rules/ai_schedule.hpp"
#include "ark/simulation/ai/rules/world_perception.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
AiRewardResult growth_commit(bool consuming, const AiRewardState &source, CharacterId id) {
    if (!consuming)
        return prepare_actor_growth_commit(source, id);
    auto disposable = source;
    return prepare_actor_growth_commit_consuming(std::move(disposable), id);
}
AiRewardState fixture() {
    AiRewardState s;
    BattleActorRecord h;
    h.id = {1};
    h.definition = 1;
    h.control.flags = 2U | 128U | 2048U;
    h.control.state = 1;
    h.capacity = 100;
    h.hp = {0, 100, 100, 100, false, 0};
    h.baseline = 5;
    h.encounter = 0;
    h.attack_count = 3;
    h.position = {500, 0, 500};
    auto m = h;
    m.id = {2};
    m.definition = 7;
    m.kind = ActorKind::monster;
    m.baseline = 17;
    s.battle.actors = {{h.id, h}, {m.id, m}};
    s.battle.humans.emplace(1, HumanBattleRecord{});
    s.battle.monsters.emplace(7, MonsterBattleRecord{0, 0, 100, 100, 1, 0U});
    s.human_order = {{1}};
    s.monster_order = {{2}};
    s.contexts = {{{1}, {{5, 5}, false, {}, {}}}, {{2}, {{5, 5}, false, {}, {}}}};
    s.monster_growth.emplace(7, RewardMonsterDefinition{0, 99, 100, 100, 100});
    s.encounters.emplace(0, RewardEncounter{{0, {5, 5}, 0, 0, 0, 1, 1, 0}, {{2}}, true, {}});
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    s.professions.push_back({{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 5, true});
    g.derived = *derive_human_stats(g.definition, s.professions).candidate;
    g.notice_pending = true;
    s.growth.emplace(1, g);
    return s;
}
void death_boundary() {
    for (bool cancelled : {false, true})
        for (int counter = 0; counter <= 16; ++counter) {
            auto s = fixture();
            auto &a = s.battle.actors.at({2});
            a.control.state = 3;
            a.state_parameter = cancelled ? 1 : 0;
            a.state_counter = counter;
            const auto c = prepare_monster_death_commit(s, {2});
            if (!c.candidate)
                std::cerr << "death counter=" << counter << " cancelled=" << cancelled
                          << " error=" << static_cast<int>(c.error) << '\n';
            check(c.candidate && c.candidate->removed == (counter >= 12),
                  "c reads old B; death at12 not at11");
            const bool reward = counter >= 12 && !cancelled;
            check(c.candidate->state.encounters.at(0).runtime.reward == (reward ? 100 : 0) &&
                      c.candidate->state.monster_growth.at(7).growth == (reward ? 100 : 99) &&
                      c.candidate->state.accounting.funds() == (reward ? 120 : 0),
                  "event d before growth increment; cash e after v99->100; cancelled no rewards");
            check(c.candidate->state.battle.actors.count({2}) == (counter < 12 ? 1U : 0U) &&
                      c.candidate->state.encounters.at(0).members.size() ==
                          (counter < 12 ? 1U : 0U) &&
                      s.battle.actors.count({2}) == 1 && s.accounting.funds() == 0,
                  "owner atomically removes corpse/member without changing source");
        }
    auto s = fixture();
    s.battle.actors.at({2}).control.state = 3;
    s.battle.actors.at({2}).state_counter = 12;
    s.monster_growth.at(7).defeats = std::numeric_limits<int>::max();
    check(!prepare_monster_death_commit(s, {2}).candidate &&
              s.encounters.at(0).members.size() == 1 && s.encounters.at(0).runtime.reward == 0,
          "late shared count failure rolls back prior event-member removal and reward");
}
void shared_and_quest(bool consuming) {
    auto s = fixture();
    s.battle.actors.erase({2});
    s.contexts.erase({2});
    s.monster_order.clear();
    s.encounters.at(0).members.clear();
    s.encounters.at(0).runtime.state = 3;
    s.encounters.at(0).runtime.reward = 100;
    s.battle.quest_encounters.insert(0);
    s.battle.humans.at(1).recent_reward = 17;
    s.battle.humans.at(1).recent_kills = 1;
    s.battle.participants = {1, 1};
    s.task_active = true;
    EncounterCommitInput i;
    i.quest.completion_delta = 8;
    i.tickets = {{100, 99}, {100, 99}, {2, 0}, {2, 1}};
    const auto c = prepare_encounter_reward_commit(s, i);
    check(c.candidate && c.candidate->state.growth.at(1).pending.amount == 117 &&
              c.candidate->state.growth.at(1).pending.counter == -44 &&
              !c.candidate->state.growth.at(1).notice_pending,
          "duplicate quest member gets67 then50, pending merges117 and clears P");
    const auto &actor = c.candidate->state.battle.actors.at({1});
    check(actor.control.state == 10 && !(actor.control.flags & 2048U) &&
              c.candidate->state.contexts.at({1}).effects.display ==
                  std::vector<ActorEffectRecord>{{24, -16, 0, 67, 0, 0}, {24, -16, 0, 50, 0, 0}} &&
              c.candidate->state.pending_completion == 8 && c.candidate->state.task_completed &&
              !c.candidate->state.task_active && c.candidate->state.accounting.funds() == 0,
          "quest display order, boost clearing, completion pending, no imaginary cash payment");
    s.contexts.at({1}).facility_category = 2;
    check(!prepare_encounter_reward_commit(s, i).candidate && s.task_active &&
              s.battle.humans.at(1).recent_reward == 17,
          "bound inn requires facility cleanup, whole reward owner rejects not silently skip r");
    s = c.candidate->state;
    auto duplicate = s.battle.actors.at({1});
    duplicate.id = {3};
    duplicate.hp.target = 20;
    s.battle.actors.emplace(duplicate.id, duplicate);
    s.contexts.emplace(duplicate.id, RewardActorContext{});
    s.human_order.push_back(duplicate.id);
    s.growth.at(1).pending = {9, 0};
    s.growth.at(1).experience = 0;
    auto step = growth_commit(consuming, s, {3});
    check(step.candidate && step.candidate->state.growth.at(1).experience == 1 &&
              step.candidate->state.battle.actors.at({3}).hp.target == 20,
          "definition grows per calling instance, no heal");
    step = growth_commit(consuming, step.candidate->state, {1});
    check(step.candidate && step.candidate->state.growth.at(1).experience == 2 &&
              step.candidate->state.growth.at(1).pending.counter == 2,
          "second instance same definition advances shared O again, not deduplicated");
    s.growth.at(1).definition.profession_levels[0] = 9;
    s.professions[0].unlocked = false;
    s.growth.at(1).pending = {9, 0};
    s.growth.at(1).experience = *human_growth_threshold(9, 5);
    step = growth_commit(consuming, s, {3});
    check(step.candidate && step.candidate->state.professions[0].unlocked &&
              step.candidate->state.battle.events.count(109) &&
              step.candidate->state.battle.events.count(113) &&
              step.candidate->state.growth.at(1).definition.profession_levels[0] == 10 &&
              step.candidate->state.battle.actors.at({1}).capacity == 109 &&
              step.candidate->state.battle.actors.at({3}).capacity == 109 &&
              step.candidate->state.battle.actors.at({3}).hp.target == 20,
          "mastery submits global job/event and shared capacity, not instance HP");
    check(
        step.candidate->state.contexts.at({3}).effects.display.back()[0] == 14 &&
            step.candidate->state.contexts.at({1}).effects.display.front()[0] == 24,
        "level display attaches only calling actor, other same-definition actor display unchanged");
    check(step.candidate->state.battle.actors.size() == s.battle.actors.size() &&
              step.candidate->state.contexts.size() == s.contexts.size() &&
              step.candidate->state.encounters.size() == s.encounters.size() &&
              !step.candidate->growth_requests.empty() &&
              s.growth.at(1).definition.profession_levels[0] == 9 && !s.professions[0].unlocked,
          "growth returns complete independent AI and full mastery request audit");
    auto missing_human = s;
    missing_human.battle.humans.erase(1);
    const auto refused = growth_commit(consuming, missing_human, {3});
    check(
        !refused.candidate && refused.error == AiRewardError::invalid_input &&
            missing_human.growth.at(1).definition.profession_levels[0] == 9 &&
            missing_human.contexts.at({3}).effects.display.empty() &&
            !missing_human.professions[0].unlocked,
        "late missing human battle definition refuses mastery without partial level/effect/unlock");
    if (consuming) {
        auto missing_growth = s;
        missing_growth.growth.erase(1);
        const auto rejected = prepare_actor_growth_commit_consuming(std::move(missing_growth), {3});
        check(!rejected.candidate && rejected.error == AiRewardError::invalid_input &&
                  missing_growth.battle.actors.size() == s.battle.actors.size(),
              "missing shared growth definition refuses before transferring the private AI");
    }
}
void timeline(bool consuming) {
    auto world = fixture();
    bool attacked{};
    int victory_round{-1};
    for (int round = 0; round < 80; ++round) {
        auto next = world;
        AiScheduleInput input;
        input.rosters[0] = {1};
        if (world.battle.actors.count({2}))
            input.rosters[1] = {2};
        input.rosters[4] = {0};
        const auto plan = prepare_ai_schedule(input, [&](const auto &visit, const auto &) {
            AiScheduleResponse response;
            if (visit.phase == AiSchedulePhase::human_execution) {
                const auto effects = advance_actor_effects(next.contexts.at({1}).effects);
                if (!effects.candidate) {
                    response.accepted = false;
                    return response;
                }
                next.contexts.at({1}).effects = effects.candidate->state;
                const auto growth = growth_commit(consuming, next, {1});
                if (!growth.candidate) {
                    response.accepted = false;
                    return response;
                }
                next = growth.candidate->state;
                if (!attacked) {
                    const auto hit =
                        prepare_battle_commit(next.battle, {{1}, {2}, 100, 0, false, 99, {}});
                    if (!hit.candidate) {
                        response.accepted = false;
                        return response;
                    }
                    next.battle = hit.candidate->state;
                    attacked = true;
                }
            } else if (visit.phase == AiSchedulePhase::monster_decision) {
                const auto death = prepare_monster_death_commit(next, {2});
                if (!death.candidate) {
                    response.accepted = false;
                    return response;
                }
                next = death.candidate->state;
                response.remove = death.candidate->removed;
            } else if (visit.phase == AiSchedulePhase::monster_execution) {
                ++next.battle.actors.at({2}).state_counter;
            } else if (visit.phase == AiSchedulePhase::encounter) {
                EncounterCommitInput event;
                event.tickets = {{1000, 999}, {100, 99}};
                const auto completion = prepare_encounter_reward_commit(next, event);
                if (!completion.candidate) {
                    response.accepted = false;
                    return response;
                }
                if (world.encounters.at(0).runtime.state == 0 &&
                    completion.candidate->state.encounters.at(0).runtime.state == 1)
                    victory_round = round;
                next = completion.candidate->state;
                response.remove = completion.candidate->removed;
            }
            return response;
        });
        check(plan.candidate.has_value(), "atomic admitted AI reward round succeeds");
        world = next;
        if (round < 12)
            check(world.battle.actors.count({2}) && world.accounting.funds() == 0 &&
                      world.growth.at(1).pending.amount == 0,
                  "corpse retained until next c sees12, no cash or XP on lethal hit");
        if (round == 12)
            check(victory_round == 12 && world.accounting.funds() == 120 &&
                      world.encounters.at(0).runtime.reward == 100 &&
                      world.growth.at(1).pending.amount == 100 &&
                      world.growth.at(1).pending.counter == -50 &&
                      world.growth.at(1).experience == 0 &&
                      world.battle.humans.at(1).recent_kills == 0,
                  "death->cash->same round encounter victory; growth starts next human d");
        if (round == 62)
            check(world.growth.at(1).pending.counter == 0 && world.growth.at(1).experience == 0,
                  "50 following d calls reach O0, first difference still0");
        if (round == 63)
            check(world.growth.at(1).experience == 11,
                  "O1 transfers floor100/9 after display phase");
    }
    check(world.growth.at(1).definition.profession_levels[0] == 2 &&
              world.growth.at(1).pending.amount == 0 && world.battle.events.count(109) &&
              world.battle.actors.at({1}).hp.target == 100 &&
              world.battle.actors.at({1}).capacity == 101 && world.accounting.entries().size() == 1,
          "delayed real level clears pending, increases capacity without healing, cash only once");
}
void groups() {
    auto s = fixture();
    s.battle.actors.at({2}).control.flags &= ~128U;
    auto join = prepare_battle_group_join(s, 0, {1}, {2});
    check(join.candidate && join.candidate->state.battle.actors.at({1}).group == 0 &&
              !join.candidate->state.battle.actors.at({2}).group &&
              !(join.candidate->state.battle.actors.at({2}).control.flags & 128U),
          "join only mutates caller128/an/dc, not opponent");
    join = prepare_battle_group_join(join.candidate->state, 0, {2}, {1});
    check(join.candidate && join.candidate->state.encounters.at(0).group.humans.size() == 2 &&
              join.candidate->state.encounters.at(0).group.monsters.size() == 2,
          "mutual joins append duplicates, no set deduplication");
    s = join.candidate->state;
    s.encounters.at(0).group.tick = 39;
    auto group = prepare_battle_group_commit(s, 0);
    check(group.candidate && group.candidate->state.battle.actors.at({1}).attack_slot == 13 &&
              group.candidate->state.battle.actors.at({2}).attack_slot == 0,
          "half-cycle repeated human assignments last wins13, monsters untouched");
    s = group.candidate->state;
    s.encounters.at(0).group.tick = 79;
    group = prepare_battle_group_commit(s, 0, {0, 99});
    check(group.candidate && group.candidate->state.battle.actors.at({2}).attack_slot == 53 &&
              group.candidate->state.battle.actors.at({2}).monster_posture == 2,
          "full-cycle repeated monster consumes two posture draws, last wins");
    s = group.candidate->state;
    s.battle.actors.at({2}).control.state = 3;
    s.battle.actors.at({2}).control.flags &= ~128U;
    s.battle.actors.at({2}).state_counter = 12;
    auto death = prepare_monster_death_commit(s, {2});
    check(death.candidate && !death.candidate->state.battle.actors.count({2}) &&
              death.candidate->state.retired_actors.count({2}) &&
              death.candidate->state.encounters.at(0).group.monsters.size() == 2,
          "roster removal retains referenced corpse until group pruning");
    s = death.candidate->state;
    s.encounters.at(0).group.tick = 79;
    group = prepare_battle_group_commit(s, 0);
    check(group.candidate && group.candidate->state.encounters.at(0).group.humans.empty() &&
              group.candidate->state.encounters.at(0).group.monsters.empty() &&
              group.candidate->state.encounters.at(0).group_exists &&
              group.candidate->state.battle.actors.at({1}).group == 0 &&
              !(group.candidate->state.battle.actors.at({1}).control.flags & 128U) &&
              group.candidate->state.retired_actors.empty(),
          "prune reads live/dead flags; disband clears128 but keeps dc/event.i, garbage collected");
    s.encounters.at(0).group.monsters.push_back({{99}, 128U});
    check(prepare_battle_group_commit(s, 0).error == AiRewardError::stale_actor &&
              s.encounters.at(0).group.tick == 79,
          "missing retained reference rejects without advancing group tick");
}
void spawning() {
    auto s = fixture();
    s.monster_growth.at(7).body = 2;
    s.monster_growth.at(7).sprite_variant = 3;
    s.battle.monsters.at(7).flags = 4U;
    s.monster_growth.at(7).growth = 1;
    for (int x = 0; x < 100; ++x)
        for (int z = 0; z < 100; ++z) {
            const auto c = prepare_encounter_monster_spawn(s, 0, {{5, 5}, 7, true}, {x, z});
            check(c.candidate && c.candidate->state.battle.actors.at({3}).control.state == 8 &&
                      c.candidate->state.battle.actors.at({3}).control.flags ==
                          (2U | 4096U | 16384U) &&
                      c.candidate->state.battle.actors.at({3}).hp.target == 200 &&
                      c.candidate->state.battle.actors.at({3}).body == 2 &&
                      c.candidate->state.battle.actors.at({3}).sprite == 63 &&
                      c.candidate->state.battle.actors.at({3}).legacy_id == 1,
                  "source monster body/sprite, boss growth/last-instance independent bits, "
                  "first-free UID");
            const auto &p = c.candidate->state.battle.actors.at({3}).position;
            check(std::abs(p.x - (510.0F + x * 80.0F / 99.0F)) < 0.0001F &&
                      std::abs(p.z - (510.0F + z * 80.0F / 99.0F)) < 0.0001F &&
                      c.candidate->state.encounters.at(0).runtime.spawned == 2 &&
                      c.candidate->state.monster_order == std::vector<CharacterId>{{2}, {3}} &&
                      s.monster_order.size() == 1,
                  "two independent float offset draws, atomic original-order append");
        }
    s.encounters.at(0).runtime.state = 3;
    s.encounters.at(0).runtime.spawned = 0;
    s.encounters.at(0).runtime.quota = 1;
    s.monster_order.clear();
    s.battle.actors.erase({2});
    s.contexts.erase({2});
    s.encounters.at(0).members.clear();
    s.task_active = true;
    EncounterCommitInput input;
    input.quest.flags = 2U;
    input.quest.boss_definition = 7;
    input.cells = {{{4, 5}, 4, false}};
    input.tickets = {{1, 0}};
    check(!prepare_encounter_reward_commit(s, input).candidate &&
              s.encounters.at(0).runtime.spawned == 0 && s.next_actor_id == 3,
          "late missing spawn offsets rejects event counter/group/ID allocation");
    input.spawn_offset_tickets = std::array<int, 2>{0, 99};
    const auto c = prepare_encounter_reward_commit(s, input);
    check(c.candidate && c.candidate->state.encounters.at(0).runtime.spawned == 1 &&
              c.candidate->state.encounters.at(0).runtime.state == 3 &&
              c.candidate->state.battle.actors.at({3}).legacy_id == 0 &&
              c.candidate->state.battle.actors.at({3}).state_counter == 0,
          "quest last spawn committed once; UID0 reused, no same-round c/d counter increment");
    input.cells[0].inside_town = true;
    input.spawn_offset_tickets.reset();
    const auto town = prepare_encounter_reward_commit(s, input);
    check(town.candidate && town.candidate->state.monster_order.empty() &&
              town.candidate->state.encounters.at(0).runtime.spawned == 0,
          "selected town cell consumes cell ticket only, no offsets/definition/spawn");
}
AiRewardState projectile_fixture() {
    auto s = fixture();
    s.battle.actors.at({1}).control.flags = 2U;
    s.growth.at(1).derived.combat[1] = 100;
    auto &m = s.battle.actors.at({2});
    m.position = {20, 0, 0};
    m.hp = {0, 500, 500, 500, false, 0};
    m.capacity = 500;
    auto original = m;
    original.id = {3};
    original.position = {1000, 0, 0};
    s.battle.actors.emplace(original.id, original);
    s.contexts.emplace(original.id, RewardActorContext{});
    s.monster_order.push_back(original.id);
    s.monster_growth.at(7).growth = 0;
    s.monster_growth.at(7).base_defense = 50;
    s.projectile_order = {10};
    s.next_projectile_id = 11;
    return s;
}
WorldProjectileInput projectile_input() {
    WorldProjectileInput i;
    i.projectile = 10;
    i.box = CollisionBox{-1, 0, 2, 2};
    i.monster_boxes[0] = CollisionBox{-10, 10, 20, 20};
    i.physical_jitter = 12;
    i.current_weapon_kind = 1;
    i.drop_ticket = 99;
    return i;
}
void projectile_world() {
    for (bool miss : {false, true}) {
        auto s = projectile_fixture();
        const auto p = prepare_projectile(ProjectileKind::arrow, {1}, {3}, {}, {1000, 0, 0}, 0);
        s.projectiles.emplace(10, *p.candidate);
        s.battle.actors.at({1}).miss = miss;
        const auto c = prepare_world_projectile(s, projectile_input());
        check(c.candidate && c.candidate->step.damage_target == CharacterId{2} &&
                  c.candidate->physical_damage->value == 120 && c.candidate->hit->landed == !miss &&
                  c.candidate->state.battle.actors.at({2}).hp.target == (miss ? 500 : 380) &&
                  c.candidate->state.battle.actors.at({3}).hp.target == 500 &&
                  c.candidate->step.contact_effect && c.candidate->state.projectiles.empty(),
              "arrow hits actual first collision not original, rederives damage/current miss; "
              "contact even miss");
        check(s.projectiles.at(10).position.x == 0 && s.battle.actors.at({2}).hp.target == 500,
              "private projectile transaction doesn't mutate input owners");
    }
    auto s = projectile_fixture();
    ProjectileState p;
    p.kind = ProjectileKind::spell;
    p.caster = {1};
    p.original_target = {3};
    p.position = {20, -1, 0};
    p.effect = 4;
    p.damage = 10;
    s.projectiles.emplace(10, p);
    auto next = s;
    AiScheduleInput schedule;
    schedule.rosters[2] = {10};
    const auto plan = prepare_ai_schedule(schedule, [&](const auto &visit, const auto &) {
        AiScheduleResponse r;
        if (visit.phase == AiSchedulePhase::projectile) {
            const auto step = prepare_world_projectile(next, projectile_input());
            if (!step.candidate) {
                r.accepted = false;
                return r;
            }
            next = step.candidate->state;
            r.remove = step.candidate->step.remove;
            if (step.candidate->spawned_projectile)
                r.append.push_back({AiRosterKind::projectile, *step.candidate->spawned_projectile});
        }
        return r;
    });
    check(plan.candidate && plan.candidate->rosters[2] == std::vector<std::uint64_t>{11} &&
              next.projectiles.at(11).counter == 0 && next.projectiles.at(11).delay == 6 &&
              next.battle.actors.at({2}).hp.target == 500,
          "spell collision appends delayed object, reverse current pass excludes new projectile");
    WorldProjectileInput delayed;
    delayed.projectile = 11;
    for (int tick = 0; tick < 6; ++tick) {
        const auto c = prepare_world_projectile(next, delayed);
        check(c.candidate && !c.candidate->hit &&
                  c.candidate->state.projectiles.at(11).counter == tick + 1,
              "delayed old-counter0..5 waits without damage");
        next = c.candidate->state;
    }
    next.battle.actors.at({1}).miss = true;
    auto c = prepare_world_projectile(next, delayed);
    check(c.candidate && c.candidate->hit && !c.candidate->hit->landed &&
              c.candidate->state.battle.actors.at({2}).hp.target == 500 &&
              c.candidate->state.projectiles.empty(),
          "seventh delayed check reads caster current miss, not launch snapshot");
    s = projectile_fixture();
    p = {};
    p.kind = ProjectileKind::delayed_damage;
    p.caster = {1};
    p.original_target = {2};
    p.damage = 10;
    p.delay = 0;
    s.projectiles.emplace(10, p);
    auto &dead = s.battle.actors.at({2});
    dead.control.state = 3;
    dead.control.action = 9;
    dead.control.flags &= ~128U;
    dead.state_parameter = 1;
    dead.state_counter = 12;
    dead.hp.target = 0;
    const auto death = prepare_monster_death_commit(s, {2});
    check(death.candidate && death.candidate->state.retired_actors.count({2}),
          "projectile reference keeps corpse alive after removal from bm");
    c = prepare_world_projectile(death.candidate->state, projectile_input());
    check(c.candidate && c.candidate->hit->lethal &&
              c.candidate->state.battle.humans.at(1).kills == 1 &&
              !c.candidate->state.battle.actors.count({2}) &&
              c.candidate->state.battle.actors.at({3}).hp.target == 500 &&
              c.candidate->state.retired_actors.empty(),
          "delayed corpse rehit commits stats without re-adding instance or hitting reused UID, "
          "then releases ref");
    s = projectile_fixture();
    s.projectiles.emplace(
        10, *prepare_projectile(ProjectileKind::arrow, {1}, {3}, {}, {1000, 0, 0}, 0).candidate);
    s.battle.actors.at({2}).hp.target = 1;
    auto invalid = projectile_input();
    invalid.drop_ticket.reset();
    check(!prepare_world_projectile(s, invalid).candidate && s.projectiles.at(10).position.x == 0 &&
              s.battle.actors.at({2}).hp.target == 1,
          "late lethal sub-consumer failure discards projectile motion/deletion and target stats");
    s.battle.actors.erase({3});
    c = prepare_world_projectile(s, {});
    check(!c.candidate, "unresolved projectile ID is invalid not fabricated");
    WorldProjectileInput missing;
    missing.projectile = 10;
    c = prepare_world_projectile(s, missing);
    check(c.candidate && c.candidate->step.remove && c.candidate->state.projectiles.empty(),
          "missing original reference deletes before collision metadata/physical draws");
}
void reference_graph() {
    auto s = fixture();
    auto corpse = s.battle.actors.at({2});
    corpse.id = {5};
    corpse.encounter.reset();
    s.retired_actors.emplace(corpse.id, corpse);
    s.battle.actors.at({1}).rescue = corpse.id;
    auto c = collect_ai_references(s);
    check(c.retired_actors.count({5}) == 1, "live R keeps removed object outside roster alive");
    s.battle.actors.at({1}).rescue.reset();
    s.battle.actors.at({2}).follow = corpse.id;
    c = collect_ai_references(s);
    check(c.retired_actors.count({5}) == 1, "monster S independently roots removed follow target");
    s.battle.actors.at({2}).follow.reset();
    s.contexts.emplace(CharacterId{5}, RewardActorContext{});
    s.battle.actors.at({1}).perceived_enemy = CharacterId{5};
    c = collect_ai_references(s);
    check(c.retired_actors.count({5}) && c.contexts.count({5}),
          "live az retains removed opponent and its cached context without roster reentry");
    s.battle.actors.at({1}).perceived_enemy.reset();
    c = collect_ai_references(s);
    check(!c.retired_actors.count({5}) && !c.contexts.count({5}),
          "last az released collects actor and context together");
    s.retired_actors.at({5}).rescue = CharacterId{6};
    corpse.id = {6};
    corpse.rescue = CharacterId{5};
    s.retired_actors.emplace(corpse.id, corpse);
    c = collect_ai_references(s);
    check(c.retired_actors.empty(), "unrooted mutual R cycle released like Java GC");
    s.battle.actors.at({1}).rescue = CharacterId{5};
    c = collect_ai_references(s);
    check(c.retired_actors.size() == 2, "reachable R chain retains entire cycle");
    s = fixture();
    s.encounters.at(0).runtime.state = 1;
    s.encounters.at(0).runtime.counter = 99;
    const auto end = prepare_encounter_reward_commit(s, {});
    check(end.candidate && end.candidate->removed && end.candidate->state.encounters.empty() &&
              end.candidate->state.retired_encounters.count(0) == 1,
          "bn deletion retains original event through current actor db");
    s = end.candidate->state;
    for (auto &[id, a] : s.battle.actors) {
        (void)id;
        a.encounter.reset();
        a.group = 0;
    }
    c = collect_ai_references(s);
    check(c.retired_encounters.count(0) == 1,
          "dc keeps removed event/group identity independently");
    for (auto &[id, a] : s.battle.actors) {
        (void)id;
        a.group.reset();
    }
    c = collect_ai_references(s);
    check(c.retired_encounters.empty(), "no db/dc roots finally release retired event");
}
void external_reference_graph() {
    auto s = fixture();
    auto corpse = s.battle.actors.at({2});
    corpse.id = {5};
    corpse.encounter = 9;
    s.retired_actors.emplace(corpse.id, corpse);
    s.contexts.emplace(corpse.id, RewardActorContext{});
    auto event = s.encounters.at(0);
    event.runtime.id = 9;
    event.members = {{5}, {5}};
    s.retired_encounters.emplace(9, event);
    s.external_actor_roots = {{5}};
    auto c = collect_ai_references(s);
    check(c.retired_actors.count({5}) && c.retired_encounters.count(9) && c.contexts.count({5}),
          "external facility root retains actor-event cycle and cached actor context");
    check(c.human_order == s.human_order && c.monster_order == s.monster_order &&
              c.encounter_order == s.encounter_order && !c.battle.actors.count({5}) &&
              !c.encounters.count(9),
          "external roots never reinsert retired references into running rosters");
    c.external_actor_roots.clear();
    c = collect_ai_references(c);
    check(!c.retired_actors.count({5}) && !c.retired_encounters.count(9) && !c.contexts.count({5}),
          "removing final external root collects even a duplicated actor-event cycle");
    s.external_actor_roots.clear();
    s.external_encounter_roots = {9};
    c = collect_ai_references(s);
    check(c.retired_encounters.count(9) && c.retired_actors.count({5}),
          "external task/page encounter root reaches retired members and their back reference");
    c.external_encounter_roots.clear();
    c = collect_ai_references(c);
    check(c.retired_encounters.empty() && c.retired_actors.empty(),
          "removing task/page root permits the same transitive cycle to be collected");
    s.external_encounter_roots = {999};
    s.external_actor_roots = {{999}};
    c = collect_ai_references(s);
    check(c.retired_encounters.empty() && c.retired_actors.empty() && !c.contexts.count({999}) &&
              !c.encounters.count(999) && !c.battle.actors.count({999}) &&
              c.external_actor_roots == s.external_actor_roots &&
              c.external_encounter_roots == s.external_encounter_roots,
          "unresolved external references neither manufacture records nor root unrelated cycles");
    s = fixture();
    s.external_actor_roots = {{2}};
    auto &m = s.battle.actors.at({2});
    m.control.state = 3;
    m.state_counter = 12;
    const auto death = prepare_monster_death_commit(s, {2});
    check(death.candidate && death.candidate->removed &&
              death.candidate->state.retired_actors.count({2}) &&
              death.candidate->state.contexts.count({2}) &&
              death.candidate->state.monster_order.empty(),
          "death consumer's internal collection respects the prepublished external actor root");
    c = death.candidate->state;
    c.external_actor_roots.clear();
    c = collect_ai_references(c);
    check(!c.retired_actors.count({2}) && !c.contexts.count({2}),
          "retired corpse released after external facility reference is cleared");
    s = fixture();
    for (auto &[id, a] : s.battle.actors) {
        (void)id;
        a.encounter.reset();
        a.group.reset();
    }
    s.encounters.at(0).runtime.state = 1;
    s.encounters.at(0).runtime.counter = 99;
    s.external_encounter_roots = {0};
    const auto end = prepare_encounter_reward_commit(s, {});
    check(end.candidate && end.candidate->removed &&
              end.candidate->state.retired_encounters.count(0) &&
              end.candidate->state.encounters.empty(),
          "encounter internal collection honors external ID0 root without an actor db/dc root");
    c = end.candidate->state;
    c.external_encounter_roots.clear();
    c = collect_ai_references(c);
    check(c.retired_encounters.empty(), "released external ID0 root no longer retains encounter");
    check(s.encounters.count(0) && s.retired_encounters.empty(),
          "collection and late retirement candidates never mutate the input owner");
}
void execution_prefix(bool consuming) {
    const auto execute = [consuming](const AiRewardState &source, CharacterId id) {
        if (!consuming)
            return prepare_world_execution_prefix(source, id);
        auto disposable = source;
        return prepare_world_execution_prefix_consuming(std::move(disposable), id);
    };
    auto s = fixture();
    auto &a = s.battle.actors.at({1});
    a.control.alternate_counter = 20;
    a.control.action_counter = 4;
    a.state_counter = 17;
    a.hit_flash = 3;
    a.label_timer = 1;
    a.miss_label = true;
    a.damage_total = 100;
    a.hit_count = 2;
    a.object_slot = -2;
    a.hp = {50, 20, 20, 70, true, 10};
    s.contexts.at({1}).effects.delayed = {{4, 0, 10, 20}};
    s.contexts.at({1}).effects.display = {{12, 0, 40, 1, 0}};
    auto r = execute(s, {1});
    check(r.candidate && r.candidate->state.battle.actors.at({1}).control.alternate_counter == 21 &&
              r.candidate->state.battle.actors.at({1}).control.action_counter == 5 &&
              r.candidate->state.battle.actors.at({1}).state_counter == 18 &&
              r.candidate->state.battle.actors.at({1}).hit_flash == 2,
          "actual d prefix advances shared i/l/B/aw exactly once");
    check(r.candidate->sounds.size() == 1 && r.candidate->sounds.front().sound == 17 &&
              r.candidate->state.contexts.at({1}).effects.display.front() ==
                  ActorEffectRecord{16, 1, 4, 10, 20},
          "ce fires/prepends before current cd pass, new spell display age1 before v");
    check(!r.candidate->state.battle.actors.at({1}).miss_label &&
              r.candidate->state.battle.actors.at({1}).damage_total == 0 &&
              r.candidate->state.battle.actors.at({1}).hit_count == 0 &&
              r.candidate->state.battle.actors.at({1}).hp.legacy_tick == 11 &&
              r.candidate->request_carry_expression,
          "label expiry then HP display then shared growth then carry expression17 request");
    check(a.state_counter == 17 && s.contexts.at({1}).effects.delayed.size() == 1,
          "all d prefix changes stay private");
    check(r.candidate->state.battle.actors.size() == 2 &&
              r.candidate->state.monster_growth.count(7) && r.candidate->state.growth.count(1) &&
              r.candidate->state.encounters.count(0),
          "execution prefix retains complete other actor, growth and encounter domains");
    r.candidate->state.encounters.at(0).runtime.counter = 99;
    check(s.encounters.at(0).runtime.counter == 0,
          "execution candidate encounter storage remains independent of its input");
    s.growth.erase(1);
    check(!execute(s, {1}).candidate && a.hp.legacy_tick == 10,
          "late missing human definition rolls back counters/display/HP together");
    s = fixture();
    s.growth.clear();
    s.contexts.at({2}).effects.delayed = {{6, 1, 0, 0}};
    r = execute(s, {2});
    check(r.candidate && r.candidate->state.contexts.at({2}).effects.delayed.front()[1] == 0 &&
              r.candidate->growth_requests.empty() && r.candidate->sounds.empty(),
          "monster d skips human growth; ce old1->0 doesn't fire until next execution");
    s.battle.actors.at({2}).control.alternate_counter = std::numeric_limits<int>::max() - 1;
    r = execute(s, {2});
    check(r.candidate && r.candidate->state.battle.actors.at({2}).control.alternate_counter == 0,
          "source positive counter modulo INTMAX boundary preserved");
    if (consuming) {
        const auto stale = prepare_world_execution_prefix_consuming(std::move(s), {999});
        check(!stale.candidate && stale.error == AiRewardError::stale_actor &&
                  s.battle.actors.size() == 2 && s.contexts.size() == 2,
              "stale execution identity refuses before transferring candidate storage");
    }
}
void retired_event_consumers() {
    auto s = fixture();
    auto e = s.encounters.at(0);
    s.encounters.erase(0);
    s.retired_encounters.emplace(0, e);
    auto joined = prepare_battle_group_join(s, 0, {1}, {2});
    check(joined.candidate && joined.candidate->state.encounters.empty() &&
              joined.candidate->state.retired_encounters.at(0).group.humans.size() == 1 &&
              joined.candidate->state.battle.actors.at({1}).group == 0,
          "referenced retired db still permits source f.a group append without reentering bn");
    s = joined.candidate->state;
    s.battle.actors.at({2}).control.state = 3;
    s.battle.actors.at({2}).state_counter = 12;
    const auto died = prepare_monster_death_commit(s, {2});
    check(died.candidate && died.candidate->removed && died.candidate->state.encounters.empty() &&
              died.candidate->state.retired_encounters.at(0).runtime.reward == 100 &&
              died.candidate->state.retired_encounters.at(0).members.empty() &&
              died.candidate->state.accounting.funds() == 120,
          "old db object remains reward/member owner even after bn removal");
    s = fixture();
    auto alias = s.encounters.at(0);
    alias.runtime.id = 7;
    s.retired_encounters.emplace(7, alias);
    s.battle.actors.at({2}).encounter = 7;
    EncounterCommitInput input;
    input.tickets = {{1000, 999}};
    const auto linked = prepare_encounter_reward_commit(s, input);
    check(linked.candidate && linked.candidate->state.encounters.at(0).runtime.state == 0,
          "current event monster count matches originalID even when instance db is retired alias");
    check(linked.candidate && linked.candidate->state.encounters.at(0).linked_monsters == 1,
          "normal event count publishes source f.n cache");
    s.encounters.at(0).linked_monsters = 9;
    input.town_overlap = true;
    const auto cancelled = prepare_encounter_reward_commit(s, input);
    check(cancelled.candidate &&
              cancelled.candidate->state.battle.actors.at({2}).control.state == 3 &&
              cancelled.candidate->state.battle.actors.at({2}).state_parameter == 1,
          "source cancel also matches originalID, not source object's stable db identity");
    check(cancelled.candidate && cancelled.candidate->state.encounters.at(0).linked_monsters == 9,
          "early town cancellation preserves prior f.n rather than publishing defaultzero");
    s.encounters.at(0).runtime.state = 1;
    input.town_overlap = false;
    const auto retiring = prepare_encounter_reward_commit(s, input);
    check(retiring.candidate && retiring.candidate->state.encounters.at(0).linked_monsters == 9,
          "retiring state1 preserves cached f.n without a new counting branch");
}
} // namespace
int main() {
    try {
        death_boundary();
        for (const bool consuming : {false, true}) {
            shared_and_quest(consuming);
            timeline(consuming);
        }
        groups();
        spawning();
        projectile_world();
        reference_graph();
        external_reference_graph();
        for (const bool consuming : {false, true})
            execution_prefix(consuming);
        retired_event_consumers();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
