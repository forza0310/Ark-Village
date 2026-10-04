#include "character_visibility.hpp"
#include <algorithm>

namespace ark::desktop {
bool character_visible(const app::Game &game, bool inspection_actor) {
    if (!game.state().adventurer)
        return false;
    if (inspection_actor)
        return true;
    // User-requested desktop policy: hide only during proven, occupied facility use. Research
    // has not delivered the original draw predicate/front door; flags bit1 is not that evidence.
    if (const auto *life = game.life_state();
        life && life->active_facility && life->control.state == 14) {
        const auto found = game.state().facility_life.find(life->active_facility->instance);
        if (found != game.state().facility_life.end()) {
            const auto &occupants = found->second.occupants;
            return std::find(occupants.begin(), occupants.end(), life->actor) == occupants.end();
        }
    }
    if (const auto *ai = game.ai_state(); ai && ai->active_facility && ai->control.state == 14) {
        const auto found = ai->facilities.find(ai->active_facility->instance);
        if (found != ai->facilities.end()) {
            const auto &occupants = found->second.occupants;
            return std::find(occupants.begin(), occupants.end(), ai->actor) == occupants.end();
        }
    }
    return true;
}
} // namespace ark::desktop
