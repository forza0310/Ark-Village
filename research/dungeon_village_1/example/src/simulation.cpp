// Early R2 single-point autonomous fixture retained for tests; the current window uses
// prototype::Village. Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_reference/simulation.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {

constexpr std::array<Position, 4> kApproaches = {Position{0, 1}, Position{1, 0}, Position{0, -1},
                                                 Position{-1, 0}};

std::size_t index_of(const RouteGrid &grid, Position position) {
    return static_cast<std::size_t>(position.y) * static_cast<std::size_t>(grid.width) +
           static_cast<std::size_t>(position.x);
}

const BuildingDefinition *definition_for(const std::vector<BuildingDefinition> &catalog,
                                         const std::string &key) {
    const auto found = std::find_if(catalog.begin(), catalog.end(), [&key](const auto &definition) {
        return definition.key == key;
    });
    return found == catalog.end() ? nullptr : &*found;
}

SimulationError validate(const GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                         const SimulationState &simulation, int elapsed_ms) {
    const auto &config = simulation.config;
    if (elapsed_ms < 0 || elapsed_ms > 60000 || !valid_route_grid(simulation.terrain) ||
        config.tick_ms < 1 || config.tick_ms > 60000 || config.ticks_per_step < 1 ||
        config.ticks_per_step > 1000000 || config.ticks_per_use < 1 ||
        config.ticks_per_use > 1000000 || config.retry_ticks < 1 || config.retry_ticks > 1000000 ||
        simulation.remainder_ms < 0 || simulation.remainder_ms >= config.tick_ms) {
        return SimulationError::invalid_input;
    }
    std::set<std::string> keys;
    for (const auto &definition : catalog) {
        if (definition.key.empty() || !keys.insert(definition.key).second ||
            definition.build_cost < 0 || definition.monthly_maintenance < 0 ||
            definition.use_effect < 0) {
            return SimulationError::invalid_input;
        }
    }
    std::set<std::pair<int, int>> occupied;
    for (const auto &[id, building] : state.buildings) {
        if (!(id == building.id) || id.value == 0 ||
            !within_route_grid(simulation.terrain, building.position) || building.rotation < 0 ||
            building.rotation > 3 || definition_for(catalog, building.definition_key) == nullptr ||
            !occupied.emplace(building.position.x, building.position.y).second) {
            return SimulationError::inconsistent_state;
        }
    }
    if (simulation.motions.size() != state.characters.size()) {
        return SimulationError::inconsistent_state;
    }
    for (const auto &[id, character] : state.characters) {
        const auto found = simulation.motions.find(id);
        if (!(id == character.id) || id.value == 0 || character.accumulated_effect < 0 ||
            found == simulation.motions.end()) {
            return SimulationError::inconsistent_state;
        }
        const auto &motion = found->second;
        if (!within_route_grid(simulation.terrain, motion.cell) ||
            motion.cursor > motion.path.size() || motion.step_ticks < 0 ||
            motion.step_ticks >= config.ticks_per_step || motion.use_ticks < 0 ||
            motion.use_ticks >= config.ticks_per_use || motion.wait_ticks < 0 ||
            motion.wait_ticks > config.retry_ticks) {
            return SimulationError::inconsistent_state;
        }
        if (character.activity != ActivityState::idle &&
            character.activity != ActivityState::travelling &&
            character.activity != ActivityState::in_use) {
            return SimulationError::inconsistent_state;
        }
        for (const auto step : motion.path) {
            if (!within_route_grid(simulation.terrain, step)) {
                return SimulationError::inconsistent_state;
            }
        }
    }
    std::set<CharacterId> owners;
    for (const auto &[building_id, owner] : state.reservations) {
        const auto character = state.characters.find(owner);
        if (state.buildings.find(building_id) == state.buildings.end() ||
            character == state.characters.end() || !owners.insert(owner).second ||
            character->second.activity == ActivityState::idle ||
            !character->second.target.has_value() || !(*character->second.target == building_id)) {
            return SimulationError::inconsistent_state;
        }
    }
    return SimulationError::none;
}

