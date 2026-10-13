// 商品79的Steam局部绘制；目录与NEW的提交仍由唯一Owner负责。
#include "dungeon_village_prototype/steam_facility_catalog_skin.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
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
} // namespace dungeon_village_prototype
