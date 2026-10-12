// Steam人物局部纯图元：ui/STEAM_HUMAN_PRESENTATION.md、STEAM_HUMAN_PINCH.md。
// raw60概览／六属性的有序绘制及输入边界见ui/STEAM_HUMAN_DETAIL.md。
#include "ark/simulation/presentation/steam_human_skin.hpp"
#include "ark/simulation/presentation/steam_facility_skin.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace ark::simulation {
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
    case Asset::portrait_background:return SteamHumanResource{39,-1,"original/common/mp_back.png",nullptr};
    case Asset::home_background:return SteamHumanResource{94,-1,"original/common/myHomeBack.png",nullptr};
    case Asset::experience_bar:return SteamHumanResource{87,-1,"original/common/wnd_expBar.png",nullptr};
    case Asset::experience_label:return SteamHumanResource{50,60,"original/common/wnd_exp.png","original/common/wnd_exp.seb"};
    case Asset::experience_remaining:return SteamHumanResource{88,-1,"steam-human-common/original/common/wnd_ato.png",nullptr};
    case Asset::smile:return SteamHumanResource{115,84,"original/common/smile00.png","original/common/smile00.seb"};
    case Asset::combat_icon:return SteamHumanResource{37,98,"steam-facility-common/original/common/icon_param00.png","original/common/icon_param00.seb"};
    case Asset::equipment_background:return SteamHumanResource{24,-1,"original/common/icon_back00.png",nullptr};
    case Asset::weapon_icon:return SteamHumanResource{12,-1,"original/common/icon_weapon00.png",nullptr};
    case Asset::armour_icon:return SteamHumanResource{20,-1,"original/common/icon_armour00.png",nullptr};
    case Asset::accessory_icon:return SteamHumanResource{21,-1,"original/common/icon_accessry00.png",nullptr};
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
namespace {
std::optional<SteamHumanOverviewPlan> human_page_skin(
    const StartupWorldRuntimeState &s,std::uint64_t page,const SteamHumanOverviewOptions &o,int tab) {
    const auto view=inspect_startup_world_human_presentation(s,page);
    if(!view||view->tab!=tab||!o.title_widths||o.finger_frame<0||o.finger_frame>5)return {};
    const auto &h=view->human;
    if(h.level<1||h.level>10||h.name.empty()||h.name.size()>4096||h.experience<0||h.threshold<0||
       h.profession<0||h.profession>=static_cast<int>(s.rules->jobs.size()))return {};
    int profession_size=0,profession_y=87;
    if(!o.japanese) {
        if(!o.profession_width||*o.profession_width<0)return {};
        profession_size=9;profession_y=88;
        if(*o.profession_width>=53) {
            if(!o.profession_repeat_width||*o.profession_repeat_width<0)return {};
            profession_size=*o.profession_repeat_width<53?8:7;profession_y=89;
        }
    }
    bool recovery{};
    if(tab==0)for(const auto &item:s.rules->items) {
        const auto stock=s.items.find(item.identity);
        if(stock==s.items.end()||stock->second.inventory<0)return {};
        if(stock->second.inventory>0&&item.recovery>0)recovery=true;
    }
    recovery=view->live&&recovery&&(view->live->state==7||view->live->hp_after<view->live->hp_max/2);
    const auto &bottom=tab==1?o.profession_change_width:o.bottom_widths[recovery?1:0];
    if(!bottom||*bottom<0)return {};
    const auto window=steam_startup_window_skin({220,175,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({17,52,219,190},o.view_y);
    const auto portrait=steam_human_detail_skin(s,page,o.down_text_width);
    if(!window||!box||!portrait)return {};
    SteamHumanOverviewPlan out;out.soft_labels={view->tracking_available?8:0,2};out.recovery_gift=recovery;
    for(const auto &draw:window->draws)out.operations.emplace_back(draw);
    for(const auto &draw:box->draws)out.operations.emplace_back(draw);
    const auto rectangle=[&](std::array<int,4> r,std::array<int,3> c,bool line=false) {
        out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{r,c,line}});
    };
    const auto label=[&](SteamHumanOverviewTextRole role,std::string value,int x,int y,int w=0,int height=0,
                         int anchor=1,int size=0,bool layout=false,int argument=0,std::array<int,3> rgb={63,44,4}) {
        out.operations.emplace_back(SteamHumanOverviewText{role,std::move(value),{x,y,w,height},anchor,size,layout,rgb,argument});
    };
    const auto picture=[&](Asset asset,int x,int y,int frame=0,std::optional<std::array<int,4>> crop={}) {
        out.operations.emplace_back(SteamHumanImage{asset,{x,y},frame,crop});
    };
    const auto number=[&](SteamFacilityAsset asset,int value,int x,int y) {
        out.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,asset,value,{x,y},0,4,-1});
    };
    label(SteamHumanOverviewTextRole::name,h.name,124,71);
    rectangle({119,84,91,25},{215,249,214});rectangle({119,109,91,13},{172,235,169});
    label(SteamHumanOverviewTextRole::profession,s.rules->jobs[h.profession].name,123,profession_y,0,0,1,profession_size);
    out.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{o.japanese?172:177,88},0,std::array<int,4>{0,0,17,10}});
    if(h.level==10)out.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::maximum,{o.japanese?191:195,90},0,std::array<int,4>{0,0,20,6}});
    else number(SteamFacilityAsset::number05,h.level,o.japanese?206:208,88);
    picture(Asset::experience_bar,123,102,0,std::array<int,4>{1,0,81,5});
    const int exp_width=h.level==10?79:h.threshold==0?0:h.experience>=h.threshold?79:
        static_cast<int>(std::int64_t(h.experience)*79/h.threshold);
    picture(Asset::experience_bar,124,103,0,std::array<int,4>{2,5,exp_width,3});
    picture(Asset::experience_label,130,112);
    picture(Asset::experience_remaining,o.japanese?148:149,o.japanese?112:113,0,std::array<int,4>{0,0,10,7});
    if(h.level!=10)number(SteamFacilityAsset::number08,h.threshold-h.experience,201,110);
    picture(Asset::portrait_background,25,69,0,std::array<int,4>{0,5,90,57});
    rectangle({25,69,90,57},{148,148,148},true);
    if(h.resident)picture(Asset::home_background,27,82,0,std::array<int,4>{0,0,47,32});
    out.operations.emplace_back(*portrait);
    if(tab==0) {
        rectangle({25,153,89,36},{193,238,247});
        label(SteamHumanOverviewTextRole::satisfaction,{},30,157);
        picture(Asset::smile,68,169);
        number(SteamFacilityAsset::number05,h.satisfaction,108,158);
        label(SteamHumanOverviewTextRole::effort,{},30,175,60,11,32,0,true);
        number(SteamFacilityAsset::number05,h.effort,108,176);
        for(int slot=0;slot<4;++slot) {
            const int x=32+18*slot;
            if(!h.equipment[slot]) {
                if(slot==0)return {};
                picture(Asset::equipment_background,x,129,0,std::array<int,4>{126,0,18,18});
            } else {
                const auto icons=startup_world_equipment_icon_draws(s,slot==0?1:slot==3?3:2,*h.equipment[slot]);
                if(!icons)return {};
                for(const auto &part:*icons) {
                    Asset asset;
                    switch(part.image) {
                    case 24:asset=Asset::equipment_background;break;
                    case 12:asset=Asset::weapon_icon;break;
                    case 20:asset=Asset::armour_icon;break;
                    case 21:asset=Asset::accessory_icon;break;
                    default:return {};
                    }
                    if(part.sprite>=0||part.resource!=StartupVisualResource::common)return {};
                    picture(asset,x+part.offset[0],129+part.offset[1],0,part.crop);
                }
            }
        }
        for(int i=0;i<4;++i) {
            const int y=123+18*i;
            rectangle({119,y,48,19},{173,209,221},true);rectangle({166,y,41,19},{173,209,221},true);
            picture(Asset::combat_icon,128,y+2,i);
            number(SteamFacilityAsset::number08,h.combat[i],203,y+5);
        }
    } else {
        for(int i=0;i<6;++i) {
            const int x=29+92*(i/3),y=134+19*(i%3);
            rectangle({x-4,y-2,93,20},{208,242,249});
            rectangle({x-4,y-2,93,20},{149,206,217},true); // 原DrawRect92×19转半开。
            // mode7裁图不是mode8/SEB98；運(index5)跳过第5列，实际取x96。
            picture(Asset::combat_icon,x,y,0,std::array<int,4>{16*(i==5?6:i),16,16,16});
            label(SteamHumanOverviewTextRole::attribute,{},x+18,y+2,0,0,1,0,false,i,{0,100,255});
            number(SteamFacilityAsset::number08,h.attributes[i],x+83,y+3);
        }
    }
    const int w=std::min(*bottom,200)+16,x=120-w/2;
    rectangle({x,202,w,16},{255,153,55});
    out.operations.emplace_back(SteamStartupTouch{4,20,std::array<int,4>{x-20,182,w+40,56},{},0});
    label(tab==1?SteamHumanOverviewTextRole::profession_change:SteamHumanOverviewTextRole::gift,{},x,202,w,16,34,0,true);
    out.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{x-4,211},o.finger_frame,{}});
    // count4、visible1，蓝块高32；页1的start=trunc(127/4)=31。
    const int top=69+(127*tab)/4;
    out.operations.emplace_back(SteamHumanOverviewScroll{{StartupSkinRect{{220,69,5,127},{7,5,78},false},
        StartupSkinRect{{220,top,5,32},{48,160,255},false}},{221,top,5,31}});
    return out;
}
} // namespace
std::optional<SteamHumanOverviewPlan> steam_human_overview_skin(
    const StartupWorldRuntimeState &s,std::uint64_t page,const SteamHumanOverviewOptions &o) {
    return human_page_skin(s,page,o,0);
}
std::optional<SteamHumanOverviewPlan> steam_human_attributes_skin(
    const StartupWorldRuntimeState &s,std::uint64_t page,const SteamHumanOverviewOptions &o) {
    return human_page_skin(s,page,o,1);
}
} // namespace ark::simulation
