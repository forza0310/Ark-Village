#pragma once

// Build-time validated loaded-reset reconstruction, not a certified APK runtime capture.
#include "ark/facilities/facility.hpp"
#include "ark/people/adventurer.hpp"
#include "ark/world/loaded_map.hpp"
#include <array>

namespace ark::app {
struct Display {
    int id{}, definition_id{};
    std::string sprite;
    int depth_offset{}, flags{}; // mapchip columns4/6: source scene ordering, not facility flags.
};
struct StartupData {
    world::SourceMap map;
    std::vector<Display> displays;
    std::vector<facilities::Definition> definitions;
    std::vector<facilities::Instance> seeds;
    std::vector<world::Cell> spawn_points;
    world::Bounds build_bounds;
    std::int64_t money{};
    int points{}, popularity{};
    std::array<int, 4> calendar{};
    std::vector<int> unlocked_people;
    int arrival_counter{};
    world::Cell camera;
    people::Adventurer first_character;
    std::vector<std::string> first_talk;
    std::array<std::int32_t, 10> initial_job_counts{};
    std::vector<world::LoadedCell> loaded_cells; // Row-major, original TSV fields preserved.
    int boundary_index{}; // Source reset region/skin index; expansion is not implemented.
    // Proven only for the first arrival after a no-inheritance reset. Later updates read the
    // current shared definition; these are not fallback values for an unknown home/retirement.
    std::array<std::int32_t, 4> first_character_home{}; // Legacy shared definition D.
    std::int32_t first_character_definition_state{};    // Legacy shared definition m.
    int character_spawn_minimum_y{}; // Region0 h.l[0][1][1], distinct from the other endpoint10.
    int world_overlap_boundary_y{};
};
const StartupData &startup_data();
} // namespace ark::app
