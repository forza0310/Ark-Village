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
    case Asset::number12:return SteamFacilityResource{"common",109,19,"steam-common/number12.png","original/common/number12.seb"};
    case Asset::number03_hud:return SteamFacilityResource{"common",102,10,"original/common/number03.png","original/common/number03.seb"};
    case Asset::facility_icon:return SteamFacilityResource{"common",91,-1,"original/common/icon_tenantInfo.png",nullptr};
    case Asset::effect_icon:return SteamFacilityResource{"common",37,-1,"steam-facility-common/original/common/icon_param00.png",nullptr};
    case Asset::level_label:return SteamFacilityResource{"common",38,-1,"original/common/wnd_lv.png",nullptr};
    case Asset::finger:return SteamFacilityResource{"common",70,21,"original/common/finger_r.png","original/common/finger_r.seb"};
    case Asset::title_arrows:return SteamFacilityResource{"common",74,3,"original/common/arrow02.png","original/common/arrow02.seb"};
    case Asset::people:return SteamFacilityResource{"common",178,-1,"original/common/ppl.png",nullptr};
    }
    return {};
}
std::optional<std::vector<SteamFacilityImage>> steam_facility_number_draws(
    const SteamFacilityNumber &n,int digit_width) {
    const bool roster_number=n.asset==Asset::number03||n.asset==Asset::number11;
    const bool comma=n.kind==SteamFacilityNumberKind::comma_number;
    if(!roster_number&&n.asset!=Asset::number05&&n.asset!=Asset::number08&&n.asset!=Asset::number09&&
       n.asset!=Asset::number12&&n.asset!=Asset::number03_hud)return {};
    if(comma&&((n.asset!=Asset::number05&&n.asset!=Asset::number11)||digit_width!=8||
               n.padding!=0||(n.anchor!=2&&n.anchor!=4)))return {};
    if(n.asset==Asset::number12&&(n.kind==SteamFacilityNumberKind::plus_value||digit_width!=8))return {};
    if(n.kind==SteamFacilityNumberKind::minus_value&&
       ((n.asset!=Asset::number05&&n.asset!=Asset::number12)||n.value<0||digit_width!=8))return {};
    // 普通SEB10/11/20不输出money/plus单位；SEB20的Comma缺帧仍保留原请求。
    if(n.asset==Asset::number03_hud&&(n.kind!=SteamFacilityNumberKind::number||digit_width!=8))return {};
    if(roster_number&&!comma&&(n.kind!=SteamFacilityNumberKind::number||
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
    if((!comma&&n.kind!=SteamFacilityNumberKind::money&&n.kind!=SteamFacilityNumberKind::plus_value&&
        n.kind!=SteamFacilityNumberKind::minus_value)||
       n.padding!=0)return {};
    // money/plus固定右锚；独立Comma另接受居中锚。signed串长度均包含负号，逗号占宽0。
    if(n.kind==SteamFacilityNumberKind::money)x-=9;
    if(!fits(x))return {};
    const auto right=x;
    const auto decimal=std::to_string(n.value);
    // Comma原比较anchor==2/4，不复用普通数字的位标志；本维护入口只开放这两种已核锚。
    x-=static_cast<std::int64_t>(decimal.size())*(comma&&n.anchor==2?4:8);
    const auto first_digit=n.value<0?std::size_t(1):std::size_t(0);
    for(std::size_t i=0;i<decimal.size();++i) {
        if(!emit(x,decimal[i]-'0'))return {};
        // CommaSeparate的逗号在下一数字左侧，原顺序先画数字再画逗号。
        if(i>first_digit&&(decimal.size()-i)%3==0)
            if(!emit(x-2,10))return {};
        x+=8;
    }
    if(comma)return result; // 原Comma本身不扣money的dx9，也不画单位/加号。
    if(n.kind==SteamFacilityNumberKind::money) {
        if(!emit(right,20))return {};
    } else {
        const auto absolute_digits=decimal.size()-first_digit;
        if(!emit(right-static_cast<std::int64_t>(absolute_digits+1)*8,
                 n.kind==SteamFacilityNumberKind::minus_value?15:14))return {};
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
std::optional<SteamFacilityScrollSkinPlan> steam_facility_scroll_skin(
    const SteamFacilityScrollRequest &r,int first,bool first_touch) {
    if(r.rectangle!=std::array<int,4>{221,85,3,110}||r.count<0||r.count>65536||
       r.visible!=5||r.prefix!=0x20000||r.margin!=std::array<int,4>{3,20,0,0}||
       first<0||first>std::max(r.count-r.visible,0))return {};
    // GameView._addTouch(12)→DrawVerticalScroll(type0)：先保留短表分母，再截断。
    const int denominator=std::max(r.count,r.visible);
    const int top=85+static_cast<int>(110LL*first/denominator);
    const int length=static_cast<int>(550LL/denominator);
    SteamFacilityScrollSkinPlan out;
    out.rectangles={StartupSkinRect{{220,85,5,110},{7,5,78},false},
        StartupSkinRect{{220,top,5,length+1},first_touch&&r.count>5
            ?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false}};
    out.thumb_touch={221,top,5,length};
    return out;
}
std::optional<SteamFacilityBonusCursor> steam_facility_bonus_cursor(
    int count,SteamFacilityBonusCursor c,bool up,bool down) {
    if(count<0||count>65536||c.selection<0||c.first<0)return {};
    if(!count)return c.selection==0&&c.first==0?std::optional<SteamFacilityBonusCursor>(c):std::nullopt;
    if(c.selection>=count||c.first>std::max(count-5,0)||
       c.selection<c.first||c.selection>=c.first+5)return {};
    // 原两次repeat依序各自修改选择；只在需要时移动视窗，不在切页时复位。
    const auto follow=[&] {
        if(c.selection<c.first)c.first=c.selection;
        if(c.selection>=c.first+5)c.first=c.selection-4;
    };
    if(up)c.selection=(c.selection+count-1)%count;
    if(down)c.selection=(c.selection+1)%count;
    follow();
    return c;
}
std::optional<SteamFacilityBonusCursor> steam_facility_bonus_scroll(
    int count,SteamFacilityBonusCursor c,SteamFacilityScrollEvent event,int value) {
    if(!steam_facility_bonus_cursor(count,c,false,false)||value==std::numeric_limits<int>::max()||
       (event!=SteamFacilityScrollEvent::relative&&event!=SteamFacilityScrollEvent::absolute))return {};
    const auto wanted=event==SteamFacilityScrollEvent::relative
        ?static_cast<long long>(c.first)+value:static_cast<long long>(value);
    const int first=static_cast<int>(std::clamp(wanted,0LL,static_cast<long long>(std::max(count-5,0))));
    const int delta=first-c.first;
    if(delta) {
        c.first=first;
        c.selection=std::clamp(c.selection+delta,0,count-1);
    }
    return c;
}
std::optional<SteamFacilityDetailSkinPlan> steam_facility_detail_skin(
    const SteamFacilityDetailSkinInput &in) {
    using DR=SteamFacilityDetailTextRole;
    using TM=SteamFacilityTextMethod;
    if(in.definition<0||in.mapchip<0||in.icon<0||in.icon>6||in.level<1||in.level>5||
       in.page<0||in.page>1||in.frame<0||in.finger_frame<0||in.finger_frame>5||
       in.name.empty()||in.name.size()>4096||in.bonuses.size()>65536||
       in.effects.size()>4096||!in.title_widths||!in.bottom_width||*in.bottom_width<0)return {};
    const auto cursor=steam_facility_bonus_cursor(static_cast<int>(in.bonuses.size()),in.cursor,false,false);
    if(!cursor)return {};
    const auto &art=startup_evidence().displays;
    const auto mapchip=std::find_if(art.begin(),art.end(),[&](const auto &a){return a.id==in.mapchip;});
    if(mapchip==art.end()||mapchip->definition_id!=in.definition)return {};
    // 隐藏行同样核载荷，不能通过滚动把坏源引用隐藏成合法空列表。
    for(const auto &r:in.bonuses)
        if(r.definition<0||r.ordinal<0||r.ordinal==std::numeric_limits<int>::max()||
           r.icon<0||r.icon>6||r.name.empty()||r.name.size()>4096||
           r.values.size()!=(r.kind==2?2U:1U))return {};
    std::size_t budget=in.effects.size();
    for(const auto &e:in.effects) {
        if(e[0]<0||e[0]>5)return {};
        if(e[1]>0)budget+=static_cast<std::size_t>(e[1]);
        if(budget>4096)return {}; // 维护短寿命输出预算，非原加号或效果规则上限。
    }
    SteamStartupWindow window{220,168,0,0,SteamStartupTextRole::slot_title,in.page+1};
    const auto frame=steam_startup_window_skin(window,in.view_y,in.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},in.view_y);
    if(!frame||!box)return {};
    SteamFacilityDetailSkinPlan out;
    const auto draw=[&](auto value){out.operations.emplace_back(SteamFacilityDraw{value});};
    const auto label=[&](DR role,int arg,std::string value,int x,int y,int w,int h,
                         std::optional<int> anchor,int size,TM method,std::array<int,3> rgb=black) {
        out.operations.emplace_back(SteamFacilityDetailText{role,arg,std::move(value),{x,y,w,h},anchor,size,method,rgb});
    };
    const auto frame_ops=[&](const SteamStartupFramePlan &parts) {
        for(const auto &v:parts.draws) {
            if(const auto *r=std::get_if<StartupSkinRect>(&v))draw(*r);
            else if(const auto *i=std::get_if<StartupSkinDraw>(&v)) {
                if(i->image!=28&&i->image!=29&&i->image!=30)return false;
                draw(SteamFacilityImage{i->image==28?Asset::wood:i->image==29?Asset::title_bar:Asset::corner,
                    i->offset,i->frame,i->sprite<0?std::optional<std::array<int,4>>(i->crop):std::nullopt});
            } else if(const auto *t=std::get_if<SteamStartupText>(&v)) {
                if(!t->rgb)return false;
                label(DR::title,in.page+1,{},static_cast<int>(t->rectangle[0]),
                      static_cast<int>(t->rectangle[1]),0,0,t->anchor,0,TM::draw_string,*t->rgb);
            }
        }
        return true;
    };
    const auto img=[&](Asset asset,int x,int y,int f=0,std::optional<std::array<int,4>> crop={}) {
        draw(SteamFacilityImage{asset,{x,y},f,crop});
    };
    const auto num=[&](SteamFacilityNumberKind kind,Asset a,int v,int x,int y,int anchor,int parameter) {
        draw(SteamFacilityNumber{kind,a,v,{x,y},0,anchor,parameter});
    };
    const auto rectangle=[&](int x,int y,int w,int h,std::array<int,3> color,bool outline) {
        draw(StartupSkinRect{{x,y,w,h},color,outline});
    };
    if(!frame_ops(*frame)||!frame_ops(*box))return {};
    const int k=(in.frame%16)/8;
    out.operations.emplace_back(SteamFacilityArrowTouch{{26-3*k,55},3,16});
    out.operations.emplace_back(SteamFacilityArrowTouch{{214+3*k,55},0,18});
    if(in.page==0) {
        img(Asset::facility_icon,21,66,0,std::array<int,4>{16*in.icon,0,16,16});
        label(DR::name,in.definition,in.name,41,69,in.japanese?0:85,in.japanese?0:11,
              in.japanese?1:32,in.japanese?0:11,in.japanese?TM::draw_string:TM::text_layout);
        label(DR::price,0,{},in.japanese?136:133,in.japanese?69:70,0,0,{},in.japanese?0:10,TM::draw_string,blue);
        if(in.values[0]>=in.limits[0])img(Asset::maximum,160,72,0,std::array<int,4>{0,0,20,6});
        num(SteamFacilityNumberKind::money,Asset::number08,in.values[0],223,70,0,0);
        rectangle(23,88,98,75,{181,219,166},true);
        rectangle(24,89,96,73,{219,249,207},false);
        draw(SteamFacilityClip{SteamFacilityClipKind::push_intersect,{23,88,98,75}});
        draw(SteamFacilityMapchip2{in.mapchip,{72,125},0});
        draw(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
        img(Asset::level_label,25,151,0,std::array<int,4>{0,0,17,10});
        if(in.level==5)img(Asset::maximum,in.japanese?44:46,154,0,std::array<int,4>{0,0,20,6});
        else num(SteamFacilityNumberKind::number,Asset::number05,in.level,in.japanese?53:55,151,4,-1);
        rectangle(123,88,91,35,{255,207,99},true);
        rectangle(124,89,89,33,{255,250,213},false);
        rectangle(123,124,91,39,{255,207,99},true);
        rectangle(124,125,89,37,{255,250,213},false);
        for(int slot=1;slot<3;++slot) {
            const int y=93+15*(slot-1);
            label(DR::parameter,slot,{},in.japanese?136:130,y+(in.japanese?0:2),0,0,{},in.japanese?0:8,TM::draw_string,blue);
            if(in.values[slot]>=in.limits[slot])img(Asset::maximum,160,y+3,0,std::array<int,4>{0,0,20,6});
            num(SteamFacilityNumberKind::number,Asset::number08,in.values[slot],in.japanese?205:207,y,4,slot);
        }
        for(std::size_t i=0;i<in.effects.size();++i) {
            const auto &e=in.effects[i];const int y=127+17*static_cast<int>(i);
            img(Asset::effect_icon,136,y,0,std::array<int,4>{e[0]==5?96:16*e[0],16,16,16});
            for(int plus=0;plus<e[1];++plus)img(Asset::number08,192-8*plus,y+3,14);
        }
        if(in.effects.empty())label(DR::no_effects,0,{},168,137,0,0,2,0,TM::draw_string);
        label(in.level==5?DR::maximum_level:DR::remaining,0,{},45,172,0,0,1,0,TM::draw_string);
        if(in.level!=5) {
            num(SteamFacilityNumberKind::number,Asset::number09,in.remaining,188,172,4,-1);
            img(Asset::number05,189,175,18);
            if(in.english)img(Asset::people,191,174,0,std::array<int,4>{0,0,13,12});
        }
    } else {
        label(DR::bonus_title,0,{},27,69,0,0,1,0,TM::draw_string);
        label(DR::maintenance,3,{},132,69,0,0,{},0,TM::draw_string,blue);
        num(SteamFacilityNumberKind::money,Asset::number08,in.values[3],214,70,0,3);
        if(in.bonuses.empty())label(DR::no_bonus,0,{},120,97,0,0,2,0,TM::draw_string);
        const int end=std::min(in.cursor.first+5,static_cast<int>(in.bonuses.size()));
        for(int i=in.cursor.first;i<end;++i) {
            const auto &r=in.bonuses[i];const int y=97+19*(i-in.cursor.first);
            if(i==in.cursor.selection)img(Asset::finger,17,y+8,in.finger_frame);
            out.operations.emplace_back(SteamStartupTouch{11,0x20000|i,std::array<int,4>{-1,y-2,231,18},{},0});
            img(Asset::facility_icon,26,y-2,0,std::array<int,4>{16*r.icon,0,16,16});
            label(DR::bonus_name,r.definition,r.name+std::to_string(r.ordinal+1),44,y+(in.japanese?0:1),
                  0,0,{},in.japanese?0:10,TM::draw_string);
            for(std::size_t n=0;n<r.values.size();++n) {
                const bool plant=r.kind==2;
                const int x=plant?(in.japanese?(n?171:121):(n?168:130)):(in.japanese?147:151);
                label(DR::bonus_value,plant?static_cast<int>(n):2,"+"+std::to_string(r.values[n]),x,y+(in.japanese?0:2),
                      plant&&!in.japanese?35:0,plant&&!in.japanese?9:0,plant&&!in.japanese?std::optional<int>(32):std::nullopt,
                      in.japanese?0:9,plant&&!in.japanese?TM::text_layout:TM::draw_string2);
            }
        }
        out.operations.emplace_back(SteamFacilityScrollRequest{{221,85,3,110},static_cast<int>(in.bonuses.size()),5,0x20000,{3,20,0,0}});
    }
    const int width=std::min(*in.bottom_width,200)+16,left=120-width/2;
    if(in.page==0) {
        rectangle(left,197,width,16,{255,153,55},false);
        out.operations.emplace_back(SteamStartupTouch{4,20,std::array<int,4>{left-20,177,width+40,56},{},0});
    }
    label(DR::bottom,in.page,{},left,197,width,16,34,0,TM::text_layout);
    if(in.page==0)img(Asset::finger,left-4,206,in.finger_frame);
    return out;
}
std::optional<SteamFacilityDetailSkinPlan> steam_facility_detail_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamFacilityDetailSkinOptions &o) {
    const auto p=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &v){return v.lifecycle!=4;});
    if(!s.rules||p==s.scripts.pages.rend()||p->id!=id||
       p->kind!=ref::WorldScriptPageKind::raw_page||p->legacy_g!=0||!valid_startup_world_facility_page(s,*p)||
       startup_world_facility_page_count(s,*p)!=2)return {};
    const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &v){return v.id==p->legacy_f;});
    const auto binding=s.facility_page_bindings.find(id);
    const auto use=s.scene.world.world.facility_uses.find(p->legacy_f);
    if(d==s.rules->facilities.end()||binding==s.facility_page_bindings.end()||
       use==s.scene.world.world.facility_uses.end()||!s.scripts.facilities.count(d->id)||
       d->exit_effects.size()>4096||s.facility_page_neighbours.find(id)->second.size()>65536)return {};
    // 已初始化世界的经济查询读所有开放人物职业；先拒绝缺字段，避免map.at例外。
    for(const auto &h:s.rules->humans) {
        const auto present=s.human_presence.find(h.identity);
        if(present==s.human_presence.end())return {};
        if(present->second!=0) {
            const auto growth=s.scene.world.world.ai.growth.find(h.identity);
            if(growth==s.scene.world.world.ai.growth.end())return {};
            const int job=growth->second.definition.current_profession;
            if(job<0||static_cast<std::size_t>(job)>=s.rules->jobs.size()||
               s.rules->jobs[job].type<0||s.rules->jobs[job].type>=10)return {};
        }
    }
    const auto values=startup_world_facility_values(s,binding->second);
    const auto rows=startup_world_facility_bonus_rows(s,id);
    if(!values||!rows)return {};
    SteamFacilityDetailSkinInput in;
    in.definition=d->id;in.mapchip=d->display_id;in.icon=d->legacy_icon;in.name=d->name;
    in.level=use->second.level;in.page=s.page_phases.find(id)->second;in.frame=s.page_counters.find(id)->second;
    in.view_y=o.view_y;in.japanese=o.japanese;in.english=o.english;in.finger_frame=o.finger_frame;
    in.cursor=o.cursor;in.title_widths=o.title_widths;in.bottom_width=o.bottom_width;
    for(int n=0;n<4;++n) {
        if(!fits(values->instance_attributes[n]))return {};
        in.values[n]=static_cast<int>(values->instance_attributes[n]);
        if(n<3) {
            const auto limit=2LL*d->economy.attributes[n].fifth;
            if(!fits(limit))return {};
            in.limits[n]=static_cast<int>(limit);
        }
    }
    if(use->second.completed_uses<0||!fits(values->upgrade_uses))return {};
    const auto remaining=values->upgrade_uses-static_cast<std::int64_t>(use->second.completed_uses);
    if(!fits(remaining))return {};
    in.remaining=static_cast<int>(remaining);
    for(const auto &e:d->exit_effects)in.effects.push_back({e.attribute_index,e.delta});
    for(const auto &r:*rows) {
        const auto source=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &v){return v.id==r.definition;});
        if(source==s.rules->facilities.end())return {};
        SteamFacilityDetailBonus row{r.definition,r.ordinal,r.icon,source->kind,source->name,{}};
        for(const auto &v:r.values){if(!fits(v.value))return {};row.values.push_back(static_cast<int>(v.value));}
        in.bonuses.push_back(std::move(row));
    }
    return steam_facility_detail_skin(in);
}
} // namespace ark::simulation
