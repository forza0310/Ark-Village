// Adapted from prototype startup projection, including source y-flip. Letterboxing and resize
// are adapter policies; physical Android touch rectangles have not yet been recovered.
#include "projection.hpp"
#include <algorithm>
#include <cmath>

namespace ark::desktop {
Rectangle viewport(int width, int height) {
    const float scale = std::min(width / static_cast<float>(canvas_width),
                                 height / static_cast<float>(canvas_height));
    return {(width - canvas_width * scale) / 2, (height - canvas_height * scale) / 2,
            canvas_width * scale, canvas_height * scale};
}
std::optional<Vector2> logical_mouse(Vector2 pixel, Rectangle destination) {
    if (!CheckCollisionPointRec(pixel, destination))
        return std::nullopt;
    return Vector2{(pixel.x - destination.x) * canvas_width / destination.width,
                   (pixel.y - destination.y) * canvas_height / destination.height};
}
Vector2 project(world::Cell cell, Vector2 camera) {
    return {120 + 30.0F * (cell.x + cell.y) - camera.x,
            165 - 15.0F * (cell.y - cell.x) - 15 + camera.y};
}
std::optional<world::Cell> pick(Vector2 logical, Vector2 camera, const world::SourceMap &map) {
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x) {
            const auto p = project({x, y}, camera);
            if (std::abs(logical.x - p.x) / 30 + std::abs(logical.y - p.y) / 15 <= 1)
                return world::Cell{x, y};
        }
    return std::nullopt;
}
} // namespace ark::desktop