RouteGrid world_grid(const GlobalState &state, const SimulationState &simulation) {
    auto result = simulation.terrain;
    for (const auto &[id, building] : state.buildings) {
        (void)id;
        result.cells[index_of(result, building.position)] = RouteCategory::terminal;
    }
    return result;
}

void reset_motion(CharacterMotion &motion, int retry_ticks) {
    motion.path.clear();
    motion.cursor = 0;
    motion.step_ticks = 0;
    motion.use_ticks = 0;
    motion.wait_ticks = retry_ticks;
    motion.activity.reset();
    motion.target.reset();
}

bool live_activity(const GlobalState &state, const CharacterState &character,
                   const CharacterMotion &motion, const RouteGrid &grid) {
    if (!motion.target.has_value() || !motion.activity.has_value() ||
        !character.target.has_value() || !character.active_activity.has_value() ||
        !(motion.target == character.target) || motion.activity != character.active_activity) {
        return false;
    }
    const auto building = state.buildings.find(*motion.target);
    const auto reservation = state.reservations.find(*motion.target);
    if (building == state.buildings.end() || reservation == state.reservations.end() ||
        !(reservation->second == character.id) ||
        !(building->second.position == motion.target_position) ||
        building->second.rotation != motion.target_rotation) {
        return false;
    }
    auto previous = motion.cell;
    for (auto cursor = motion.cursor; cursor < motion.path.size(); ++cursor) {
        const auto next = motion.path[cursor];
        const auto category = grid.cells[index_of(grid, next)];
        if (category == RouteCategory::terminal || category == RouteCategory::blocked ||
            std::abs(next.x - previous.x) + std::abs(next.y - previous.y) != 1) {
            return false;
        }
        previous = next;
    }
    return std::abs(previous.x - motion.target_position.x) +
               std::abs(previous.y - motion.target_position.y) ==
           1;
}

std::optional<RouteResult> approach_route(const RouteGrid &grid, Position start,
                                          Position building) {
    std::optional<RouteResult> best;
    for (const auto offset : kApproaches) {
        const Position goal{building.x + offset.x, building.y + offset.y};
        if (!within_route_grid(grid, goal)) {
            continue;
        }
        const auto category = grid.cells[index_of(grid, goal)];
        if (category == RouteCategory::terminal || category == RouteCategory::blocked) {
            continue;
        }
        auto route = find_route(grid, start, goal);
        if (route.error == RouteError::none && (!best.has_value() || route.cost < best->cost)) {
            best = std::move(route);
        }
    }
    return best;
}

SimulationError begin_next(GlobalState &state, CharacterState &character, CharacterMotion &motion,
                           const SimulationConfig &config, const RouteGrid &grid,
                           SimulationResult &report) {
    ++report.decisions;
    std::vector<BuildingId> candidates;
    for (const auto &[id, building] : state.buildings) {
        (void)building;
        if (state.reservations.find(id) == state.reservations.end()) {
            candidates.push_back(id);
        }
    }
    // Deterministic round-robin is a fixture policy, not the original weighted selector.
    if (motion.last_visited.has_value()) {
        const auto next =
            std::upper_bound(candidates.begin(), candidates.end(), *motion.last_visited);
        std::rotate(candidates.begin(), next, candidates.end());
    }
    for (const auto id : candidates) {
        const auto &building = state.buildings.at(id);
        auto route = approach_route(grid, motion.cell, building.position);
        if (!route.has_value()) {
            continue;
        }
        if (state.next_activity_id == 0 ||
            state.next_activity_id == std::numeric_limits<std::uint64_t>::max()) {
            return SimulationError::numeric_overflow;
        }
        // Domain transactions replace GlobalState; do not retain references across them.
        const auto target_position = building.position;
        const auto target_rotation = building.rotation;
        const auto character_id = character.id;
        const auto result = start_visit(state, character_id, id, true);
        if (result.error != Error::none || !result.activity_id.has_value()) {
            return SimulationError::rule_failure;
        }
        motion.path = std::move(route->steps);
        motion.cursor = 0;
        motion.step_ticks = 0;
        motion.use_ticks = 0;
        motion.target = id;
        motion.target_position = target_position;
        motion.target_rotation = target_rotation;
        motion.activity = result.activity_id;
        ++report.started;
        return SimulationError::none;
    }
    motion.wait_ticks = config.retry_ticks;
    return SimulationError::none;
}

