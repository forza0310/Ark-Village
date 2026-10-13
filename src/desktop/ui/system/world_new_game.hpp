#pragma once

// The title controller owns the draft. This module only measures, lays out and paints raw91.
#include "../common/layout.hpp"
#include "ark/app/session/world_system.hpp"
#include "ark/simulation/presentation/steam_new_game_skin.hpp"

namespace ark::desktop::ui {
class Skin;
struct WorldNewGameLayout {
    Extent extent{};
    Vector2 origin{};
    Rectangle panel{};
    std::array<Rectangle, 4> fields{};
    std::array<Rectangle, 2> arrows{};
};
struct WorldNewGamePaint {
    simulation::SteamNewGamePlan source;
    WorldNewGameLayout layout;
    std::array<simulation::SteamManualArrowPlan, 2> arrows;
};
// Explicit presentation clocks keep a static PC preview distinct from an unbound source Owner.
// Empty names remain legal product drafts; a drawing-only placeholder never changes the draft.
std::optional<simulation::SteamNewGamePlan>
world_new_game_plan(const app::WorldNewGameDraft &draft, int selected, int main, int finger_frame,
                    std::array<int, 2> title_widths, int start_width);
// Derive hit regions from the source Touch and actual AddTouch bounds, then clip to the viewport.
WorldNewGameLayout
world_new_game_layout(const simulation::SteamNewGamePlan &source, Extent extent,
                      const std::array<simulation::SteamManualArrowPlan, 2> &arrows);
WorldNewGamePaint prepare_world_new_game(const app::WorldNewGameDraft &draft, int selected,
                                         Extent extent, const Skin &skin, int main,
                                         int finger_frame);
void draw_world_new_game(const WorldNewGamePaint &paint, const Skin &skin);
} // namespace ark::desktop::ui
