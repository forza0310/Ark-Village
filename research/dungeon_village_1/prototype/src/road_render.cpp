#include "dungeon_village_prototype/road_render.hpp"

namespace dungeon_village_prototype {
std::optional<RoadPatchDraw> road_patch_draw(const LoadedStartupCell &cell) {
    if (cell.display_id == -1)
        return std::nullopt;
    if (cell.road_quad)
        return RoadPatchDraw{"common/road4block00.png", 0, 30, 20, 14, 21, -10};
    if (cell.edge_road_pair)
        return RoadPatchDraw{"common/road4block01.png", 155, 27, 15, 20, 19, -10};
    return std::nullopt;
}
} // namespace dungeon_village_prototype
