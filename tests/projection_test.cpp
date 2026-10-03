// Input must invert the same transform at letterboxed and portrait/wide window sizes.
#include "projection.hpp"
#include <cmath>
#include <stdexcept>
int main() {
    ark::world::SourceMap map{24, 24, {}};
    for (auto size :
         {Vector2{480, 660}, Vector2{960, 512}, Vector2{1280, 600}, Vector2{240, 256}}) {
        const auto extent =
            ark::desktop::canvas_extent(static_cast<int>(size.x), static_cast<int>(size.y));
        const auto box =
            ark::desktop::viewport(static_cast<int>(size.x), static_cast<int>(size.y), extent);
        if (extent.width < 240 || extent.height < 256 || box.width < size.x - 2 ||
            box.height < size.y - 2)
            throw std::runtime_error("Responsive view did not fill window");
        for (auto cell : {ark::world::Cell{9, 3}, ark::world::Cell{12, 5}}) {
            const Vector2 camera{426, -72};
            const auto p = ark::desktop::tile_center(cell, camera, extent);
            const Vector2 pixels{box.x + p.x * box.width / extent.width,
                                 box.y + p.y * box.height / extent.height};
            const auto logical = ark::desktop::logical_mouse(pixels, box, extent);
            if (!logical || std::abs(logical->x - p.x) > 0.01F ||
                std::abs(logical->y - p.y) > 0.01F ||
                ark::desktop::pick(*logical, camera, map, extent) != cell)
                throw std::runtime_error("Projection inverse mismatch");
        }
        if (ark::desktop::logical_mouse({-1, -1}, box, extent))
            throw std::runtime_error("Letterbox input accepted");
    }
}
