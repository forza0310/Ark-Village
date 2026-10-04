#pragma once
#include "ark/app/game.hpp"
#include "resources.hpp"
#include "ui/layout.hpp"
#include "ui/state.hpp"
namespace ark::desktop {
void draw_scene(const app::Game &game, Sprites &sprites, Vector2 camera, Extent extent, float zoom,
                std::optional<world::WorldPosition> inspection_actor = std::nullopt,
                int actor_frame = 0, int actor_facing = 0);
void draw_preview(const app::Game &game, const ui::State &view, const ui::Layout &layout,
                  Sprites &sprites, std::optional<world::Cell> hovered);
} // namespace ark::desktop
