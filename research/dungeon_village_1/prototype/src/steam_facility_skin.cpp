// Steam81只读皮肤：具名局部合同见ui/STEAM_FACILITY_UPGRADE.md。
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using Asset=SteamFacilityAsset;
using Role=SteamFacilityTextRole;
constexpr std::array<int,3> black{92,51,31},blue{0,100,255};
bool fits(std::int64_t n) { return n>=std::numeric_limits<int>::min()&&n<=std::numeric_limits<int>::max(); }
// 显式float32每步舍入，避免编译器FMA融合把源mulss/addss改成单次舍入。
float product(float a,float b) { volatile float value=a*b; return value; }
float sum(float a,float b) { volatile float value=a+b; return value; }
float quotient(float a,float b) { volatile float value=a/b; return value; }
int parabola(int distance,int time,int tick) {
    const int half=time/2;
    const float velocity=quotient(sum(float(distance),float(distance)),float(half-1));
    const float acceleration=quotient(product(float(distance),-2.F),float(half*(half-1)));
    const float linear=product(float(tick),velocity);
    const float curved=product(product(product(float(tick),acceleration),float(tick+1)),0.5F);
    return static_cast<int>(std::max(0.F,sum(curved,linear)));
}
std::optional<int> count_value(int frame,int old,int next) {
    if(frame<34)return old;
    if(frame>=49)return next;
    if(old==next)return old;
    const auto first=std::int64_t(old)+(next>old?1:-1);
    const auto remaining=std::int64_t(next)-first;
    if(!remaining)return old;
    const auto proposed=34+3*remaining;
    if(!fits(proposed))return {};
    const auto end=std::clamp(proposed,std::int64_t(34),std::int64_t(49));
    if(end==34)return old;
    if(frame>end)return next;
    const auto numerator=remaining*(frame-34);
    if(!fits(numerator)||!fits(first+numerator/(end-34)))return {};
    return static_cast<int>(first+numerator/(end-34));
}
void image(SteamFacilitySkinPlan &p,Asset asset,int x,int y,int frame=0,
           std::optional<std::array<int,4>> crop={}) {
    p.draws.emplace_back(SteamFacilityImage{asset,{x,y},frame,crop});
}
void text(SteamFacilitySkinPlan &p,Role role,int argument,int x,int y,std::optional<int> anchor,
          bool japanese,int size=0,std::array<int,3> color=black) {
    p.draws.emplace_back(SteamFacilityText{role,argument,{x,y,0,0},anchor,color,size,japanese});
}
void number(SteamFacilitySkinPlan &p,SteamFacilityNumberKind kind,Asset asset,int value,
            int x,int y,int anchor,int parameter) {
    p.draws.emplace_back(SteamFacilityNumber{kind,asset,value,{x,y},0,anchor,parameter});
}
void clip(SteamFacilitySkinPlan &p,std::array<int,4> rect) {
    p.draws.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,rect});
}
void pop(SteamFacilitySkinPlan &p) {p.draws.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});}
bool frame_parts(SteamFacilitySkinPlan &p,const SteamStartupFramePlan &frame,bool japanese) {
    for(const auto &part:frame.draws) {
        if(const auto *rect=std::get_if<StartupSkinRect>(&part))p.draws.emplace_back(*rect);
        else if(const auto *draw=std::get_if<StartupSkinDraw>(&part)) {
            const Asset asset=draw->image==28?Asset::wood:draw->image==29?Asset::title_bar:Asset::corner;
            if(draw->image!=28&&draw->image!=29&&draw->image!=30)return false;
            image(p,asset,draw->offset[0],draw->offset[1],draw->frame,
                  draw->sprite<0?std::optional<std::array<int,4>>(draw->crop):std::nullopt);
        } else if(const auto *label=std::get_if<SteamStartupText>(&part)) {
            if(!label->rgb)return false;
            p.draws.emplace_back(SteamFacilityText{Role::upgrade_title,0,
                {static_cast<int>(label->rectangle[0]),static_cast<int>(label->rectangle[1]),0,0},
                label->anchor,*label->rgb,0,japanese});
        }
    }
    return true;
}
}
std::optional<SteamFacilityResource> steam_facility_resource(Asset asset) {
    switch(asset) {
    case Asset::wood:return SteamFacilityResource{"common",28,-1,"original/common/wnd_back.png",nullptr};
    case Asset::title_bar:return SteamFacilityResource{"common",29,-1,"original/common/wnd_bar.png",nullptr};
    case Asset::corner:return SteamFacilityResource{"common",30,6,"original/common/wnd_conner.png","original/common/wnd_conner.seb"};
    case Asset::arrow:return SteamFacilityResource{"common",72,2,"original/common/arrow01.png","original/common/arrow01.seb"};
    case Asset::mini:return SteamFacilityResource{"common",93,80,"original/common/chara_mini.png","original/common/chara_mini.seb"};
    case Asset::number05:return SteamFacilityResource{"common",103,12,"steam-build-common/original/common/number05.png","original/common/number05.seb"};
    case Asset::number08:return SteamFacilityResource{"common",105,15,"steam-facility-common/original/common/number08.png","original/common/number08.seb"};
    case Asset::number09:return SteamFacilityResource{"common",106,16,"original/common/number09.png","original/common/number09.seb"};
    case Asset::maximum:return SteamFacilityResource{"common",129,-1,"original/common/wnd_max.png",nullptr};
    case Asset::upgrade_background:return SteamFacilityResource{"event",14,-1,"original/event/event_getItem_back.png",nullptr};
    case Asset::mini_background:return SteamFacilityResource{"event",16,-1,"original/event/event_BackMini02.png",nullptr};
    }
    return {};
}
std::optional<SteamFacilitySkinPlan> steam_facility_upgrade_skin(const SteamFacilityUpgradeSkinInput &in) {
    if(in.definition<0||in.mapchip<0||in.level<1||in.level>5||in.phase<0||in.phase>1||
       in.frame<0||in.frame2<0||!in.title_widths||
       (!in.japanese&&in.phase==0&&!in.notice_widths))return {};
    for(const auto &a:in.attributes)
        if(std::int64_t(a[1])-a[0]!=a[2])return {};
    if(in.notice_widths&&std::any_of(in.notice_widths->begin(),in.notice_widths->end(),[](int n){return n<0;}))return {};
    const auto window=steam_startup_window_skin({222,170,0,0,SteamStartupTextRole::message_title,0},in.view_y,in.title_widths);
    const auto box=steam_startup_box_skin({15,139,224,202},in.view_y);
    if(!window||!box)return {};
    SteamFacilitySkinPlan plan;
    if(!frame_parts(plan,*window,in.japanese)||!frame_parts(plan,*box,in.japanese))return {};
    if(in.phase==0) {
        const int height=in.frame<=5?0:in.frame>=40?48:(in.frame-5)*48/35;
        clip(plan,{0,158,240,height});
        if(in.japanese) {
            text(plan,Role::facility_notice,in.definition,120,158,2,true);
            text(plan,Role::level_prefix,0,85,176,1,true);
            number(plan,SteamFacilityNumberKind::number,Asset::number09,in.level,130,175,2,-1);
            text(plan,Role::level_suffix,0,141,176,1,true);
            text(plan,Role::level_completed,0,120,194,2,true);
        } else {
            const auto &w=*in.notice_widths;
            const auto scaled=10LL*w[0];
            const auto span=std::int64_t(w[1])+w[2]+scaled/6+7;
            const auto half=span/2,level_x=124LL+w[3]-half;
            if(!fits(scaled)||!fits(span)||!fits(120-half)||!fits(120+half)||!fits(level_x))return {};
            text(plan,Role::facility_notice,in.definition,120,166,2,false);
            text(plan,Role::level_prefix,0,static_cast<int>(120-half),184,1,false);
            number(plan,SteamFacilityNumberKind::number,Asset::number09,in.level,static_cast<int>(level_x),183,1,-1);
            text(plan,Role::level_suffix,0,static_cast<int>(120+half),184,4,false);
        }
        pop(plan);
        if(in.frame>=50&&in.frame%25<15)image(plan,Asset::arrow,212,204);
    }
    // Draw_tenantStrength(-1,false)在phase0仅绘背景；phase1才读冻结三属性。
    image(plan,Asset::upgrade_background,30,68,0,std::array<int,4>{0,0,180,80});
    image(plan,Asset::mini_background,77,71,0,std::array<int,4>{0,0,86,70});
    clip(plan,{80,74,80,64});
    plan.draws.emplace_back(SteamFacilityMapchip2{in.mapchip,{120,105},0});
    pop(plan);
    if(in.phase==1) {
        for(int slot=0;slot<3;++slot) {
            const int y=158+18*slot;
            const auto &a=in.attributes[slot];
            const auto value=count_value(in.frame,a[0],a[1]);
            if(!value)return {};
            text(plan,Role::parameter_name,slot,in.japanese?48:39,y+(in.japanese?0:1),{},
                 in.japanese,in.japanese?0:9,blue);
            // Steam原顺序：标签→MAX（若合格）→主值→差值及货币字。
            if(*value>=in.limits[slot])image(plan,Asset::maximum,72,y+3,0,std::array<int,4>{0,0,20,6});
            number(plan,slot==0?SteamFacilityNumberKind::money:SteamFacilityNumberKind::number,
                   Asset::number08,*value,134,y,slot==0?0:4,slot);
            const int local=in.frame-2-6*slot;
            if(a[2]!=0&&in.frame>=2&&(in.frame>=32||local>=0)) {
                const int delta_y=y-(in.frame<32?parabola(8,12,local):0);
                number(plan,SteamFacilityNumberKind::plus_value,Asset::number05,a[2],slot==0?190:198,delta_y,0,slot);
                if(slot==0)image(plan,Asset::number05,190,delta_y,20);
            }
        }
        if(in.frame>=55&&(in.frame-55)%25<15)image(plan,Asset::arrow,212,204);
    }
    const int t=in.frame2%30,jumping=t<20?1:0,h=jumping?parabola(6,20,t):0;
    image(plan,Asset::mini,52,144,jumping);
    image(plan,Asset::mini,52,144-h,jumping?5:3);
    image(plan,Asset::mini,188,144,jumping);
    image(plan,Asset::mini,188,144-h,jumping?4:2);
    plan.touches.push_back({2,0,{},{},2});
    return plan;
}
} // namespace dungeon_village_prototype
