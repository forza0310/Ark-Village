// Steam81只读皮肤：具名局部合同见ui/STEAM_FACILITY_UPGRADE.md。
#include "ark/simulation/presentation/steam_facility_skin.hpp"
#include "ark/simulation/world/startup.hpp"
#include "ark/simulation/map/startup_world_projection.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/map/rules/geometry.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace ark::simulation {
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
    case Asset::number03:return SteamFacilityResource{"common",102,11,"original/common/number03.png","original/common/number04.seb"};
    case Asset::number11:return SteamFacilityResource{"common",108,20,"original/common/number11.png","original/common/number13.seb"};
    }
    return {};
}
std::optional<std::vector<SteamFacilityImage>> steam_facility_number_draws(
    const SteamFacilityNumber &n,int digit_width) {
    const bool roster_number=n.asset==Asset::number03||n.asset==Asset::number11;
    if(!roster_number&&n.asset!=Asset::number05&&n.asset!=Asset::number08&&n.asset!=Asset::number09)return {};
    // 两套35专用SEB只有数字0..9；不借它们输出money/plus所需的单位、逗号帧。
    if(roster_number&&(n.kind!=SteamFacilityNumberKind::number||
        digit_width!=(n.asset==Asset::number03?8:7)))return {};
    if(n.padding<0)return {}; // 维护输入范围；当前74/81只传0，不推断负padding调用资格。
    std::vector<SteamFacilityImage> result;
    const auto emit=[&](std::int64_t x,int frame) {
        if(!fits(x))return false;
        result.push_back({n.asset,{static_cast<int>(x),n.position[1]},frame,{}});
        return true;
    };
    std::int64_t x=n.position[0];
    if(n.kind==SteamFacilityNumberKind::number) {
        if(digit_width<=0||n.anchor<0)return {};
        int digits=1;
        for(auto rest=n.value/10;rest>0;rest/=10)++digits;
        const auto step=std::int64_t(digit_width)+n.padding;
        const auto width=digits*step-n.padding;
        if(!fits(step)||!fits(width))return {};
        if(n.anchor&2)x-=width/2;
        else if(n.anchor&4)x-=width;
        if(n.value<0) {
            // 原普通数字负值位数保持1，直接把负商交SEB，不改成abs+负号。
            if(!emit(x,n.value))return {};
        } else {
            const auto decimal=std::to_string(n.value);
            for(const char c:decimal) {
                if(!emit(x,c-'0'))return {};
                x+=step;
            }
        }
        return result;
    }
    if((n.kind!=SteamFacilityNumberKind::money&&n.kind!=SteamFacilityNumberKind::plus_value)||
       n.padding!=0)return {};
    // 两个实际helper都传padding0/comma_padding0/anchor4；signed串长度包含负号。
    if(n.kind==SteamFacilityNumberKind::money)x-=9;
    if(!fits(x))return {};
    const auto right=x;
    const auto decimal=std::to_string(n.value);
    x-=static_cast<std::int64_t>(decimal.size())*8;
    const auto first_digit=n.value<0?std::size_t(1):std::size_t(0);
    for(std::size_t i=0;i<decimal.size();++i) {
        if(!emit(x,decimal[i]-'0'))return {};
        // CommaSeparate的逗号在下一数字左侧，原顺序先画数字再画逗号。
        if(i>first_digit&&(decimal.size()-i)%3==0)
            if(!emit(x-2,10))return {};
        x+=8;
    }
    if(n.kind==SteamFacilityNumberKind::money) {
        if(!emit(right,20))return {};
    } else {
        const auto absolute_digits=decimal.size()-first_digit;
        if(!emit(right-static_cast<std::int64_t>(absolute_digits+1)*8,14))return {};
    }
    return result;
}
std::optional<std::vector<SteamFacilityMapchipDraw>> steam_facility_mapchip2_draws(
    const SteamFacilityMapchip2 &request) {
    if(request.mapchip<0||(request.orientation!=0&&request.orientation!=1))return {};
    const auto &evidence=startup_evidence();
    const auto art=std::find_if(evidence.displays.begin(),evidence.displays.end(),
        [&](const auto &d){return d.id==request.mapchip;});
    if(art==evidence.displays.end()||art->sprite.empty())return {};
    const auto &definitions=startup_world_rules().facilities;
    const auto definition=std::find_if(definitions.begin(),definitions.end(),
        [&](const auto &d){return d.id==art->definition_id;});
    if(definition==definitions.end()||definition->display_id!=request.mapchip||
       definition->shape<0||definition->shape>2)return {};
    const auto pieces=ark::simulation::rules::facility_footprint(
        static_cast<ark::simulation::rules::FacilityShape>(definition->shape),
        static_cast<ark::simulation::rules::FacilityOrientation>(request.orientation),{1,1},3,3);
    if(pieces.error!=ark::simulation::rules::GeometryError::none)return {};
    constexpr std::array<std::array<int,2>,3> centers{{{-30,0},{-45,7},{-30,15}}};
    const auto center=centers[definition->shape];
    std::vector<SteamFacilityMapchipDraw> result;
    for(const auto &piece:pieces.cells) {
        const auto u=piece.position.x-1,v=piece.position.y-1;
        const auto x=std::int64_t(request.position[0])+center[0]+30*(u+v);
        const auto y=std::int64_t(request.position[1])+center[1]+15*(u-v);
        if(!fits(x)||!fits(y))return {};
        // DrawMapchip2没有普通DrawMapchip的道路kind6帧11/1覆盖，直接取pattern帧。
        result.push_back({art->sprite,piece.fragment_index,{static_cast<int>(x),static_cast<int>(y)}});
    }
    return result;
}
std::optional<SteamFacilitySkinPlan> steam_facility_upgrade_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamFacilityUpgradeSkinOptions &options) {
    const auto view=inspect_startup_world_facility_upgrade(state,page);
    if(!view)return {};
    SteamFacilityUpgradeSkinInput in;
    in.definition=view->definition;in.mapchip=view->mapchip;in.level=view->level;
    in.phase=view->phase;in.frame=view->frame;in.frame2=view->frame2;
    in.view_y=options.view_y;in.japanese=options.japanese;
    in.title_widths=options.title_widths;in.notice_widths=options.notice_widths;
    for(std::size_t slot=0;slot<3;++slot) {
        if(!fits(view->limits[slot]))return {};
        in.limits[slot]=static_cast<int>(view->limits[slot]);
        // Owner的原o.ap按[前/后/差][属性]保存，皮肤输入按[属性][前/后/差]读取。
        for(std::size_t value=0;value<3;++value) {
            if(!fits(view->attributes[value][slot]))return {};
            in.attributes[slot][value]=static_cast<int>(view->attributes[value][slot]);
        }
    }
    return steam_facility_upgrade_skin(in);
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
} // namespace ark::simulation
