#pragma once

#include "../common/layout.hpp"
#include "../common/world_list_controls.hpp"
#include "ark/simulation/presentation/steam_facility_effect_skin.hpp"
#include "ark/simulation/presentation/steam_facility_reputation_skin.hpp"

namespace ark::desktop::ui {
class Skin;
// VIEW_Y23 is already handled by the source helper; translate its center just once.
Vector2 world_facility_skin_origin(Rectangle panel);
void draw_world_facility_effect(const simulation::SteamFacilityEffectPlan &plan, Vector2 origin,
                                const Skin &skin);
void draw_world_facility_reputation(const simulation::SteamFacilityReputationPlan &plan,
                                    Vector2 origin, const Skin &skin);
WorldFacilityPaintedControls world_facility_controls(const simulation::SteamHumanOverviewPlan &plan,
                                                     Vector2 origin, Rectangle panel,
                                                     const Skin &skin,
                                                     WorldFacilityPaintedControls controls,
                                                     const std::vector<int> &ids);
WorldFacilityPaintedControls
world_facility_controls(const simulation::SteamFacilityEffectPlan &plan, Vector2 origin,
                        Rectangle panel, WorldFacilityPaintedControls controls,
                        const std::vector<int> &ids);
} // namespace ark::desktop::ui
