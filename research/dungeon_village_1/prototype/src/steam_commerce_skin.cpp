// Steam SubForm raw83；静态合同见rules/COMMERCE.md#steam-commerce-top。
#include "dungeon_village_prototype/steam_commerce_skin.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
std::optional<SteamCommerceSkinInput> steam_commerce_input(
    const StartupWorldRuntimeState &s,std::uint64_t page) {
    const auto view=inspect_startup_world_commerce_page(s,page);
    if(!view||view->raw!=83||s.commerce_page_lists.count(page)||s.rank<0||s.rank==std::numeric_limits<int>::max())return {};
    SteamCommerceSkinInput input{view->selection,s.rank,false,false};
    // 原顺序短路：NEW不要求道具已开放，也不沿85维护策略收紧为facility p0。
    for(const auto &item:s.rules->items) {
        const auto stock=s.shop_item_stock.find(item.identity);
        if(stock==s.shop_item_stock.end()||stock->second.quantity<0)return {};
        if(stock->second.quantity<=0)continue;
        const auto read=s.item_commerce_read.find(item.identity);
        if(read==s.item_commerce_read.end())return {};
        if(!read->second){input.item_new=true;break;}
    }
    for(const auto &facility:s.rules->facilities) {
        const auto presence=s.facility_presence.find(facility.id);
        if(presence==s.facility_presence.end()||presence->second<0||presence->second>2)return {};
        if(presence->second==2||facility.unlock_rank==-1||facility.unlock_rank>s.rank)continue;
        const auto read=s.facility_commerce_read.find(facility.id);
        if(read==s.facility_commerce_read.end())return {};
        if(!read->second){input.facility_new=true;break;}
    }
    return input;
}
std::optional<SteamCommerceResource> steam_commerce_resource(SteamCommerceAsset asset) {
    switch(asset) {
    case SteamCommerceAsset::row_background:
        return SteamCommerceResource{"common",19,-1,"original/common/bar_middleWindow.png",nullptr};
    case SteamCommerceAsset::category_icon:
        return SteamCommerceResource{"common2",2,1,"original/common2/icon_shopTop.png","original/common2/icon_shopTop.seb"};
    case SteamCommerceAsset::private_pumpkin:
        return SteamCommerceResource{"common",173,38,"original/common/chara_pumpkin.png","original/common/chara_pumpkin.seb"};
    }
    return {};
}
std::optional<SteamCommerceSkinPlan> steam_commerce_skin(
    const SteamCommerceSkinInput &in,const SteamCommerceSkinOptions &o) {
    const auto valid_text=[](const std::string &s){return !s.empty()&&s.size()<=4096;};
    // 预算/溢出拒绝是维护策略，不为原rank添加未经来源核验的星级上限。
    if(in.selection<0||in.selection>=3||in.rank<0||in.rank==std::numeric_limits<int>::max()||
       o.view_y< -65536||o.view_y>65536||o.finger_frame<0||o.finger_frame>5||
       !valid_text(o.text.title)||!valid_text(o.text.welcome)||!o.text.title_widths||
       std::any_of(o.text.title_widths->begin(),o.text.title_widths->end(),
                   [](int w){return w<0||w>4096;}))return {};
    for(const auto &s:o.text.names)if(!valid_text(s))return {};
    for(const auto &s:o.text.explanations)if(!valid_text(s))return {};
    const auto window=steam_startup_window_skin(
        {220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.text.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!window||!box)return {};
    SteamCommerceSkinPlan p;p.title=o.text.title;
    for(const auto &op:window->draws)p.operations.emplace_back(op);
    for(const auto &op:box->draws)p.operations.emplace_back(op);
    const auto text=[&](const std::string &value,std::array<int,4> rect,int anchor,
                        int size=0,bool layout=false) {
        p.operations.emplace_back(SteamHumanOverviewText{
            SteamHumanOverviewTextRole::equipment_name,value,rect,anchor,size,layout,{92,51,31}});
    };
    text(o.text.welcome,{45,70,0,0},1);
    p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{181,71},0,{}});
    p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,
        SteamFacilityAsset::number03_hud,in.rank+1,{202,69},0,1,-1});
    // 私有Draw_spCharaClip槽2：交集clip→SEB38 frame0@(30,94)→PopClip。
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{21,66,18,18}});
    p.operations.emplace_back(SteamCommerceImage{SteamCommerceAsset::private_pumpkin,{30,94},0,{}});
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
    for(int row=0;row<3;++row) {
        const int y=101+26*row;
        p.operations.emplace_back(SteamCommerceImage{SteamCommerceAsset::row_background,{53,y},0,
            std::array<int,4>{0,row==in.selection?0:26,134,26}});
        p.operations.emplace_back(SteamStartupTouch{11,0x20000|row,
            std::array<int,4>{33,y,174,26},{},0});
        if(o.japanese)text(o.text.names[row],{127,y+7,0,0},2);
        else text(o.text.names[row],{99,y+7,85,11},32,11,true);
        if((row==0&&in.item_new)||(row==2&&in.facility_new))
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::catalogue_new,{164,y+14},0,{}});
        p.operations.emplace_back(SteamCommerceImage{
            SteamCommerceAsset::category_icon,{o.japanese?77:75,y+4},row,{}});
        if(row==in.selection)p.operations.emplace_back(SteamFacilityImage{
            SteamFacilityAsset::finger,{o.japanese?68:66,y+13},o.finger_frame,{}});
    }
    text(o.text.explanations[in.selection],{120,198,0,0},2);
    return p;
}
} // namespace dungeon_village_prototype
