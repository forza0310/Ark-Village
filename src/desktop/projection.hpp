#pragma once

// 2D-only projection and viewport mapping; both render and input use the same transforms.
#include "ark/world/grid.hpp"
#include <optional>
#include <raylib.h>
namespace ark::desktop {
constexpr int canvas_width = 240, canvas_height = 330;
Rectangle viewport(int width, int height);
std::optional<Vector2> logical_mouse(Vector2 pixel, Rectangle destination);
Vector2 project(world::Cell cell, Vector2 camera);
std::optional<world::Cell> pick(Vector2 logical, Vector2 camera, const world::SourceMap &map);
} // namespace ark::desktop
