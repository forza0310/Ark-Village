#pragma once

// Explicit window diagnostics only: actions use the maintained source consumer and real
// new-world funds/catalogue. This driver neither edits rule values nor fabricates elapsed time.
#include "ark/simulation/world/startup_world_runtime.hpp"
#include "world_human_inspection.hpp"
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
    int expansion_level_before{}; // Observation only; the source53 consumer changes the level.
    // Map-edit diagnostics retain only observations; all changes use the published Owner
    // consumers. These values also make window logs distinguish real commits from previews.
    std::optional<simulation::rules::Position> edit_endpoint;
    std::optional<std::uint64_t> edited_old;
    std::optional<simulation::rules::Position> edited_old_anchor;
    std::int64_t edit_cash_before{}, edit_cash_after{};
    std::uint64_t edit_draws_before{}, edit_draws_after{};
    int edit_cells{}, edit_old_raw{-1}, edit_old_ordinal{-1};
    bool edit_completed{};
    // Reuse the existing real recruitment/gift/admission player policy until its home
    // finishes. Stages: 0 housing, 1 demolition scripts, 2 raw21, 3 real rebuilding.
    WorldHumanInspection housing_policy;
    int home_rebuild_stage{};
    std::optional<int> home_resident;
    int home_credit_before{}, home_credit_after{}, home_credit_remaining{};
    std::int64_t home_build_quote{}, home_build_cash_before{}, home_build_cash_after{};
    std::uint64_t home_build_draws_before{}, home_build_draws_after{};
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
