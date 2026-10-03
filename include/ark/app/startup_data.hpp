#pragma once

// Build-time validated research data. Source map seeds are not a certified post-init snapshot.
#include "ark/facilities/facility.hpp"
#include "ark/people/adventurer.hpp"
#include <array>

namespace ark::app {
struct Display {
    int id{}, definition_id{};
    std::string sprite;
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
};
const StartupData &startup_data();
} // namespace ark::app
