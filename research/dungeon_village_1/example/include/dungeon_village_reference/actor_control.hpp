#pragma once

#include "dungeon_village_reference/domain.hpp"

namespace dungeon_village_reference {
using LegacyActorControl = std::vector<int>; // Fixed APK m queue, not cd/ce or a global event bus.
enum class ActorControlError { none, malformed_command, invalid_state };
enum class ActorControlFlow { empty, waiting, moving, departure_started, delegated };
struct ActorControlState {
    std::uint32_t flags{};
    int state{};             // A.
    int action{};            // k.
    int action_counter{};    // l.
    int alternate_counter{}; // i.
    int facing{};            // j.
    std::vector<LegacyActorControl> queue;
};
struct ActorControlContext {
    std::optional<bool> move_arrived; // First opcode0 result only; subsequent0 needs a fresh query.
    std::optional<bool> departure_succeeded; // o(activity); failure delegates full cleanup.
};
struct ActorControlCandidate {
    ActorControlState state;
    ActorControlFlow flow{ActorControlFlow::empty};
    std::size_t removed_commands{};
    bool consumed_motion_result{};
};
struct ActorControlResult {
    ActorControlError error{ActorControlError::none};
    std::optional<ActorControlCandidate> candidate;
};
// Decode all34 opcode minimum shapes; unknown/malformed returns false, not silent no-op.
bool valid_actor_control(const LegacyActorControl &command);
// Execute ONLY local0/1/3..7/9/11/20/31 and successful8; delegate every domain side effect.
// Wait reaching0 continues this same d(); successful8 stops before remaining attribute commands.
// Full queue preflight is a maintenance safety contract; candidates never mutate the input queue.
ActorControlResult prepare_local_control_prefix(const ActorControlState &state,
                                                const ActorControlContext &context = {});

struct ActorStateTransitionInput {
    ActorControlState control;
    bool human{true};
    int next_state{};
    int baseline{};                  // D updates only when entering0/5/17, not every transition.
    int legacy_u{};                  // Definition u, state18 boost threshold; not diligence(e.E).
    std::optional<int> boost_ticket; // [0,100), consumed even at threshold0 unless already2048.
    bool boost_event116_seen{};
    std::optional<int> current_facility_category;
};
struct ActorStateTransitionCandidate {
    ActorControlState control;
    int baseline{};
    bool clear_encounter{};
    bool reset_state_counter_and_parameter{true};
    bool reset_attack_count{};
    bool copy_attack_position{}; // State3 with old action3/6 copies n=au, including height.
    bool request_cleanup{};      // State10 while bound to category2 delegates r() AFTER reset.
    bool request_boost_event116{};
    bool consumed_boost_ticket{};
};
// c(state), distinct from b() baseline restoration and opcode8's direct A=0 write.
std::optional<ActorStateTransitionCandidate>
prepare_actor_state_transition(const ActorStateTransitionInput &input);

struct FailedActivityCandidate {
    std::uint32_t flags{};
    bool expression18{};
    bool delete_instance{};
    bool cleanup{};
};
// Called AFTER opcode8 was removed and o failed. Newly set1024 does not re-check32768.
FailedActivityCandidate prepare_failed_activity(std::uint32_t flags);

struct ActorWanderCell {
    int state{4};
    bool inside_town{};
    std::uint32_t flags{};
};
struct ActorWanderInput {
    int opcode{10};
    int parameter{};
    Position actor;
    std::optional<Position> center; // Required for12(db) and13(S); absent consumes no draws.
    int width{};
    int height{};
    std::vector<ActorWanderCell> cells; // Row-major; no facility/path-category reinterpretation.
    std::vector<int> tickets; // Exact draw order, each value checked against its current bound.
};
struct ActorWanderCandidate {
    std::vector<Position> cells;
    std::vector<LegacyActorControl> append;
    std::size_t consumed_tickets{};
};
// Control10/12/13 removed first; append behind existing tail. Direct world targets, no route.
std::optional<ActorWanderCandidate> prepare_actor_wander(const ActorWanderInput &input);

struct EquipmentCommitCandidate {
    int slot{}; // Definition v:0 weapon,1 shield,2 armor,3 accessory.
    int equipment{};
    int reselect_counter{6};    // Definition A[slot], not an actor state counter.
    bool update_actor_weapon{}; // Only28 updates the initiating actor's ae.
    bool recalculate_definition{true};
};
// Display27/29 cannot be submitted here. No propagation to other same-definition actors.
std::optional<EquipmentCommitCandidate> prepare_equipment_commit(const LegacyActorControl &command);

struct EquipmentExitTailInput {
    int category{1};
    int detail{1};
    int old_weapon{};
    int new_weapon{};
    int armor{};
    int armor_type{}; // Definition b.d==2 chooses slot1, all other types slot2.
    int accessory{};
};
// Called after release/reset/c0; starts with activity0. Successful activity8 delays all following
// equipment animation/commit until the next d(), it is not a synchronous equipment replacement.
std::optional<std::vector<LegacyActorControl>>
prepare_equipment_exit_tail(const EquipmentExitTailInput &input);
} // namespace dungeon_village_reference
