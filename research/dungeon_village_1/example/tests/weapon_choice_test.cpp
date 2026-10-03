#include "dungeon_village_reference/weapon_choice.hpp"

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
} // namespace
int main() {
    selection();
    std::cout << checks << " checks passed\n";
}
