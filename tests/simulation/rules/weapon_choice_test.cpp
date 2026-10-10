#include "ark/simulation/ai/rules/actor_control.hpp"
#include "ark/simulation/ai/rules/weapon_choice.hpp"

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
void selection() {
    // Deliberately unsorted IDs/ranks test source vector order; not the startup inventory.
    const std::vector<WeaponChoiceDefinition> table{{7, 3, true}, {0, 1, true}, {2, 2, false},
                                                    {1, 2, true}, {5, 4, true}, {6, 0, true}};
    for (int counter : {1, 6, std::numeric_limits<int>::max()}) {
        const auto result = prepare_weapon_choice(table, 0, counter);
        check(result.candidate && result.candidate->weapon_id == 0 &&
                  result.candidate->eligible_ids.empty() && !result.candidate->consumes_ticket,
              "positive A0 retains current without consuming randomness");
    }
    const std::vector<int> expected{7, 0, 1};
    for (int ticket = 0; ticket < 3; ++ticket) {
        const auto result = prepare_weapon_choice(table, 0, 0, ticket);
        check(result.candidate && result.candidate->eligible_ids == expected &&
                  result.candidate->weapon_id == expected[static_cast<std::size_t>(ticket)] &&
                  result.candidate->consumes_ticket,
              "inclusive rank through plus two, unlocked only, source order, one ticket");
    }
    check(prepare_weapon_choice(table, 0, 0).error == WeaponChoiceError::missing_ticket,
          "random branch cannot invent a ticket");
    for (int ticket : {-1, 3, std::numeric_limits<int>::max()}) {
        const auto result = prepare_weapon_choice(table, 0, 0, ticket);
        check(result.error == WeaponChoiceError::invalid_ticket && !result.candidate,
              "invalid random ticket has no partial selection");
    }
    const auto empty = prepare_weapon_choice({{8, 10, false}}, 8, 0);
    check(empty.candidate && empty.candidate->weapon_id == 8 && !empty.candidate->consumes_ticket &&
              empty.candidate->eligible_ids.empty(),
          "no unlocked candidates retains current without a draw");
    const auto maximum = std::numeric_limits<int>::max();
    const auto high = prepare_weapon_choice({{3, maximum, true}}, 3, 0, 0);
    check(high.candidate && high.candidate->weapon_id == 3,
          "rank upper endpoint uses wide arithmetic");
    for (const auto &bad : std::vector<std::vector<WeaponChoiceDefinition>>{
             {}, {{-1, 1, true}}, {{0, -1, true}}, {{0, 1, true}, {0, 2, true}}}) {
        const auto result = prepare_weapon_choice(bad, 0, 6);
        check(result.error == WeaponChoiceError::invalid_input && !result.candidate,
              "missing current, duplicate identity or invalid definition rejected");
    }
    for (int counter : {-1, std::numeric_limits<int>::min()}) {
        const auto result = prepare_weapon_choice(table, 0, counter);
        check(result.error == WeaponChoiceError::invalid_input && !result.candidate,
              "negative counter is not silently treated as zero");
    }
    check(prepare_weapon_choice(table, -1, 6).error == WeaponChoiceError::invalid_input,
          "missing equipped weapon does not fall back to ID zero");
}
void armor_and_accessory() {
    EquipmentChoiceInput i;
    i.catalogue = {{9, 4, 2, true},  {1, 1, 0, true}, {7, 2, 2, true},
                   {2, 3, 2, false}, {4, 0, 2, true}, {8, 3, 1, true}};
    i.current = 7;
    i.reselect_counter = 6;
    check(prepare_equipment_choice(i).candidate->equipment_id == 7 &&
              !prepare_equipment_choice(i).candidate->consumes_ticket,
          "armor existing+positive A slot returns current without candidate draw");
    i.reselect_counter = 0;
    for (int ticket = 0; ticket < 2; ++ticket) {
        i.ticket = ticket;
        const auto c = prepare_equipment_choice(i);
        check(c.candidate && c.candidate->eligible_ids == std::vector<int>{9, 7} &&
                  c.candidate->equipment_id == (ticket == 0 ? 9 : 7) &&
                  c.candidate->selected_slot == 1,
              "armor slot1 only type2 rank1..4, source order, current included");
    }
    i.slot = 2;
    i.current = 1;
    i.ticket = 1;
    auto c = prepare_equipment_choice(i);
    check(c.candidate && c.candidate->eligible_ids == std::vector<int>{1, 8} &&
              c.candidate->selected_slot == 2,
          "armor slot2 excludes type2, inclusive rank0..3");
    i.current.reset();
    i.reselect_counter = 6;
    i.ticket = 0;
    c = prepare_equipment_choice(i);
    check(c.candidate && c.candidate->consumes_ticket && c.candidate->equipment_id == 1,
          "missing armor rank0 still draws even with positive A, guard requires current");
    i.catalogue.clear();
    i.ticket.reset();
    c = prepare_equipment_choice(i);
    check(c.candidate && c.candidate->equipment_id == 0 && c.candidate->literal_zero_fallback &&
              !c.candidate->consumes_ticket && !c.candidate->selected_slot,
          "source null armor+empty returns literal0 without draw");
    i.slot = 1;
    i.catalogue = {{0, 1, 0, false}};
    c = prepare_equipment_choice(i);
    check(c.candidate && c.candidate->literal_zero_fallback && c.candidate->selected_slot == 2,
          "literal fallback armor0 may commit different slot than original arrival draw");
    i.kind = EquipmentChoiceKind::accessory;
    i.slot = 3;
    i.catalogue.clear();
    check(!prepare_equipment_choice(i).candidate,
          "source null accessory+empty would crash, explicit invalid_input not fabricated0");
    i.catalogue = {{6, 3, 0, true}, {9, 2, 2, true}, {1, 0, 0, true}, {2, 1, 0, false}};
    i.ticket = 1;
    c = prepare_equipment_choice(i);
    check(c.candidate && c.candidate->eligible_ids == std::vector<int>{9, 1} &&
              c.candidate->equipment_id == 1 && c.candidate->selected_slot == 3,
          "missing accessory rank0 keeps rank0..2, types ignored");
    i.current = 9;
    c = prepare_equipment_choice(i);
    check(c.candidate && c.candidate->equipment_id == 9 && !c.candidate->consumes_ticket,
          "existing accessory respects A3 positive guard");
    i.reselect_counter = 0;
    i.ticket = 0;
    c = prepare_equipment_choice(i);
    check(c.candidate && c.candidate->eligible_ids == std::vector<int>{6, 9},
          "accessory rank[current-1,current+2] includes lower item unlike weapon");
    EquipmentExitTailInput exit;
    exit.detail = 5;
    exit.accessory = c.candidate->equipment_id;
    const auto tail = prepare_equipment_exit_tail(exit);
    const auto commit = prepare_equipment_commit((*tail)[9]);
    check(commit && commit->slot == 3 && commit->equipment == 6 && commit->reselect_counter == 6,
          "accessory selection->delayed exit->actual slot3 commit composition");
    i.ticket.reset();
    check(prepare_equipment_choice(i).error == WeaponChoiceError::missing_ticket,
          "missing selection ticket no invented equipment");
    i.ticket = 2;
    check(prepare_equipment_choice(i).error == WeaponChoiceError::invalid_ticket,
          "candidate-bound ticket validation");
    for (int slot : {0, 1, 2, 4}) {
        i.slot = slot;
        check(!prepare_equipment_choice(i).candidate, "accessory only accepts slot3");
    }
    for (int rank = 0; rank <= 10; ++rank)
        for (int slot : {1, 2}) {
            i = {};
            i.slot = slot;
            i.current = 0;
            i.catalogue.push_back({0, rank, slot == 1 ? 2 : 0, false});
            for (int candidate_rank = 0; candidate_rank <= 13; ++candidate_rank)
                i.catalogue.push_back(
                    {candidate_rank + 1, candidate_rank, slot == 1 ? 2 : 0, true});
            int count{};
            for (int candidate_rank = 0; candidate_rank <= 13; ++candidate_rank)
                if (candidate_rank >= rank - 1 && candidate_rank <= rank + 2)
                    ++count;
            for (int ticket = 0; ticket < count; ++ticket) {
                i.ticket = ticket;
                c = prepare_equipment_choice(i);
                check(c.candidate &&
                          c.candidate->eligible_ids.size() == static_cast<std::size_t>(count) &&
                          c.candidate->equipment_id == (rank == 0 ? ticket + 1 : rank + ticket),
                      "all armor rank0..10 endpoints and exact catalogue ticket mapping");
            }
        }
}
} // namespace
int main() {
    selection();
    armor_and_accessory();
    std::cout << checks << " checks passed\n";
}
