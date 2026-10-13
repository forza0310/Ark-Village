#pragma once

// Raw17 is a measured, read-only application view. The Owner supplies both clocks and scores.
#include "../common/layout.hpp"
#include "../common/skin.hpp"
#include "ark/app/session/world_system.hpp"
#include "ark/simulation/presentation/steam_clear_score_skin.hpp"

namespace ark::desktop::ui {
// Product equivalent of the maintained application bridge; missing or stale mirrors are refused.
std::optional<simulation::SteamClearScoreInput>
world_clear_score_input(const simulation::StartupWorldRuntimeState &state,
                        const app::WorldClearPage &clear);
std::optional<simulation::SteamClearScorePlan>
prepare_world_clear_score(const simulation::SteamClearScoreInput &input, const Skin &skin);
// The logical 240x256 source canvas is translated once, with no added confirmation button.
void draw_world_clear_score(const simulation::SteamClearScorePlan &plan, Extent extent,
                            const Skin &skin);
} // namespace ark::desktop::ui
