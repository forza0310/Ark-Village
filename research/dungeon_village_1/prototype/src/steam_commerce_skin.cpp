// Steam SubForm raw83；静态合同见rules/COMMERCE.md#steam-commerce-top。
#include "dungeon_village_prototype/steam_commerce_skin.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_visuals.hpp"
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
std::optional<SteamCommerceSkinPlan> steam_commerce_result_skin(
    const StartupWorldRuntimeState &s,std::uint64_t page,const SteamCommerceResultOptions &o) {
    const auto view=inspect_startup_world_commerce_page(s,page);
    const auto valid_text=[](const std::string &value){return !value.empty()&&value.size()<=4096;};
    const auto valid_width=[](const std::optional<int> &width){return width&&*width>=0&&*width<=4096;};
    if(!view||view->raw!=86||s.commerce_page_lists.count(page)||view->mode<0||view->mode>1||
       o.view_y< -65536||o.view_y>65536||o.finger_frame<0||o.finger_frame>5||
       !valid_text(o.text.title)||!valid_text(o.text.cash_label)||!valid_text(o.text.acknowledgement)||
       !valid_text(o.text.messages[0])||!valid_text(o.text.messages[1])||!o.text.title_widths||
       std::any_of(o.text.title_widths->begin(),o.text.title_widths->end(),[](int w){return w<0||w>4096;})||
       !valid_width(o.text.name_width)||!valid_width(o.text.acknowledgement_width))return {};
    const auto item=std::find_if(s.rules->items.begin(),s.rules->items.end(),
        [&](const auto &definition){return definition.identity==view->binding;});
    if(item==s.rules->items.end()||!valid_text(item->name)||item->commerce_price<0)return {};
    const auto icons=startup_world_item_icon_draws(s,view->binding);
    if(!icons)return {};
    const auto window=steam_startup_window_skin(
        {170,125,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.text.title_widths);
    const auto box=steam_startup_box_skin({42,95,197,162},o.view_y);
    if(!window||!box)return {};
    SteamCommerceSkinPlan p;p.title=o.text.title;p.soft_labels={0,0};
    for(const auto &op:window->draws)p.operations.emplace_back(op);
    for(const auto &op:box->draws)p.operations.emplace_back(op);
    const auto text=[&](const std::string &value,std::array<int,4> rect,int anchor,bool layout=false) {
        p.operations.emplace_back(SteamHumanOverviewText{
            SteamHumanOverviewTextRole::equipment_name,value,rect,anchor,0,layout,{92,51,31}});
    };
    text(o.text.cash_label,{47,91,0,0},0);
    const bool sell=view->mode==1;
    const auto digits=sell?SteamFacilityAsset::number05:SteamFacilityAsset::number12;
    // 单件原定义价；余额/商会反馈数量不参与原86差额显示。
    p.operations.emplace_back(SteamFacilityNumber{
        sell?SteamFacilityNumberKind::plus_value:SteamFacilityNumberKind::minus_value,
        digits,sell?item->commerce_price/2:item->commerce_price,{184,92},0,4,-1});
    p.operations.emplace_back(SteamFacilityImage{digits,{185,92},20,{}});
    const int half=(*o.text.name_width+20)/2;
    for(const auto &part:*icons) {
        if(part.resource!=StartupVisualResource::common||(part.image!=24&&part.image!=9)||part.sprite!=-1)return {};
        p.operations.emplace_back(SteamHumanImage{
            part.image==24?SteamHumanAsset::equipment_background:SteamHumanAsset::item_icon,
            {116-half+part.offset[0],119+part.offset[1]},0,part.crop});
    }
    text(item->name,{136-half,122,0,0},0);
    text(o.text.messages[view->mode],{120,146,0,0},2);
    // 原了解仅绘制/触区；保持自动Pop的原更新合同，不添加等待确认状态。
    const int width=std::min(*o.text.acknowledgement_width,200)+16,left=120-width/2;
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{left,175,width,16},{255,153,55},false}});
    p.operations.emplace_back(SteamStartupTouch{4,20,std::array<int,4>{left-20,155,width+40,56},{},0});
    text(o.text.acknowledgement,{left,175,width,16},34,true);
    p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{left-4,184},o.finger_frame,{}});
    return p;
}
} // namespace dungeon_village_prototype
