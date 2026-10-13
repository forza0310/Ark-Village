#pragma once
// Source-bound effort, medal and residence ceremonies share one measured GPU adapter.
#include "../common/world_panels.hpp"
#include "ark/simulation/presentation/steam_human_skin.hpp"
namespace ark::desktop::ui {
std::optional<simulation::SteamHumanOverviewPlan>
prepare_world_human_ceremony(const simulation::StartupWorldRuntimeState &,
                             const simulation::rules::WorldScriptPage &, const Skin &);
void draw_world_human_ceremony(const std::optional<simulation::SteamHumanOverviewPlan> &,
                               const WorldPageLayout &, const Skin &, bool,
                               const std::string &title = "自宅完成");
} // namespace ark::desktop::ui
