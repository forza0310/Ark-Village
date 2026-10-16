#pragma once

#include "ark/app/session/world_system.hpp"
#include "ark/simulation/presentation/startup_world_presentation.hpp"

namespace ark::app::detail {
// Read product application references before preparing the maintained private world candidate.
// Scheduling/deduplication and durable publication remain the session's responsibility.
simulation::StartupPresentationResult
prepare_world_presentation(const simulation::StartupWorldRuntimeState &state,
                           const WorldSystemState &system, std::uint64_t ordinal);
} // namespace ark::app::detail
