#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

// 现有持久化套件共享的纯恢复拒绝场景；输入为自然稳定前缀。
int check_startup_world_restore_contracts(
    const dungeon_village_prototype::StartupWorldRuntimeState &baseline);