SimulationError tick(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                     SimulationState &simulation, SimulationResult &report) {
    const auto grid = world_grid(state, simulation);
    for (auto &[id, motion] : simulation.motions) {
        auto &character = state.characters.at(id);
        if (character.activity == ActivityState::idle) {
            if (motion.activity.has_value()) {
                reset_motion(motion, simulation.config.retry_ticks);
                ++report.cancelled;
                continue;
            }
            if (character.target.has_value() || character.active_activity.has_value()) {
                return SimulationError::inconsistent_state;
            }
            if (motion.wait_ticks > 0) {
                --motion.wait_ticks;
                continue;
            }
            const auto result =
                begin_next(state, character, motion, simulation.config, grid, report);
            if (result != SimulationError::none) {
                return result;
            }
            continue;
        }
        if (!live_activity(state, character, motion, grid)) {
            if (cancel_visit(state, id) != Error::none) {
                return SimulationError::rule_failure;
            }
            reset_motion(motion, simulation.config.retry_ticks);
            ++report.cancelled;
            continue;
        }
        if (character.activity == ActivityState::travelling) {
            if (motion.cursor < motion.path.size()) {
                ++motion.step_ticks;
                if (motion.step_ticks < simulation.config.ticks_per_step) {
                    continue;
                }
                motion.step_ticks = 0;
                motion.cell = motion.path[motion.cursor++];
            }
            if (motion.cursor == motion.path.size()) {
                if (arrive(state, id) != Error::none) {
                    return SimulationError::rule_failure;
                }
                ++report.arrived;
            }
        } else {
            ++motion.use_ticks;
            if (motion.use_ticks < simulation.config.ticks_per_use) {
                continue;
            }
            const auto &building = state.buildings.at(*motion.target);
            const auto *definition = definition_for(catalog, building.definition_key);
            if (definition->use_effect >
                    std::numeric_limits<int>::max() - character.accumulated_effect ||
                building.completed_uses == std::numeric_limits<std::uint64_t>::max()) {
                return SimulationError::numeric_overflow;
            }
            if (complete_use(state, catalog, id, *motion.activity) != Error::none) {
                return SimulationError::rule_failure;
            }
            motion.last_visited = motion.target;
            reset_motion(motion, simulation.config.retry_ticks);
            ++report.completed;
        }
    }
    return SimulationError::none;
}

} // namespace

SimulationResult advance_simulation(GlobalState &state,
                                    const std::vector<BuildingDefinition> &catalog,
                                    SimulationState &simulation, int elapsed_ms) {
    const auto error = validate(state, catalog, simulation, elapsed_ms);
    if (error != SimulationError::none) {
        return {error, 0, 0, 0, 0, 0};
    }
    if (simulation.paused || elapsed_ms == 0) {
        return {};
    }
    auto candidate_state = state;
    auto candidate_simulation = simulation;
    SimulationResult report;
    const int total_ms = candidate_simulation.remainder_ms + elapsed_ms;
    const int tick_count = total_ms / candidate_simulation.config.tick_ms;
    if (simulation.ticks >
        std::numeric_limits<std::uint64_t>::max() - static_cast<std::uint64_t>(tick_count)) {
        return {SimulationError::numeric_overflow, 0, 0, 0, 0, 0};
    }
    candidate_simulation.remainder_ms = total_ms % candidate_simulation.config.tick_ms;
    for (int index = 0; index < tick_count; ++index) {
        report.error = tick(candidate_state, catalog, candidate_simulation, report);
        if (report.error != SimulationError::none) {
            return {report.error, 0, 0, 0, 0, 0};
        }
        ++candidate_simulation.ticks;
    }
    state = std::move(candidate_state);
    simulation = std::move(candidate_simulation);
    return report;
}

} // namespace dungeon_village_reference
