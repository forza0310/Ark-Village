#pragma once

#include "ark/simulation/facilities/startup_world_building.hpp"

namespace ark::simulation {
// 原MainScene1内的a/o.aa：道路1/2、撤除3/5、移动6/7；人物仍由自主AI更新。
StartupBuildResult begin_startup_world_road(StartupWorldRuntimeState &state, int definition);
StartupBuildResult begin_startup_world_edit(StartupWorldRuntimeState &state, bool move);
// 原线段选较长轴，等长取纵轴；返回真实格序（从较小坐标到较大坐标）。
std::optional<std::vector<ref::Position>>
startup_world_edit_segment(const StartupWorldRuntimeState &state, ref::Position endpoint);
StartupBuildResult confirm_startup_world_edit(StartupWorldRuntimeState &state,
                                              ref::Position position,
                                              ref::FacilityOrientation orientation);
// 2→1、5→3、7→6；起始模式返回主场景，不撤销已完成的操作。
StartupWorldRuntimeError cancel_startup_world_edit(StartupWorldRuntimeState &state);
} // namespace ark::simulation
