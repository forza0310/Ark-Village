// Numerical boundaries and entry ordering remain active in Release; no assert/NDEBUG dependency.
#include "dungeon_village_reference/character_motion.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace ref = dungeon_village_reference;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
bool near(float a, float b) { return std::abs(a - b) < 0.0001F; }
void waypoints() {
    for (const int state : {6, 7}) {
        const std::array<ref::WorldPosition, 4> expected = {
            {{150, 290}, {150, 210}, {190, 250}, {110, 250}}};
        for (int direction = 0; direction < 4; ++direction) {
            const auto result = ref::character_waypoint({1, 2}, state, direction);
            check(result.error == ref::CharacterMotionError::none && result.target &&
                      result.target->x == expected[direction].x &&
                      result.target->z == expected[direction].z,
                  "entrance offset uses definition direction");
        }
        check(ref::character_waypoint({1, 2}, state, -1).error ==
                  ref::CharacterMotionError::invalid_input,
              "reject unresolved entrance definition direction");
        check(ref::character_waypoint({1, 2}, state, 4).error ==
                  ref::CharacterMotionError::invalid_input,
              "reject bad entrance direction");
    }
    const auto ordinary = ref::character_waypoint({1, 2}, 3, -1);
    check(ordinary.target->x == 150 && ordinary.target->z == 250,
          "ordinary tile centre ignores entrance direction");
    check(ref::character_waypoint({10000, 0}, 4, 0).error ==
              ref::CharacterMotionError::invalid_input,
          "waypoint implementation bound");
    check(ref::character_waypoint({0, 0}, 13, 0).error == ref::CharacterMotionError::invalid_input,
          "reject unknown cell state");
}
void movement() {
    for (const auto direction : std::array<ref::Position, 4>{{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}}) {
        const auto result = ref::advance_character_motion(
            {150, 150}, {150 + 100.0F * direction.x, 150 + 100.0F * direction.y}, 2U);
        check(result.step && near(result.step->position.x, 150 + 6.7F * direction.x) &&
                  near(result.step->position.z, 150 + 6.7F * direction.y) &&
                  !result.step->waypoint_overlap,
              "four directions advance world units, not cells or milliseconds");
    }
    const auto diagonal = ref::advance_character_motion({0, 0}, {30, 40}, 0);
    check(diagonal.step && near(diagonal.step->position.x, 4.02F) &&
              near(diagonal.step->position.z, 5.36F),
          "normalized diagonal");
    const auto overshoot = ref::advance_character_motion({0, 0}, {3, 0}, 0);
    check(overshoot.step && near(overshoot.step->position.x, 6.7F) &&
              overshoot.step->waypoint_overlap,
          "overshoot is not clamped to centre");
    const auto tiny = ref::advance_character_motion({0, 0}, {1, 0}, 0);
    check(tiny.step && !tiny.step->waypoint_overlap,
          "initial overlap does not skip advance or guarantee post-step overlap");
    const auto same = ref::advance_character_motion({10, 20}, {10, 20}, 0);
    check(same.step && same.step->position.x == 10 && same.step->position.z == 20 &&
              same.step->waypoint_overlap,
          "zero distance avoids division");
    for (const float delta : {3.999F, 4.0F, 4.001F}) {
        const auto result =
            ref::advance_character_motion({100 + delta, 100 + delta}, {100, 100}, 64U | 2U);
        check(result.step && result.step->position.x == 100 + delta &&
                  result.step->waypoint_overlap == (delta <= 4.0F),
              "bit64 stops displacement but preserves inclusive rectangle test");
    }
    check(ref::character_world_cell({-99, -101}) == ref::Position{0, -1},
          "negative logical coordinates truncate, not floor");
    const auto cell = ref::character_world_cell({199.99F, 200});
    check(cell && *cell == ref::Position{1, 2}, "logical cell boundaries");
    for (const float value : {std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN(), 1000001.0F}) {
        check(!ref::character_world_cell({value, 0}), "reject unsafe world conversion");
        check(ref::advance_character_motion({value, 0}, {0, 0}, 0).error ==
                  ref::CharacterMotionError::invalid_input,
              "reject current numeric input");
        check(ref::advance_character_motion({0, 0}, {0, value}, 0).error ==
                  ref::CharacterMotionError::invalid_input,
              "reject target numeric input");
    }
}
void entry() {
    ref::LegacyMap map{
        2,
        1,
        {{4, ref::RouteCategory::ground, {}},
         {1, ref::RouteCategory::terminal, ref::FacilityTileBinding{ref::BuildingId{1}, 28, 0}}}};
    const ref::ArrivalBinding target{{1, 0}, ref::BuildingId{1}, 28};
    check(ref::inspect_facility_entry(map, target, {99.99F, 50}, true) ==
              ref::FacilityEntryStatus::not_entered,
          "not charged before entering goal cell");
    check(ref::inspect_facility_entry(map, target, {100, 50}, true) ==
              ref::FacilityEntryStatus::ready,
          "entry is ready fifty units before centre");
    check(!ref::advance_character_motion({100, 50}, {150, 50}, 64).step->waypoint_overlap,
          "tile entry and waypoint rectangle are not equivalent");
    check(ref::inspect_facility_entry(map, target, {100, 50}, false) ==
              ref::FacilityEntryStatus::inactive_route,
          "cleared route suppresses repeat entry");
    map.cells[1].facility->instance_id = ref::BuildingId{2};
    check(ref::inspect_facility_entry(map, target, {100, 50}, true) ==
              ref::FacilityEntryStatus::stale_binding,
          "reused definition is not target instance");
    map.cells[1].facility->instance_id = ref::BuildingId{1};
    map.cells[1].facility->definition_id = 33;
    check(ref::inspect_facility_entry(map, target, {100, 50}, true) ==
              ref::FacilityEntryStatus::stale_binding,
          "changed definition rejected");
    map.cells[1].facility.reset();
    check(ref::inspect_facility_entry(map, target, {100, 50}, true) ==
              ref::FacilityEntryStatus::stale_binding,
          "demolished binding rejected");
    check(ref::inspect_facility_entry(map, {{2, 0}, ref::BuildingId{1}, 28}, {100, 50}, true) ==
              ref::FacilityEntryStatus::invalid_input,
          "outside target is not ordinary no-goal");
    map.cells.pop_back();
    check(ref::inspect_facility_entry(map, target, {100, 50}, true) ==
              ref::FacilityEntryStatus::invalid_input,
          "invalid snapshot rejected");
}
} // namespace
int main() {
    try {
        waypoints();
        movement();
        entry();
        std::cout << "character motion checks=" << checks << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
