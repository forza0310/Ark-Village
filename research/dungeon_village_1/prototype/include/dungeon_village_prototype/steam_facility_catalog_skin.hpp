#pragma once

#include "dungeon_village_prototype/steam_human_skin.hpp"

namespace dungeon_village_prototype {
// Steam raw79 四行两页：只读冻结目录、NEW、售价与原属性列；确认不是购买。
std::optional<SteamHumanOverviewPlan> steam_facility_commodity_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamHumanOverviewOptions &options);
// raw72 始终显示序号和左右箭头，即便目录只有一项；与73共用装备内容绘制。
std::optional<SteamHumanOverviewPlan> steam_facility_commodity_detail_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamHumanOverviewOptions &options);
} // namespace dungeon_village_prototype
