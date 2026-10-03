// Joining and tutorial display are separate transitions; closing a page never creates a person.
#include "ark/people/adventurer.hpp"

namespace ark::people {
Adventurer first_visit(const Adventurer &definition, world::Cell spawn) {
    auto actor = definition;
    actor.cell = spawn;
    actor.flags |= 8192U;
    return actor;
}
} // namespace ark::people
