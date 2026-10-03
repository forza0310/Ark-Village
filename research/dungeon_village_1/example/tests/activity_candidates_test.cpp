#include "dungeon_village_reference/activity_candidates.hpp"

#include "dungeon_village_reference/activity_choice.hpp"
#include "dungeon_village_reference/ranked_facility_choice.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <set>

using namespace dungeon_village_reference;

namespace {

int checks = 0;

void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

std::size_t index_of(const LegacyMap &map, Position position) {
    return static_cast<std::size_t>(position.y * map.width + position.x);
}

LegacyMap terrain(int width, int height) {
    return {width, height, std::vector<LegacyMapCell>(static_cast<std::size_t>(width * height))};
}

LegacyDistanceField search(const LegacyMap &map, Position start) {
    const auto result = search_legacy_map(map, start);
    check(result.error == MapAccessError::none && result.field.has_value(),
          "fixture search succeeds");
    return *result.field;
}

ActivityCandidateInput metadata(const LegacyDistanceField &field, int category = 1, int phase = 1) {
    ActivityCandidateInput input;
    input.town = {0, field.map.width - 1, 0, field.map.height - 1};
    input.cell_definition_ids.assign(field.map.cells.size(), 0);
    input.definitions.push_back({0, 0, 0});
    std::set<std::int32_t> definitions{0};
    std::set<BuildingId> instances;
    for (std::size_t index = 0; index < field.map.cells.size(); ++index) {
        const auto &binding = field.map.cells[index].facility;
        if (!binding) {
            continue;
        }
        input.cell_definition_ids[index] = binding->definition_id;
        if (definitions.insert(binding->definition_id).second) {
            input.definitions.push_back({binding->definition_id, category, 5});
        }
        if (instances.insert(binding->instance_id).second) {
            input.instances.push_back({binding->instance_id, binding->definition_id, phase});
        }
    }
    return input;
}

ActivityCandidateSnapshot collect(const LegacyDistanceField &field,
                                  const ActivityCandidateInput &input) {
    const auto result = collect_activity_candidates(field, input);
    check(result.error == ActivityCandidateError::none && result.snapshot.has_value(),
          "complete candidate snapshot");
    return *result.snapshot;
}

bool same_cell(const ActivityCandidateCell &left, const ActivityCandidateCell &right) {
    if (!(left.position == right.position) ||
        left.definition.definition_id != right.definition.definition_id ||
        left.definition.legacy_category != right.definition.legacy_category ||
        left.definition.definition_charm != right.definition.definition_charm ||
        left.instance.has_value() != right.instance.has_value() || left.cost != right.cost ||
        left.origin != right.origin || left.source_index != right.source_index) {
        return false;
    }
    return !left.instance || (left.instance->instance_id == right.instance->instance_id &&
                              left.instance->definition_id == right.instance->definition_id &&
                              left.instance->legacy_phase == right.instance->legacy_phase);
}

void ordinary_truth_table() {
    for (int state = 0; state <= 12; ++state) {
        for (int phase = 0; phase <= 3; ++phase) {
            for (const bool start_has_instance : {false, true}) {
                for (int last = 0; last <= 3; ++last) {
                    auto map = terrain(3, 3);
                    map.cells[4] = {state, RouteCategory::terminal,
                                    FacilityTileBinding{{1}, 10, 0}};
                    if (start_has_instance) {
                        map.cells[0] = {6, RouteCategory::access, FacilityTileBinding{{2}, 20, 0}};
                    }
                    const auto field = search(map, {0, 0});
                    auto input = metadata(field, 4, phase);
                    if (last != 0) {
                        input.last_visited_instance = BuildingId{static_cast<std::uint64_t>(last)};
                    }
                    const auto output = collect(field, input);
                    const bool state_allowed = state == 1 || state == 6 || state == 7 ||
                                               state == 8 || state == 9 || state == 10;
                    const bool included =
                        state_allowed && phase != 0 && !(start_has_instance && last == 1);
                    check(output.cells.size() == (included ? 1U : 0U),
                          "ordinary state phase and conditional last-instance truth table");
                    check(output.category_counts[4] == (included && phase == 1 ? 1 : 0),
                          "only active instance occurrence contributes category count");
                    if (included) {
                        check(output.cells[0].position == Position{1, 1} &&
                                  output.cells[0].instance->instance_id.value == 1 &&
                                  output.cells[0].origin == CandidateOrigin::map_scan &&
                                  output.cells[0].source_index == 4,
                              "map candidate identity and source survive");
                    }
                }
            }
        }
    }
    auto map = terrain(3, 3);
    map.cells[0] = {6, RouteCategory::access, FacilityTileBinding{{1}, 10, 0}};
    map.cells[1] = {6, RouteCategory::access, FacilityTileBinding{{1}, 10, 2}};
    const auto field = search(map, {0, 0});
    auto input = metadata(field);
    check(collect(field, input).cells.size() == 1,
          "only current coordinate excluded, not other cells of the same instance");
    input.last_visited_instance = BuildingId{1};
    check(collect(field, input).cells.empty(), "last-instance filter covers every matching cell");
}

void town_and_special_activities() {
    const TownBounds town{1, 5, 1, 4};
    for (int y = 0; y < 6; ++y) {
        for (int x = 0; x < 7; ++x) {
            check(inside_town({x, y}, town) == (x > 1 && x < 5 && y > 1 && y < 4),
                  "strict town bounds exclude all four border lines");
        }
    }
    check(!inside_town({2, 2}, {5, 1, 1, 4}), "reversed bounds never classify interior");
    auto map = terrain(7, 6);
    map.cells[index_of(map, {0, 2})] = {3, RouteCategory::road, std::nullopt};
    map.cells[index_of(map, {6, 2})] = {9, RouteCategory::ground, std::nullopt};
    const auto field = search(map, {0, 0});
    auto input = metadata(field);
    input.town = town;
    input.events = {{{3, 3}, 1}, {{0, 5}, 1}};
    input.legacy_activity = 6;
    const auto exterior = collect(field, input);
    check(exterior.cells.size() == 21, "exterior excludes north band, interior and road state");
    for (const auto &cell : exterior.cells) {
        check(cell.position.y > 1 && !inside_town(cell.position, town) &&
                  cell.origin == CandidateOrigin::map_scan,
              "exterior keeps boundary and southern cells but does not append events");
    }
    check(std::any_of(exterior.cells.begin(), exterior.cells.end(),
                      [](const auto &cell) { return cell.position == Position{6, 2}; }),
          "exterior retains state nine without requiring a facility");
    input.legacy_activity = 8;
    const auto equivalent = collect(field, input);
    check(equivalent.cells.size() == exterior.cells.size(), "activities six and eight same size");
    for (std::size_t index = 0; index < exterior.cells.size(); ++index) {
        check(same_cell(equivalent.cells[index], exterior.cells[index]),
              "activities six and eight identical ordered candidates");
    }
    input.legacy_activity = 7;
    const auto interior = collect(field, input);
    check(interior.cells.size() == 6, "interior does not require an ordinary facility state");
    for (const auto &cell : interior.cells) {
        check(inside_town(cell.position, town), "interior only strict inner rectangle");
    }
    const auto inside_field = search(map, {3, 2});
    check(collect(inside_field, input).cells.size() == 5,
          "special candidate collection removes start once after scanning");
}

void events_counts_and_tickets() {
    auto map = terrain(5, 3);
    for (int y = 0; y < 3; ++y) {
        map.cells[index_of(map, {2, y})] = {5, RouteCategory::blocked, std::nullopt};
    }
    map.cells[index_of(map, {1, 1})] = {1, RouteCategory::terminal,
                                        FacilityTileBinding{{1}, 10, 0}};
    const auto field = search(map, {0, 1});
    auto input = metadata(field, 4, 2);
    input.definitions[0].legacy_category = 4;
    input.events = {{{1, 1}, 1}, {{1, 1}, 1}, {{4, 1}, 1}, {{4, 1}, 1},
                    {{0, 1}, 1}, {{0, 1}, 1}, {{1, 1}, 2}, {{4, 2}, 0}};
    const auto output = collect(field, input);
    check(output.cells.size() == 6 && output.category_counts[4] == 3,
          "event duplicates and nonactive definitions differ from category count");
    check(output.cells[0].position == Position{0, 1} && output.cells[0].source_index == 5,
          "remove first start occurrence, preserve second event at start");
    check(!output.cells[4].cost && !output.cells[5].cost &&
              output.cells[4].position == Position{4, 1},
          "unreachable event occurrences retained after all finite costs");
    for (std::int64_t ticket = 0; ticket < 3; ++ticket) {
        const auto result = select_counted_category_four(output, ticket);
        check(result.error == ActivityCandidateError::none && result.target &&
                  result.target->index == static_cast<std::size_t>(ticket),
              "category four scans every definition occurrence within active-count ticket range");
        if (ticket > 0) {
            check(result.target->cell.instance && result.target->cell.instance->legacy_phase == 2,
                  "nonactive earlier definition can be selected by original category-four scan");
        }
    }
    for (const std::int64_t ticket : {-1, 3, 6}) {
        const auto result = select_counted_category_four(output, ticket);
        check(result.error == ActivityCandidateError::invalid_ticket && !result.target,
              "ticket range uses category count, not number of all matching definitions");
    }
    input.events.clear();
    const auto inactive = collect(field, input);
    check(inactive.cells.size() == 1 && inactive.category_counts[4] == 0,
          "nonactive ordinary candidate remains without a counted ticket");
    const auto none = select_counted_category_four(inactive, 0);
    check(none.error == ActivityCandidateError::no_candidate && !none.target,
          "zero count does not scan a nonactive category-four definition");
    input.instances[0].legacy_phase = 0;
    input.events = {{{1, 1}, 1}};
    check(collect(field, input).cells.size() == 1,
          "event bypasses construction-phase filter applied to map scan");
    input.events = {{{4, 1}, 1}};
    const auto unreachable = collect(field, input);
    const auto selected = select_counted_category_four(unreachable, 0);
    check(selected.error == ActivityCandidateError::none && selected.target &&
              !selected.target->cell.cost,
          "selected event is not automatically a reachable or executable path");
}

void exchange_sort_tie() {
    const auto field = search(terrain(3, 3), {1, 1});
    auto input = metadata(field);
    input.definitions[0].legacy_category = 4;
    input.events = {{{0, 2}, 1}, {{2, 2}, 1}, {{0, 1}, 1}};
    const auto output = collect(field, input);
    check(output.cells.size() == 3 && output.cells[0].source_index == 2 &&
              output.cells[1].source_index == 1 && output.cells[2].source_index == 0,
          "costs 120 120 50 become C B A, not stable C A B");
    check(output.cells[1].cost == 120 && output.cells[2].cost == 120,
          "equal-cost order reverses indirectly without comparing equals as lower");
    const auto choice = select_counted_category_four(output, 1);
    check(choice.error == ActivityCandidateError::none && choice.target->cell.source_index == 1,
          "category-four ticket preserves verified exchange tie order");
}

void refusals() {
    auto map = terrain(3, 3);
    map.cells[4] = {1, RouteCategory::terminal, FacilityTileBinding{{1}, 10, 0}};
    const auto field = search(map, {0, 0});
    const auto original = metadata(field, 4);
    const auto refuse = [&](const auto &candidate_field, const auto &input, auto expected) {
        const auto result = collect_activity_candidates(candidate_field, input);
        check(result.error == expected && !result.snapshot,
              "input refusal has no partial snapshot");
    };
    auto bad_field = field;
    bad_field.distances[0] = 1;
    refuse(bad_field, original, ActivityCandidateError::invalid_field);
    for (int mutation = 0; mutation < 15; ++mutation) {
        auto input = original;
        switch (mutation) {
        case 0:
            input.town.right = input.town.left;
            break;
        case 1:
            input.town.bottom = input.town.top;
            break;
        case 2:
            input.cell_definition_ids.pop_back();
            break;
        case 3:
            input.cell_definition_ids[0] = 99;
            break;
        case 4:
            input.definitions[0].definition_id = -1;
            break;
        case 5:
            input.definitions[0].legacy_category = -1;
            break;
        case 6:
            input.definitions[0].legacy_category = 11;
            break;
        case 7:
            input.definitions[0].definition_charm = -1;
            break;
        case 8:
            input.definitions.push_back(input.definitions[0]);
            break;
        case 9:
            input.instances[0].instance_id.value = 0;
            break;
        case 10:
            input.instances[0].legacy_phase = -1;
            break;
        case 11:
            input.instances[0].definition_id = 99;
            break;
        case 12:
            input.instances.push_back(input.instances[0]);
            break;
        case 13:
            input.last_visited_instance = BuildingId{0};
            break;
        case 14:
            input.events = {{{1, 1}, -1}};
            break;
        }
        refuse(field, input, ActivityCandidateError::invalid_input);
    }
    for (const auto position : {Position{-1, 0}, Position{0, -1}, Position{3, 0}, Position{0, 3}}) {
        auto input = original;
        input.events = {{position, 0}};
        refuse(field, input, ActivityCandidateError::invalid_input);
    }
    auto input = original;
    input.instances.clear();
    refuse(field, input, ActivityCandidateError::binding_mismatch);
    input = original;
    input.instances[0].definition_id = 0;
    refuse(field, input, ActivityCandidateError::binding_mismatch);
    input = original;
    input.cell_definition_ids[4] = 0;
    refuse(field, input, ActivityCandidateError::binding_mismatch);
    auto no_binding = field;
    no_binding.map.cells[4].facility.reset();
    input = original;
    input.instances.clear();
    refuse(no_binding, input, ActivityCandidateError::binding_mismatch);
    const auto snapshot = collect(field, original);
    for (int mutation = 0; mutation < 12; ++mutation) {
        auto bad = snapshot;
        auto &cell = bad.cells[0];
        switch (mutation) {
        case 0:
            bad.category_counts[4] = -1;
            break;
        case 1:
            bad.category_counts[3] = 1;
            break;
        case 2:
            cell.position.x = -1;
            break;
        case 3:
            cell.definition.definition_id = -1;
            break;
        case 4:
            cell.definition.legacy_category = 11;
            break;
        case 5:
            cell.definition.definition_charm = -1;
            break;
        case 6:
            cell.cost = -1;
            break;
        case 7:
            cell.origin = static_cast<CandidateOrigin>(99);
            break;
        case 8:
            cell.instance->instance_id.value = 0;
            break;
        case 9:
            cell.instance->legacy_phase = -1;
            break;
        case 10:
            cell.instance->definition_id = 0;
            break;
        case 11:
            bad.cells.push_back(cell);
            break;
        }
        const auto result = select_counted_category_four(bad, 0);
        check(result.error == ActivityCandidateError::invalid_snapshot && !result.target,
              "forged snapshot rejected before selection");
    }
    input = original;
    input.events.assign(4096, {{0, 1}, 1});
    refuse(field, input, ActivityCandidateError::candidate_limit);
    input.events.resize(4095);
    check(collect(field, input).cells.size() == 4096, "exact bounded sorting budget accepted");
    auto too_many = snapshot;
    too_many.cells.assign(4097, snapshot.cells[0]);
    too_many.category_counts[4] = 4097;
    check(select_counted_category_four(too_many, 0).error ==
              ActivityCandidateError::invalid_snapshot,
          "snapshot size guard before expensive validation");
}

void map_plan_and_ranked_composition() {
    const auto ground = terrain(7, 6);
    const std::vector<BoundFacility> buildings{
        {{{1}, 10, FacilityShape::square, FacilityOrientation::first, {3, 2}}, 3},
        {{{2}, 20, FacilityShape::single, FacilityOrientation::first, {5, 2}}, 3}};
    const auto bound = bind_facility_map(ground, buildings);
    check(bound.error == MapAccessError::none && bound.map, "composed geometry binding");
    const auto field = search(*bound.map, {0, 0});
    const auto input = metadata(field);
    const auto snapshot = collect(field, input);
    check(snapshot.cells.size() == 5 && snapshot.category_counts[1] == 5,
          "snapshot preserves all square and single occurrences");
    ActivityChoiceInput choice;
    choice.available_category_counts = snapshot.category_counts;
    const auto plan = plan_activity_categories(choice);
    check(plan.error == ActivityChoiceError::none && plan.plan->options.size() == 1 &&
              plan.plan->options[0].category == 1 && plan.plan->options[0].weight == 40,
          "map-derived counts flow to first-level plan without quantity multiplying weight");
    std::vector<RankedFacilityCell> ranked;
    for (const auto &cell : snapshot.cells) {
        ranked.push_back({cell.position, cell.instance->instance_id, cell.definition.definition_id,
                          cell.definition.legacy_category, cell.instance->legacy_phase,
                          cell.definition.definition_charm, *cell.cost});
    }
    int square_tickets = 0;
    for (std::int64_t ticket = 0; ticket < 25; ++ticket) {
        const auto result = select_ranked_facility(ranked, 1, ticket);
        check(result.error == RankedFacilityError::none && result.target,
              "event-free finite bound snapshot composes with R2-I");
        const auto &goal = result.target->goal;
        if (goal.instance_id.value == 1) {
            ++square_tickets;
        }
        const auto route = trace_legacy_path(field, goal.position);
        check(route.error == MapAccessError::none && route.cost == goal.cost &&
                  arrival_binding_matches(*bound.map,
                                          {goal.position, goal.instance_id, goal.definition_id},
                                          goal.position),
              "candidate goal traces and revalidates physical binding");
    }
    check(square_tickets == 20, "composed square remains four times single definition weight");
}

void snapshot_consistency() {
    auto map = terrain(3, 3);
    map.cells[4] = {6, RouteCategory::access, FacilityTileBinding{{1}, 10, 0}};
    map.cells[5] = {6, RouteCategory::access, FacilityTileBinding{{1}, 10, 2}};
    const auto field = search(map, {0, 0});
    auto input = metadata(field, 4);
    input.events = {{{1, 1}, 1}};
    const auto original = collect(field, input);
    check(original.cells.size() == 3 && original.category_counts[4] == 3,
          "valid repeated coordinate and multiple cells of one instance");
    for (int mutation = 0; mutation < 8; ++mutation) {
        auto bad = original;
        auto &cell = bad.cells.back();
        switch (mutation) {
        case 0:
            cell.definition.definition_charm += 1;
            break;
        case 1:
            cell.definition.legacy_category = 2;
            bad.category_counts[4] -= 1;
            bad.category_counts[2] += 1;
            break;
        case 2:
            cell.instance->legacy_phase = 2;
            bad.category_counts[4] -= 1;
            break;
        case 3:
            std::swap(bad.cells.front(), bad.cells.back());
            break;
        case 4:
            bad.cells.front().cost.reset();
            break;
        case 5:
            cell.position = bad.cells.front().position;
            break;
        case 6:
            cell.instance->definition_id = 20;
            cell.definition.definition_id = 20;
            break;
        case 7:
            cell.position = bad.cells.front().position;
            cell.cost = bad.cells.front().cost;
            cell.instance.reset();
            break;
        }
        const auto result = select_counted_category_four(bad, 0);
        check(
            result.error == ActivityCandidateError::invalid_snapshot && !result.target,
            "shared fields, duplicate identity, finite ordering and instance consistency checked");
    }
}

void random_collection_oracle() {
    std::mt19937 random(0xCAAD1DA7U);
    for (int trial = 0; trial < 200; ++trial) {
        auto map = terrain(6, 6);
        std::uint64_t next_id = 1;
        for (std::size_t index = 0; index < map.cells.size(); ++index) {
            if (random() % 4U == 0) {
                const int states[] = {1, 6, 7, 8, 9, 10, 2};
                const auto state = states[random() % 7U];
                const auto category = state == 2                 ? RouteCategory::blocked
                                      : state == 6 || state == 7 ? RouteCategory::access
                                                                 : RouteCategory::terminal;
                map.cells[index] = {
                    state, category,
                    FacilityTileBinding{{next_id}, static_cast<std::int32_t>(next_id), 0}};
                ++next_id;
            }
        }
        const Position start{static_cast<int>(random() % 6U), static_cast<int>(random() % 6U)};
        const auto field = search(map, start);
        auto input = metadata(field);
        input.town = {1, 4, 1, 4};
        input.legacy_activity = static_cast<int>(random() % 13U) - 2;
        if (next_id > 1) {
            input.last_visited_instance = BuildingId{random() % (next_id - 1) + 1};
        }
        for (auto &definition : input.definitions) {
            definition.legacy_category = static_cast<int>(random() % 11U);
            definition.definition_charm = random() % 10U;
        }
        for (auto &instance : input.instances) {
            instance.legacy_phase = static_cast<int>(random() % 4U);
        }
        for (int event = 0; event < 20; ++event) {
            input.events.push_back(
                {{static_cast<int>(random() % 6U), static_cast<int>(random() % 6U)},
                 static_cast<int>(random() % 3U)});
        }
        struct Entry {
            std::size_t grid;
            CandidateOrigin origin;
            std::size_t source;
        };
        std::vector<Entry> expected;
        const auto phase_of = [&](std::size_t grid) {
            if (!map.cells[grid].facility) {
                return -1;
            }
            const auto id = map.cells[grid].facility->instance_id;
            return std::find_if(input.instances.begin(), input.instances.end(),
                                [&](const auto &instance) { return instance.instance_id == id; })
                ->legacy_phase;
        };
        const auto category_of = [&](std::size_t grid) {
            const auto definition_id = input.cell_definition_ids[grid];
            return std::find_if(input.definitions.begin(), input.definitions.end(),
                                [&](const auto &definition) {
                                    return definition.definition_id == definition_id;
                                })
                ->legacy_category;
        };
        const bool special_outside = input.legacy_activity == 6 || input.legacy_activity == 8;
        const bool special_inside = input.legacy_activity == 7;
        for (int y = 0; y < 6; ++y) {
            for (int x = 0; x < 6; ++x) {
                const auto grid = static_cast<std::size_t>(y * 6 + x);
                if (!field.distances[grid]) {
                    continue;
                }
                const bool in = x > 1 && x < 4 && y > 1 && y < 4;
                const auto state = map.cells[grid].legacy_state;
                bool include;
                if (special_outside) {
                    include = y > 1 && (state == 4 || state == 9) && !in;
                } else if (special_inside) {
                    include = in;
                } else {
                    include = !(Position{x, y} == start) &&
                              (state == 1 || state == 6 || state == 7 || state == 8 || state == 9 ||
                               state == 10) &&
                              phase_of(grid) != 0;
                    if (include && map.cells[index_of(map, start)].facility &&
                        input.last_visited_instance) {
                        include = !(map.cells[grid].facility->instance_id ==
                                    *input.last_visited_instance);
                    }
                }
                if (include) {
                    expected.push_back({grid, CandidateOrigin::map_scan, grid});
                }
            }
        }
        if (!special_inside && !special_outside) {
            for (std::size_t index = 0; index < input.events.size(); ++index) {
                if (input.events[index].legacy_type == 1) {
                    expected.push_back({index_of(map, input.events[index].position),
                                        CandidateOrigin::event, index});
                }
            }
        }
        const auto start_item =
            std::find_if(expected.begin(), expected.end(),
                         [&](const auto &entry) { return entry.grid == index_of(map, start); });
        if (start_item != expected.end()) {
            expected.erase(start_item);
        }
        for (std::size_t first = 0; first + 1 < expected.size(); ++first) {
            for (auto back = expected.size(); back-- > first + 1;) {
                const auto &a = field.distances[expected[first].grid];
                const auto &b = field.distances[expected[back].grid];
                if (b && (!a || *b < *a)) {
                    std::swap(expected[first], expected[back]);
                }
            }
        }
        const auto actual = collect(field, input);
        check(actual.cells.size() == expected.size(),
              "random candidate cardinality matches scalar oracle");
        std::array<std::int64_t, 11> counts{};
        std::vector<std::size_t> all_four;
        for (std::size_t index = 0; index < expected.size(); ++index) {
            const auto &entry = expected[index];
            const auto &cell = actual.cells[index];
            const auto grid = entry.grid;
            check(index_of(map, cell.position) == grid && cell.origin == entry.origin &&
                      cell.source_index == entry.source && cell.cost == field.distances[grid] &&
                      cell.definition.definition_id == input.cell_definition_ids[grid],
                  "random order, event multiplicity and source match index oracle");
            const auto category = category_of(grid);
            if (phase_of(grid) == -1 || phase_of(grid) == 1) {
                ++counts[static_cast<std::size_t>(category)];
            }
            if (category == 4) {
                all_four.push_back(index);
            }
        }
        check(actual.category_counts == counts, "random active or absent-instance count oracle");
        for (std::int64_t ticket = 0; ticket < counts[4]; ++ticket) {
            const auto target = select_counted_category_four(actual, ticket);
            check(target.error == ActivityCandidateError::none && target.target &&
                      target.target->index == all_four[static_cast<std::size_t>(ticket)] &&
                      same_cell(target.target->cell, actual.cells[target.target->index]),
                  "random category-four active count against all-definition scan oracle");
        }
        const auto end = select_counted_category_four(actual, counts[4]);
        check(end.error == (counts[4] == 0 ? ActivityCandidateError::no_candidate
                                           : ActivityCandidateError::invalid_ticket) &&
                  !end.target,
              "random counted ticket exclusive upper bound");
    }
}

} // namespace

int main() {
    ordinary_truth_table();
    town_and_special_activities();
    events_counts_and_tickets();
    exchange_sort_tie();
    refusals();
    map_plan_and_ranked_composition();
    snapshot_consistency();
    random_collection_oracle();
    std::cout << checks << " checks passed\n";
}
