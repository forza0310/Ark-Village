// Ordered footprint tables adapted from research/example geometry; not screen-space rotation.
#include "ark/facilities/facility.hpp"
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
} // namespace ark::facilities
