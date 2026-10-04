#pragma once

// R1 safety fixture: single-cell buildings, exclusive visits and idempotent completion; not APK
// parity.

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ark::simulation::rules {

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

// Stage a new single-cell building and its charge; this fixture has no original footprint or town
// limits.
BuildingResult construct(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                         const std::string &definition_key, Position position, int rotation = 0);
// Preserve the stable ID while moving a fixture building; no APK relocation fee is modelled here.
Error move(GlobalState &state, BuildingId building_id, Position destination, int rotation);
// Remove the building and clear all reservations and character targets that reference it.
Error remove(GlobalState &state, BuildingId building_id);

// Reserve exclusively only when the supplied path result succeeds; failures leave the state
// unchanged.
ActivityResult start_visit(GlobalState &state, CharacterId character_id, BuildingId building_id,
                           bool path_found);
// Release the character-owned reservation; repeating cancellation while idle is harmless.
Error cancel_visit(GlobalState &state, CharacterId character_id);
// Only transition travelling to in_use; this R1 fixture does not implement arrival income.
Error arrive(GlobalState &state, CharacterId character_id);
// Apply the fixture effect once per activity ID, then release the reservation.
Error complete_use(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                   CharacterId character_id, ActivityId activity_id);

// Apply the fixture cash reward once; real task deadlines and point rewards remain outside this
// model.
Error complete_task(GlobalState &state, std::uint64_t task_id);
// Commit all missing fixture months together; do not combine this net-payment model with
// PeriodAccounting.
Error settle_through(GlobalState &state, const std::vector<BuildingDefinition> &catalog,
                     int target_month, const std::map<int, MonthlyInput> &inputs);

} // namespace ark::simulation::rules
