#include "dungeon_village_reference/actor_control.hpp"

namespace dungeon_village_reference {
bool valid_actor_control(const LegacyActorControl &c) {
    static constexpr std::size_t sizes[34] = {3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1,
                                              1, 3, 4, 1, 1, 3, 1, 1, 2, 1, 3, 2, 3, 3, 1, 1, 1};
    if (c.empty() || c[0] < 0 || c[0] >= 34 || c.size() < sizes[c[0]])
        return false;
    if (c[0] == 1 && (c[1] < 0 || (c[2] != 0 && c[2] != 1)))
        return false;
    if (c[0] == 2 && (c[1] < 0 || c[1] > 20))
        return false;
    if (c[0] == 3 && (c[1] < 0 || c[1] > 11))
        return false;
    if (c[0] == 4 && (c[1] < 0 || c[1] > 3))
        return false;
    if (c[0] == 8 && (c[1] < 0 || c[1] > 8))
        return false;
    return true;
}
ActorControlResult prepare_local_control_prefix(const ActorControlState &s,
                                                const ActorControlContext &context) {
    if (s.state < 0 || s.state > 20 || s.action < 0 || s.action > 11 || s.action_counter < 0 ||
        s.alternate_counter < 0 || s.facing < 0 || s.facing > 3)
        return {ActorControlError::invalid_state, std::nullopt};
    for (const auto &command : s.queue)
        if (!valid_actor_control(command))
            return {ActorControlError::malformed_command, std::nullopt};
    ActorControlCandidate c;
    c.state = s;
    while (!c.state.queue.empty()) {
        auto &command = c.state.queue.front();
        switch (command[0]) {
        case 0:
            if (!context.move_arrived || c.consumed_motion_result) {
                c.flow = ActorControlFlow::delegated;
                return {ActorControlError::none, c};
            }
            c.consumed_motion_result = true;
            if (!*context.move_arrived) {
                c.flow = ActorControlFlow::moving;
                return {ActorControlError::none, c};
            }
            break;
        case 1:
            if (command[2] == 1)
                c.state.flags |= 1U;
            --command[1];
            if (command[1] > 0) {
                c.flow = ActorControlFlow::waiting;
                return {ActorControlError::none, c};
            }
            break;
        case 3:
            c.state.action = command[1];
            c.state.action_counter = 0;
            c.state.alternate_counter = 0;
            break;
        case 4:
            c.state.facing = command[1];
            break;
        case 5:
            c.state.flags = static_cast<std::uint32_t>(command[1]);
            break;
        case 6:
            c.state.flags |= static_cast<std::uint32_t>(command[1]);
            break;
        case 7:
            c.state.flags &= ~static_cast<std::uint32_t>(command[1]);
            break;
        case 8:
            if (context.departure_succeeded && *context.departure_succeeded) {
                c.state.queue.erase(c.state.queue.begin());
                ++c.removed_commands;
                c.state.state = 0; // Direct A write, NOT c(0): no B/D/queue reset here.
                c.flow = ActorControlFlow::departure_started;
                return {ActorControlError::none, c};
            }
            c.flow = ActorControlFlow::delegated;
            return {ActorControlError::none, c};
        case 9:
        case 11:
        case 20:
        case 31:
            break;
        default:
            c.flow = ActorControlFlow::delegated;
            return {ActorControlError::none, c};
        }
        c.state.queue.erase(c.state.queue.begin());
        ++c.removed_commands;
    }
    return {ActorControlError::none, c};
}
} // namespace dungeon_village_reference
