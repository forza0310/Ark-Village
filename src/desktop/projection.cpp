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
} // namespace ark::desktop
