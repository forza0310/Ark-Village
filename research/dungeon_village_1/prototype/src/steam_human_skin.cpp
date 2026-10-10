// Steam人物局部纯图元：ui/STEAM_HUMAN_PRESENTATION.md、STEAM_HUMAN_PINCH.md。
#include "dungeon_village_prototype/steam_human_skin.hpp"
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using Asset = SteamHumanAsset;
bool fits(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
bool image(SteamHumanSkinPlan &plan, Asset asset, std::int64_t x, std::int64_t y,
           int frame, std::optional<std::array<int,4>> crop = {}) {
    if (!fits(x) || !fits(y)) return false;
    if (crop && (!fits(x+(*crop)[2]) || !fits(y+(*crop)[3]))) return false;
    plan.draws.emplace_back(SteamHumanImage{asset,{static_cast<int>(x),static_cast<int>(y)},frame,crop});
    return true;
}
bool rect(SteamHumanSkinPlan &plan, std::int64_t x, std::int64_t y,
          int w, int h, std::array<int,3> rgb, bool outline) {
    if (!fits(x) || !fits(y) || !fits(x+w) || !fits(y+h)) return false;
    plan.draws.emplace_back(StartupSkinRect{{static_cast<int>(x),static_cast<int>(y),w,h},rgb,outline});
    return true;
}
}
std::optional<SteamHumanResource> steam_human_resource(Asset asset) {
    switch (asset) {
    case Asset::medal: return SteamHumanResource{35,-1,"original/common/icon_medal00.png",nullptr};
    case Asset::pinch: return SteamHumanResource{22,39,"original/common/ef_pinch.png","original/common/ef_pinch.seb"};
    case Asset::bubble: return SteamHumanResource{7,28,"original/common/fukidashi_back.png","original/common/fukidashi_back.seb"};
    case Asset::number05: return SteamHumanResource{103,12,"steam-build-common/original/common/number05.png","original/common/number05.seb"};
    }
    return {};
}
std::optional<SteamHumanSkinPlan> steam_human_power_skin(
    std::array<int,2> position, int direction, int frame, std::optional<SteamHumanHpInput> live) {
    if (direction<0 || direction>3 || frame<0) return {};
    SteamHumanSkinPlan plan;
    if (!live) return plan;
    if (live->maximum<=0) return {};
    // 原RateConvert先检查端点，超上限的显示HP直接满条，不执行可能溢出的乘法。
    int width{};
    if (live->now>=live->maximum) width=18;
    else if (live->now>0) {
        const auto numerator=std::int64_t(live->now)*18;
        if (!fits(numerator)) return {};
        width=static_cast<int>(numerator/live->maximum);
    }
    const std::int64_t x=position[0], y=position[1];
    // DrawRect原21×5／19×3经Steam helper转换为半开22×6／20×4；FillRect不加1。
    if (!rect(plan,x-11,y-26,22,6,{246,246,246},true) ||
        !rect(plan,x-10,y-25,20,4,{39,53,74},true) ||
        !rect(plan,x-9,y-24,width,2,{83,255,0},false) ||
        !rect(plan,x-9+width,y-24,18-width,2,{68,100,104},false)) return {};
    if ((frame%12)/6==0) {
        const auto danger=std::int64_t(live->after)*100;
        if (!fits(danger)) return {}; // 仅原分支确实求百分比时检查int32乘法。
        if (danger/live->maximum<=30) {
            const int offset=(direction==2 || direction==3)?5:-14;
            if (!image(plan,Asset::pinch,x+offset,y-28,0)) return {};
        }
    }
    return plan;
}
std::optional<SteamHumanSkinPlan> steam_human_medal_skin(int medals, std::array<int,2> position) {
    if (medals<0) return {};
    SteamHumanSkinPlan plan;
    const std::int64_t x=position[0], y=position[1];
    // UserData.Draw_icon RVA30C350: mode6分支VA1030C7FB–1030C88F，
    // image35，源x=14*(index%10)、y=0、14×14，目标锚不偏移。index1即源x14。
    const auto medal=[&](std::int64_t left) { return image(plan,Asset::medal,left,y,0,std::array<int,4>{14,0,14,14}); };
    if (medals<5) {
        for (int i=0;i<medals;++i) if (!medal(x+20-10*i)) return {};
        return plan;
    }
    int digits=1;
    for (int remaining=medals/10;remaining>0;remaining/=10) ++digits;
    const auto right=x+30;
    const int width=8*digits;
    if (!fits(right) || !fits(y+4) || !medal(right-width-22) ||
        !image(plan,Asset::number05,right-width-10,y+4,16)) return {};
    const SteamFacilityNumber number{SteamFacilityNumberKind::number,SteamFacilityAsset::number05,
        medals,{static_cast<int>(right),static_cast<int>(y+4)},0,4,-1};
    const auto numbers=steam_facility_number_draws(number,8);
    if (!numbers) return {};
    for (const auto &n:*numbers)
        if (!image(plan,Asset::number05,n.position[0],n.position[1],n.frame,n.crop)) return {};
    return plan;
}
std::optional<SteamHumanSkinPlan> steam_human_down_bubble_skin(int width, std::array<int,2> position) {
    if (width<0 || width>4096) return {};
    SteamHumanSkinPlan plan;
    const std::int64_t x=position[0], y=position[1], half=width/2, left=x-half;
    if (width<=60) {
        if (!image(plan,Asset::bubble,left,y,0,std::array<int,4>{36-static_cast<int>(half),0,width,21})) return {};
    } else {
        for (int used=0;used<width;) {
            const int w=std::min(27,width-used);
            if (!image(plan,Asset::bubble,left+used,y,0,std::array<int,4>{6,0,w,21})) return {};
            used+=w;
        }
        if (!image(plan,Asset::bubble,x-2,y,0,std::array<int,4>{33,0,5,21})) return {};
    }
    if (!image(plan,Asset::bubble,left-6,y,0) || !image(plan,Asset::bubble,x+half,y,1) || !fits(y+3)) return {};
    plan.draws.emplace_back(SteamHumanText{SteamHumanTextRole::down,{position[0],static_cast<int>(y+3)},2,{0,0,0}});
    return plan;
}
std::optional<SteamHumanDetailSkin> steam_human_detail_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page, std::optional<int> down_text_width) {
    const auto view=inspect_startup_world_human_presentation(state,page);
    if (!view) return {};
    const auto medals=steam_human_medal_skin(view->human.medals);
    if (!medals) return {};
    SteamHumanDetailSkin result;
    result.medals=*medals;
    if (view->live && view->live->state==7) {
        if (!down_text_width) return {};
        auto actor=startup_title_actor_skin(view->human.profession,view->human.sex,-1,0,1,false);
        const auto bubble=steam_human_down_bubble_skin(*down_text_width);
        if (!actor || !bubble) return {};
        // Steam action7: HUMAN_ANIME_SEB[7]=12，direction1；F[7][0]=0，body_off[7]=0。
        actor->body.sprite=13;actor->body.frame=0;
        result.portrait_position={75,121};result.portrait=*actor;result.after_portrait=*bubble;
        return result;
    }
    constexpr std::array<std::array<int,4>,6> movement{{
        {{1,75,75,10}},{{1,75,98,30}},{{1,98,98,10}},
        {{2,98,98,10}},{{2,98,75,30}},{{2,75,75,10}}
    }};
    int time=view->frame%100;
    int direction=1,x=75;
    for (const auto &segment:movement) {
        if (time<segment[3]) {
            direction=segment[0];x=segment[1]+(segment[2]-segment[1])*time/segment[3];
            break;
        }
        time-=segment[3];
    }
    if (!view->human.resident) x-=12;
    result.portrait_position={x,121};
    // GetAnimeIndex(0,完整frame)阈值[4,8,12,16]，与100槽位移周期独立。
    const auto actor=startup_title_actor_skin(view->human.profession,view->human.sex,
        view->human.equipment[0].value_or(-1),(view->frame%16)/4,direction,false);
    std::optional<SteamHumanHpInput> live;
    if (view->live) live=SteamHumanHpInput{view->live->hp_now,view->live->hp_after,view->live->hp_max};
    const auto power=steam_human_power_skin(result.portrait_position,direction,view->frame,live);
    if (!actor || !power) return {};
    result.portrait=*actor;result.after_portrait=*power;
    return result;
}
} // namespace dungeon_village_prototype
