#include "ark/simulation/rules/encounter_ai.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
void group() {
    for (std::size_t count = 0; count < 40; ++count) {
        BattleGroupState s;
        s.tick = 39;
        for (std::size_t n = 0; n < count; ++n)
            s.humans.push_back({{n + 1}, 128});
        auto r = prepare_battle_group_step(s);
        check(r.candidate && r.candidate->state.tick == 40 &&
                  r.candidate->state.alternating_side == 1 &&
                  r.candidate->assignments.size() == count && !r.candidate->disband,
              "half cycle only assigns human slots, no empty-side disband yet");
        for (std::size_t n = 0; n < count; ++n) {
            const int expected =
                count <= 7 ? static_cast<int>(n * 3 + 10) : static_cast<int>(10 + n * 22 / count);
            check(r.candidate->assignments[n].slot == expected &&
                      !r.candidate->assignments[n].monster_posture,
                  "small and dense roster scheduling at floor integer boundary");
        }
    }
    BattleGroupState s{79, 80, 2, 1, {{{1}, 128}}, {{{2}, 128}, {{3}, 128 | 16384}}};
    for (int ticket = 0; ticket < 100; ++ticket) {
        auto r = prepare_battle_group_step(s, {ticket, ticket});
        check(r.candidate && r.candidate->state.tick == 0 && r.candidate->state.cycle == 0 &&
                  r.candidate->consumed_tickets == 2,
              "whole cycle advances modulo3 and consumes each surviving monster draw");
        check(r.candidate->assignments[0].monster_posture == (ticket < 60   ? 0
                                                              : ticket < 80 ? 1
                                                                            : 2) &&
                  r.candidate->assignments[1].monster_posture == (ticket < 50   ? 0
                                                                  : ticket < 67 ? 1
                                                                                : 2),
              "normal60/80 boss50/67 strict thresholds");
        check(r.candidate->assignments[0].slot == 50 && r.candidate->assignments[1].slot == 53,
              "monster slots offset40");
    }
    check(prepare_battle_group_step(s).error == EncounterAiError::missing_ticket,
          "missing draw is transactional error");
    check(prepare_battle_group_step(s, {0, 100}).error == EncounterAiError::invalid_ticket,
          "invalid second draw cannot partially assign first monster");
    s.monsters[1].flags = 0;
    auto r = prepare_battle_group_step(s);
    check(r.candidate && r.candidate->state.monsters.size() == 1 &&
              r.candidate->assignments[0].monster_posture == 0 && !r.candidate->consumed_tickets,
          "prune first, single monster posture0 no random draw");
    s.humans[0].flags = 0;
    r = prepare_battle_group_step(s);
    check(r.candidate && r.candidate->disband && r.candidate->state.monsters.empty() &&
              r.candidate->release_group_flag.size() == 1 &&
              r.candidate->release_group_flag[0].value == 2,
          "empty side releases opposite flags and disbands only at cycle boundary");
    s = {39, 80, 0, 0, {{{1}, 128}, {{1}, 128}}, {}};
    r = prepare_battle_group_step(s);
    check(r.candidate && r.candidate->assignments.size() == 2 &&
              r.candidate->assignments[0].slot == 10 && r.candidate->assignments[1].slot == 13,
          "source duplicate group references retained, last owner assignment wins");
}
void definitions() {
    std::vector<MonsterChoiceDefinition> table;
    for (int n = 0; n < 8; ++n)
        table.push_back({n, 0, 0, true, 0, true});
    for (int ticket = 0; ticket < 5; ++ticket) {
        auto r = prepare_monster_choice(table, 0, false, ticket);
        check(r.candidate && r.candidate->eligible == std::vector<int>({7, 6, 5, 4, 3}) &&
                  r.candidate->selected == 7 - ticket,
              "latest five unlocked reverse definition order");
    }
    table[7].flags = 4;
    check(prepare_monster_choice(table, 0, false, 0).candidate->selected == 6,
          "boss definitions excluded");
    table = {{9, 0, 0, true, 7, true},
             {3, 0, 0, false, 9, false},
             {2, 0, 0, true, 8, true},
             {7, 0, 0, false, 0, false}};
    auto r = prepare_monster_choice(table, 0, false, 0);
    check(r.candidate && !r.candidate->unlock_definition &&
              r.candidate->eligible == std::vector<int>({2, 9}),
          "first qualified pair with v7 stops instead of checking later v8 pair");
    table[0].growth_counter = 8;
    r = prepare_monster_choice(table, 0, false, 1);
    check(r.candidate && r.candidate->unlock_definition == 3 && r.candidate->selected == 3 &&
              r.candidate->request_introduction,
          "v8 unlock contributes immediately to candidates and new introduction request");
    check(!prepare_monster_choice(table, 0, true, 1).candidate->request_introduction,
          "active task suppresses monster introduction");
    check(prepare_monster_choice({}, 0, false, 0).error == EncounterAiError::no_candidates,
          "empty source candidates error not invented monster");
}
void counts() {
    for (int year : {0, 1, 3, 4, 99})
        for (int month : {0, 6, 7, 11})
            for (int people = 0; people <= 8; ++people)
                for (int ticket = 0; ticket < 100; ++ticket)
                    for (int near = 0; near < (people > 0 ? people : 1); ++near) {
                        auto r = prepare_normal_monster_count({year, month, people, ticket, near});
                        const int base = year >= 4
                                             ? 1 + (ticket >= 50) + (ticket >= 80) + (ticket >= 95)
                                             : 1 + (ticket >= 80) + (ticket >= 95);
                        const int expected = year == 0 && month < 7
                                                 ? 1
                                                 : base + (people ? (near > 3 ? 3 : near) : 0);
                        check(r.candidate && r.candidate->count == expected &&
                                  r.candidate->consumes_nearby_ticket == (people > 0),
                              "year buckets, nearby cap and post-draw first-year override");
                    }
    check(prepare_normal_monster_count({0, 0, 1, 0, std::nullopt}).error ==
              EncounterAiError::missing_ticket,
          "early override still requires nearby draw");
    check(prepare_normal_monster_count({0, 0, 1, 0, 1}).error == EncounterAiError::invalid_ticket,
          "nearby draw must fit original bound");
}
} // namespace
int main() {
    group();
    definitions();
    counts();
    std::cout << checks << " checks passed\n";
}
