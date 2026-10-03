// Joining and tutorial display are separate transitions; closing a page never creates a person.
#include "ark/people/adventurer.hpp"

namespace ark::people {
Adventurer first_visit(const Adventurer &definition, world::Cell spawn) {
    auto actor = definition;
    actor.cell = spawn;
    actor.flags |= 2U | 8192U;
    actor.position = {spawn.x * 100.0F + 50, spawn.y * 100.0F + 50};
    actor.pending_activity = 0;
    return actor;
}
} // namespace ark::people
