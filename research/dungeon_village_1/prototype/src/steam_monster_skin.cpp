// Steam SubForm._draw2_4 VA1034AFD2–1034B395与APK b/g raw89的独立只读表达。
#include "dungeon_village_prototype/steam_monster_skin.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const SteamMonsterIntroductionInput &in,const SteamMonsterSkinOptions &o) {
    constexpr std::array<int,4> images{9,12,8,6},speeds{6,6,8,10};
    if(in.counter<0||in.counter==std::numeric_limits<int>::max()||in.body<0||in.body>3||
       in.variant<0||in.variant>=images[in.body]||in.shadow<0||in.shadow>3||
       in.name.empty()||in.name.size()>4096||!o.title_widths)return {};
    const auto outer=steam_startup_window_skin(
        {222,170,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto inner=steam_startup_box_skin({15,139,224,202},o.view_y);
    if(!outer||!inner)return {};
    SteamMonsterSkinPlan p;p.title=o.japanese?"新種ﾓﾝｽﾀｰ":"新型怪物";
    for(const auto &draw:outer->draws)p.operations.emplace_back(draw);
    for(const auto &draw:inner->draws)p.operations.emplace_back(draw);
    for(const auto &part:std::array<std::array<int,6>,3>{{
        {30,67,0,0,93,82},{109,67,1,0,91,82},{189,67,1,0,20,82}}})
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::residence_background,
            {part[0],part[1]},0,std::array<int,4>{part[2],part[3],part[4],part[5]}});
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,67,179,81},{148,148,148},true}});
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::shadow,{120,133},in.shadow,{}});
    p.operations.emplace_back(SteamMonsterBody{in.body*30+in.variant,in.body*4+2,
        (in.counter%(speeds[in.body]*4))/speeds[in.body],{120,132}});
    // 原RateConvert截限后进行整数乘除；主钟不受纯绘制推进。
    const int height=std::clamp(in.counter-5,0,35)*48/35;
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{0,158,240,height}});
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        in.name+(o.japanese?"が":""),{120,165,0,0},2,0,false,{92,51,31}});
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        o.japanese?"出現しました!!":"出现了!!",{120,187,0,0},2,0,false,{92,51,31}});
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
    if(in.counter>=40&&(in.counter-40)%25<15)
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
    // 原Touch2没有显式矩形，平台不得据此杜撰全屏点击区域。
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return p;
}
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamMonsterSkinOptions &o) {
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
        [](const auto &p){return p.lifecycle!=4;});
    if(page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||
       !valid_startup_world_monster_intro_page(s,*page))return {};
    const auto d=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),
        [&](const auto &m){return m.identity==*page->monster_definition;});
    if(d==s.rules->monsters.end())return {};
    return steam_monster_introduction_skin(
        {s.page_counters.at(id),d->initial.body,d->initial.sprite_variant,d->shadow,d->name},o);
}
} // namespace dungeon_village_prototype
