// Adapted from research/prototype/road_render and STARTUP's road mark generation.
#include "road_render.hpp"
#include <stdexcept>
namespace ark::desktop {
std::optional<RoadPatch> road_patch(world::Cell cell, bool visible, bool quad, bool edge) {
    if (!visible)
        return {};
    if (quad)
        return RoadPatch{cell, "road4block00.png", 30, 20, 14, 21, -10};
    if (edge)
        return RoadPatch{cell, "road4block01.png", 27, 15, 20, 19, -10};
    return {};
}
std::vector<RoadPatch> road_patches(const world::SourceMap &terrain, int road_display) {
    if (terrain.width <= 0 || terrain.height <= 0 || road_display < 0 ||
        terrain.cells.size() != static_cast<std::size_t>(terrain.width) * terrain.height)
        throw std::invalid_argument("Invalid road patch terrain");
    const auto road = [&](int x, int y) {
        return terrain.contains({x, y}) &&
               terrain.cells[terrain.index({x, y})].display_id == road_display;
    };
    std::vector<RoadPatch> patches;
    for (int y = terrain.height - 1; y >= 0; --y)
        for (int x = 0; x < terrain.width; ++x) {
            if (!road(x, y) || !road(x + 1, y))
                continue;
            const auto patch = road_patch({x, y}, true, road(x, y - 1) && road(x + 1, y - 1),
                                          y == 0 || y == terrain.height - 1);
            if (patch)
                patches.push_back(*patch);
        }
    return patches;
}
} // namespace ark::desktop
