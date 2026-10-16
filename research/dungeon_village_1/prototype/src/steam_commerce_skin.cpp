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
    case SteamCommerceAsset::point_unit:
        return SteamCommerceResource{"common",31,44,"steam-common/icon_result00.png","original/common/icon_result00.seb"};
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
std::optional<SteamCommerceSkinPlan> steam_commerce_facility_skin(
    const StartupWorldRuntimeState &s,std::uint64_t page,const SteamCommerceFacilityOptions &o) {
    const auto view=inspect_startup_world_commerce_page(s,page);
    const auto valid_text=[](const std::string &v){return !v.empty()&&v.size()<=4096;};
    if(!view||view->raw!=85||view->entries.empty()||view->entries.size()>65536||s.village_points<0||
       o.view_y< -65536||o.view_y>65536||!valid_text(o.text.title)||!valid_text(o.text.facility_header)||
       !valid_text(o.text.price_header)||!valid_text(o.text.explanation)||!o.text.title_widths||
       std::any_of(o.text.title_widths->begin(),o.text.title_widths->end(),[](int w){return w<0||w>4096;})||
       !o.text.price_width||*o.text.price_width<0||*o.text.price_width>4096)return {};
    const auto window=steam_startup_window_skin(
        {220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.text.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!window||!box)return {};
    SteamCommerceSkinPlan p;p.title=o.text.title;p.soft_labels={7,2};
    for(const auto &op:window->draws)p.operations.emplace_back(op);
    for(const auto &op:box->draws)p.operations.emplace_back(op);
    const auto text=[&](const std::string &value,std::array<int,4> rect,int anchor,bool layout=false) {
        p.operations.emplace_back(SteamHumanOverviewText{
            SteamHumanOverviewTextRole::equipment_name,value,rect,anchor,0,layout,{92,51,31}});
    };
    const auto rectangle=[&](std::array<int,4> rect,std::array<int,3> rgb,bool outline) {
        p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{rect,rgb,outline}});
    };
    text(o.text.facility_header,{51,69,0,0},0);
    text(o.text.price_header,{o.japanese?162:158,69,0,0},0);
    p.operations.emplace_back(SteamCommerceImage{SteamCommerceAsset::point_unit,{166+*o.text.price_width,67},4,{}});
    for(int absolute=view->first_visible;absolute<static_cast<int>(view->entries.size())&&
        absolute<view->first_visible+3;++absolute) {
        const int definition=view->entries[absolute],y=89+34*(absolute-view->first_visible);
        const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
            [&](const auto &f){return f.id==definition;});
        const auto read=s.facility_commerce_read.find(definition);
        if(d==s.rules->facilities.end()||!valid_text(d->name)||definition<0||
           static_cast<std::size_t>(definition)>=s.rules->facility_initial.size()||
           s.rules->facility_initial[definition].capacity<0||read==s.facility_commerce_read.end())return {};
        const SteamFacilityMapchip2 map{d->display_id,{65,y+11},0};
        if(!steam_facility_mapchip2_draws(map))return {};
        // 原DrawRect端点包含边线；图块不是绿色填底，名称框独立选择色。
        rectangle({21,y,90,35},{0,255,128},true);
        p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{22,y+1,88,33}});
        p.operations.emplace_back(map);
        p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
        if(!read->second)p.operations.emplace_back(SteamHumanImage{
            SteamHumanAsset::catalogue_new,{22,y+25},0,{}});
        rectangle({110,y,106,35},{0,255,128},true);
        if(absolute==view->selection)rectangle({111,y+1,104,33},{255,153,55},false);
        p.operations.emplace_back(SteamHumanEquipmentRow{
            SteamStartupTouch{11,0x20000|absolute,std::array<int,4>{110,y,106,35},{},16},{29,0,0,0}});
        text(d->name,{116,y+5,90,11},32,true); // Steam所有语言均TextLayout，继承字号。
        const int cost=s.rules->facility_initial[definition].capacity;
        p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,
            s.village_points>=cost?SteamFacilityAsset::number11:SteamFacilityAsset::number12,
            cost,{206,y+20},0,4,-1});
    }
    // DrawVerticalScroll2原三行、110高；同一组件12后跟25，不另生成方向箭头。
    const int count=static_cast<int>(view->entries.size()),denominator=std::max(count,3);
    const int top=85+110*view->first_visible/denominator,height=330/denominator;
    p.operations.emplace_back(SteamHumanCatalogueScroll{
        {StartupSkinRect{{220,85,5,110},{7,5,78},false},StartupSkinRect{{220,top,5,height+1},
            o.scroll_first_touch&&count>3?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false}},
        SteamStartupTouch{12,0x40000,std::array<int,4>{221,85,3,110},{},0},{3,20,0,0},
        {count,3,0x20000},SteamStartupTouch{25,0,std::array<int,4>{221,85,3,110},{},4}});
    text(o.text.explanation,{45,199,0,0},1);
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{21,195,18,18}});
    p.operations.emplace_back(SteamCommerceImage{SteamCommerceAsset::private_pumpkin,{30,223},0,{}});
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
    return p;
}
std::optional<SteamCommerceSkinPlan> steam_commerce_item_skin(
    const StartupWorldRuntimeState &s,std::uint64_t page,const SteamCommerceItemOptions &o) {
    const auto source=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),
        [=](const auto &p){return p.id==page;});
    const auto valid_text=[](const std::string &v){return !v.empty()&&v.size()<=4096;};
    if(!page||source==s.scripts.pages.end()||source->kind!=ref::WorldScriptPageKind::raw_page||
       source->legacy_page!=84||source->lifecycle<1||source->lifecycle>3||
       std::count_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==page;})!=1||
       !valid_startup_world_commerce_page(s,*source)||!valid_startup_world_commerce_feedback(s,*source)||
       o.view_y< -65536||o.view_y>65536||o.finger_frame<0||o.finger_frame>5||
       !valid_text(o.text.title)||!valid_text(o.text.name_header)||!o.text.title_widths||
       std::any_of(o.text.title_widths->begin(),o.text.title_widths->end(),[](int w){return w<0||w>4096;}))return {};
    const auto &v=s.commerce_page_data.at(page);
    const auto &list=s.commerce_page_lists.at(page);
    const bool sale=v[0]==1;
    if(list.empty()||list.size()>65536||s.scene.world.world.ai.accounting.funds()<0||
       ((sale||v[1]==1)&&!valid_text(o.text.owned_header))||
       ((sale||v[1]==0)&&!valid_text(o.text.price_header))||
       (!sale&&v[1]==0&&!valid_text(o.text.remaining_header))||
       (v[5]>0&&!valid_text(o.text.feedback)))return {};
    const auto window=steam_startup_window_skin(
        {220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.text.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!window||!box)return {};
    SteamCommerceSkinPlan p;p.title=o.text.title;
    for(const auto &op:window->draws)p.operations.emplace_back(op);
    for(const auto &op:box->draws)p.operations.emplace_back(op);
    const auto text=[&](const std::string &value,std::array<int,4> rect,int anchor,bool layout=false) {
        p.operations.emplace_back(SteamHumanOverviewText{
            SteamHumanOverviewTextRole::equipment_name,value,rect,anchor,0,layout,{92,51,31}});
    };
    if(!sale) {
        const int shift=3*((s.page_counters.at(page)%16)/8);
        p.operations.emplace_back(SteamFacilityArrowTouch{{26-shift,55},3,16});
        p.operations.emplace_back(SteamFacilityArrowTouch{{214+shift,55},0,18});
    }
    text(o.text.name_header,{30,69,0,0},0);
    if(sale)text(o.text.owned_header,{o.japanese?137:128,69,0,0},0);
    else if(v[1]==0)text(o.text.remaining_header,{137,69,40,13},32,true);
    else text(o.text.owned_header,{o.japanese?185:175,69,0,0},0);
    if(sale||v[1]==0)text(o.text.price_header,{o.japanese?185:179,69,0,0},0);
    for(int absolute=v[3];absolute<static_cast<int>(list.size())&&absolute<v[3]+5;++absolute) {
        const int entry=list[absolute],y=97+19*(absolute-v[3]);
        const auto definition=std::find_if(s.rules->items.begin(),s.rules->items.end(),
            [=](const auto &d){return d.identity==entry;});
        const auto read=s.item_commerce_read.find(entry);
        const auto stock=s.shop_item_stock.find(entry);
        const auto owned=s.items.find(entry);
        if(definition==s.rules->items.end()||!valid_text(definition->name)||definition->commerce_price<0||
           read==s.item_commerce_read.end()||stock==s.shop_item_stock.end()||owned==s.items.end()||
           stock->second.quantity<0||owned->second.inventory<0)return {};
        const auto icons=startup_world_item_icon_draws(s,entry);
        if(!icons)return {};
        if(absolute==v[2])p.operations.emplace_back(SteamStartupFrameDraw{
            StartupSkinRect{{23,y-2,191,16},{255,153,55},false}});
        p.operations.emplace_back(SteamHumanEquipmentRow{
            SteamStartupTouch{11,0x20000|absolute,std::array<int,4>{23,y-2,191,16},{},16},{29,0,0,0}});
        if(!read->second)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::catalogue_new,{10,y+2},0,{}});
        if(absolute==v[2])p.operations.emplace_back(SteamFacilityImage{
            SteamFacilityAsset::finger,{21,y+8},o.finger_frame,{}});
        // helper type1是普通道具图标，不是“仅出售mode1”；买卖都保留两层。
        for(const auto &icon:*icons) {
            if(icon.resource!=StartupVisualResource::common||(icon.image!=24&&icon.image!=9)||icon.sprite!=-1)return {};
            p.operations.emplace_back(SteamHumanImage{
                icon.image==24?SteamHumanAsset::equipment_background:SteamHumanAsset::item_icon,
                {30+icon.offset[0],y-2+icon.offset[1]},0,icon.crop});
        }
        text(definition->name,o.japanese?std::array<int,4>{50,y,0,0}:std::array<int,4>{50,y-3,85,17},
             o.japanese?0:32,!o.japanese);
        if(sale||v[1]==0)p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::count,
            SteamFacilityAsset::number05,sale?owned->second.inventory:stock->second.quantity,{161,y+1},0,4,-1});
        if(!sale&&v[1]==1)p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::count,
            SteamFacilityAsset::number05,owned->second.inventory,{210,y+1},0,4,-1});
        else p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::money,
            !sale&&s.scene.world.world.ai.accounting.funds()<definition->commerce_price?
                SteamFacilityAsset::number12:SteamFacilityAsset::number08,
            sale?definition->commerce_price/2:definition->commerce_price,{210,y+1},0,4,-1});
    }
    // 独立first只读；即便尾部不足五项也不反推或修改Owner首行。
    const int count=static_cast<int>(list.size()),denominator=std::max(count,5);
    const int top=85+110*v[3]/denominator,height=550/denominator;
    p.operations.emplace_back(SteamHumanCatalogueScroll{
        {StartupSkinRect{{220,85,5,110},{7,5,78},false},StartupSkinRect{{220,top,5,height+1},
            o.scroll_first_touch&&count>5?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false}},
        SteamStartupTouch{12,0x40000,std::array<int,4>{221,85,3,110},{},0},{3,20,0,0},
        {count,5,0x20000},SteamStartupTouch{25,0,std::array<int,4>{221,85,3,110},{},4}});
    if(v[5]>0) {
        const auto amount=s.commerce_page_feedback_amount.find(page);
        if(amount==s.commerce_page_feedback_amount.end()||amount->second<0)return {};
        text(o.text.feedback,{54,199,0,0},1);
        p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{21,195,18,18}});
        p.operations.emplace_back(SteamCommerceImage{SteamCommerceAsset::private_pumpkin,{30,223},0,{}});
        p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
        const auto digits=sale?SteamFacilityAsset::number05:SteamFacilityAsset::number12;
        p.operations.emplace_back(SteamFacilityNumber{sale?SteamFacilityNumberKind::plus_value:SteamFacilityNumberKind::minus_value,
            digits,amount->second,{201,200},0,4,-1});
        p.operations.emplace_back(SteamFacilityImage{digits,{201,200},20,{}});
    } else {
        const int selected=list[v[2]];
        const auto definition=std::find_if(s.rules->items.begin(),s.rules->items.end(),
            [=](const auto &d){return d.identity==selected;});
        if(definition==s.rules->items.end()||
           std::any_of(o.item_explanation.item_explain_widths.begin(),o.item_explanation.item_explain_widths.end(),
               [](const auto &w){return w&&(*w<0||*w>4096);}))return {};
        const auto explanation=steam_human_item_explain_skin(*definition,o.item_explanation);
        if(!explanation)return {};
        for(const auto &op:explanation->operations) {
            if(const auto *image=std::get_if<SteamFacilityImage>(&op))p.operations.emplace_back(*image);
            else if(const auto *image=std::get_if<SteamHumanImage>(&op))p.operations.emplace_back(*image);
            else if(const auto *value=std::get_if<SteamHumanOverviewText>(&op))p.operations.emplace_back(*value);
            else if(const auto *value=std::get_if<SteamHumanLearningText>(&op))p.operations.emplace_back(*value);
            else return {};
        }
    }
    return p;
}
} // namespace dungeon_village_prototype
