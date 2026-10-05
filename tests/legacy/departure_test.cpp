// Real loaded startup -> candidate snapshot -> two draws -> route -> continuous entry.
// The ordinary-branch precondition is explicit; this is not a default AI runtime test.
#include "ark/app/game.hpp"
#include "ark/people/motion.hpp"
#include "legacy/support/people_test_support.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <set>

namespace {
using namespace ark;
using namespace ark::people;
using namespace ark::test;

ActivityCandidateInput startup_input(const app::Game &game, const world::RouteMap &map) {
    ActivityCandidateInput input;
    const auto &data = app::startup_data();
    input.town = {data.build_bounds.min_x, data.build_bounds.max_x, data.build_bounds.min_y,
                  data.build_bounds.max_y};
    for (const auto &tile : map.cells)
        input.cell_definition_ids.push_back(tile.definition_id);
    // Same fixed level1 definition charm as prototype/startup_map_test, not instance adjacency.
    for (const auto &d : data.definitions)
        input.definitions.push_back({d.id, d.activity_category, d.economy.attributes[2].first});
    for (auto id : game.state().instance_order) {
        const auto &i = game.state().facilities.at(id);
        input.instances.push_back({id, i.definition_id, i.remaining_ticks == 0 ? 1 : 0});
    }
    return input;
}
void real_startup_decisions() {
    app::Game game;
    const auto map = game.route_map();
    const auto input = startup_input(game, map);
    for (const auto spawn : app::startup_data().spawn_points) {
        const auto field = *world::search(map, spawn).field;
        const auto collected = collect_activity_candidates(field, input);
        check(collected.snapshot && collected.error == ActivityCandidateError::none,
              "real startup candidate snapshot");
        const auto &scene = *collected.snapshot;
        check(scene.category_counts[1] == 2 && scene.category_counts[2] == 1,
              "both spawn points see weapons, buns and inn");
        std::set<int> moved;
        int weapons{}, buns{}, inns{};
        for (int category = 0; category < 70; ++category)
            for (int facility = 0; facility < (category < 40 ? 9 : 5); ++facility) {
                const auto result = prepare_facility_departure(
                    field, scene, {0, {}, 2U | 8192U, category, facility});
                check(result.departure && result.error == FacilityDepartureError::none,
                      "all valid first ordinary tickets prepare complete departure");
                const auto &d = *result.departure;
                const auto target = d.binding.definition_id;
                weapons += target == 30;
                buns += target == 33;
                inns += target == 28;
                check(d.category == (category < 40 ? 1 : 2) &&
                          d.route.steps.back() == d.binding.cell &&
                          d.route.cost == *d.selection.goal.cost &&
                          world::arrival_matches(map, d.binding, d.binding.cell),
                      "selected category, field cost and original stable identity agree");
                const int facing = target == 30 ? (spawn.x == 11 ? 1 : 0) : (spawn.x == 12 ? 3 : 0);
                check(d.legacy_direction == facing, "maintained path tie determines first facing");
                if (!moved.insert(target).second)
                    continue;
                // Consume exactly the already selected route; do not choose/search again.
                Travel travel{spawn, d.binding, d.route.steps, waypoint(spawn, 4, -1)};
                int entered{};
                for (int step = 0; step < 1000 && travel.phase == TravelPhase::travelling; ++step) {
                    const auto next = advance_travel(map, travel, 2U | 8192U);
                    check(std::hypot(next.travel.position.x - travel.position.x,
                                     next.travel.position.z - travel.position.z) <= 6.71F,
                          "decision route feeds bounded continuous movement");
                    entered += next.entered;
                    travel = next.travel;
                }
                check(travel.phase == TravelPhase::entered && entered == 1 &&
                          !advance_travel(map, travel, 8194).entered,
                      "each chosen real target produces one logical entry");
            }
        check(weapons == 200 && buns == 160 && inns == 150 && moved == std::set<int>{28, 30, 33},
              "40:30 category and5:4 inner ticket intervals; not APK probabilities");
    }
    check(game.state().money == 5000 && game.state().points == 10 &&
              game.state().expenses.empty() && !game.state().adventurer &&
              game.state().next_id == 9 && game.state().simulation_steps == 0,
          "conditional decisions and travel commit no domain effects");
    for (const auto &[id, progress] : game.state().definition_progress) {
        (void)id;
        check(progress.completed_uses == 0 && !progress.upgrade_pending,
              "planning cannot complete facility use");
    }
    // Product construction changes the snapshot, without enabling an automatic chooser.
    game.open_catalog();
    game.select(28);
    check(game.confirm({9, 8}) == app::Error::none, "real product construction before AI query");
    auto current = game.route_map();
    auto field = *world::search(current, {11, 0}).field;
    auto candidates = collect_activity_candidates(field, startup_input(game, current));
    check(candidates.snapshot->category_counts[2] == 1,
          "unfinished new inn does not gain ordinary candidate weight");
    game.cancel();
    for (int tick = 0; tick < 280; ++tick)
        game.update();
    current = game.route_map();
    field = *world::search(current, {11, 0}).field;
    candidates = collect_activity_candidates(field, startup_input(game, current));
    check(candidates.snapshot->category_counts[2] == 2 && game.state().money == 4000,
          "completed inn becomes candidate; read-only query cannot charge another construction");
}

void rejection_and_facing_contracts() {
    const world::Cell goals[] = {{2, 3}, {3, 2}, {2, 1}, {1, 2}};
    for (int direction = 0; direction < 4; ++direction) {
        DecisionFixture f;
        f.add(goals[direction], 1, 30);
        const auto result = prepare_facility_departure(f.field(), f.collect(), {});
        check(result.departure && result.departure->legacy_direction == direction,
              "departure facing uses grid axes");
    }
    DecisionFixture f;
    f.add({3, 2}, 1, 30);
    const auto scene = f.collect();
    const auto field = f.field();
    const auto refuse = [&](const auto &candidate, FacilityDepartureInput input,
                            FacilityDepartureError error) {
        const auto result = prepare_facility_departure(field, candidate, input);
        check(result.error == error && !result.departure, "refused departure is never partial");
    };
    for (int ticket : {-1, 40})
        refuse(scene, {0, {}, 0, ticket, 0}, FacilityDepartureError::invalid_ticket);
    for (int ticket : {-1, 5})
        refuse(scene, {0, {}, 0, 0, ticket}, FacilityDepartureError::invalid_ticket);
    refuse(scene, {5}, FacilityDepartureError::unsupported_activity);
    refuse(scene, {0, {-1}}, FacilityDepartureError::invalid_input);
    refuse(ActivityCandidateSnapshot{}, {}, FacilityDepartureError::no_category);
    auto bad = scene;
    bad.cells[0].cost = 51;
    refuse(bad, {}, FacilityDepartureError::invalid_snapshot);
    bad = scene;
    bad.cells[0].instance->instance_id = 2;
    refuse(bad, {}, FacilityDepartureError::binding_mismatch);
    bad = scene;
    bad.cells[0].definition.definition_charm = 0;
    refuse(bad, {}, FacilityDepartureError::no_facility_weight);
    bad = scene;
    bad.cells[0].definition.legacy_category = 4;
    bad = snapshot(bad.cells);
    refuse(bad, {}, FacilityDepartureError::unsupported_category);
    refuse(bad, {0, {5}, 0, -1, -1}, FacilityDepartureError::unsupported_category);
    // Forced3 is reported as unsupported rather than misread as a random-ticket error.
    bad.cells[0].definition.legacy_category = 8;
    bad = snapshot(bad.cells);
    refuse(bad, {6, {}, 0, 40, 0}, FacilityDepartureError::unsupported_category);
    auto corrupt = field;
    corrupt.distances.pop_back();
    const auto invalid = prepare_facility_departure(corrupt, scene, {});
    check(invalid.error == FacilityDepartureError::invalid_field && !invalid.departure &&
              invalid.route_error == world::RouteError::invalid_field,
          "corrupt field distinct from no target");
    const auto same_field = f.field({3, 2});
    const auto same_scene = snapshot({candidate({3, 2}, 1, 30, 1, 5, 0)});
    const auto same = prepare_facility_departure(same_field, same_scene, {});
    check(same.departure && same.departure->route.steps.empty() &&
              !same.departure->legacy_direction,
          "empty departure path preserves facing and is not a charge/arrival event");
}

void selection_oracle() {
    // Explicitly expanded ticket bags independently check full/active/goal index mapping.
    std::mt19937 random(0x5AA9507U);
    for (int trial = 0; trial < 100; ++trial) {
        std::vector<ActivityCandidateCell> cells;
        const int categories[]{1, 2, 6, 8};
        for (std::uint64_t id = 1; id <= 6; ++id) {
            const int category = categories[random() % 4];
            const int phase = random() % 3;
            const auto charm = random() % 6;
            for (int count = 1 + random() % 4; count > 0; --count)
                cells.push_back(candidate({static_cast<int>(cells.size()), 0}, id,
                                          static_cast<int>(id), category, charm, random() % 6,
                                          phase));
        }
        cells.push_back(candidate({100, 0}, 0, 0, 1, 999, 0));
        cells.push_back(cells[0]);
        std::stable_sort(cells.begin(), cells.end(),
                         [](const auto &a, const auto &b) { return *a.cost < *b.cost; });
        const auto scene = snapshot(cells);
        for (const auto category : categories) {
            std::vector<std::pair<std::size_t, std::size_t>> bag;
            std::size_t active{};
            for (std::size_t i = 0; i < cells.size(); ++i) {
                const auto &c = cells[i];
                if (!c.instance || c.instance->legacy_phase != 1)
                    continue;
                if (c.definition.legacy_category == category)
                    for (int ticket = 0; ticket < c.definition.definition_charm; ++ticket)
                        bag.emplace_back(active, i);
                ++active;
            }
            for (std::size_t ticket = 0; ticket < bag.size(); ++ticket) {
                const auto output = select_snapshot_facility(scene, category, ticket);
                const auto [expected_active, drawn] = bag[ticket];
                std::size_t goal{};
                while (!cells[goal].instance ||
                       cells[goal].instance->instance_id != cells[drawn].instance->instance_id)
                    ++goal;
                check(output.target && output.target->drawn_active_index == expected_active &&
                          output.target->drawn_snapshot_index == drawn &&
                          output.target->goal_snapshot_index == goal,
                      "independent expanded-ticket oracle preserves duplicates and all indices");
            }
            const auto end = select_snapshot_facility(scene, category, bag.size());
            check(!end.target && end.error == (bag.empty() ? SnapshotFacilityError::no_weight
                                                           : SnapshotFacilityError::invalid_ticket),
                  "draw boundary distinct from empty weight");
        }
    }
    const auto max = std::numeric_limits<std::int64_t>::max();
    auto large =
        snapshot({candidate({0, 0}, 1, 30, 1, max - 1, 0), candidate({1, 0}, 2, 33, 1, 1, 1)});
    check(select_snapshot_facility(large, 1, max - 1).target->goal_snapshot_index == 1,
          "last exact-maximum ticket works");
    large.cells[1].definition.definition_charm = 2;
    const auto overflow = select_snapshot_facility(large, 1, 0);
    check(overflow.error == SnapshotFacilityError::numeric_overflow && !overflow.target,
          "total overflow refuses before choosing even first ticket");
}
} // namespace
int main() {
    try {
        real_startup_decisions();
        rejection_and_facing_contracts();
        selection_oracle();
        std::cout << "PASS facility choice/departure checks=" << ark::test::checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
