#pragma once

// Explicit window diagnostics only: actions use the maintained source consumer and real
// new-world funds/catalogue. This driver neither edits rule values nor fabricates elapsed time.
#include "ark/simulation/startup_world_runtime.hpp"
#include "world_task_inspection.hpp"
#include <string>

namespace ark::desktop {
struct WorldManagementInspection {
    std::optional<std::uint64_t> created;
    std::optional<int> selection;
    std::optional<int> awarded_human;
    std::optional<simulation::rules::Position> preview_anchor;
    simulation::rules::FacilityOrientation preview_orientation{
        simulation::rules::FacilityOrientation::first};
    bool award_applied{};
    std::optional<int> activity;
    bool activity_started{}, activity_completed{};
    WorldTaskInspection task_policy;
};
bool management_inspection_mode(const std::string &mode);
// Called once before the bounded advance loop, so initial raw21/raw74 can be inspected
// without first creating an unrelated arrival page. Award mode starts no artificial page.
void begin_management_inspection(simulation::StartupWorldRuntimeState &state,
                                 const std::string &mode, WorldManagementInspection &inspection);
bool management_inspection_ready(const simulation::StartupWorldRuntimeState &state,
                                 const std::string &mode,
                                 const WorldManagementInspection &inspection);
// True blocks generic acknowledgement for a handled action or source animation wait.
bool apply_management_inspection_input(simulation::StartupWorldRuntimeState &state,
                                       const std::string &mode,
                                       WorldManagementInspection &inspection);
} // namespace ark::desktop
