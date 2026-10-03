#pragma once

// Presentation metadata only; domain rules do not know PNGs or screen coordinates.
#include "dungeon_village_prototype/startup_map.hpp"

#include <optional>

namespace dungeon_village_prototype {
struct RoadPatchDraw {
    const char *asset_path{};
    int common_image_id{};
    int width{};
    int height{};
    int offset_x{};
    int offset_y{};
    int depth_offset{};
};
// Offsets are relative to the transformed tile SEB anchor D, not tile centre. Draw the whole PNG.
// Submit on the second y-descending/x-ascending tile pass to the shared depth queue; not last/top.
// Visibility is inherited from the base-image rectangle, not determined by this patch's size.
std::optional<RoadPatchDraw> road_patch_draw(const LoadedStartupCell &cell);
} // namespace dungeon_village_prototype
