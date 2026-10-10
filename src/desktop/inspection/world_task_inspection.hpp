#pragma once

// Explicit --inspect-page policy only. Normal play never invokes automatic task input.
#include "ark/simulation/world/startup_world_runtime.hpp"
#include <string>

namespace ark::desktop {
struct WorldTaskInspection {
    std::optional<std::uint64_t> accepted_task;
    std::optional<std::uint64_t> departed_task;
};
bool world_task_inspection_mode(const std::string &mode);
bool world_task_inspection_ready(const simulation::StartupWorldRuntimeState &state,
                                 const std::string &mode, const WorldTaskInspection &inspection);
// Issues at most one researched player command per call. Ordinary page/report confirmation
// remains shared with the existing inspection path; true means this specialized input handled it.
bool apply_world_task_inspection_input(simulation::StartupWorldRuntimeState &state,
                                       WorldTaskInspection &inspection);
} // namespace ark::desktop
