// Real loaded snapshot + dynamic bindings + numerical motion. Assertions stay active in Release.
#include "ark/app/game.hpp"
#include "ark/facilities/map_binding.hpp"
#include "ark/people/motion.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
using namespace ark;
int checks{};
void check(bool ok, const char *message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}
bool near(float a, float b) { return std::abs(a - b) < 0.0001F; }
template <class F> void rejects(F f) {
    bool rejected{};
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, "invalid input must be rejected");
}
void loaded_and_dynamic() {
    app::Game game;
    const auto &data = app::startup_data();
    const auto initial = game.route_map();
    check(data.loaded_cells.size() == 576 && world::valid_map(initial), "published dimensions");
    check(game.state().instance_order == std::vector<std::uint64_t>{3, 4, 5, 6, 7, 8, 1, 2},
          "original vector order");
    check(game.state().next_id == 9 && game.state().facilities.at(1).reset_legacy_id == 0,
          "raw zero is an instance; next ID independent of vector tail");
    for (std::size_t i = 0; i < 576; ++i) {
        const auto &loaded = data.loaded_cells[i];
        const auto &cell = initial.cells[i];
        check(cell.legacy_state == loaded.legacy_state && cell.category == loaded.category &&
                  cell.definition_id == loaded.definition_id,
              "all loaded logic fields");
        check(data.map.cells[i].display_id == loaded.display_id &&
                  data.map.cells[i].variant == loaded.variant,
              "all loaded display fields independent of logic");
        check(bool(cell.facility) == bool(loaded.instance_id), "loaded occupancy presence");
        if (cell.facility)
            check(cell.facility->instance == loaded.instance_id && cell.facility->fragment == 0 &&
                      loaded.legacy_instance_id == static_cast<int>(loaded.instance_id - 1),
                  "reset mapping");
    }
    for (auto [cell, fragment] : std::vector<std::pair<world::Cell, int>>{
             {{6, 10}, 4}, {{17, 10}, 2}, {{6, 2}, 3}, {{17, 2}, 5}})
        check(data.loaded_cells[data.map.index(cell)].boundary_fragment == fragment &&
                  initial.cells[initial.index(cell)].category == world::RouteCategory::blocked,
              "boundary corners");
    const auto &exterior = data.loaded_cells[data.map.index({11, 10})];
    check(exterior.definition_id == 83 && exterior.display_id == 27 && exterior.legacy_state == 7 &&
              exterior.external_direction == 2 && exterior.boundary_fragment == -1,
          "exterior logical/display separation");
    check(data.loaded_cells[data.map.index({11, 3})].road_mask == 3 &&
              data.map.cells[data.map.index({11, 3})].variant == 3,
          "published corner road mask/frame");
    game.open_catalog();
    game.select(28);
    check(game.preview({9, 8}) == app::Error::none && game.confirm({9, 8}) == app::Error::none,
          "cleared former flower ground is buildable");
    check(game.facility_at({9, 8}) == 9 && game.state().money == 4000,
          "new stable ID and single expense");
    check(game.confirm({11, 4}) == app::Error::none, "ordinary construction may cover road");
    const auto changed = game.route_map();
    check(changed.cells[changed.index({11, 4})].category == world::RouteCategory::terminal &&
              changed.cells[changed.index({11, 4})].facility->instance == 10,
          "dynamic binding replaces road");
    check(initial.cells[initial.index({11, 4})].category == world::RouteCategory::road &&
              !initial.cells[initial.index({9, 8})].facility,
          "queries leave prior snapshots immutable");
    check(game.state().instance_order.back() == 10 && game.state().next_id == 11,
          "append order and monotonic IDs");
    check(game.neighbourhood(4).road_cells == 2, "road charm follows current occupancy");
    const auto funds = game.state().money;
    const world::ArrivalTarget goal{{10, 5}, 4, 28};
    check(game.route_to({11, 0}, goal).error == world::RouteError::none &&
              game.state().money == funds,
          "current-map route query has no monetary effect");
    check(game.route_to({11, 0}, {{10, 5}, 3, 28}).error == world::RouteError::binding_mismatch,
          "stale target identity");
}
void binding_and_search() {
    app::Game game;
    world::RouteMap ground{5, 5, std::vector<world::RouteCell>(25)};
    for (int shape = 0; shape < 3; ++shape)
        for (int orientation = 0; orientation < 2; ++orientation) {
            auto definition = game.definition(28);
            definition.shape = shape;
            const facilities::Instance instance{99, 28, {2, 2}, orientation, 0, false};
            const auto map = facilities::bind_map(ground, {definition}, {{99, instance}});
            for (const auto &part : facilities::footprint(shape, orientation, {2, 2})) {
                const auto &binding = map.cells[map.index(part.cell)].facility;
                check(binding && binding->instance == 99 && binding->fragment == part.fragment,
                      "every footprint fragment");
            }
        }
    rejects([&] {
        facilities::bind_map(ground, {game.definition(28)}, {{7, {8, 28, {1, 1}, 0, 0, false}}});
    });
    rejects([&] {
        facilities::bind_map(
            ground, {game.definition(28)},
            {{7, {7, 28, {1, 1}, 0, 0, false}}, {8, {8, 28, {1, 1}, 0, 0, false}}});
    });
    rejects([&] {
        facilities::bind_map(ground, {game.definition(28)}, {{7, {7, 28, {5, 1}, 0, 0, false}}});
    });
    world::RouteMap map{3, 2, std::vector<world::RouteCell>(6)};
    auto field = world::search(map, {0, 0});
    check(field.field && world::trace(*field.field, {2, 1}).cost == 170,
          "anisotropic ground costs");
    for (auto &cell : map.cells) {
        cell.category = world::RouteCategory::road;
        cell.legacy_state = 3;
    }
    field = world::search(map, {0, 0});
    check(world::trace(*field.field, {2, 1}).cost == 17, "road costs one tenth of ground");
    // Mixed terrain exposes whether cost is incorrectly charged on arrival.
    world::RouteMap mixed{2, 1, {{}, {3, world::RouteCategory::road, 15, -1, {}}}};
    check(world::trace(*world::search(mixed, {0, 0}).field, {1, 0}).cost == 50 &&
              world::trace(*world::search(mixed, {1, 0}).field, {0, 0}).cost == 5,
          "cost belongs to departure cell");
    // Published 5x5 admission table, with state-based first-expansion exception separate.
    constexpr bool transitions[5][5] = {{false, false, false, false, false},
                                        {true, true, true, false, true},
                                        {true, true, true, false, true},
                                        {false, false, false, false, false},
                                        {true, true, true, false, true}};
    constexpr int states[]{1, 3, 4, 2, 6};
    for (int from = 0; from < 5; ++from)
        for (int to = 0; to < 5; ++to) {
            const world::RouteCell a{
                states[from], static_cast<world::RouteCategory>(from), 17, 0, {}};
            const world::RouteCell b{states[to], static_cast<world::RouteCategory>(to), 17, 0, {}};
            check(world::route_transition(a, b) == transitions[from][to],
                  "published category transition matrix");
            check(world::route_transition(a, b, true) ==
                      (to == 1 || to == 2 || transitions[from][to]),
                  "first expansion permits state3/4");
        }
    check(world::trace(*field.field, {0, 0}).steps.empty(), "same endpoint");
    auto corrupt = *field.field;
    corrupt.previous[1] = 1;
    check(world::trace(corrupt, {1, 0}).error == world::RouteError::invalid_field,
          "predecessor cycle rejected");
    check(world::search(map, {0, 0}, {0, 100, true}).error == world::RouteError::cost_limit,
          "cost limit is not unreachable");
    check(world::search(map, {0, 0}, {100, 0, true}).error == world::RouteError::expansion_limit,
          "expansion limit");
    check(world::search(map, {0, 0}, {-1, 100, true}).error == world::RouteError::invalid_limits,
          "negative cost limit");
    map.cells[1] = {1, world::RouteCategory::terminal, 28, 0, world::TileBinding{7, 28, 0}};
    map.cells[3].category = map.cells[4].category = map.cells[5].category =
        world::RouteCategory::blocked;
    field = world::search(map, {0, 0});
    check(world::trace(*field.field, {1, 0}).error == world::RouteError::none &&
              world::trace(*field.field, {2, 0}).error == world::RouteError::unreachable,
          "shop reachable but cannot transit");
    auto exit = world::search(map, {1, 0});
    check(world::trace(*exit.field, {2, 0}).error == world::RouteError::none,
          "first expansion exits occupied terminal");
    exit = world::search(map, {1, 0}, {100, 100, false});
    check(world::trace(*exit.field, {2, 0}).error == world::RouteError::unreachable,
          "first-exit exception can be disabled");
    map.cells.pop_back();
    check(world::search(map, {0, 0}).error == world::RouteError::invalid_map,
          "truncated route map");
}
void motion_and_entry() {
    for (int state : {6, 7})
        for (int d = 0; d < 4; ++d) {
            const world::WorldPosition expected[] = {
                {150, 290}, {150, 210}, {190, 250}, {110, 250}};
            const auto p = people::waypoint({1, 2}, state, d);
            check(p.x == expected[d].x && p.z == expected[d].z,
                  "definition-directed entrance offset");
        }
    rejects([] { people::waypoint({1, 2}, 6, -1); });
    const auto diagonal = people::advance_motion({0, 0}, {30, 40}, 0);
    check(near(diagonal.position.x, 4.02F) && near(diagonal.position.z, 5.36F),
          "normalized movement");
    check(near(people::advance_motion({0, 0}, {3, 0}, 0).position.x, 6.7F),
          "overshoot not clamped");
    for (float delta : {3.999F, 4.0F, 4.001F}) {
        const auto v = people::advance_motion({100 + delta, 100 + delta}, {100, 100}, 64);
        check(v.position.x == 100 + delta && v.waypoint_overlap == (delta <= 4),
              "flag64 and inclusive rectangle");
    }
    check(people::world_cell({-99, -101}) == world::Cell{0, -1}, "truncate toward zero");
    check(!people::world_cell({std::numeric_limits<float>::infinity(), 0}),
          "nonfinite conversion rejected");
    rejects(
        [] { people::advance_motion({0, 0}, {std::numeric_limits<float>::quiet_NaN(), 0}, 0); });
    world::RouteMap map{
        2, 1, {{}, {1, world::RouteCategory::terminal, 28, 0, world::TileBinding{7, 28, 0}}}};
    const world::ArrivalTarget target{{1, 0}, 7, 28};
    check(people::inspect_entry(map, target, {99.99F, 50}, true) ==
              people::EntryStatus::not_entered,
          "entry before boundary");
    check(people::inspect_entry(map, target, {100, 50}, true) == people::EntryStatus::ready,
          "entry before centre");
    check(people::inspect_entry(map, target, {100, 50}, false) ==
              people::EntryStatus::inactive_route,
          "cleared route does not enter twice");
    auto travel = *people::plan_travel(map, {0, 0}, target);
    auto forged = travel;
    forged.path = {{0, 0}, {1, 0}};
    rejects([&] { people::advance_travel(map, forged, 0); });
    auto stale = map;
    stale.cells[1].facility->instance = 8;
    check(people::advance_travel(stale, travel, 0).travel.phase ==
              people::TravelPhase::stale_target,
          "obsolete instance stops travel");
    world::RouteMap corridor{3, 1, {{}, {}, map.cells[1]}};
    const auto planned = *people::plan_travel(corridor, {0, 0}, {{2, 0}, 7, 28});
    corridor.cells[1] = {2, world::RouteCategory::blocked, 66, 0, world::TileBinding{8, 66, 0}};
    const auto blocked = people::advance_travel(corridor, planned, 0);
    check(blocked.travel.phase == people::TravelPhase::blocked && !blocked.entered &&
              blocked.travel.position.x == planned.position.x &&
              blocked.travel.position.z == planned.position.z,
          "construction across planned route stops without moving or entering");
    map.cells[0].category = world::RouteCategory::blocked;
    travel.next = 1;
    rejects([&] { people::advance_travel(map, travel, 0); });
}
void real_travel() {
    app::Game game;
    const auto map = game.route_map();
    for (const auto spawn : app::startup_data().spawn_points)
        for (int definition : {28, 33, 30}) {
            const auto id = definition == 28 ? 4U : definition == 33 ? 3U : 5U;
            const auto &instance = game.state().facilities.at(id);
            auto travel = people::plan_travel(map, spawn, {instance.anchor, id, definition});
            check(travel.has_value(), "real spawn-to-shop route");
            int entries = 0;
            for (int i = 0; i < 1000 && travel->phase == people::TravelPhase::travelling; ++i) {
                const auto next = people::advance_travel(map, *travel, 2U | 8192U);
                check(std::hypot(next.travel.position.x - travel->position.x,
                                 next.travel.position.z - travel->position.z) <= 6.71F,
                      "continuous bounded displacement");
                entries += next.entered;
                travel = next.travel;
            }
            check(travel->phase == people::TravelPhase::entered && entries == 1,
                  "all six real routes enter once");
            check(people::world_cell(travel->position) == instance.anchor,
                  "target occupied cell reached");
            check(!people::advance_travel(map, *travel, 0).entered,
                  "repeat advance after entry inert");
        }
    check(game.state().money == 5000 && game.state().expenses.empty() && !game.state().adventurer,
          "conditional travel never creates actors, visits or cash");
}
} // namespace
int main() {
    try {
        loaded_and_dynamic();
        binding_and_search();
        motion_and_entry();
        real_travel();
        std::cout << "PASS loaded map/navigation/motion checks=" << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
