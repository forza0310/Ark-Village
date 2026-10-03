// Grid bounds and indexing; source cells remain distinct from derived facility occupancy.
#include "ark/world/grid.hpp"
#include <stdexcept>

namespace ark::world {
bool Bounds::contains(Cell c) const {
    return c.x >= min_x && c.x <= max_x && c.y >= min_y && c.y <= max_y;
}
bool SourceMap::contains(Cell c) const {
    return c.x >= 0 && c.x < width && c.y >= 0 && c.y < height;
}
std::size_t SourceMap::index(Cell c) const {
    if (!contains(c))
        throw std::out_of_range("Grid coordinate outside source map");
    return static_cast<std::size_t>(c.y * width + c.x);
}
} // namespace ark::world
