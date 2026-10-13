#pragma once

#include "ark/simulation/presentation/steam_human_skin.hpp"

namespace ark::simulation {
// Steam raw79 四行两页：只读冻结目录、NEW、售价与原属性列；确认不是购买。
std::optional<SteamHumanOverviewPlan> steam_facility_commodity_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamHumanOverviewOptions &options);
// raw72 始终显示序号和左右箭头，即便目录只有一项；与73共用装备内容绘制。
std::optional<SteamHumanOverviewPlan> steam_facility_commodity_detail_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamHumanOverviewOptions &options);

// raw80：X身份原序冻结，行姓名／职业／性别／入住费与余额资格由调用方实时投影。
struct SteamResidenceHuman {
    int identity{},profession{},sex{},fee{};
    std::string name;
    bool affordable{};
};
struct SteamResidenceCursor { int selection{},first{}; };
struct SteamResidenceOptions {
    int view_y{},finger_frame{};
    bool scroll_first_touch{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> bottom_width;
};
struct SteamResidenceInput {
    SteamResidenceCursor cursor;
    std::vector<int> order;
    std::vector<SteamResidenceHuman> humans;
    SteamResidenceOptions options;
};
// 原上后下循环，再按四行修首行；绘制仍五行。仅转换显式游标，不读写Owner。
std::optional<SteamResidenceCursor> steam_residence_cursor(
    SteamResidenceCursor cursor,int count,bool up,bool down);
// 无Owner桥、页面初始化、扣款、入住或随机；marker16热区复用现EquipmentRow载荷。
std::optional<SteamHumanOverviewPlan> steam_residence_skin(const SteamResidenceInput &);
} // namespace ark::simulation
