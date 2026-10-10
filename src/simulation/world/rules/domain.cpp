// R1 safety fixture: single-cell buildings, exclusive visits and idempotent completion; not APK
// parity. Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/world/rules/domain.hpp"

#include <algorithm>

namespace ark::simulation::rules {

bool operator==(BuildingId left, BuildingId right) {
    return left.value == right.value;
}
bool operator<(BuildingId left, BuildingId right) {
    return left.value < right.value;
}
bool operator==(CharacterId left, CharacterId right) {
    return left.value == right.value;
}
bool operator<(CharacterId left, CharacterId right) {
    return left.value < right.value;
}
bool operator==(ActivityId left, ActivityId right) {
    return left.value == right.value;
}
bool operator!=(ActivityId left, ActivityId right) {
    return !(left == right);
}
bool operator<(ActivityId left, ActivityId right) {
    return left.value < right.value;
}
bool operator==(Position left, Position right) {
    return left.x == right.x && left.y == right.y;
}

namespace {

const BuildingDefinition *find_definition(const std::vector<BuildingDefinition> &catalog,
                                          const std::string &key) {
    const auto found = std::find_if(catalog.begin(), catalog.end(), [&key](const auto &definition) {
        return definition.key == key;
    });
    return found == catalog.end() ? nullptr : &*found;
}

bool is_occupied(const GlobalState &state, Position position,
                 std::optional<BuildingId> ignored = std::nullopt) {
    return std::any_of(state.buildings.begin(), state.buildings.end(),
                       [position, ignored](const auto &entry) {
                           return (!ignored.has_value() || !(entry.first == *ignored)) &&
                                  entry.second.position == position;
                       });
}

void release_character(GlobalState &state, CharacterState &character) {
    if (character.target.has_value()) {
        const auto reservation = state.reservations.find(*character.target);
        if (reservation != state.reservations.end() && reservation->second == character.id) {
            state.reservations.erase(reservation);
        }
    }
    character.activity = ActivityState::idle;
    character.target.reset();
    character.active_activity.reset();
}

} // namespace

BuildingResult construct(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                         const std::string &definition_key, Position position, int rotation) {
    const auto *definition = find_definition(catalog, definition_key);
    if (definition == nullptr) {
        return {Error::unknown_definition, std::nullopt};
    }
    if (is_occupied(state, position)) {
        return {Error::occupied, std::nullopt};
    }
    if (state.funds < definition->build_cost) {
        return {Error::insufficient_funds, std::nullopt};
    }

    GlobalState candidate = state;
    const BuildingId id{candidate.next_building_id++};
    candidate.funds -= definition->build_cost;
    candidate.buildings.emplace(id, BuildingInstance{id, definition_key, position, rotation, 0});
    state = std::move(candidate);
    return {Error::none, id};
}

Error move(GlobalState &state, BuildingId building_id, Position destination, int rotation) {
    if (state.buildings.find(building_id) == state.buildings.end()) {
        return Error::not_found;
    }
    if (is_occupied(state, destination, building_id)) {
        return Error::occupied;
    }

    GlobalState candidate = state;
    auto &building = candidate.buildings.at(building_id);
    building.position = destination;
    building.rotation = rotation;
    state = std::move(candidate);
    return Error::none;
}

Error remove(GlobalState &state, BuildingId building_id) {
    if (state.buildings.find(building_id) == state.buildings.end()) {
        return Error::not_found;
    }

    GlobalState candidate = state;
    candidate.buildings.erase(building_id);
    candidate.reservations.erase(building_id);
    for (auto &[character_id, character] : candidate.characters) {
        (void)character_id;
        if (character.target.has_value() && *character.target == building_id) {
            release_character(candidate, character);
        }
    }
    state = std::move(candidate);
    return Error::none;
}

ActivityResult start_visit(GlobalState &state, CharacterId character_id, BuildingId building_id,
                           bool path_found) {
    const auto character = state.characters.find(character_id);
    if (character == state.characters.end() ||
        state.buildings.find(building_id) == state.buildings.end()) {
        return {Error::not_found, std::nullopt};
    }
    if (character->second.activity != ActivityState::idle) {
        return {Error::invalid_state, std::nullopt};
    }
    if (state.reservations.find(building_id) != state.reservations.end()) {
        return {Error::reserved, std::nullopt};
    }

    GlobalState candidate = state;
    const ActivityId activity_id{candidate.next_activity_id++};
    auto &candidate_character = candidate.characters.at(character_id);
    candidate.reservations.emplace(building_id, character_id);
    candidate_character.target = building_id;
    candidate_character.active_activity = activity_id;
    if (!path_found) {
        release_character(candidate, candidate_character);
        return {Error::path_unreachable, std::nullopt};
    }
    candidate_character.activity = ActivityState::travelling;
    state = std::move(candidate);
    return {Error::none, activity_id};
}

Error cancel_visit(GlobalState &state, CharacterId character_id) {
    const auto character = state.characters.find(character_id);
    if (character == state.characters.end()) {
        return Error::not_found;
    }
    if (character->second.activity == ActivityState::idle) {
        return Error::none;
    }
    release_character(state, character->second);
    return Error::none;
}

Error arrive(GlobalState &state, CharacterId character_id) {
    const auto character = state.characters.find(character_id);
    if (character == state.characters.end()) {
        return Error::not_found;
    }
    if (character->second.activity != ActivityState::travelling ||
        !character->second.target.has_value()) {
        return Error::invalid_state;
    }
    character->second.activity = ActivityState::in_use;
    return Error::none;
}

Error complete_use(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                   CharacterId character_id, ActivityId activity_id) {
    if (state.completed_activities.find(activity_id) != state.completed_activities.end()) {
        return Error::none;
    }
    const auto character = state.characters.find(character_id);
    if (character == state.characters.end()) {
        return Error::not_found;
    }
    if (character->second.activity != ActivityState::in_use ||
        character->second.active_activity != activity_id || !character->second.target.has_value()) {
        return Error::invalid_state;
    }
    const auto building = state.buildings.find(*character->second.target);
    if (building == state.buildings.end()) {
        return Error::not_found;
    }
    const auto *definition = find_definition(catalog, building->second.definition_key);
    if (definition == nullptr) {
        return Error::unknown_definition;
    }

    GlobalState candidate = state;
    auto &candidate_character = candidate.characters.at(character_id);
    candidate_character.accumulated_effect += definition->use_effect;
    candidate.buildings.at(*candidate_character.target).completed_uses++;
    candidate.completed_activities.insert(activity_id);
    release_character(candidate, candidate_character);
    state = std::move(candidate);
    return Error::none;
}

Error complete_task(GlobalState &state, std::uint64_t task_id) {
    const auto task = state.tasks.find(task_id);
    if (task == state.tasks.end()) {
        return Error::not_found;
    }
    if (task->second.completed) {
        return Error::none;
    }

    GlobalState candidate = state;
    auto &candidate_task = candidate.tasks.at(task_id);
    candidate_task.completed = true;
    candidate.funds += candidate_task.reward;
    state = std::move(candidate);
    return Error::none;
}

Error settle_through(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                     int target_month, const std::map<int, MonthlyInput> &inputs) {
    if (target_month < 0) {
        return Error::invalid_month;
    }
    if (target_month <= state.last_settled_month) {
        return Error::none;
    }

    GlobalState candidate = state;
    std::int64_t maintenance = 0;
    for (const auto &[building_id, building] : candidate.buildings) {
        (void)building_id;
        const auto *definition = find_definition(catalog, building.definition_key);
        if (definition == nullptr) {
            return Error::unknown_definition;
        }
        maintenance += definition->monthly_maintenance;
    }

    for (int month = candidate.last_settled_month + 1; month <= target_month; ++month) {
        const auto input = inputs.find(month);
        if (input == inputs.end()) {
            return Error::missing_month_input;
        }
        const std::int64_t net = input->second.operating_income - maintenance;
        candidate.funds += net;
        candidate.monthly_records.push_back(
            MonthlyRecord{month, input->second.operating_income, maintenance, net});
        candidate.last_settled_month = month;
    }

    state = std::move(candidate);
    return Error::none;
}

} // namespace ark::simulation::rules
