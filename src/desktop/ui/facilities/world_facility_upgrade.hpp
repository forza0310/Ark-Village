#pragma once
#include "world_building.hpp"

namespace ark::desktop::ui {
// Executes the maintained Steam plan, with measured translated labels and logical clipping.
void draw_world_facility_upgrade(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                                 const Skin &skin);
} // namespace ark::desktop::ui
