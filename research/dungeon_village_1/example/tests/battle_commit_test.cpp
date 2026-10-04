#include "dungeon_village_reference/actor_housekeeping.hpp"
#include "dungeon_village_reference/ai_schedule.hpp"
#include "dungeon_village_reference/battle_commit.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
BattleCommitState fixture() {
    BattleCommitState s;
    BattleActorRecord human;
    human.id = {1};
    human.definition = 1;
    human.control.flags = 2U | 128U | 2048U;
    human.hp = {0, 100, 100, 100, false, 0};
    human.capacity = 100;
    human.baseline = 5;
    human.control.state = 1;
    human.position = {100, 0, 100};
    human.attack_position = {80, 0, 80};
    human.encounter = 0;
    auto monster = human;
    monster.id = {2};
    monster.definition = 7;
    monster.kind = ActorKind::monster;
    monster.baseline = 17;
    monster.position = {110, 0, 110};
    monster.attack_position = {90, 0, 90};
    s.actors.emplace(human.id, human);
    s.actors.emplace(monster.id, monster);
    s.humans.emplace(1, HumanBattleRecord{1, 2, 3, 4, 5, 6, 7, 80});
    s.monsters.emplace(7, MonsterBattleRecord{2, 11, 12, 35, 5, 4U});
    s.participants = {1, 5, 1, 1};
    s.quest_encounters.insert(0);
    s.global_downs = 4;
    return s;
}
void hits() {
    for (int damage = 0; damage <= 101; ++damage)
        for (bool miss : {false, true}) {
            auto s = fixture();
            s.actors.at({1}).miss = miss;
            const auto c = prepare_battle_commit(s, {{1}, {2}, damage, 0, true, 99, {}});
            check(c.candidate && c.candidate->hit.landed == !miss &&
                      c.candidate->state.actors.at({2}).hp.target == (miss ? 100 : 100 - damage) &&
                      c.candidate->state.actors.at({2}).hit_flash == 7,
                  "atomic hit applies targetHP/labels without changing display or attackerHP");
            const bool lethal = !miss && damage >= 100;
            check(c.candidate->state.humans.at(1).task_kills == (lethal ? 7 : 4) &&
                      c.candidate->state.humans.at(1).recent_reward == (lethal ? 23 : 6) &&
                      c.candidate->state.humans.at(1).recent_kills == (lethal ? 8 : 7) &&
                      c.candidate->state.defeated_definitions.size() == (lethal ? 1U : 0U),
                  "duplicate participant matches count3; recent reward floor35*50/100, globalN "
                  "record");
            if (lethal) {
                const auto &a = c.candidate->state.actors.at({2});
                check(a.control.state == 3 && a.control.action == 9 && a.state_counter == 0 &&
                          a.baseline == 17 && a.attack_position.x == 110 &&
                          c.candidate->state.humans.at(1).kills == 2 &&
                          c.candidate->state.humans.at(1).killed_stat1 == 13 &&
                          c.candidate->state.humans.at(1).battle_reward_stat == 15 &&
                          c.candidate->state.events.count(217) &&
                          c.candidate->state.objects.empty(),
                      "death setter and shared stats committed, drop99 absent, boss event217");
            }
            check(s.actors.at({2}).hp.target == 100 && s.humans.at(1).kills == 1,
                  "preparation never changes caller-owned state");
        }
    auto s = fixture();
    auto rescued = s.actors.at({1});
    rescued.id = {3};
    rescued.control.state = 16;
    rescued.rescue = CharacterId{1};
    s.actors.emplace(rescued.id, rescued);
    s.actors.at({1}).object_slot = -2;
    s.actors.at({1}).rescue = CharacterId{3};
    auto c = prepare_battle_commit(s, {{2}, {1}, 100, 0, true, {}, {}});
    check(c.candidate && c.candidate->state.actors.at({1}).control.state == 2 &&
              c.candidate->state.actors.at({3}).control.state == 2 &&
              !c.candidate->state.actors.at({1}).rescue &&
              !c.candidate->state.actors.at({3}).rescue &&
              c.candidate->state.actors.at({1}).object_slot == -1 &&
              c.candidate->state.actors.at({3}).hp.target == 100 &&
              c.candidate->state.global_downs == 5 &&
              c.candidate->state.humans.at(1).participant_downs == 8,
          "carrier down drops other into state2 without second HP damage/global down, clears both "
          "links");
    check(c.candidate->state.humans.at(1).recent_reward == 0 &&
              c.candidate->state.humans.at(1).recent_kills == 0 &&
              c.candidate->state.humans.at(1).kills == 1 && c.candidate->state.events.count(131) &&
              c.candidate->state.monsters.at(7).human_kills == 3,
          "e.p clears J/K only, not entire definition; event131 and killer monsterx");
    s.actors.at({1}).rescue.reset();
    c = prepare_battle_commit(s, {{2}, {1}, 100, 0, false, {}, {}});
    check(c.candidate && c.candidate->state.actors.at({1}).object_slot == -1,
          "null rescue clears local sentinel, no fabricated other actor");
    s.actors.at({1}).rescue = CharacterId{99};
    check(prepare_battle_commit(s, {{2}, {1}, 100, 0, false, {}, {}}).error ==
                  BattleCommitError::stale_actor &&
              s.actors.at({1}).hp.target == 100 && s.global_downs == 4,
          "stale stable rescue ID rejects whole world without partial victim damage");
}
void drops_and_rehits() {
    auto s = fixture();
    BattleCommitInput i{{1}, {2}, 100, 0, false, 0, {}};
    check(!prepare_battle_commit(s, i).candidate && s.defeated_definitions.empty(),
          "late drop selection required after consumed100 success, no partial stats");
    DropSelectionInput drop;
    drop.equipment_ticket = 99;
    drop.rank_ticket = 0;
    drop.definitions = {{4, 0, 0, 2U, 0}};
    drop.selection_ticket = 0;
    i.drop_selection = drop;
    auto c = prepare_battle_commit(s, i);
    check(c.candidate && c.candidate->drop && c.candidate->drop->selected &&
              c.candidate->state.objects.size() == 1 && c.candidate->state.next_object_id == 2 &&
              c.candidate->state.objects.at(1).kind == 0 &&
              c.candidate->state.objects.at(1).definition == 4,
          "drop chooses actual provided catalog then allocates unique object in same atomic state");
    s.actors.at({1}).control.flags |= 8192U;
    i.drop_selection.reset();
    c = prepare_battle_commit(s, i);
    check(c.candidate && c.candidate->hit.consumed_drop_ticket && !c.candidate->drop &&
              c.candidate->state.objects.empty(),
          "first visitor consumes100 but suppresses drop sub-consumer draws");
    i.drop_ticket = 99;
    s = c.candidate->state;
    c = prepare_battle_commit(s, i);
    check(c.candidate && c.candidate->state.humans.at(1).kills == 3 &&
              c.candidate->state.defeated_definitions == std::vector<int>{7, 7},
          "corpse rehit source has no once-only barrier, repeated record/stat accepted before "
          "deletion");
    s = fixture();
    s.humans.at(1).killed_stat1 = std::numeric_limits<int>::max();
    check(prepare_battle_commit(s, i).error == BattleCommitError::numeric_overflow &&
              s.actors.at({2}).hp.target == 100 && s.humans.at(1).kills == 1,
          "mid-stat overflow rolls back prior hit/setter/kill updates");
}
void same_round() {
    auto world = fixture();
    AiScheduleInput schedule;
    schedule.rosters[0] = {1};
    schedule.rosters[1] = {2};
    const auto r = prepare_ai_schedule(schedule, [&](const auto &v, const auto &) {
        AiScheduleResponse response;
        if (v.phase == AiSchedulePhase::human_execution) {
            const auto hit = prepare_battle_commit(world, {{1}, {2}, 100, 0, false, 99, {}});
            if (!hit.candidate) {
                response.accepted = false;
                return response;
            }
            world = hit.candidate->state;
        } else if (v.phase == AiSchedulePhase::monster_decision) {
            auto &a = world.actors.at({2});
            TimedLifecycleInput i;
            i.kind = ActorKind::monster;
            i.state = a.control.state;
            i.old_counter = a.state_counter;
            const auto death = prepare_timed_lifecycle(i);
            check(death.candidate && !death.candidate->delete_instance && a.control.state == 3,
                  "human d lethal hit seen by later monster c as oldB0 corpse, not immediate "
                  "removal");
        } else if (v.phase == AiSchedulePhase::monster_execution) {
            ++world.actors.at({2}).state_counter;
        }
        return response;
    });
    check(r.candidate && r.candidate->rosters[1] == std::vector<std::uint64_t>{2} &&
              world.actors.at({2}).state_counter == 1 && world.humans.at(1).recent_kills == 8,
          "all source owners and live two-pass schedule share one same-round damage result");
}
} // namespace
int main() {
    try {
        hits();
        drops_and_rehits();
        same_round();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
