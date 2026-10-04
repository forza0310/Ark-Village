#include "ark/simulation/rules/world_encounters.hpp"

#include <algorithm>
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
AiRewardState fixture() {
    AiRewardState s;
    BattleActorRecord a;
    a.id = {1};
    a.definition = 1;
    a.control.state = 5;
    a.capacity = 100;
    a.hp = {0, 100, 100, 100, false, 0};
    a.position = {550, 0, 550};
    a.state_counter = 5;
    s.battle.actors.emplace(a.id, a);
    s.battle.humans.emplace(1, HumanBattleRecord{});
    s.growth.emplace(1, RewardHumanDefinition{});
    RewardActorContext ctx;
    ctx.cell = {5, 5};
    ctx.half_cell = {11, 11};
    ctx.move_area = true;
    s.contexts.emplace(a.id, ctx);
    s.human_order = {a.id};
    s.task_active = true;
    s.monster_growth.emplace(7, RewardMonsterDefinition{0, 0, 100, 100, 100});
    s.battle.monsters.emplace(7, MonsterBattleRecord{});
    return s;
}
WorldMapFacts facts() {
    return {{8, 8, std::vector<LegacyMapCell>(64)},
            std::vector<int>(64, 1),
            std::vector<std::uint32_t>(64, 4U | 2U | 1U),
            {0, 2, 0, 2}};
}
void map_bits() {
    for (int mode = 0; mode <= 3; ++mode)
        for (int x = 0; x < 8; ++x)
            for (int y = 0; y < 8; ++y) {
                auto s = fixture();
                RewardEncounter e;
                e.runtime.id = 0;
                e.runtime.center = {x, y};
                e.runtime.state = mode;
                s.encounters.emplace(0, e);
                s.encounter_order = {0};
                const auto f = facts();
                const auto r = prepare_world_event_map(s, f);
                check(r.facts.has_value(), "valid current bn map refresh");
                for (int px = 0; px < 8; ++px)
                    for (int py = 0; py < 8; ++py) {
                        const bool nearby =
                            px >= x - 1 && px <= x + 1 && py >= y - 1 && py <= y + 1;
                        const bool inclusive_town = px <= 2 && py <= 2;
                        const bool expected = (mode == 0 || mode == 3) && nearby && !inclusive_town;
                        check(r.facts->flags[static_cast<std::size_t>(py * 8 + px)] ==
                                  (5U | (expected ? 2U : 0U)),
                              "clear only2 then outside-inclusive-town clipped nine-square k0/k3");
                    }
                check(f.flags.front() == 7U, "original map untouched");
            }
    auto s = fixture();
    RewardEncounter retired;
    retired.runtime.center = {5, 5};
    s.retired_encounters.emplace(0, retired);
    const auto r = prepare_world_event_map(s, facts());
    check(r.facts && r.facts->flags[45] == 5U,
          "retired event object held by db is not a running bn map influence");
    s.encounter_order = {0};
    check(!prepare_world_event_map(s, facts()).facts, "current bn must match live map storage");
}
void quota_and_entry() {
    for (int completed = 0; completed <= 20; ++completed)
        for (unsigned flags : {0U, 2U, 4U, 6U}) {
            const auto r = prepare_task_encounter_quota(3, completed, flags);
            check(r == (3 + ((flags & 4U) ? std::min(completed * 2, 10) : 0)),
                  "task quota onlyflags4 scales t with upper10, not bossflags2");
        }
    auto s = fixture();
    auto f = facts();
    f.flags.assign(64, 5U);
    WorldEventEntryInput i;
    i.actor = {1};
    i.task = {true, 1, {5, 5}, {}};
    i.base_quota = 3;
    i.task_completions = 2;
    i.task_flags = 4U;
    auto r = prepare_world_event_entry(s, f, i);
    check(r.candidate && r.candidate->gate.ready && r.candidate->created == 1 &&
              r.candidate->task.encounter == 1 && r.candidate->music2 && r.candidate->notice24 &&
              r.candidate->state.encounters.at(1).runtime.quota == 7 &&
              !r.candidate->state.battle.actors.at({1}).encounter &&
              (r.candidate->facts.flags[45] & 2U),
          "F creation commits k.g/quota7/map/music/notice but does not same-call bind actor db");
    check(s.encounters.empty() && s.next_encounter_id == 1 && !i.task.encounter,
          "successful creation still leaves input owner/task/map unchanged");
    i.task = r.candidate->task;
    const auto second = prepare_world_event_entry(r.candidate->state, r.candidate->facts, i);
    check(second.candidate && !second.candidate->created &&
              second.candidate->state.battle.actors.at({1}).encounter == 1 &&
              !second.candidate->music2 && second.candidate->state.encounters.size() == 1,
          "next F binds existing task and cannot recreate or replay creation music/notice");
    i.task.encounter.reset();
    i.base_quota = std::numeric_limits<int>::max();
    check(!prepare_world_event_entry(s, f, i).candidate && s.next_encounter_id == 1,
          "late quota overflow returns no partial allocation/map/task installation");
    i.base_quota = 3;
    i.task.center = {3, 3};
    s.contexts.at({1}).cell = {3, 3};
    r = prepare_world_event_entry(s, f, i);
    check(r.candidate && r.candidate->gate.ready && !r.candidate->created &&
              r.candidate->denial == EncounterCreationDenial::town &&
              !r.candidate->task.encounter && !r.candidate->notice24,
          "inclusive upper-band-town denial preserves Ftrue without quota/sound/notice");
}
void snapshot_and_retirement() {
    auto s = fixture();
    RewardEncounter e;
    e.runtime.id = 0;
    e.runtime.center = {5, 5};
    e.runtime.state = 0;
    e.runtime.spawned = 1;
    s.encounters.emplace(0, e);
    s.encounter_order = {0};
    const auto f = facts();
    CombatInfluenceCandidate field;
    field.width = field.height = 16;
    field.human_field.assign(256, 17);
    field.monster_field.assign(256, 23);
    EncounterCommitInput i;
    i.encounter = 0;
    i.tickets = {{1000, 999}, {100, 99}};
    const auto victory = prepare_world_encounter_update(s, f, i, field);
    check(victory.candidate && victory.candidate->state.encounters.at(0).runtime.state == 1 &&
              victory.candidate->state.encounters.at(0).influence->human_field ==
                  field.human_field &&
              victory.candidate->state.encounters.at(0).monster_scratch == field.monster_field &&
              victory.candidate->facts.flags[45] == 5U,
          "event snapshot consumes captured global field before victory; k1 map removes bit2");
    s = fixture();
    e.runtime.state = 3;
    e.runtime.spawned = 0;
    e.runtime.quota = 2;
    s.encounters.emplace(0, e);
    s.encounter_order = {0};
    i.cells = {{{4, 4}, 4, false}};
    i.quest.normal_definitions = {7};
    i.quest.boss_definition = 7;
    i.tickets = {{8, 0}, {1, 0}};
    i.spawn_offset_tickets = std::array<int, 2>{0, 0};
    const auto spawn = prepare_world_encounter_update(s, f, i, field);
    check(
        spawn.candidate && spawn.candidate->state.monster_order.size() == 1 &&
            spawn.candidate->state.encounters.at(0).runtime.spawned == 1 &&
            spawn.candidate->state.contexts.at(spawn.candidate->state.monster_order.front()).cell ==
                Position{4, 6} &&
            spawn.candidate->state.encounters.at(0).influence->human_field == field.human_field,
        "actual task spawn precedes snapshot; captured start-field isn't recomputed for new "
        "monster");
    field.width = 15;
    check(!prepare_world_encounter_update(s, f, i, field).candidate && s.monster_order.empty(),
          "invalid field dimensions refuse entire world update, no partial monster/control/group");
    auto alias = e;
    alias.runtime.id = 7;
    alias.legacy_id = 9;
    s.encounters.at(0).legacy_id = 9;
    s.retired_encounters.emplace(7, alias);
    const auto gate = prepare_world_event_gate(s, {1}, f, {true, 1, {5, 5}, 7});
    check(gate.candidate && gate.candidate->gate.bind_encounter == 0,
          "task F scans by originalID9 while binding actual current bn object0");
}
} // namespace
int main() {
    map_bits();
    quota_and_entry();
    snapshot_and_retirement();
    std::cout << "world encounter checks: " << checks << '\n';
}
