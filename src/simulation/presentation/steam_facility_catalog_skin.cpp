// 商品79的Steam局部绘制；目录与NEW的提交仍由唯一Owner负责。
#include "ark/simulation/presentation/steam_facility_catalog_skin.hpp"
#include "ark/simulation/facilities/startup_world_facility_catalog.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace ark::simulation {
std::optional<SteamHumanOverviewPlan> steam_facility_commodity_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamHumanOverviewOptions &o) {
    const auto view=inspect_startup_world_facility_catalog_page(s,id);
    if(!view||view->raw!=79||!o.title_widths||!o.bottom_widths[0]||
       *o.bottom_widths[0]<0||o.finger_frame<0||view->entries.size()>4096)return {};
    const int kind=view->mode==1?1:view->mode==4?2:3;
    const auto window=steam_startup_window_skin({220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!window||!box)return {};
    SteamHumanOverviewPlan out;out.title="商品 "+std::to_string(view->phase+1)+"/2";out.soft_labels={7,2};
    for(const auto &part:window->draws)out.operations.emplace_back(part);
    for(const auto &part:box->draws)out.operations.emplace_back(part);
    const int shift=3*((view->counter%16)/8);
    out.operations.emplace_back(SteamFacilityArrowTouch{{26-shift,55},3,16});
    out.operations.emplace_back(SteamFacilityArrowTouch{{214+shift,55},0,18});
    const auto text=[&](std::string value,int x,int y,int w,int h,int anchor,int size,bool layout) {
        out.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::equipment_name,
            std::move(value),{x,y,w,h},anchor,size,layout});
    };
    text("名称",30,69,0,0,0,0,false);
    if(view->phase==0)text("价格",193,69,0,0,2,0,false);
    else for(int col=0;col<2;++col)out.operations.emplace_back(SteamHumanImage{
        SteamHumanAsset::combat_icon,{134+47*col,67},kind==1?(col==0?1:3):(col==0?0:2),{}});
    for(int row=view->first_visible;row<static_cast<int>(view->entries.size())&&row<view->first_visible+4;++row) {
        const int definition=view->entries[row],y=98+24*(row-view->first_visible);
        const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[=](const auto &e) {
            return e.shop.kind==kind&&e.shop.id==definition;
        });
        const auto progress=s.catalog.find({kind,definition});
        const auto icons=startup_world_equipment_icon_draws(s,kind,definition);
        if(d==s.rules->equipment.end()||progress==s.catalog.end()||d->name.empty()||d->shop.price<0||!icons)return {};
        // 原Steam额外Margin(0,-20,0,0)，没有道具赠礼的marker16二次点击资格。
        out.operations.emplace_back(SteamHumanEquipmentRow{
            SteamStartupTouch{11,0x20000|row,std::array<int,4>{1,y-2,233,16},{},0},{0,-20,0,0}});
        if(progress->second.newly_unlocked)out.operations.emplace_back(SteamHumanImage{
            SteamHumanAsset::catalogue_get,{10,y+2},0,std::array<int,4>{0,0,23,9}});
        for(const auto &part:*icons)out.operations.emplace_back(SteamHumanImage{
            part.image==24?SteamHumanAsset::equipment_background:kind==1?SteamHumanAsset::weapon_icon:
                kind==2?SteamHumanAsset::armour_icon:SteamHumanAsset::accessory_icon,
            {30+part.offset[0],y-3+part.offset[1]},0,part.crop});
        if(row==view->selection)out.operations.emplace_back(SteamFacilityImage{
            SteamFacilityAsset::finger,{19,y+8},o.finger_frame,{}});
        text(d->name,50,y+(o.japanese?0:1),0,0,0,o.japanese?0:10,false);
        if(view->phase==0)out.operations.emplace_back(SteamFacilityNumber{
            SteamFacilityNumberKind::money,SteamFacilityAsset::number08,d->shop.price,{211,y+1},0,4,-1});
        else for(int col=0;col<2;++col) {
            const int attribute=kind==1?(col==0?1:3):(col==0?0:2),value=d->shop.combat[attribute];
            if(value>0)out.operations.emplace_back(SteamFacilityNumber{
                SteamFacilityNumberKind::plus_value,SteamFacilityAsset::number05,value,{160+46*col,y+1},0,4,attribute});
        }
    }
    const int count=static_cast<int>(view->entries.size()),denominator=std::max(count,4);
    const int top=85+static_cast<int>(std::int64_t(110)*view->first_visible/denominator),height=440/denominator;
    out.operations.emplace_back(SteamHumanCatalogueScroll{
        {StartupSkinRect{{220,85,5,110},{7,5,78},false},StartupSkinRect{{220,top,5,height+1},
            o.catalogue_scroll_first_touch&&count>4?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false}},
        SteamStartupTouch{12,0x40000,std::array<int,4>{221,85,3,110},{},0},{3,20,0,0},
        {count,4,0x20000},SteamStartupTouch{25,0,std::array<int,4>{221,85,3,110},{},4}});
    const std::string bottom=std::to_string(count)+"种"+(kind==1?"武器":kind==2?"防具":"饰品")+"出售中";
    const int width=std::min(*o.bottom_widths[0],200)+16;
    text(bottom,120-width/2,197,width,16,34,0,true);
    return out;
}
std::optional<SteamResidenceCursor> steam_residence_cursor(
    SteamResidenceCursor cursor,int count,bool up,bool down) {
    // Steam Update2 VA1031A887..1031A94B：上、下各自执行，之后才修四行首行。
    if(count<=0||count>65536||cursor.selection<0||cursor.selection>=count||
       cursor.first<0||cursor.first>=count)return {};
    if(up)cursor.selection=(cursor.selection+count-1)%count;
    if(down)cursor.selection=(cursor.selection+1)%count;
    if(cursor.first>cursor.selection)cursor.first=cursor.selection;
    if(cursor.first+4<=cursor.selection)cursor.first=cursor.selection-3;
    return cursor;
}
std::optional<SteamHumanOverviewPlan> steam_residence_skin(const SteamResidenceInput &in) {
    const auto &o=in.options;
    if(in.order.empty()||in.order.size()>65536||in.humans.size()>65536||
       o.view_y< -65536||o.view_y>65536||o.finger_frame<0||o.finger_frame>5||
       !o.title_widths||!o.bottom_width||*o.bottom_width<0||*o.bottom_width>4096)return {};
    const int count=static_cast<int>(in.order.size());
    const auto cursor=steam_residence_cursor(in.cursor,count,false,false);
    if(!cursor||cursor->selection!=in.cursor.selection||cursor->first!=in.cursor.first)return {};
    std::map<int,const SteamResidenceHuman *> humans;
    for(const auto &h:in.humans) {
        if(h.identity<0||h.fee<0||h.name.empty()||h.name.size()>4096||
           !startup_title_actor_skin(h.profession,h.sex,-1,0,1,false)||
           !humans.emplace(h.identity,&h).second)return {};
    }
    std::set<int> seen;
    // 原EnumerateMyHomeChara遍历定义一次；不接受重复候选或可见行之外的悬空身份。
    for(int id:in.order)if(!humans.count(id)||!seen.insert(id).second)return {};
    const auto w=steam_startup_window_skin({220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto b=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!w||!b)return {};
    SteamHumanOverviewPlan out;out.title="选择入住者";out.soft_labels={0,2};
    for(const auto &part:w->draws)out.operations.emplace_back(part);
    for(const auto &part:b->draws)out.operations.emplace_back(part);
    const auto text=[&](std::string value,int x,int y,int anchor=0,bool layout=false,int width=0,int height=0) {
        out.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::equipment_name,
            std::move(value),{x,y,width,height},anchor,0,layout});
    };
    text("姓名",30,69);text("费用",192,69,2);
    // Steam Draw2_3 VA10342F17..1034339B确实绘五行；不得随Update四行修首行裁成四行。
    for(int row=cursor->first;row<count&&row-cursor->first<5;++row) {
        const auto &h=*humans.find(in.order[row])->second;const int y=97+19*(row-cursor->first);
        if(row==cursor->selection)out.operations.emplace_back(SteamStartupFrameDraw{
            StartupSkinRect{{23,y-2,191,16},{255,153,55},false}});
        out.operations.emplace_back(SteamHumanEquipmentRow{
            SteamStartupTouch{11,0x20000|row,std::array<int,4>{23,y-2,191,16},{},16},{29,29,0,0}});
        out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{29,y-2,16,16},{196,236,169},false}});
        out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{29,y-2,16,16},{204,204,204},true}});
        const auto actor=startup_title_actor_skin(h.profession,h.sex,-1,0,1,false);
        if(!actor)return {};
        out.operations.emplace_back(SteamHumanPortrait{*actor,{37,y+20},std::array<int,4>{29,y-1,15,14},false});
        if(row==cursor->selection)out.operations.emplace_back(SteamFacilityImage{
            SteamFacilityAsset::finger,{21,y+8},o.finger_frame,{}});
        text(h.name,48,y);
        out.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::money,
            h.affordable?SteamFacilityAsset::number08:SteamFacilityAsset::number12,h.fee,{212,y+1},0,4,-1});
    }
    // 原DrawVerticalScroll2实参w4/h111；内条／触区宽为w−1，largeChange仍5。
    const int denominator=std::max(count,5),top=85+111*cursor->first/denominator,height=555/denominator;
    const std::array<int,4> track{221,85,3,111};
    out.operations.emplace_back(SteamHumanCatalogueScroll{
        {StartupSkinRect{{220,85,5,111},{7,5,78},false},StartupSkinRect{{220,top,5,height+1},
            o.scroll_first_touch&&count>5?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false}},
        SteamStartupTouch{12,0x40000,track,{},0},{3,20,0,0},{count,5,0x20000},SteamStartupTouch{25,0,track,{},4}});
    const int width=std::min(*o.bottom_width,200)+16;
    // VA10343376..1034338D：select/cursor均false，无Touch4；只有原提示TextLayout。
    text("请选择入住者",120-width/2,197,34,true,width,16);
    return out;
}
} // namespace ark::simulation
