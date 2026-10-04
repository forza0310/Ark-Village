// Exact submitted parameters; these are neither PNG rectangles nor a regenerated boundary ring.
#include "boundary_render.hpp"
#include <array>
#include <limits>
#include <stdexcept>

namespace ark::desktop {
std::vector<BoundaryOverlay> boundary_overlays(const world::LoadedCell &cell, int boundary_index,
                                               int display_flags, int display_depth_offset,
                                               bool eligible) {
    if (!eligible)
        return {};
    if (cell.boundary_fragment < -1 || cell.boundary_fragment > 5 || cell.external_direction < -1 ||
        cell.external_direction > 3)
        throw std::invalid_argument("Invalid boundary fragment or external direction");
    std::vector<BoundaryOverlay> result;
    if (cell.boundary_fragment >= 0) {
        if (boundary_index < 0 || boundary_index > 2)
            throw std::invalid_argument("Invalid boundary region index");
        constexpr std::array<const char *, 3> sprites{
            {"fence010.seb", "fence011.seb", "fence012.seb"}};
        constexpr std::array<std::array<int, 2>, 6> offsets{
            {{{13, 22}}, {{16, 22}}, {{13, 21}}, {{42, 22}}, {{15, 23}}, {{16, 9}}}};
        const auto depth =
            static_cast<long long>(display_depth_offset) + ((display_flags & 1) ? -50 : 15) + 60;
        if (depth < std::numeric_limits<int>::min() || depth > std::numeric_limits<int>::max())
            throw std::invalid_argument("Boundary display depth overflow");
        const auto offset = offsets[cell.boundary_fragment];
        result.push_back({sprites[boundary_index], cell.boundary_fragment, offset[0], offset[1],
                          static_cast<int>(depth)});
    }
    // Original consumers are independent branches. Do not invent mutual exclusivity here,
    // even though the published reset's external entrance cells have no ordinary fence mark.
    if (cell.external_direction >= 0) {
        constexpr std::array<std::array<int, 2>, 4> offsets{
            {{{15, 24}}, {{26, 19}}, {{15, 18}}, {{28, 24}}}};
        const auto offset = offsets[cell.external_direction];
        result.push_back(
            {"door00.seb", cell.external_direction / 2, offset[0], offset[1], offset[1]});
    }
    return result;
}
} // namespace ark::desktop
