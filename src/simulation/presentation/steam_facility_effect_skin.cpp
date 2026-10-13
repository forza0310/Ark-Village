// 设施75/76/77原序计划；业务扣除、共享改良与页面计数仍由唯一Owner负责。
#include "ark/simulation/presentation/steam_facility_effect_skin.hpp"
#include "ark/simulation/facilities/startup_world_facility_items.hpp"
#include "ark/simulation/map/startup_world_projection.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
bool fits(std::int64_t value) {
    return value>=std::numeric_limits<int>::min()&&value<=std::numeric_limits<int>::max();
}
void label(SteamFacilityEffectPlan &p,std::string text,int x,int y,int anchor=0,int size=0,
           std::array<int,3> color={92,51,31},int width=0,int height=0,bool layout=false) {
    p.operations.emplace_back(SteamFacilityEffectText{std::move(text),{x,y,width,height},anchor,size,layout,color});
}
bool window(SteamFacilityEffectPlan &p,int w,int h,std::array<int,4> box,
            int view_y,std::optional<std::array<int,2>> widths) {
    if(!widths)return false;
    const auto outer=steam_startup_window_skin({w,h,0,0,SteamStartupTextRole::message_title,0},view_y,widths);
    const auto inner=steam_startup_box_skin({box[0],box[1],box[2],box[3]},view_y);
    if(!outer||!inner)return false;
    for(const auto &draw:outer->draws)p.operations.emplace_back(draw);
    for(const auto &draw:inner->draws)p.operations.emplace_back(draw);
    return true;
}
bool icon(SteamFacilityEffectPlan &p,const std::vector<StartupVisualDraw> &draws,int x,int y) {
    if(draws.size()!=2)return false;
    for(const auto &d:draws) {
        if(d.resource!=StartupVisualResource::common||d.sprite!=-1||
           (d.image!=24&&d.image!=9)||!fits(std::int64_t(x)+d.offset[0])||
           !fits(std::int64_t(y)+d.offset[1]))return false;
        p.operations.emplace_back(SteamHumanImage{d.image==24?SteamHumanAsset::equipment_background:
            SteamHumanAsset::item_icon,{x+d.offset[0],y+d.offset[1]},0,d.crop});
    }
    return true;
}
float product(float a,float b) {volatile float out=a*b;return out;}
float sum(float a,float b) {volatile float out=a+b;return out;}
float quotient(float a,float b) {volatile float out=a/b;return out;}
// GameUtil.GetParabolaValue单精度步骤，随后调用点向零转整数。
int parabola(int distance,int time,int tick) {
    const int half=time/2;
    const float velocity=quotient(sum(float(distance),float(distance)),float(half-1));
    const float acceleration=quotient(product(float(distance),-2.F),float(half*(half-1)));
    return static_cast<int>(std::max(0.F,sum(product(float(tick),velocity),
        product(product(product(float(tick),acceleration),float(tick+1)),0.5F))));
}
bool actor(SteamFacilityEffectPlan &p,int profession,int sex,int action,int step,std::array<int,2> at) {
    auto skin=startup_title_actor_skin(profession,sex,-1,step,2,false);
    if(!skin)return false;
    // Steam SetDrawParam的action8/10方向2；两者step0与body_off0由cctor交叉。
    if(action==8)skin->body={skin->body.image,18,step};
    else if(action==10)skin->body={skin->body.image,26,step};
    else if(action!=0)return false;
    p.operations.emplace_back(SteamHumanPortrait{*skin,at,{},false});return true;
}
}
std::optional<SteamFacilityEffectResource> steam_facility_effect_resource(SteamFacilityEffectAsset asset) {
    switch(asset) {
    case SteamFacilityEffectAsset::response:return SteamFacilityEffectResource{
        "common",136,92,"steam-facility-common/original/common/eff_tenantStren.png","original/common/eff_tenantStren.seb"};
    case SteamFacilityEffectAsset::throw_flash:return SteamFacilityEffectResource{
        "effect",6,6,"original/effect/dropItem00.png","original/effect/dropItem00.seb"};
    }
    return {};
}
std::optional<SteamFacilityBonusCursor> steam_facility_item_cursor(
    int count,SteamFacilityBonusCursor c,bool up,bool down) {
    if(count<=0||count>65536||c.selection<0||c.selection>=count||c.first<0||c.first>=count||
       c.selection<c.first||c.selection>=c.first+5)return {};
    if(up)c.selection=(c.selection+count-1)%count;
    if(down)c.selection=(c.selection+1)%count;
    if(c.first>c.selection)c.first=c.selection;
    if(c.first+5<=c.selection)c.first=c.selection-4;
    return c;
}
std::optional<SteamFacilityEffectPlan> steam_facility_item_skin(const SteamFacilityItemSkinInput &in) {
    const int count=static_cast<int>(in.items.size());
    if(in.items.empty()||in.items.size()>65536||in.finger_frame<0||in.arrow_count<0||
       !steam_facility_item_cursor(count,in.cursor,false,false))return {};
    std::set<int> seen;
    for(const auto &r:in.items)if(r.definition<0||r.inventory<=0||r.inventory>999||r.name.empty()||
        !seen.insert(r.definition).second)return {};
    SteamFacilityEffectPlan p;
    if(!window(p,220,168,{17,74,219,184},in.view_y,in.title_widths))return {};
    label(p,"名称",30,69);
    // Steam使用TextLayout(185,69,40,12,anchor32)，不照APK的直接DrawString。
    label(p,"剩余",185,69,32,0,{92,51,31},40,12,true);
    for(int row=in.cursor.first;row<count&&row-in.cursor.first<5;++row) {
        const int y=97+19*(row-in.cursor.first);const auto &r=in.items[row];
        if(row==in.cursor.selection)p.operations.emplace_back(SteamStartupFrameDraw{
            StartupSkinRect{{23,y-2,191,16},{255,153,55},false}});
        p.operations.emplace_back(SteamFacilityEffectTouch{SteamStartupTouch{11,0x20000|row,
            std::array<int,4>{23,y-2,191,16},{},16},{29,29,0,0}});
        if(row==in.cursor.selection)p.operations.emplace_back(SteamFacilityImage{
            SteamFacilityAsset::finger,{21,y+8},in.finger_frame,{}});
        if(!icon(p,r.icon,30,y-2))return {};
        label(p,r.name,50,y);
        p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,
            SteamFacilityAsset::number05,r.inventory,{210,y+1},0,4,-1});
    }
    // 同DrawVerticalScroll几何，但75 Update2没有74 helper的维护尾窗上限。
    const int denominator=std::max(count,5),top=85+110*in.cursor.first/denominator,height=550/denominator;
    const std::array<int,4> scroll_rectangle{221,85,3,110};
    SteamFacilityScrollSkinPlan scrollbar{
        std::array<StartupSkinRect,2>{{{{220,85,5,110},{7,5,78},false},
            {{220,top,5,height+1},in.scroll_first_touch&&count>5?
                std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false}}},
        {221,top,5,height}};
    p.operations.emplace_back(SteamFacilityEffectScroll{scrollbar,
        SteamStartupTouch{12,0x40000,scroll_rectangle,{},0},{3,20,0,0},
        {count,5,0x20000},SteamStartupTouch{25,0,scroll_rectangle,{},4}});
    const auto &selected=in.items[in.cursor.selection];
    constexpr const char *names[]{"价格","品质","魅力"};
    for(int slot=0;slot<3;++slot) {
        label(p,names[slot],30+64*slot,in.japanese?199:201,0,in.japanese?0:8);
        const int amount=selected.improvements[slot];
        const int rank=amount==0?0:amount<(slot==0?50:3)?1:amount<(slot==0?100:6)?2:3;
        // AppData.Draw_plus_left RVA2554F0：SEB15/frame14，每枚固定8像素，0级无图。
        for(int mark=0;mark<rank;++mark)p.operations.emplace_back(SteamFacilityImage{
            SteamFacilityAsset::number08,{(in.japanese?56:60)+64*slot+8*mark,200},14,{}});
    }
    if(in.arrow_count==0) {
        p.operations.emplace_back(SteamStartupTouch{2,786632,std::array<int,4>{90,48,70,30},{},2});
        p.operations.emplace_back(SteamStartupTouch{2,786632,std::array<int,4>{90,223,70,30},{},2});
    } else for(int side=0;side<2;++side) {
        const auto index=p.operations.size();
        p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::title_arrows,
            {120,side==0?85:200},side,{}});
        p.operations.emplace_back(SteamFacilityEffectTouch{SteamStartupTouch{23,side==0?17:19,
            {},index,2},{40,40,side==0?45:-15,side==0?0:60}});
    }
    return p;
}
std::optional<SteamFacilityEffectPlan> steam_facility_improvement_skin(const SteamFacilityImprovementSkinInput &in) {
    if((in.raw_page!=76&&in.raw_page!=77)||in.frame<0||in.response< -1||in.response>2||
       (in.raw_page==76&&in.frame>=49)||!in.title_widths)return {};
    // 与81共用Draw_tenantStrength：保留其完整数字插值/差值弹跳/MAX/继续箭头。
    SteamFacilityUpgradeSkinInput base;
    base.mapchip=in.mapchip;base.level=1;base.phase=1;base.frame=in.raw_page==76?0:in.frame;
    base.view_y=in.view_y;base.japanese=in.japanese;base.attributes=in.attributes;
    base.limits=in.limits;base.title_widths=in.title_widths;
    const auto shared=steam_facility_upgrade_skin(base);if(!shared)return {};
    SteamFacilityEffectPlan p;p.soft_labels={0,0};
    bool inserted=false;
    for(const auto &draw:shared->draws) {
        if(const auto *text=std::get_if<SteamFacilityText>(&draw)) {
            if(text->role==SteamFacilityTextRole::parameter_name&&!inserted) {
                inserted=true;
                if(in.raw_page==77) {
                    const int response=in.response==0?1:in.response==1?2:in.response==-1?3:0;
                    p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,{85,132,70,17}});
                    const int y=149+(in.frame>=6?0:17-in.frame*17/6);
                    p.operations.emplace_back(SteamFacilityEffectImage{SteamFacilityEffectAsset::response,{120,y},response});
                    p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
                } else {
                    const int frame=in.frame,local=frame-15;
                    int throw_x=0,throw_y=0;
                    if(local>=0&&local<=20) {throw_x=198+local*(90-198)/26;throw_y=128-parabola(28,26,local);}
                    if(local>=23&&local<35)p.operations.emplace_back(SteamFacilityEffectImage{
                        SteamFacilityEffectAsset::throw_flash,{198+20*(96-198)/26,126-parabola(28,26,20)},((local-23)%12)/4});
                    if(frame<15) {throw_x=198;throw_y=144-16-1;}
                    p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,{30,0,180,240}});
                    if(!actor(p,in.profession,in.sex,frame<15?8:frame<29?10:0,0,{198,144})||
                       !icon(p,in.item_icon,throw_x-8,throw_y-16))return {};
                    p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
                }
            }
            if(text->role==SteamFacilityTextRole::upgrade_title) {
                p.operations.emplace_back(SteamFacilityEffectText{p.title,text->rectangle,text->anchor.value_or(0),
                    text->font_size,false,text->rgb});
            } else if(text->role==SteamFacilityTextRole::parameter_name) {
                constexpr const char *names[]{"价格","品质","魅力"};
                if(text->argument<0||text->argument>2)return {};
                p.operations.emplace_back(SteamFacilityEffectText{names[text->argument],text->rectangle,
                    text->anchor.value_or(0),text->font_size,false,text->rgb});
            } else return {};
        } else if(const auto *image=std::get_if<SteamFacilityImage>(&draw)) {
            if(image->asset!=SteamFacilityAsset::mini)p.operations.emplace_back(*image);
        } else if(const auto *rect=std::get_if<StartupSkinRect>(&draw))p.operations.emplace_back(SteamStartupFrameDraw{*rect});
        else if(const auto *clip=std::get_if<SteamFacilityClip>(&draw))p.operations.emplace_back(*clip);
        else if(const auto *chip=std::get_if<SteamFacilityMapchip2>(&draw))p.operations.emplace_back(*chip);
        else if(const auto *number=std::get_if<SteamFacilityNumber>(&draw))p.operations.emplace_back(*number);
    }
    if(!inserted)return {};
    if(in.raw_page==77&&!actor(p,in.profession,in.sex,0,0,{198,144}))return {};
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return p;
}
std::optional<SteamFacilityEffectPlan> steam_facility_effect_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamFacilityEffectOptions &o) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),
        [=](const auto &p){return p.id==id&&p.lifecycle!=4;});
    const auto top=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &p){return p.lifecycle!=4;});
    if(page==s.scripts.pages.end()||top==s.scripts.pages.rend()||top->id!=id||!s.facility_item_pages_initialized.count(id)||
       !valid_startup_world_facility_item_page(s,*page))return {};
    if(page->legacy_page==75) {
        const auto list=s.facility_item_page_lists.find(id),end=s.facility_item_page_lists.end();
        const auto selected=s.facility_item_page_selections.find(id);
        const auto first=s.facility_item_page_scroll.find(id);
        if(list==end||selected==s.facility_item_page_selections.end()||first==s.facility_item_page_scroll.end())return {};
        SteamFacilityItemSkinInput in;in.cursor={selected->second,first->second};in.finger_frame=o.finger_frame;
        in.arrow_count=o.arrow_count;in.view_y=o.view_y;in.japanese=o.japanese;
        in.scroll_first_touch=o.scroll_first_touch;in.title_widths=o.title_widths;
        for(int item:list->second) {
            const auto d=std::find_if(s.rules->items.begin(),s.rules->items.end(),[=](const auto &r){return r.identity==item;});
            const auto stock=s.items.find(item);const auto draws=startup_world_item_icon_draws(s,item);
            if(d==s.rules->items.end()||stock==s.items.end()||!draws)return {};
            in.items.push_back({item,stock->second.inventory,d->name,*draws,d->facility_improvements});
        }
        return steam_facility_item_skin(in);
    }
    const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [&](const auto &r){return r.id==page->legacy_f;});
    const auto human=std::find_if(s.rules->humans.begin(),s.rules->humans.end(),[](const auto &r){return r.identity==0;});
    const auto growth=s.scene.world.world.ai.growth.find(0),ge=s.scene.world.world.ai.growth.end();
    const auto count=s.page_counters.find(id),item=s.facility_item_page_items.find(id);
    if(d==s.rules->facilities.end()||human==s.rules->humans.end()||growth==ge||count==s.page_counters.end()||
       item==s.facility_item_page_items.end())return {};
    SteamFacilityImprovementSkinInput in;in.raw_page=page->legacy_page;in.frame=count->second;
    in.mapchip=d->display_id;in.profession=growth->second.definition.current_profession;in.sex=human->sex;
    in.response=s.facility_item_response;in.view_y=o.view_y;in.japanese=o.japanese;in.title_widths=o.title_widths;
    if(in.raw_page==76) {const auto draws=startup_world_item_icon_draws(s,item->second);if(!draws)return {};in.item_icon=*draws;}
    for(int slot=0;slot<3;++slot) {
        const auto limit=2LL*d->economy.attributes[slot].fifth;if(!fits(limit))return {};in.limits[slot]=static_cast<int>(limit);
        for(int value=0;value<3;++value) {const auto v=s.facility_upgrade_display[value][slot];if(!fits(v))return {};
            in.attributes[slot][value]=static_cast<int>(v);}
    }
    return steam_facility_improvement_skin(in);
}
} // namespace ark::simulation
