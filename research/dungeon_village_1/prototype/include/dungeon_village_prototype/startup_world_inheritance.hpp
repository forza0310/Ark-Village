#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace dungeon_village_prototype {
struct StartupWorldRuntimeState;
// J.e[0]/e[1]：设施定义序G、职业定义序p，均是大端有符号short。
// 空段跳过；非空段须恰为完整定义数×2。仅在私有新局候选安装，整批失败无部分修改。
// 维护强校验G1..5、p0..1；原setter不显式拒坏值，差异不得写成原校验。
bool install_startup_world_inheritance(
    StartupWorldRuntimeState &candidate,
    const std::array<std::vector<std::uint8_t>, 2> &sections);
} // namespace dungeon_village_prototype
