#include "dungeon_village_reference/simulation.hpp"

#include <climits>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

using namespace dungeon_village_reference;

namespace {

int checks = 0;
const std::vector<BuildingDefinition> catalog = {{"inn", 100, 12, 5}, {"cafe", 150, 20, 3}};

void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

struct Fixture {
    GlobalState state;
    SimulationState simulation;
};

Fixture fixture(int characters = 1) {
    Fixture result;
    result.state.funds = 1000;
    result.simulation.terrain = {7, 7, std::vector<RouteCategory>(49, RouteCategory::ground)};
    result.simulation.config = {100, 2, 3, 2};
    for (int index = 0; index < characters; ++index) {
        const CharacterId id{static_cast<std::uint64_t>(index + 1)};
        result.state.characters.emplace(
            id, CharacterState{id, ActivityState::idle, std::nullopt, std::nullopt, 0});
        CharacterMotion motion;
        motion.cell = {0, 3 + index};
        result.simulation.motions.emplace(id, motion);
    }
    return result;
}

BuildingId build(Fixture &fixture, Position position = {3, 3}, const std::string &key = "inn") {
    const auto result = construct(fixture.state, catalog, key, position);
    check(result.error == Error::none && result.building_id.has_value(), "construct fixture");
    return *result.building_id;
}

SimulationResult advance(Fixture &fixture, int elapsed_ms = 100) {
    const auto result = advance_simulation(fixture.state, catalog, fixture.simulation, elapsed_ms);
    check(result.error == SimulationError::none, "simulation advances");
    return result;
}

void autonomous_lifecycle() {
    auto world = fixture();
    const auto inn = build(world);
    const auto funds = world.state.funds;
    const auto started = advance(world);
    check(started.started == 1 && started.decisions == 1, "idle chooses without player command");
    check(world.state.characters.at(CharacterId{1}).activity == ActivityState::travelling,
          "automatic decision starts travelling");
    check(world.state.reservations.at(inn) == CharacterId{1}, "automatic reservation owns ID");
    const auto activity = *world.state.characters.at(CharacterId{1}).active_activity;
    advance(world, 400);
    check(world.state.characters.at(CharacterId{1}).activity == ActivityState::in_use,
          "route reaches approach and enters use");
    check(world.state.characters.at(CharacterId{1}).accumulated_effect == 0,
          "arrival has no completion effect");
    const auto completed = advance(world, 300);
    check(completed.completed == 1, "use completes automatically");
    check(world.state.characters.at(CharacterId{1}).activity == ActivityState::idle &&
              world.state.reservations.empty(),
          "completion releases and returns idle");
    check(world.state.buildings.at(inn).completed_uses == 1 &&
              world.state.characters.at(CharacterId{1}).accumulated_effect == 5,
          "effect and use count exactly once");
    check(world.state.funds == funds, "simulation does not invent original arrival income");
    check(complete_use(world.state, catalog, CharacterId{1}, activity) == Error::none &&
              world.state.characters.at(CharacterId{1}).accumulated_effect == 5,
          "late duplicate completion cannot repeat reward");
    advance(world, 300);
    check(world.state.characters.at(CharacterId{1}).activity == ActivityState::travelling,
          "retry timer reselects without input");
}

void multiple_characters_and_instances() {
    auto world = fixture(2);
    const auto inn = build(world);
    advance(world);
    check(world.state.reservations.size() == 1, "one facility has one owner");
    check(world.state.characters.at(CharacterId{2}).activity == ActivityState::idle,
          "second character waits for available facility");
    const auto cafe = build(world, {4, 4}, "cafe");
    advance(world, 300);
    check(world.state.reservations.size() == 2 &&
              world.state.reservations.at(inn) == CharacterId{1} &&
              world.state.reservations.at(cafe) == CharacterId{2},
          "independent characters use independent instances");
    auto single = fixture();
    const auto first = build(single, {1, 3});
    const auto second = build(single, {3, 3});
    advance(single, 500);
    check(single.simulation.motions.at(CharacterId{1}).last_visited == first,
          "first fixture choice follows stable ID");
    advance(single, 300);
    check(single.state.characters.at(CharacterId{1}).target == second,
          "fixture policy rotates across same-definition instances");
}

void no_candidates_and_unreachable() {
    auto world = fixture();
    const auto empty = advance(world, 1000);
    check(empty.decisions == 4 && empty.started == 0 && world.state.next_activity_id == 1,
          "empty world retries on bounded timer, not every tick");
    const auto inn = build(world);
    world.simulation.terrain.cells.assign(49, RouteCategory::blocked);
    world.simulation.terrain.cells[21] = RouteCategory::ground;
    const auto rejected = advance(world, 1000);
    check(rejected.started == 0 && world.state.reservations.empty() &&
              world.state.buildings.at(inn).completed_uses == 0,
          "unreachable world does not reserve or reward");
    world.simulation.terrain.cells.assign(49, RouteCategory::ground);
    advance(world, 300);
    check(world.state.characters.at(CharacterId{1}).target == inn,
          "newly reachable fixture recovers automatically");
}

void changed_layout_and_cancellation() {
    auto world = fixture();
    const auto inn = build(world);
    advance(world);
    const auto activity = *world.state.characters.at(CharacterId{1}).active_activity;
    check(move(world.state, inn, {5, 5}, 1) == Error::none, "move current target");
    const auto cancelled = advance(world);
    check(cancelled.cancelled == 1 && world.state.reservations.empty() &&
              world.simulation.motions.at(CharacterId{1}).path.empty(),
          "moving target releases stale route and reservation");
    check(complete_use(world.state, catalog, CharacterId{1}, activity) == Error::invalid_state,
          "cancelled activity cannot complete");
    advance(world, 300);
    check(world.state.characters.at(CharacterId{1}).target == inn,
          "moved facility can be selected with new geometry");
    check(remove(world.state, inn) == Error::none, "remove travelling target");
    const auto removed = advance(world);
    check(removed.cancelled == 1 && world.simulation.motions.at(CharacterId{1}).path.empty() &&
              world.state.characters.at(CharacterId{1}).accumulated_effect == 0,
          "removal clears runtime route without effect");

    auto obstacle = fixture();
    build(obstacle, {5, 3});
    advance(obstacle);
    const auto first_step = obstacle.simulation.motions.at(CharacterId{1}).path.front();
    build(obstacle, first_step, "cafe");
    check(advance(obstacle).cancelled == 1, "new building on old route invalidates activity");

    auto use = fixture();
    const auto used_inn = build(use, {1, 3});
    advance(use, 200);
    check(use.state.characters.at(CharacterId{1}).activity == ActivityState::in_use,
          "already-adjacent approach enters use without walking");
    check(move(use.state, used_inn, {1, 3}, 2) == Error::none, "rotate used target");
    check(advance(use).cancelled == 1 &&
              use.state.characters.at(CharacterId{1}).accumulated_effect == 0,
          "rotated target cancels use before effect");
}

void segmentation_and_pause() {
    auto batched = fixture();
    auto sliced = fixture();
    const auto inn = build(batched);
    build(sliced);
    advance(batched, 4567);
    for (int index = 0; index < 456; ++index) {
        advance(sliced, 10);
    }
    advance(sliced, 7);
    const auto &left = batched.simulation.motions.at(CharacterId{1});
    const auto &right = sliced.simulation.motions.at(CharacterId{1});
    check(batched.simulation.ticks == sliced.simulation.ticks &&
              batched.simulation.remainder_ms == sliced.simulation.remainder_ms &&
              left.cell == right.cell && left.path == right.path && left.cursor == right.cursor &&
              left.step_ticks == right.step_ticks && left.use_ticks == right.use_ticks &&
              left.wait_ticks == right.wait_ticks && left.activity == right.activity &&
              left.last_visited == right.last_visited &&
              batched.state.buildings.at(inn).completed_uses ==
                  sliced.state.buildings.at(inn).completed_uses &&
              batched.state.characters.at(CharacterId{1}).accumulated_effect ==
                  sliced.state.characters.at(CharacterId{1}).accumulated_effect,
          "large and small deltas produce identical state");
    batched.simulation.paused = true;
    const auto ticks = batched.simulation.ticks;
    advance(batched, 999);
    check(batched.simulation.ticks == ticks && batched.simulation.remainder_ms == 67,
          "pause discards elapsed input without advancing remainder");
    check(advance_simulation(batched.state, catalog, batched.simulation, -1).error ==
              SimulationError::invalid_input,
          "invalid delta rejected even while paused");
}

void invalid_inputs_and_atomic_overflow() {
    auto world = fixture();
    check(advance_simulation(world.state, catalog, world.simulation, 60001).error ==
              SimulationError::invalid_input,
          "unbounded delta rejected");
    auto duplicate_catalog = catalog;
    duplicate_catalog.push_back(catalog.front());
    check(advance_simulation(world.state, duplicate_catalog, world.simulation, 100).error ==
              SimulationError::invalid_input,
          "duplicate definition rejected");
    world.simulation.motions.at(CharacterId{1}).cell = {-1, 0};
    check(advance_simulation(world.state, catalog, world.simulation, 100).error ==
              SimulationError::inconsistent_state,
          "out of bounds character rejected");
    world = fixture();
    build(world);
    world.state.next_activity_id = std::numeric_limits<std::uint64_t>::max();
    check(advance_simulation(world.state, catalog, world.simulation, 100).error ==
                  SimulationError::numeric_overflow &&
              world.state.reservations.empty() && world.simulation.ticks == 0,
          "ID exhaustion leaves state unchanged");
    world = fixture();
    const auto inn = build(world, {1, 3});
    world.state.characters.at(CharacterId{1}).accumulated_effect = INT_MAX - 4;
    const auto overflow = advance_simulation(world.state, catalog, world.simulation, 500);
    check(overflow.error == SimulationError::numeric_overflow && overflow.started == 0 &&
              overflow.completed == 0 && world.simulation.ticks == 0 &&
              world.state.next_activity_id == 1 && world.state.reservations.empty() &&
              world.state.buildings.at(inn).completed_uses == 0,
          "late overflow rolls back all earlier ticks of batch");
    world = fixture();
    build(world, {1, 3});
    world.state.buildings.begin()->second.completed_uses =
        std::numeric_limits<std::uint64_t>::max();
    check(advance_simulation(world.state, catalog, world.simulation, 500).error ==
                  SimulationError::numeric_overflow &&
              world.simulation.ticks == 0,
          "use counter overflow rolls back entire batch");
    world = fixture();
    world.simulation.ticks = std::numeric_limits<std::uint64_t>::max();
    check(advance_simulation(world.state, catalog, world.simulation, 100).error ==
              SimulationError::numeric_overflow,
          "simulation clock overflow rejected");
    world = fixture();
    const auto reserved_inn = build(world);
    world.state.reservations.emplace(reserved_inn, CharacterId{1});
    check(advance_simulation(world.state, catalog, world.simulation, 100).error ==
              SimulationError::inconsistent_state,
          "orphan reservation rejected instead of deadlocking");
}

} // namespace

int main() {
    autonomous_lifecycle();
    multiple_characters_and_instances();
    no_candidates_and_unreachable();
    changed_layout_and_cancellation();
    segmentation_and_pause();
    invalid_inputs_and_atomic_overflow();
    std::cout << checks << " checks passed\n";
}
