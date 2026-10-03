// Source tile basis and y-flip from STARTUP; responsive extent is a desktop adaptation.
#include "projection.hpp"
#include <algorithm>
#include <cmath>

namespace ark::desktop {
Extent canvas_extent(int width, int height) {
    const float scale = std::min(width / 240.0F, height / 256.0F);
    return {static_cast<int>(std::round(width / scale)),
            static_cast<int>(std::round(height / scale))};
}
Rectangle viewport(int width, int height, Extent canvas) {
    const float scale = std::min(width / static_cast<float>(canvas.width),
                                 height / static_cast<float>(canvas.height));
    return {(width - canvas.width * scale) / 2, (height - canvas.height * scale) / 2,
            canvas.width * scale, canvas.height * scale};
}
std::optional<Vector2> logical_mouse(Vector2 pixel, Rectangle destination, Extent canvas) {
    if (!CheckCollisionPointRec(pixel, destination))
        return std::nullopt;
    return Vector2{(pixel.x - destination.x) * canvas.width / destination.width,
                   (pixel.y - destination.y) * canvas.height / destination.height};
}
Camera2D canvas_camera(Rectangle destination, Extent canvas) {
    return {{destination.x, destination.y}, {0, 0}, 0, destination.width / canvas.width};
}
Vector2 project(world::Cell cell, Vector2 camera, Extent canvas, float zoom) {
    return {canvas.width / 2.0F + zoom * (30.0F * (cell.x + cell.y) - camera.x),
            canvas.height / 2.0F + zoom * (-15.0F * (cell.y - cell.x) - 15 + camera.y)};
}
std::optional<world::Cell> pick(Vector2 logical, Vector2 camera, const world::SourceMap &map,
                                Extent canvas, float zoom) {
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x) {
            const auto p = tile_center({x, y}, camera, canvas, zoom);
            if (std::abs(logical.x - p.x) / (30 * zoom) + std::abs(logical.y - p.y) / (15 * zoom) <=
                1)
                return world::Cell{x, y};
        }
    return std::nullopt;
}
Vector2 tile_center(world::Cell cell, Vector2 camera, Extent canvas, float zoom) {
    const auto origin = project(cell, camera, canvas, zoom);
    return {origin.x + 30 * zoom, origin.y + 14.5F * zoom};
}
void zoom_at(Vector2 anchor, Extent canvas, float wheel, Vector2 &camera, float &zoom) {
    const float next = std::clamp(zoom * std::pow(1.05F, wheel), 0.5F, 2.0F);
    const float shift = 1 / zoom - 1 / next;
    camera.x += (anchor.x - canvas.width / 2.0F) * shift;
    camera.y -= (anchor.y - canvas.height / 2.0F) * shift;
    zoom = next;
}
} // namespace ark::desktop
