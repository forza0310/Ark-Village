#include "dungeon_village_reference/ai_rewards.hpp"
#include "dungeon_village_reference/ai_schedule.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
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
void shared_and_quest() {
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
    auto step = prepare_actor_growth_commit(s, {3});
    check(step.candidate && step.candidate->state.growth.at(1).experience == 1 &&
              step.candidate->state.battle.actors.at({3}).hp.target == 20,
          "definition grows per calling instance, no heal");
    step = prepare_actor_growth_commit(step.candidate->state, {1});
    check(step.candidate && step.candidate->state.growth.at(1).experience == 2 &&
              step.candidate->state.growth.at(1).pending.counter == 2,
          "second instance same definition advances shared O again, not deduplicated");
    s.growth.at(1).definition.profession_levels[0] = 9;
    s.professions[0].unlocked = false;
    s.growth.at(1).pending = {9, 0};
    s.growth.at(1).experience = *human_growth_threshold(9, 5);
    step = prepare_actor_growth_commit(s, {3});
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
}
void timeline() {
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
                const auto growth = prepare_actor_growth_commit(next, {1});
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
} // namespace
int main() {
    try {
        death_boundary();
        shared_and_quest();
        timeline();
        groups();
        spawning();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
