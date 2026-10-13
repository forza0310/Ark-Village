#pragma once
// GPU execution is separate from village navigation, timing, payment and reward consumers.
#include "../common/world_list_controls.hpp"
#include "ark/simulation/presentation/steam_town_progress_skin.hpp"
#include "ark/simulation/presentation/steam_village_activity_skin.hpp"

namespace ark::desktop::ui {
class Skin;
WorldFacilityPaintedControls
draw_world_village_skin(const simulation::SteamVillageActivityPlan &plan, Rectangle panel,
                        const Skin &skin, WorldFacilityPaintedControls controls,
                        const std::vector<int> &ids);
WorldFacilityPaintedControls draw_world_town_skin(const simulation::SteamTownProgressPlan &plan,
                                                  Rectangle panel, const Skin &skin,
                                                  WorldFacilityPaintedControls controls);
} // namespace ark::desktop::ui
