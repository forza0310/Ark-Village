#pragma once
// Residence completion is a source-bound ceremony, independent from human catalogue input.
#include "../common/world_panels.hpp"
#include "ark/simulation/presentation/steam_human_skin.hpp"
namespace ark::desktop::ui {
std::optional<simulation::SteamHumanOverviewPlan>
prepare_world_residence_result(const simulation::StartupWorldRuntimeState &,
                               const simulation::rules::WorldScriptPage &, const Skin &);
void draw_world_residence_result(const std::optional<simulation::SteamHumanOverviewPlan> &,
                                 const WorldPageLayout &, const Skin &, bool);
} // namespace ark::desktop::ui
