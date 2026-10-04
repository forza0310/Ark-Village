#pragma once

#include "ark/simulation/startup_world_runtime.hpp"

#include <string>

namespace ark::desktop {
// Format the raw49 caches already initialized by the runtime; never query or advance rank rules.
std::string world_rank_conditions(const simulation::StartupWorldRuntimeState &state);
} // namespace ark::desktop
