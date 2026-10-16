#pragma once

#include "../common/layout.hpp"
#include "ark/app/session/world_session.hpp"
#include "ark/simulation/presentation/steam_dungeon_footer_skin.hpp"

namespace ark::desktop::ui {
class Skin;
// Reads only the publication's matched state/plan. Empty operations leave the desktop HUD intact.
simulation::SteamDungeonFooterPlan world_dungeon_footer(const app::WorldFrame &frame,
                                                        Extent extent);
void draw_world_dungeon_footer(const simulation::SteamDungeonFooterPlan &plan, const Skin &skin,
                               const simulation::SteamDungeonFooterLanguageProfile &language);
} // namespace ark::desktop::ui
