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
} // namespace dungeon_village_reference
