#pragma once

// Early R2 single-point autonomous fixture retained for tests; the current window uses
// prototype::Village.

#include "dungeon_village_reference/navigation.hpp"

#include <map>
#include <optional>

namespace dungeon_village_reference {

struct SimulationConfig {
    int tick_ms{100};
    int ticks_per_step{3};
    int ticks_per_use{12};
    int retry_ticks{5};
};

struct CharacterMotion {
    Position cell;
    std::vector<Position> path;
    std::size_t cursor{};
    int step_ticks{};
    int use_ticks{};
    int wait_ticks{};
    std::optional<ActivityId> activity;
    std::optional<BuildingId> target;
    Position target_position;
    int target_rotation{};
    std::optional<BuildingId> last_visited;
};

struct SimulationState {
    RouteGrid terrain;
    SimulationConfig config;
    std::map<CharacterId, CharacterMotion> motions;
    std::uint64_t ticks{};
    int remainder_ms{};
    bool paused{};
};

enum class SimulationError {
    none,
    invalid_input,
    inconsistent_state,
    rule_failure,
    numeric_overflow
};

struct SimulationResult {
    SimulationError error{SimulationError::none};
    std::uint64_t decisions{};
    std::uint64_t started{};
    std::uint64_t arrived{};
    std::uint64_t completed{};
    std::uint64_t cancelled{};
};

// Single-point buildings and four exterior approach cells are fixture geometry.
SimulationResult advance_simulation(GlobalState &state,
                                    const std::vector<BuildingDefinition> &catalog,
                                    SimulationState &simulation, int elapsed_ms);

} // namespace dungeon_village_reference
