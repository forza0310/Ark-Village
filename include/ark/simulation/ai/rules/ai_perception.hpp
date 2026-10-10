#pragma once

#include "ark/simulation/ai/rules/actor_ai.hpp"

#include <array>

namespace ark::simulation::rules {
struct EncounterSnapshot {
    std::uint64_t id{};
    Position center;
    std::optional<int> original_id{}; // Present in world queries; stable identity stays separate.
};
struct EventGateInput {
    BattleGateInput actor; // F ignores1024, unlike G.
    Position cell;
    bool cell_in_map{};
    bool cell_event_flag{};
    std::optional<std::uint64_t> previous_encounter;
    std::vector<EncounterSnapshot> encounters;
    bool active_task{};
    bool definition_task_flag{};
    int task_kind{};
    Position task_center;
    std::optional<std::uint64_t> task_encounter;
    std::optional<int> task_original_id{}; // F compares f164b, not pointer/stable-ID equality.
};
struct EventGateCandidate {
    bool ready{};
    std::optional<std::uint64_t> bind_encounter; // Absent means no db write, not db clear.
    bool request_task_encounter{}; // Returns ready even if owner's creation subsequently fails.
};
struct EventGateResult {
    ActorAiError error{ActorAiError::none};
    std::optional<EventGateCandidate> candidate;
};
// F() prepares the first matching original-order event or a type2 creation request.
// Newly-created task encounter is NOT bound to db in this call; an existing one is.
EventGateResult prepare_event_gate(const EventGateInput &input);

struct MoveAreaProbe {
    bool cell_in_map{};
    int legacy_surface{}; // i.g: 3 fails.
    int logical_state{};  // i.e: 0 fails.
    bool in_encounter_square{};
};
struct MoveAreaCandidate {
    bool allowed{};
    int legacy_diagnostic{}; // K(): 0 OK,1 bounds,2 cell,3 event region.
};
// Caller supplies all four world+unit probes in d.b order after original half-grid /2 truncation.
// This is local movement qualification, not a connectivity or route query.
std::optional<MoveAreaCandidate> prepare_move_area(int state, bool has_encounter,
                                                   const std::array<MoveAreaProbe, 4> &probes);

struct RescueTargetSnapshot {
    CharacterId id;
    int state{};
    WorldPosition position;
    Position cell;
    Position half_cell;
    bool inside_town{};
    bool on_event_cell{};
    bool low_hp{}; // ak uses am[3] < capacity/2, not g().
};
struct NearbyObjectSnapshot {
    std::uint64_t id{};
    int state{}; // e.i, desired3.
    WorldPosition position;
    Position cell;
};
struct PreemptionInput {
    ActorKind kind{ActorKind::human};
    int state{};
    bool has_object{};
    bool rescue_enabled{};
    bool definition_task_flag{};
    bool inside_town{};
    Position cell;
    std::vector<RescueTargetSnapshot> people;
    std::vector<NearbyObjectSnapshot> objects;
};
// c()'s rescue scan precedes object scan and may change state before the latter sees it.
std::optional<int> select_idle_preemption(const PreemptionInput &input);
// I() excludes event cells, but DOES NOT repeat the preemption same-town-side test.
EnemySelectionResult select_rescue_target(WorldPosition position,
                                          const std::vector<RescueTargetSnapshot> &people);
// J() returns the first eligible person, not nearest; includes self/down targets.
std::optional<CharacterId> select_healing_target(Position half_cell,
                                                 const std::vector<RescueTargetSnapshot> &people);

struct SpawnProbeInput {
    ActorKind kind{ActorKind::human};
    int state{}; // Must be the resulting state after idle preemptions/reselection.
    bool inside_town{};
    bool destination_inside_town{};
    int cell_y{};
    int minimum_y{};
    int monster_count{};
    int monster_limit{};
    std::optional<int> ticket; // L() draws0..999 BEFORE checking the global monster limit.
};
struct SpawnProbeCandidate {
    bool consumes_ticket{};
    bool request_event_probe{}; // f.b() still checks k.a(cell) and event creation guards.
};
struct SpawnProbeResult {
    ActorAiError error{ActorAiError::none};
    std::optional<SpawnProbeCandidate> candidate;
};
SpawnProbeResult prepare_spawn_probe(const SpawnProbeInput &input);

enum class DepartureOverrideKind { ordinary, task, rescue, object, encounter };
struct OtherDestination {
    CharacterId id;
    Position destination; // O[0/1], regardless of that person's current state/path.
};
struct DepartureOverrideInput {
    ActorKind kind{ActorKind::human};
    CharacterId self;
    std::uint32_t flags{};
    int activity{};
    bool has_object{};
    bool active_task{};
    bool definition_task_flag{};
    Position task_center;
    std::vector<Position> reachable;      // Original l.a() order, duplicates retained.
    std::optional<Position> nearest_down; // I(), caller resolves original distance/tie rules.
    std::optional<Position>
        nearest_object; // H(), not all objects searched for reachable alternatives.
    std::optional<Position> nearest_encounter; // Floor grid-Euclidean distance, same reverse ties.
    std::vector<OtherDestination> people;
};
struct DepartureOverrideCandidate {
    DepartureOverrideKind kind{DepartureOverrideKind::ordinary};
    std::optional<Position> destination;
    bool request_task_attribute{}; // a.e.a(0,self) occurs when task goal is found.
};
// o(activity)'s pre-policy target priority. Ordinary means continue activity-specific selection.
// Only activity6 considers nearest event; a reserved nearest down target skips to object.
std::optional<DepartureOverrideCandidate>
prepare_departure_override(const DepartureOverrideInput &input);
} // namespace ark::simulation::rules
