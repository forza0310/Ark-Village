#include "ark/simulation/world/rules/domain.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace ark::simulation::rules;

namespace {

int checks = 0;

void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

const std::vector<BuildingDefinition> catalog = {
    {"inn", 100, 12, 5},
    {"shop", 150, 20, 3},
};

GlobalState state_with_character(std::int64_t funds = 1000) {
    GlobalState state;
    state.funds = funds;
    state.characters.emplace(CharacterId{1}, CharacterState{CharacterId{1}, ActivityState::idle,
                                                            std::nullopt, std::nullopt, 0});
    state.characters.emplace(CharacterId{2}, CharacterState{CharacterId{2}, ActivityState::idle,
                                                            std::nullopt, std::nullopt, 0});
    return state;
}

BuildingId add_inn(GlobalState &state, Position position = {1, 1}) {
    const auto result = construct(state, catalog, "inn", position);
    check(result.error == Error::none && result.building_id.has_value(), "construct inn");
    return *result.building_id;
}

void failed_construction_is_atomic() {
    auto state = state_with_character(50);
    const auto before = state;
    const auto result = construct(state, catalog, "inn", {1, 1});
    check(result.error == Error::insufficient_funds, "insufficient funds is rejected");
    check(state.funds == before.funds, "failed construction preserves funds");
    check(state.next_building_id == before.next_building_id,
          "failed construction preserves ID sequence");
    check(state.buildings.empty(), "failed construction adds no instance");
}

void movement_preserves_identity() {
    auto state = state_with_character();
    const BuildingId id = add_inn(state);
    check(move(state, id, {4, 3}, 2) == Error::none, "move succeeds");
    check(state.buildings.size() == 1, "move keeps one instance");
    check(state.buildings.at(id).id == id, "move preserves stable ID");
    check(state.buildings.at(id).position == Position{4, 3}, "move updates position");
    check(state.buildings.at(id).rotation == 2, "move updates rotation");
}

void removal_cleans_references() {
    auto state = state_with_character();
    const BuildingId id = add_inn(state);
    const auto visit = start_visit(state, CharacterId{1}, id, true);
    check(visit.error == Error::none, "visit starts before removal");
    check(remove(state, id) == Error::none, "removal succeeds");
    check(state.reservations.empty(), "removal clears reservation");
    const auto &character = state.characters.at(CharacterId{1});
    check(character.activity == ActivityState::idle, "removal resets activity");
    check(!character.target.has_value(), "removal clears target");
}

void reservation_is_unique() {
    auto state = state_with_character();
    const BuildingId id = add_inn(state);
    check(start_visit(state, CharacterId{1}, id, true).error == Error::none,
          "first character reserves facility");
    check(start_visit(state, CharacterId{2}, id, true).error == Error::reserved,
          "second character cannot reserve facility");
    check(state.reservations.at(id) == CharacterId{1}, "reservation owner stays unchanged");
}

void failure_and_cancellation_release_reservation() {
    auto state = state_with_character();
    const BuildingId id = add_inn(state);
    check(start_visit(state, CharacterId{1}, id, false).error == Error::path_unreachable,
          "unreachable path is explicit");
    check(state.reservations.empty(), "path failure leaves no reservation");
    check(state.characters.at(CharacterId{1}).activity == ActivityState::idle,
          "path failure leaves character idle");

    check(start_visit(state, CharacterId{1}, id, true).error == Error::none,
          "reachable visit starts");
    check(cancel_visit(state, CharacterId{1}) == Error::none, "visit cancellation succeeds");
    check(state.reservations.empty(), "cancellation releases reservation");
}

void effects_wait_for_completion() {
    auto state = state_with_character();
    const BuildingId id = add_inn(state);
    const auto visit = start_visit(state, CharacterId{1}, id, true);
    check(visit.activity_id.has_value(), "visit has stable activity ID");
    check(state.characters.at(CharacterId{1}).accumulated_effect == 0,
          "travel does not apply effect");
    check(arrive(state, CharacterId{1}) == Error::none, "arrival enters use state");
    check(state.characters.at(CharacterId{1}).accumulated_effect == 0,
          "arrival does not apply effect");
    check(complete_use(state, catalog, CharacterId{1}, *visit.activity_id) == Error::none,
          "use completion succeeds");
    check(state.characters.at(CharacterId{1}).accumulated_effect == 5,
          "completion applies effect once");
    check(state.buildings.at(id).completed_uses == 1, "completion records facility use");
    check(complete_use(state, catalog, CharacterId{1}, *visit.activity_id) == Error::none,
          "duplicate completion is idempotent");
    check(state.characters.at(CharacterId{1}).accumulated_effect == 5,
          "duplicate completion does not repeat effect");
}

void task_completion_is_idempotent() {
    auto state = state_with_character();
    state.tasks.emplace(7, TaskState{7, 40, false});
    const auto opening_funds = state.funds;
    check(complete_task(state, 7) == Error::none, "task completion succeeds");
    check(complete_task(state, 7) == Error::none, "duplicate task completion succeeds");
    check(state.funds == opening_funds + 40, "task reward posts once");
}

void monthly_settlement_is_ordered_and_idempotent() {
    auto state = state_with_character();
    add_inn(state);
    const auto opening_funds = state.funds;
    const std::map<int, MonthlyInput> inputs = {
        {1, {50}},
        {2, {60}},
        {3, {70}},
    };
    check(settle_through(state, catalog, 3, inputs) == Error::none,
          "crossed months settle successfully");
    check(state.monthly_records.size() == 3, "no crossed month is skipped");
    check(state.monthly_records[0].month == 1 && state.monthly_records[2].month == 3,
          "months settle in ascending order");
    check(state.funds == opening_funds + (50 - 12) + (60 - 12) + (70 - 12),
          "each month applies income and maintenance");
    const auto settled_funds = state.funds;
    check(settle_through(state, catalog, 3, inputs) == Error::none,
          "duplicate settlement request succeeds");
    check(state.funds == settled_funds && state.monthly_records.size() == 3,
          "settled months are not replayed");
}

void missing_month_input_is_atomic() {
    auto state = state_with_character();
    add_inn(state);
    const auto funds = state.funds;
    const std::map<int, MonthlyInput> inputs = {{1, {50}}, {3, {70}}};
    check(settle_through(state, catalog, 3, inputs) == Error::missing_month_input,
          "missing intermediate month is rejected");
    check(state.funds == funds && state.last_settled_month == 0 && state.monthly_records.empty(),
          "failed settlement leaves state unchanged");
}

} // namespace

int main() {
    failed_construction_is_atomic();
    movement_preserves_identity();
    removal_cleans_references();
    reservation_is_unique();
    failure_and_cancellation_release_reservation();
    effects_wait_for_completion();
    task_completion_is_idempotent();
    monthly_settlement_is_ordered_and_idempotent();
    missing_month_input_is_atomic();
    std::cout << checks << " checks passed\n";
    return 0;
}
