#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace dungeon_village_reference {

struct BuildingId {
    std::uint64_t value{};
};

struct CharacterId {
    std::uint64_t value{};
};

struct ActivityId {
    std::uint64_t value{};
};

bool operator==(BuildingId left, BuildingId right);
bool operator<(BuildingId left, BuildingId right);
bool operator==(CharacterId left, CharacterId right);
bool operator<(CharacterId left, CharacterId right);
bool operator==(ActivityId left, ActivityId right);
bool operator!=(ActivityId left, ActivityId right);
bool operator<(ActivityId left, ActivityId right);

struct Position {
    int x{};
    int y{};
};

bool operator==(Position left, Position right);

struct BuildingDefinition {
    std::string key;
    std::int64_t build_cost{};
    std::int64_t monthly_maintenance{};
    int use_effect{};
};

struct BuildingInstance {
    BuildingId id;
    std::string definition_key;
    Position position;
    int rotation{};
    std::uint64_t completed_uses{};
};

enum class ActivityState { idle, travelling, in_use };

struct CharacterState {
    CharacterId id;
    ActivityState activity{ActivityState::idle};
    std::optional<BuildingId> target;
    std::optional<ActivityId> active_activity;
    int accumulated_effect{};
};

struct TaskState {
    std::uint64_t id{};
    std::int64_t reward{};
    bool completed{};
};

struct MonthlyInput {
    std::int64_t operating_income{};
};

struct MonthlyRecord {
    int month{};
    std::int64_t income{};
    std::int64_t maintenance{};
    std::int64_t net{};
};

struct GlobalState {
    std::int64_t funds{};
    std::uint64_t next_building_id{1};
    std::uint64_t next_activity_id{1};
    std::map<BuildingId, BuildingInstance> buildings;
    std::map<CharacterId, CharacterState> characters;
    std::map<BuildingId, CharacterId> reservations;
    std::set<ActivityId> completed_activities;
    std::map<std::uint64_t, TaskState> tasks;
    int last_settled_month{};
    std::vector<MonthlyRecord> monthly_records;
};

enum class Error {
    none,
    unknown_definition,
    insufficient_funds,
    occupied,
    not_found,
    invalid_state,
    reserved,
    path_unreachable,
    missing_month_input,
    invalid_month,
};

struct BuildingResult {
    Error error{Error::none};
    std::optional<BuildingId> building_id;
};

struct ActivityResult {
    Error error{Error::none};
    std::optional<ActivityId> activity_id;
};

BuildingResult construct(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                         const std::string &definition_key, Position position, int rotation = 0);
Error move(GlobalState &state, BuildingId building_id, Position destination, int rotation);
Error remove(GlobalState &state, BuildingId building_id);

ActivityResult start_visit(GlobalState &state, CharacterId character_id, BuildingId building_id,
                           bool path_found);
Error cancel_visit(GlobalState &state, CharacterId character_id);
Error arrive(GlobalState &state, CharacterId character_id);
Error complete_use(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                   CharacterId character_id, ActivityId activity_id);

Error complete_task(GlobalState &state, std::uint64_t task_id);
Error settle_through(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                     int target_month, const std::map<int, MonthlyInput> &inputs);

} // namespace dungeon_village_reference
