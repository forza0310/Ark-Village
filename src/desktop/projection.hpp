#pragma once

// 2D-only projection and viewport mapping; both render and input use the same transforms.
#include "ark/world/grid.hpp"
#include <optional>
#include <raylib.h>
namespace ark::desktop {
struct Extent {
    int width{}, height{};
};
// Resize extends the world view instead of letterboxing a fixed portrait canvas.
Extent canvas_extent(int width, int height);
Rectangle viewport(int width, int height, Extent canvas);
// Render logical coordinates directly into a destination measured in framebuffer pixels.
// Mouse coordinates still use the window-point viewport, without applying DPI a second time.
Camera2D canvas_camera(Rectangle destination, Extent canvas);
std::optional<Vector2> logical_mouse(Vector2 pixel, Rectangle destination, Extent canvas);
Vector2 project(world::Cell cell, Vector2 camera, Extent canvas, float zoom = 1);
// Published ground SEB occupies [0,60]x[0,29] relative to its projected drawing origin.
Vector2 tile_center(world::Cell cell, Vector2 camera, Extent canvas, float zoom = 1);
std::optional<world::Cell> pick(Vector2 logical, Vector2 camera, const world::SourceMap &map,
                                Extent canvas, float zoom = 1);
// Desktop zoom preserves the world position under the pointer; UI rectangles stay unscaled.
void zoom_at(Vector2 anchor, Extent canvas, float wheel, Vector2 &camera, float &zoom);
} // namespace ark::desktop
