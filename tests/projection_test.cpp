// Input must invert the same transform at letterboxed and portrait/wide window sizes.
#include "projection.hpp"
#include <cmath>
#include <stdexcept>
int main() {
    ark::world::SourceMap map{24, 24, {}};
    for (auto size : {Vector2{480, 660}, Vector2{960, 600}, Vector2{240, 330}}) {
        const auto box = ark::desktop::viewport(static_cast<int>(size.x), static_cast<int>(size.y));
        for (auto cell : {ark::world::Cell{9, 3}, ark::world::Cell{12, 5}}) {
            const Vector2 camera{426, -72};
            const auto p = ark::desktop::project(cell, camera);
            const Vector2 pixels{box.x + p.x * box.width / 240, box.y + p.y * box.height / 330};
            const auto logical = ark::desktop::logical_mouse(pixels, box);
            if (!logical || std::abs(logical->x - p.x) > 0.01F ||
                std::abs(logical->y - p.y) > 0.01F ||
                ark::desktop::pick(*logical, camera, map) != cell)
                throw std::runtime_error("Projection inverse mismatch");
        }
        if (ark::desktop::logical_mouse({-1, -1}, box))
            throw std::runtime_error("Letterbox input accepted");
    }
}
