#pragma once
#include "world_building.hpp"

namespace ark::desktop::ui {
// Execute the maintained raw74 operations in source order, translated into the approved
// compact desktop panel. Touch requests remain input metadata, never world commands.
void draw_world_facility_detail(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                                const Skin &skin, const WorldBuildingSelection &selection);
}
