#pragma once

#include "ark/simulation/startup_world_runtime.hpp"

namespace ark::simulation {
// 原c/h.f→g→c→d：扩张固定地图的村界，重建入口并恢复新村界内的人物。
// 自身使用候选Owner；任何缺失字段/刷新失败均不提交。已到最高合法级别成功且无修改。
bool expand_startup_world_map(StartupWorldRuntimeState &state);
} // namespace ark::simulation
