#pragma once

// 2D-only projection and viewport mapping; both render and input use the same transforms.
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
} // namespace ark::desktop
