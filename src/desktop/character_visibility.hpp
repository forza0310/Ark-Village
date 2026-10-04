#pragma once

// Desktop-only presentation policy; the model retains the actor and actual service ownership.
#include "ark/app/game.hpp"

namespace ark::desktop {
bool character_visible(const app::Game &game, bool inspection_actor = false);
} // namespace ark::desktop
