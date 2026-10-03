// Exhaustive source-to-loaded grid invariants plus all three first-play shop branches.
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_reference/character_motion.hpp"
#include "dungeon_village_reference/facility_departure.hpp"

#include <iostream>
#include <set>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
const LoadedStartupCell &at(const LoadedStartupMap &m, int x, int y) {
    return m.cells.at(static_cast<std::size_t>(y * m.width + x));
}
void map_state() {
    StartupSession session;
    const auto &map = session.state().loaded_map;
    check(map.cells.size() == 576 && map.instances.size() == 8, "loaded dimensions");
    const std::array<int, 8> ids{{2, 3, 4, 5, 6, 7, 0, 1}};
    const std::array<int, 8> definitions{{33, 28, 30, 66, 76, 75, 83, 84}};
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const auto &instance = map.instances[i];
        check(instance.legacy_id == ids[i] && instance.definition_id == definitions[i],
              "post-rebuild vector order and recycled IDs");
        const auto &cell = at(map, instance.anchor.x, instance.anchor.y);
        check(cell.legacy_instance_id == ids[i] && cell.definition_id == definitions[i],
              "complete identity binding");
        const auto stable = static_cast<std::uint64_t>(ids[i]) + 1;
        check(session.state().facilities.at(stable).legacy_id == ids[i], "zero-safe mapping");
    }
    check(session.facility_at({11, 2}) == 8 && session.facility_at({12, 2}) == 7,
          "town entrance IDs swapped by reconstruction");
    check(session.facility_at({11, 10}) == 1 && session.facility_at({12, 10}) == 2,
          "external raw zero is a real instance");
    check(at(map, 11, 10).display_id == 27 && at(map, 11, 10).legacy_state == 7 &&
              at(map, 11, 10).category == ref::RouteCategory::access &&
              at(map, 11, 10).external_direction == 2 && at(map, 12, 10).external_direction == 3 &&
              at(map, 11, 10).boundary_fragment == -1,
          "external entry logical definition differs from grass display");
    check(at(map, 11, 2).external_direction == -1 && at(map, 11, 2).display_id == 0,
          "town entrance does not use external direction overlay");
    check(at(map, 9, 8).definition_id == 17 && at(map, 9, 8).legacy_state == 4,
          "special interior ground cleared");
    check(at(map, 6, 2).boundary_fragment == 3 && at(map, 17, 2).boundary_fragment == 5 &&
              at(map, 6, 10).boundary_fragment == 4 && at(map, 17, 10).boundary_fragment == 2,
          "four fence corners");
    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 24; ++x) {
            const auto &cell = at(map, x, y);
            check(cell.legacy_state >= 1 && cell.legacy_state <= 12 &&
                      static_cast<int>(cell.category) >= 0 && static_cast<int>(cell.category) <= 4,
                  "every tile has an explained logical state");
            check(session.display(cell.display_id).id == cell.display_id,
                  "every tile has a published display");
            if (cell.legacy_instance_id)
                check(session.facility_at({x, y}) ==
                          static_cast<std::uint64_t>(*cell.legacy_instance_id) + 1,
                      "every occupancy matches prototype owner");
            if (cell.boundary_fragment != -1)
                check(cell.legacy_state == 5 && cell.category == ref::RouteCategory::blocked,
                      "fence state controls routing, not only drawing");
        }
    }
    check(at(map, 11, 0).road_mask == 7 && at(map, 11, 0).variant == 7 &&
              at(map, 11, 0).edge_road_pair,
          "off-map road neighbor counts as connected");
    // Source (12,3) and (11,4) are roads; (11,2) is an entrance, not another road.
    check(at(map, 11, 3).road_mask == 3 && at(map, 11, 3).variant == 3,
          "road below town entrance does not connect to entrance definition");
    check(session.state().next_id == 9 && session.state().accounting.entries().empty(),
          "loaded instances allocate no construction fees");
}
void departures() {
    StartupSession session;
    const auto &loaded = session.state().loaded_map;
    const auto route_map = startup_route_map(loaded);
    for (const auto birth : startup_evidence().spawn_points) {
        const auto search = ref::search_legacy_map(route_map, birth);
        check(search.error == ref::MapAccessError::none && search.field.has_value(),
              "both actual birth points produce a distance field");
        ref::ActivityCandidateInput input;
        input.legacy_activity = 0;
        input.town = {6, 17, 2, 10};
        for (const auto &cell : loaded.cells)
            input.cell_definition_ids.push_back(cell.definition_id);
        for (const auto &d : startup_evidence().definitions)
            input.definitions.push_back({d.id, d.category, d.definition_charm});
        for (const auto &i : loaded.instances)
            input.instances.push_back(
                {ref::BuildingId{static_cast<std::uint64_t>(i.legacy_id) + 1}, i.definition_id, 1});
        const auto candidates = ref::collect_activity_candidates(*search.field, input);
        check(candidates.error == ref::ActivityCandidateError::none && candidates.snapshot,
              "loaded grid supplies real candidate definitions and instances");
        const auto &snapshot = *candidates.snapshot;
        check(snapshot.category_counts[1] == 2 && snapshot.category_counts[2] == 1,
              "two shops and one inn accessible from each actual birth point");
        std::set<int> selected;
        // Inject every valid category/facility ticket, not a claim about original RNG probability.
        for (int category_ticket = 0; category_ticket < 70; ++category_ticket) {
            const int weight = category_ticket < 40 ? 9 : 5;
            for (int facility_ticket = 0; facility_ticket < weight; ++facility_ticket) {
                const auto result = ref::prepare_facility_departure(
                    *search.field, snapshot, {0, {}, 2U | 8192U, category_ticket, facility_ticket});
                check(result.error == ref::FacilityDepartureError::none && result.departure,
                      "every ordinary first-play facility ticket prepares a bound route");
                const auto &d = *result.departure;
                // Equal-cost paths prefer the lower grid index in this maintained solver.
                // Crossing between the two birth roads can therefore be the first step; this
                // is not evidence of the APK's route tie-breaking or final tutorial facing.
                const int expected_direction = d.binding.definition_id == 30
                                                   ? (birth.x == 11 ? 1 : 0)
                                                   : (birth.x == 12 ? 3 : 0);
                check(d.legacy_direction && *d.legacy_direction == expected_direction &&
                          !d.route.steps.empty(),
                      "first direction follows the maintained equal-cost route ordering");
                check(ref::arrival_binding_matches(route_map, d.binding, d.binding.goal),
                      "arrival identity survives raw-zero mapping");
                selected.insert(d.binding.definition_id);
                // Replay representative branches using the actual loaded entrance definitions.
                // Stop at logical entry; no charge, occupation, weapon draw or exit is invented.
                if ((category_ticket == 0 || category_ticket == 40) &&
                    (facility_ticket == 0 || facility_ticket == 5)) {
                    ref::WorldPosition position{birth.x * 100.0F + 50.0F, birth.y * 100.0F + 50.0F};
                    std::size_t next_waypoint = 0;
                    bool entered = false;
                    for (int tick = 0; tick < 1000; ++tick) {
                        const auto status =
                            ref::inspect_facility_entry(route_map, d.binding, position, true);
                        if (status == ref::FacilityEntryStatus::ready) {
                            entered = true;
                            break;
                        }
                        check(status == ref::FacilityEntryStatus::not_entered,
                              "initial-route identity remains valid until logical entry");
                        const auto cell = d.route.steps.at(next_waypoint);
                        const auto &loaded_cell = at(loaded, cell.x, cell.y);
                        const auto target = ref::character_waypoint(
                            cell, loaded_cell.legacy_state,
                            session.definition(loaded_cell.definition_id).direction);
                        check(target.error == ref::CharacterMotionError::none && target.target,
                              "waypoint uses entrance definition even when cell.m is cleared");
                        const auto step =
                            ref::advance_character_motion(position, *target.target, 2U | 8192U);
                        check(step.error == ref::CharacterMotionError::none && step.step,
                              "continuous first-play motion accepts real world coordinates");
                        position = step.step->position;
                        if (step.step->waypoint_overlap && next_waypoint + 1 < d.route.steps.size())
                            ++next_waypoint;
                    }
                    check(entered, "each ordinary first-play route reaches its bound facility");
                    check(ref::inspect_facility_entry(route_map, d.binding, position, false) ==
                              ref::FacilityEntryStatus::inactive_route,
                          "caller-cleared route cannot consume entry twice");
                }
            }
        }
        check(selected == std::set<int>{28, 30, 33}, "do not omit the unsupported weapon arrival");
    }
}
void rejection() {
    const auto reject = [](auto edit) {
        auto data = startup_evidence();
        edit(data);
        try {
            reconstruct_startup_map(data);
        } catch (const std::invalid_argument &) {
            return true;
        }
        return false;
    };
    check(reject([](auto &d) { d.cells.pop_back(); }), "reject truncated map");
    check(reject([](auto &d) { d.seeds[0].cell = {-1, 0}; }), "reject outside seed");
    check(reject([](auto &d) { d.seeds[1] = d.seeds[0]; }), "reject duplicate seed");
    check(reject([](auto &d) { d.build_bounds[0] = 6; }), "reject changed town scope");
    check(reject([](auto &d) { d.definitions.clear(); }), "reject missing definitions");
}
void cleared_ground_construction() {
    StartupSession session;
    session.open_catalog();
    session.select(28);
    check(session.preview({9, 8}) == StartupError::none,
          "initial special source tile was cleared by loading, so construction is allowed");
    check(session.confirm({9, 8}) == StartupError::none &&
              session.state().accounting.funds() == 4000 && session.facility_at({9, 8}),
          "construction consumes loaded logical map rather than immutable source display");
}
} // namespace
int main() {
    try {
        map_state();
        departures();
        rejection();
        cleared_ground_construction();
        std::cout << "loaded map checks=" << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
