#pragma once

#include "dungeon_village_prototype/steam_magic_pot_skin.hpp"

namespace dungeon_village_prototype {
// Steam44—47仅表现；声音请求由Owner在恰35适配，绘制不执行PlaySe或业务提交。
struct SteamMagicPotEffectOptions {
    SteamMagicPotOptions common;
    // 45依次为旧字体数字、11号素材、消费、再次素材的实际StringWidth。
    std::optional<std::array<int,4>> report_header_widths;
    std::optional<int> comment_width; // 44只有frame>=97才读取。
    std::optional<int> reward_name_width, confirm_width; // 46/47实际奖励名称与底部按钮。
};
struct SteamMagicPotEffectInput {
    int raw{44}, frame{}, frame2{}, level{1}, processed{}, profession{}, sex{};
    std::array<std::array<int,5>,3> attributes{}; // 原mpParam[aM]：旧/新/请求量，四元素＋知识。
    std::vector<StartupVisualDraw> item_icon; // 44投入物裁片，业务已在42提交。
    std::string comment;
    int reward_kind{}, reward_definition{};
    std::string reward_name;
    std::vector<StartupVisualDraw> reward_icon; // 46/47实际结果图标，不以Mapchip2冒充。
    std::array<int,4> costs{}, elements{};
    SteamMagicPotEffectOptions options;
};
// 原countAnime的单步增量仍保留before至终段，饱和的请求量和真正差额分别使用。
std::optional<SteamMagicPotPlan> steam_magic_pot_effect_skin(const SteamMagicPotEffectInput &input);
// 只读已初始化真实栈顶；44独立frame2、冻结评语/3×5与配方绑定来自Owner。
std::optional<SteamMagicPotPlan> steam_magic_pot_effect_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamMagicPotEffectOptions &options);
} // namespace dungeon_village_prototype
