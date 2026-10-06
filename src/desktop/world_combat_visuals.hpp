#pragma once

// Read-only combat feedback from ui/COMBAT_RENDER.md. Relative source-pixel plans are
// independent of raylib, camera, zoom and animation clocks; the world owns every timer/reward.
#include "ark/simulation/startup_world_runtime.hpp"
#include "world_overlay.hpp"

namespace ark::desktop {
// Uses the ordinary body anchor. Definition P/cd24 alternates the pending level badge;
// the actual cd14 badge has its independent timer. Special-action bl offsets, MISS/combo and
// bounce/particles need their complete maintained contracts before being added here.
// EXP remainder preserves the maintained growth-consumption arithmetic; exact original visual
// map truncation is not yet specified by research and is not claimed as pixel-exact behavior.
OverlayPlan world_actor_combat_visuals(const simulation::StartupWorldRuntimeState &state,
                                       simulation::rules::CharacterId actor);
// X2 is delayed facility income with source upward motion; X3 is fixed death-site income.
// The caller supplies the projected effect[2..3] anchor. Neither plan posts cash or advances X.
OverlayPlan world_cash_visuals(const std::vector<int> &effect);
} // namespace ark::desktop
