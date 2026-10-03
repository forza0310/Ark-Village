// Candidate order, duplicate weight and refusal contracts from research/ACTIVITY.
#include "people_test_support.hpp"
#include <algorithm>
#include <iostream>
#include <limits>

namespace {
using namespace ark;
using namespace ark::people;
using namespace ark::test;
void duplicates_and_order() {
    DecisionFixture f;
    // Row-major costs120,120,50 must exchange into C,B,A, not stable-sort into C,A,B.
    f.add({1, 1}, 1, 30);
    f.add({3, 1}, 2, 30);
    f.add({3, 2}, 3, 33);
    auto result = f.collect();
    check(result.cells.size() == 3 && result.cells[0].instance->instance_id == 3 &&
              result.cells[1].instance->instance_id == 2 &&
              result.cells[2].instance->instance_id == 1,
          "exchange order can reverse equal-cost predecessors");
    f.input.events = {{{2, 2}, 1}, {{2, 2}, 1}, {{3, 2}, 1}, {{3, 2}, 0}};
    result = f.collect();
    int starts{}, food{};
    for (const auto &cell : result.cells) {
        starts += cell.position == world::Cell{2, 2};
        food += cell.definition.definition_id == 33;
    }
    check(result.cells.size() == 5 && starts == 1 && food == 2 && result.category_counts[0] == 1 &&
              result.category_counts[1] == 4,
          "remove first start only; retain scan/event duplicates and unbound category counts");
    f.input.events.clear();
    f.input.instances[0].legacy_phase = 0;
    f.input.instances[1].legacy_phase = 2;
    result = f.collect();
    check(result.cells.size() == 2 && result.category_counts[1] == 1,
          "phase0 excluded from ordinary scan; phase2 kept but not weighted/counted");
    f.input.events = {{{1, 1}, 1}};
    result = f.collect();
    check(result.cells.size() == 3 && result.category_counts[1] == 1,
          "event appends phase0 without enabling its ordinary category weight");
}
void origin_and_region_filters() {
    DecisionFixture f;
    f.add({2, 2}, 1, 30);
    f.add({3, 2}, 2, 33);
    f.input.last_visited_instance = 2;
    const auto bound = f.collect();
    check(bound.cells.empty(), "bound start filters last visited identity, not current identity");
    check(f.collect({0, 0}).cells.size() == 2,
          "unbound start does not filter last visited identity");
    DecisionFixture region;
    region.input.legacy_activity = 7;
    auto inside = region.collect();
    check(inside.cells.size() == 8, "strict3x3 interior minus start");
    for (const auto &cell : inside.cells)
        check(cell.position.x > 0 && cell.position.x < 4 && cell.position.y > 0 &&
                  cell.position.y < 4,
              "all boundary edges excluded from interior");
    region.input.events = {{{2, 2}, 1}, {{2, 2}, 1}};
    check(region.collect().cells.size() == 8, "activity7 ignores event append");
    for (int activity : {6, 8}) {
        region.input.legacy_activity = activity;
        const auto exterior = region.collect();
        check(exterior.cells.size() == 11, "exterior excludes upper edge and strict interior");
        for (const auto &cell : exterior.cells)
            check(cell.position.y > 0 && !inside_town(cell.position, region.input.town),
                  "exterior has exact geometric admission");
    }
    auto category4 =
        snapshot({candidate({0, 1}, 1, 75, 4, 0, 1, 2), candidate({1, 1}, 2, 75, 4, 0, 2)});
    const auto chosen = select_counted_category_four(category4, 0);
    check(category4.category_counts[4] == 1 && chosen.target && chosen.target->index == 0 &&
              chosen.target->cell.instance->legacy_phase == 2,
          "category4 draws counted range but scans full category view");
    check(select_counted_category_four(category4, 1).error ==
              ActivityCandidateError::invalid_ticket,
          "category4 exclusive ticket bound");
}
void unreachable_and_full_footprint() {
    DecisionFixture f(7, 3);
    f.add({1, 1}, 1, 30);
    f.add({5, 1}, 2, 33);
    for (int y = 0; y < 3; ++y)
        f.map.cells[f.map.index({3, y})] = {5, world::RouteCategory::blocked, 17, -1, {}};
    f.input.events = {{{5, 1}, 1}, {{5, 1}, 1}};
    const auto scene = f.collect({0, 1});
    check(scene.cells.size() == 3 && scene.category_counts[1] == 3 && !scene.cells[1].cost &&
              !scene.cells[2].cost,
          "unreachable event duplicates retain category count and unknown cost");
    const auto target = select_snapshot_facility(scene, 1, 5);
    check(target.target && target.target->goal.instance->instance_id == 2 &&
              !target.target->goal.cost,
          "selected unreachable occurrence is not silently removed");
    const auto blocked = prepare_facility_departure(f.field({0, 1}), scene, {0, {}, 8192, 0, 5});
    check(blocked.error == FacilityDepartureError::route_failure &&
              blocked.route_error == world::RouteError::unreachable && !blocked.departure,
          "unreachable selection fails whole departure without redraw");
    DecisionFixture multi;
    multi.add({2, 2}, 3, 30);
    multi.add({3, 2}, 3, 30, 1, 1);
    const auto cells = multi.collect({0, 0});
    check(cells.cells.size() == 2 && cells.category_counts[1] == 2,
          "each reachable footprint cell contributes weight");
    const auto last = select_snapshot_facility(cells, 1, 9);
    check(last.target && last.target->drawn_snapshot_index == 1 &&
              last.target->goal_snapshot_index == 0,
          "second weighted fragment selects first snapshot goal of same instance");
}
void rejects_corrupt_input() {
    DecisionFixture f;
    f.add({3, 2}, 1, 30);
    const auto field = f.field();
    const auto refuse = [&](const ActivityCandidateInput &input, ActivityCandidateError error) {
        const auto result = collect_activity_candidates(field, input);
        check(result.error == error && !result.snapshot, "refusal exposes no partial candidates");
    };
    for (int mutation = 0; mutation < 10; ++mutation) {
        auto bad = f.input;
        switch (mutation) {
        case 0:
            bad.cell_definition_ids.pop_back();
            break;
        case 1:
            bad.town.right = bad.town.left;
            break;
        case 2:
            bad.definitions.push_back(bad.definitions[0]);
            break;
        case 3:
            bad.definitions[0].definition_charm = -1;
            break;
        case 4:
            bad.instances.push_back(bad.instances[0]);
            break;
        case 5:
            bad.last_visited_instance = 0;
            break;
        case 6:
            bad.events = {{{5, 0}, 1}};
            break;
        case 7:
            bad.instances[0].legacy_phase = -1;
            break;
        case 8:
            bad.events = {{{0, 0}, -1}};
            break;
        case 9:
            bad.definitions[0].legacy_category = 11;
            break;
        }
        refuse(bad, ActivityCandidateError::invalid_input);
    }
    auto missing = f.input;
    missing.instances.clear();
    refuse(missing, ActivityCandidateError::binding_mismatch);
    auto wrong = field;
    wrong.previous[wrong.map.index({3, 2})] = wrong.map.index({3, 2});
    check(collect_activity_candidates(wrong, f.input).error ==
              ActivityCandidateError::invalid_field,
          "cyclic field refused before candidate scan");
    auto limit = f.input;
    limit.events.assign(4095, {{3, 2}, 1});
    check(collect_activity_candidates(field, limit).snapshot->cells.size() == 4096,
          "exact bounded snapshot size accepted");
    limit.events.push_back({{3, 2}, 1});
    refuse(limit, ActivityCandidateError::candidate_limit);
    auto good = f.collect();
    for (int mutation = 0; mutation < 6; ++mutation) {
        auto bad = good;
        switch (mutation) {
        case 0:
            bad.cells[0].instance->instance_id = 0;
            break;
        case 1:
            bad.cells[0].instance->definition_id = 28;
            break;
        case 2:
            bad.category_counts[1] = 2;
            break;
        case 3:
            bad.cells[0].cost = -1;
            break;
        case 4:
            bad.cells[0].origin = static_cast<CandidateOrigin>(99);
            break;
        case 5:
            bad.cells[0].position.x = -1;
            break;
        }
        check(!valid_activity_candidate_snapshot(bad), "forged snapshot rejected");
    }
}
} // namespace
int main() {
    try {
        duplicates_and_order();
        origin_and_region_filters();
        unreachable_and_full_footprint();
        rejects_corrupt_input();
        std::cout << "PASS activity candidates checks=" << ark::test::checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
