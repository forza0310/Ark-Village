// 原版局部合同见ui/PAGES.md：raw3缓存/输入与Steam DrawMenu2(type0)。
#include "dungeon_village_prototype/steam_main_menu_skin.hpp"
#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
bool coordinate(std::int64_t value) {
    return value>=std::numeric_limits<int>::min() && value<=std::numeric_limits<int>::max();
}
std::vector<int> tags(bool magic) { return magic?std::vector<int>{0,1,2,3,5,6}:std::vector<int>{0,1,2,5,6}; }
void image(SteamMainMenuSkinPlan &plan,int id,int x,int y,std::array<int,4> crop,
           SteamMainMenuPackage package=SteamMainMenuPackage::common) {
    plan.draws.emplace_back(SteamMainMenuImage{package,id,-1,0,crop,{x,y}});
}
}
std::optional<SteamMainMenuSkinPlan> steam_main_menu_skin(
    const SteamMainMenuSkinInput &input,const SteamMainMenuSkinOptions &options) {
    const auto entries=tags(input.magic_unlocked);
    if(input.frame<0 || input.frame>3 || input.selection<0 ||
       input.selection>=static_cast<int>(entries.size()) || options.canvas[0]<=0 ||
       options.canvas[1]<=0 || options.safe_left<0)return {};
    SteamMainMenuSkinPlan plan;plan.origin=options.origin;
    if(options.covered_by_nonmenu_subform)return plan;
    if(input.frame>=3 && (!input.notices || (input.magic_unlocked && !input.magic_period)))return {};
    if(input.frame>=3 && input.magic_unlocked &&
       (input.magic_period->processed<0 || input.magic_period->pending_materials<0 ||
        input.magic_period->processed>input.magic_period->pending_materials))return {};
    const int width=options.japanese?63:91,height=static_cast<int>(entries.size())*28+5;
    std::int64_t x=options.safe_left,y=0;
    if(std::int64_t(options.origin[0])+width>options.canvas[0])
        x=std::int64_t(options.canvas[0])-width-options.origin[0];
    if(std::int64_t(options.origin[1])+height>options.canvas[1])
        y=std::int64_t(options.canvas[1])-height-options.origin[1];
    // 检查全部本局部锚点，负坐标合法；拒绝C++有符号溢出属于维护保护。
    if(!coordinate(x-2) || !coordinate(x+191) || !coordinate(y) || !coordinate(y+height+22))return {};
    const int dx=static_cast<int>(x),dy=static_cast<int>(y),ratio=input.frame*1000/3;
    for(int row=0;row<static_cast<int>(entries.size());++row) {
        const int tag=entries[static_cast<std::size_t>(row)],row_y=dy+28*row;
        const bool selected=row==input.selection;
        if(input.frame>0) {
            image(plan,25,dx+1,row_y+1,{0,selected?0:29,68*ratio/1000,29*ratio/1000});
            image(plan,168,dx+4,row_y+6,{18*tag,0,18*ratio/1000,18*ratio/1000});
        }
        if(options.top_is_main_menu)
            plan.touches.push_back({8,0x20000|row,std::array<int,4>{dx,row_y,width,28},{},0});
        if(input.frame<3)continue;
        plan.draws.emplace_back(SteamMainMenuText{row,tag,{dx+28-(options.english?2:0),row_y+9},
            selected?std::array<int,3>{76,58,50}:std::array<int,3>{255,242,220}});
        const auto &notice=*input.notices;
        const bool quest_new=notice.quest&&!notice.selected_quest;
        const bool is_new=tag==0?notice.construction:tag==1?quest_new:
            tag==2?((notice.commerce_open&&(notice.commerce_items||notice.commerce_facilities))||notice.activities):
            tag==3?notice.magic_pot:tag==5?notice.adventurers:false;
        const int language=options.japanese?8:options.english?36:0;
        if(is_new)image(plan,147,dx+47+language,row_y+16,{0,0,20,9});
        else if(tag==1&&notice.equipment)
            image(plan,148,dx+(options.japanese?47:44)+language,row_y+16,{0,0,23,9});
        if(tag==3&&input.magic_period->processed>0) {
            const int extra=options.japanese?10:options.english?28:0;
            image(plan,0,dx+69+extra,row_y+4,{0,0,94,22},SteamMainMenuPackage::common2);
            for(const auto &part:std::array<std::array<int,2>,2>{{
                {input.magic_period->processed,104},{input.magic_period->pending_materials,128}}}) {
                plan.draws.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,
                    SteamFacilityAsset::number08,part[0],{dx+part[1]+extra,row_y+10},0,2,-1});
                if(part[1]==104)image(plan,85,dx+112+extra,row_y+10,{47,0,7,10});
            }
        }
        if(selected&&options.on_top)
            plan.draws.emplace_back(SteamMainMenuImage{SteamMainMenuPackage::common,70,
                dx<120?22:21,-1,{}, {dx<120?dx+width+6:dx-2,row_y+13}});
    }
    return plan;
}
std::optional<std::array<int,2>> steam_main_menu_child_position(
    std::array<int,2> parent,int selection,bool magic_unlocked,bool english) {
    const auto entries=tags(magic_unlocked);
    if(selection<0||selection>=static_cast<int>(entries.size()))return {};
    const int tag=entries[static_cast<std::size_t>(selection)];
    if(tag!=1&&tag!=2&&tag!=5&&tag!=6)return {};
    const auto x=std::int64_t(parent[0])+68+(english?28:0),y=std::int64_t(parent[1])+28*selection;
    if(!coordinate(x)||!coordinate(y))return {};
    return std::array<int,2>{static_cast<int>(x),static_cast<int>(y)};
}
std::optional<std::string_view> steam_main_menu_image(SteamMainMenuPackage package,int image) {
    if(package==SteamMainMenuPackage::common2)
        return image==0?std::optional<std::string_view>{"steam-common/mpot_event.png"}:std::nullopt;
    if(package!=SteamMainMenuPackage::common)return {};
    switch(image) {
    case 25:return "original/common/menu.png";
    case 70:return "original/common/finger_r.png";
    case 85:return "steam-common/menuRT01.png";
    case 147:return "original/common/wnd_new.png";
    case 148:return "original/common/wnd_get.png";
    case 168:return "original/common/wnd_menuIcon.png";
    default:return {};
    }
}
} // namespace dungeon_village_prototype
