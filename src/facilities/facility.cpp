// Ordered footprint tables adapted from research/example geometry; not screen-space rotation.
#include "ark/facilities/facility.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::facilities {
std::vector<Binding> footprint(int shape, int orientation, world::Cell anchor) {
    if (orientation < 0 || orientation > 1 || shape < 0 || shape > 2)
        throw std::invalid_argument("Invalid facility shape or orientation");
    std::vector<world::Cell> offsets;
    if (shape == 0)
        offsets = {{0, 0}};
    else if (shape == 1)
        offsets = orientation == 0 ? std::vector<world::Cell>{{0, 1}, {0, 0}}
                                   : std::vector<world::Cell>{{-1, 0}, {0, 0}};
    else
        offsets = orientation == 0 ? std::vector<world::Cell>{{-1, 1}, {-1, 0}, {0, 1}, {0, 0}}
                                   : std::vector<world::Cell>{{-1, 1}, {0, 1}, {-1, 0}, {0, 0}};
    std::vector<Binding> result;
    for (std::size_t i = 0; i < offsets.size(); ++i)
        result.push_back({{anchor.x + offsets[i].x, anchor.y + offsets[i].y},
                          static_cast<int>(2 * i) + orientation});
    return result;
}
std::vector<world::Cell> surroundings(int shape, int orientation, world::Cell anchor,
                                      const world::SourceMap &map) {
    const auto cells = footprint(shape, orientation, anchor);
    auto minimum = cells.front().cell, maximum = minimum;
    for (const auto &part : cells) {
        if (!map.contains(part.cell))
            throw std::invalid_argument("Facility footprint outside map");
        minimum.x = std::min(minimum.x, part.cell.x);
        minimum.y = std::min(minimum.y, part.cell.y);
        maximum.x = std::max(maximum.x, part.cell.x);
        maximum.y = std::max(maximum.y, part.cell.y);
    }
    const int left = minimum.x - 1, right = maximum.x + 1;
    const int top = minimum.y - 1, bottom = maximum.y + 1;
    std::vector<world::Cell> ring;
    for (int y = minimum.y; y >= top; --y)
        ring.push_back({right, y});
    for (int x = right - 1; x >= left; --x)
        ring.push_back({x, top});
    for (int y = top + 1; y <= bottom; ++y)
        ring.push_back({left, y});
    for (int x = left + 1; x <= right; ++x)
        ring.push_back({x, bottom});
    for (int y = bottom - 1; y > minimum.y; --y)
        ring.push_back({right, y});
    const bool below = shape == 0 || (shape == 1 && orientation == 0);
    const world::Cell first =
        below ? world::Cell{minimum.x, bottom} : world::Cell{right, minimum.y};
    std::rotate(ring.begin(), std::find(ring.begin(), ring.end(), first), ring.end());
    ring.erase(
        std::remove_if(ring.begin(), ring.end(), [&](auto cell) { return !map.contains(cell); }),
        ring.end());
    return ring;
}
} // namespace ark::facilities
