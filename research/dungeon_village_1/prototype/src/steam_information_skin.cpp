// Steam raw36具名局部合同：ui/INFORMATION_MENU.md末节；不是APK坐标的同名替代。
#include "dungeon_village_prototype/steam_information_skin.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_visuals.hpp"
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include <algorithm>
#include <limits>
#include <utility>

namespace dungeon_village_prototype {
namespace {
using Role=SteamInformationTextRole;
constexpr std::array<int,3> brown{92,51,31},gray{156,155,155},blue{0,100,255},red{255,14,1};
void text(SteamInformationSkinPlan &plan,Role role,int period,int slot,int x,int y,
          std::optional<int> anchor,std::array<int,3> rgb,int size=0,std::string value={}) {
    plan.draws.emplace_back(SteamInformationText{role,slot,period,{x,y},anchor,rgb,size,std::move(value)});
}
bool frame(SteamInformationSkinPlan &plan,const SteamStartupFramePlan &parts,int period) {
    for(const auto &part:parts.draws) {
        if(const auto *rect=std::get_if<StartupSkinRect>(&part))plan.draws.emplace_back(*rect);
        else if(const auto *image=std::get_if<StartupSkinDraw>(&part))plan.draws.emplace_back(*image);
        else if(const auto *title=std::get_if<SteamStartupText>(&part)) {
            // 本helper只请求标题；已核窗口产生整数坐标、显式颜色及原默认文字锚。
            if(!title->rgb || title->role!=SteamStartupTextRole::message_title)return false;
            text(plan,Role::title,period,-1,static_cast<int>(title->rectangle[0]),
                 static_cast<int>(title->rectangle[1]),title->anchor,*title->rgb,title->font_size);
        }
    }
    return true;
}
void arrow(SteamInformationSkinPlan &plan,int x,int frame,int value,int y=51) {
    const auto index=plan.draws.size();
    // Steam已核common image74/SEB3；SEB自身offset(0,-3)留给消费者且只加一次。
    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,74,3,frame,0,{}, {x,y}});
    plan.touches.push_back({{1,value,{},index,0},{},{}});
}
void sprite(SteamInformationSkinPlan &plan,int image,int seb,int frame,int x,int y) {
    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,image,seb,frame,0,{}, {x,y}});
}
void image(SteamInformationSkinPlan &plan,int image,int width,int height,int x,int y) {
    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,image,-1,0,0,
                                           {0,0,width,height},{x,y}});
}
bool icons(SteamInformationSkinPlan &plan,const std::optional<std::vector<StartupVisualDraw>> &parts,
           int x,int y) {
    if(!parts)return false;
    for(const auto &part:*parts) {
        if(part.resource!=StartupVisualResource::common)return false;
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,part.image,part.sprite,
            part.frame,part.layer,part.crop,{x+part.offset[0],y+part.offset[1]}});
    }
    return true;
}
void row_touch(SteamInformationSkinPlan &plan,int row,int y) {
    // flag0没有marker/闪底，源SetColor(橙色)不等于画选中背景。
    plan.touches.push_back({{11,0x20000|row,std::array<int,4>{3,y-2,231,16},{},0},
                           {0,-20,0,0},{}});
}
void scroll(SteamInformationSkinPlan &plan,int count,int first,int visible,bool first_touch,int track_height=110) {
    const std::array<int,4> rectangle{221,85,3,track_height};
    plan.touches.push_back({{12,0x40000,rectangle,{},0},{3,20,0,0},
                           std::array<int,3>{count,visible,0x20000}});
    // _addTouch的组件12同步调用GameView.DrawVerticalScroll；type0原轨道扩大1。
    const int denominator=std::max(count,visible);
    const int top=85+static_cast<int>(std::int64_t(track_height)*first/denominator);
    const int height=track_height*visible/denominator;
    plan.draws.emplace_back(StartupSkinRect{{220,85,5,track_height},{7,5,78},false});
    plan.draws.emplace_back(StartupSkinRect{{220,top,5,height+1},
        first_touch&&count>visible?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false});
    plan.touches.push_back({{25,0,rectangle,{},4},{},{}});
}
bool directory_frame(SteamInformationSkinPlan &plan,const SteamInformationSkinOptions &options,
                     int raw,int tab) {
    if(!options.title_widths)return false;
    for(const int width:*options.title_widths)if(width<0)return false;
    const auto outer=steam_startup_window_skin(
        {220,168,0,0,SteamStartupTextRole::message_title,(raw==35||raw==38)?tab+1:0},options.view_y,options.title_widths);
    const auto inner=steam_startup_box_skin({17,74,219,184},options.view_y);
    plan.raw=raw;
    return outer&&inner&&frame(plan,*outer,tab)&&frame(plan,*inner,tab);
}
void layout(SteamInformationSkinPlan &plan,int tab,int row,int y,int width,int size,const std::string &value) {
    text(plan,Role::row_name,tab,row,50,y-2,0x20,brown,size,value);
    auto &label=std::get<SteamInformationText>(plan.draws.back());
    label.mode=SteamInformationTextMode::layout;
    label.extent={width,15};
    label.line_space=0;
}
void heading_layout(SteamInformationSkinPlan &plan,Role role,int tab,int x,int y,
                    int width,int height,int anchor,int size=0) {
    text(plan,role,tab,-1,x,y,anchor,brown,size);
    auto &label=std::get<SteamInformationText>(plan.draws.back());
    label.mode=SteamInformationTextMode::layout;
    label.extent={width,height};
    label.line_space=0;
}
void number(SteamInformationSkinPlan &plan,SteamFacilityAsset asset,int value,int x,int y,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number) {
    plan.draws.emplace_back(SteamFacilityNumber{kind,asset,value,{x,y},0,4,-1});
}
}
std::optional<SteamInformationSkinPlan> steam_town_information_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamInformationSkinOptions &options) {
    const auto view=inspect_startup_world_information_page(state,page);
    if(!view||view->raw!=34||!view->town||!options.bottom_width||*options.bottom_width<0)return {};
    SteamInformationSkinPlan plan;
    if(!directory_frame(plan,options,34,0))return {};
    const auto &town=*view->town;
    text(plan,Role::village_name,0,-1,30,69,{},brown,0,view->village_name);
    // Draw_star(207,69,rank,5)首锚固定dx-60，不把末星位置当首星位置。
    for(int slot=0;slot<5;++slot)sprite(plan,13,33,slot<town.rank?0:1,147+12*slot,69);
    const std::array<int,5> statistics{{town.adventurers,town.residents,town.facilities,
                                      town.completed_tasks,town.activities_held}};
    for(int slot=0;slot<5;++slot) {
        const int y=95+20*slot,value=statistics[static_cast<std::size_t>(slot)];
        if(options.japanese)text(plan,Role::town_stat_label,0,slot,27,y,{},blue);
        else {
            text(plan,Role::town_stat_label,0,slot,22,y-2,0x20,blue,slot==0?9:slot==3?8:10);
            auto &label=std::get<SteamInformationText>(plan.draws.back());
            label.mode=SteamInformationTextMode::layout;label.extent={68,15};label.line_space=0;
        }
        if(options.japanese&&slot==0) {
            text(plan,Role::town_adventurer_count,0,slot,109,y,4,brown);
            std::get<SteamInformationText>(plan.draws.back()).argument=value;
        } else number(plan,SteamFacilityAsset::number05,value,109,y+1);
    }
    for(int slot=0;slot<4;++slot) {
        sprite(plan,128,88,slot+1,options.japanese?122:121,91+20*slot);
        const int count=town.known_equipment[static_cast<std::size_t>(slot)];
        text(plan,options.japanese?Role::town_equipment_kinds:Role::town_equipment_count,
             0,slot,options.japanese?211:213,(options.japanese?93:94)+20*slot,4,brown,
             options.japanese?0:11,options.japanese?std::string{}:std::to_string(count));
        std::get<SteamInformationText>(plan.draws.back()).argument=count;
    }
    // Draw_btmMsg以原字符串测宽为输入，橙底、扩展触摸、TextLayout、手形依次输出。
    const int width=std::min(*options.bottom_width,200)+16,left=120-width/2;
    plan.draws.emplace_back(StartupSkinRect{{left,197,width,16},{255,153,55},false});
    plan.touches.push_back({{4,20,std::array<int,4>{left-20,177,width+40,56},{},0},{},{}});
    text(plan,Role::facility_income_list,0,-1,left,197,0x22,brown);
    auto &footer=std::get<SteamInformationText>(plan.draws.back());
    footer.mode=SteamInformationTextMode::layout;footer.extent={width,16};footer.line_space=0;
    sprite(plan,70,21,-1,left-4,206);
    return plan;
}

std::optional<SteamInformationSkinPlan> steam_facility_information_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamInformationSkinOptions &options) {
    const auto view=inspect_startup_world_information_page(state,page);
    if(!view||view->raw!=39||!view->facilities||view->facilities->empty()||
       view->facilities->size()>static_cast<std::size_t>(std::numeric_limits<int>::max()))return {};
    SteamInformationSkinPlan plan;
    if(!directory_frame(plan,options,39,0))return {};
    text(plan,Role::name_header,0,-1,options.japanese?26:28,69,{},brown);
    text(plan,Role::facility_profit_header,0,-1,options.japanese?185:179,69,{},brown);
    const int count=static_cast<int>(view->facilities->size());
    for(int row=view->first_visible;row<count&&row-view->first_visible<5;++row) {
        const auto &facility=(*view->facilities)[static_cast<std::size_t>(row)];
        if(facility.icon<0||facility.icon>=7||facility.ordinal<0||
           facility.ordinal==std::numeric_limits<int>::max())return {};
        const int y=97+19*(row-view->first_visible);
        if(row==view->selection)
            plan.draws.emplace_back(StartupSkinRect{{23,y-2,191,16},{255,153,55},false});
        row_touch(plan,row,y);
        if(row==view->selection)sprite(plan,70,21,-1,21,y+8);
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,91,-1,0,0,
                                               {16*facility.icon,0,16,16},{30,y-2}});
        text(plan,Role::facility_name,0,row,48,y,0x20,brown,0,facility.name);
        auto &name=std::get<SteamInformationText>(plan.draws.back());
        name.mode=SteamInformationTextMode::layout;name.extent={110,11};name.line_space=0;
        name.argument=facility.ordinal+1;
        // Steam先neg int32再扩long：INT_MIN仍负；不使用溢出的C++取负或64位abs。
        const bool negative=facility.profit<0;
        const int amount=negative&&facility.profit!=std::numeric_limits<std::int32_t>::min()
            ?-facility.profit:facility.profit;
        number(plan,negative?SteamFacilityAsset::number12:SteamFacilityAsset::number05,
               amount,210,y+1,SteamFacilityNumberKind::money);
    }
    scroll(plan,count,view->first_visible,5,options.scroll_first_touch);
    // 原普通DrawString含<btn>标记；不额外生成底部按钮热区、选中底或第二个手形。
    text(plan,Role::facility_tracking_hint,0,-1,120,198,2,blue);
    return plan;
}

std::optional<SteamInformationSkinPlan> steam_adventurer_information_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamInformationSkinOptions &options) {
    const auto view=inspect_startup_world_information_page(state,page);
    if(!view||view->raw!=35||!view->humans||view->humans->empty()||
       view->humans->size()>static_cast<std::size_t>(std::numeric_limits<int>::max()))return {};
    SteamInformationSkinPlan plan;
    const int tab=view->selection_or_period;
    if(tab<0||tab>3||!directory_frame(plan,options,35,tab))return {};
    const int shift=3*((view->frame%16)/8);
    arrow(plan,26-shift,3,16,55);arrow(plan,214+shift,0,18,55);
    if(tab==0) {
        text(plan,Role::name_header,tab,-1,30,69,{},brown);
        image(plan,38,17,10,133,70);
        sprite(plan,50,60,0,179,72);
    } else if(tab==1) {
        heading_layout(plan,Role::satisfaction_header,tab,47,69,40,12,0x22);
        heading_layout(plan,Role::effort_header,tab,95,69,40,12,0x22);
        heading_layout(plan,Role::equipment_header,tab,142,69,60,12,0x22);
    } else if(tab==2) {
        for(int slot=0;slot<4;++slot)sprite(plan,37,98,slot,(options.english?48:53)+36*slot,67);
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,35,-1,0,0,
                                               {14,0,14,14},{199,67}});
    } else if(options.japanese) {
        text(plan,Role::contribution_header,tab,-1,82,69,2,brown);
        sprite(plan,31,44,4,116,67);
        text(plan,Role::town_points_header,tab,-1,136,69,1,brown);
        text(plan,Role::spending_header,tab,-1,196,69,2,brown);
    } else {
        heading_layout(plan,Role::contribution_header,tab,48,70,60,10,0x20,10);
        sprite(plan,31,44,4,114,67);
        heading_layout(plan,Role::town_points_header,tab,132,70,40,10,0x20,10);
        heading_layout(plan,Role::spending_header,tab,179,70,40,10,0x22,10);
    }
    const int count=static_cast<int>(view->humans->size());
    for(int row=view->first_visible;row<count&&row-view->first_visible<5;++row) {
        const auto &entry=(*view->humans)[static_cast<std::size_t>(row)];
        const auto &human=entry.details;
        const int y=97+19*(row-view->first_visible);
        if(row==view->selection)
            plan.draws.emplace_back(StartupSkinRect{{23,y-3,191,18},{255,153,55},false});
        plan.touches.push_back({{11,0x20000|row,std::array<int,4>{3,y-3,231,18},{},0},
                               {0,-20,0,0},{}});
        plan.draws.emplace_back(StartupSkinRect{{29,y-2,16,16},{196,236,169},false});
        // Steam Graphics.DrawRect的15×15参数含端点，半开描边为16×16；clip仍15×15。
        plan.draws.emplace_back(StartupSkinRect{{29,y-2,16,16},{204,204,204},true});
        plan.draws.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,{29,y-2,15,15}});
        const auto body=startup_title_actor_skin(human.profession,human.sex,-1,0,1,false);
        if(!body)return {};
        plan.draws.emplace_back(SteamInformationHumanBody{body->body,{37,y+21}});
        plan.draws.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
        if(entry.newly_unlocked)image(plan,147,20,9,10,y+2);
        if(row==view->selection)sprite(plan,70,21,-1,21,y+8);
        if(tab==0) {
            if(human.level<1||human.level>10||human.experience<0||human.threshold<=0)return {};
            text(plan,Role::row_name,tab,row,48,y,{},brown,0,human.name);
            if(human.level==10)image(plan,129,20,6,132,y+2);
            else number(plan,SteamFacilityAsset::number03,human.level,147,y);
            plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,87,-1,0,0,
                                                   {0,0,44,5},{164,y+4}});
            plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,87,-1,0,0,
                                                   {82,0,1,5},{164,y+4}});
            // Steam int RateConvert先有界检查，再整数乘42/阈值向零截断。
            // 有效等级/难度阈值内乘法不溢出；int64也让超阈值的已存经验安全夹到42。
            const int width=human.level==10?42:static_cast<int>(std::clamp(
                std::int64_t(human.experience)*42/human.threshold,std::int64_t{0},std::int64_t{42}));
            plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,87,-1,0,0,
                                                   {2,5,width,3},{165,y+5}});
        } else if(tab==1) {
            number(plan,SteamFacilityAsset::number05,human.satisfaction,80,y+1);
            number(plan,SteamFacilityAsset::number05,human.effort,119,y+1);
            for(int slot=0;slot<4;++slot) {
                const auto equipment=human.equipment[static_cast<std::size_t>(slot)];
                if(equipment) {
                    if(!icons(plan,startup_world_equipment_icon_draws(state,slot==0?1:slot==3?3:2,*equipment),
                              141+18*slot,y-3))return {};
                } else {
                    if(slot==0)return {};
                    // Draw_icon mode5直接使用dx/dy；不能套普通道具mode1的背景(-1,-1)。
                    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,24,-1,0,0,
                                                           {126,0,18,18},{141+18*slot,y-3}});
                }
            }
        } else if(tab==2) {
            for(int slot=0;slot<4;++slot)
                number(plan,SteamFacilityAsset::number08,human.combat[static_cast<std::size_t>(slot)],80+36*slot,y+2);
            number(plan,SteamFacilityAsset::number05,human.medals,211,y+1);
        } else {
            number(plan,SteamFacilityAsset::number05,entry.contribution,83,y+1);
            sprite(plan,104,13,13,84,y+(options.japanese?0:2));
            number(plan,SteamFacilityAsset::number11,entry.yearly_town_points,152,y+1);
            number(plan,SteamFacilityAsset::number08,entry.yearly_spending,212,y+1,SteamFacilityNumberKind::money);
        }
    }
    scroll(plan,count,view->first_visible,5,options.scroll_first_touch,111);
    text(plan,Role::adventurer_count,tab,-1,120,200,2,brown);
    auto &footer=std::get<SteamInformationText>(plan.draws.back());
    footer.mode=SteamInformationTextMode::rich_text;
    footer.extent={-1,-1};
    footer.argument=count;
    return plan;
}
std::optional<SteamInformationSkinPlan> steam_income_information_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,
    const SteamInformationSkinOptions &options) {
    if(!options.title_widths)return {};
    for(const int width:*options.title_widths)
        if(width<0)return {};
    const auto view=inspect_startup_world_information_page(state,page);
    if(!view || view->raw!=36 || !view->income)return {};
    const int period=view->selection_or_period;
    const SteamStartupWindow window{228,176,0,0,SteamStartupTextRole::message_title,period+1};
    const auto outer=steam_startup_window_skin(window,options.view_y,options.title_widths);
    const auto inner=steam_startup_box_skin({12,69,227,190},options.view_y);
    if(!outer || !inner)return {};
    SteamInformationSkinPlan plan;
    if(!frame(plan,*outer,period) || !frame(plan,*inner,period))return {};
    const int shift=3*((view->frame%16)/8);
    arrow(plan,22-shift,3,16);
    arrow(plan,218+shift,0,18);
    // 页签先画；非日文仅接下来的收入/支出表头临时请求10字号。
    text(plan,Role::period,period,-1,options.japanese?28:26,65,{},brown);
    text(plan,Role::income_header,period,-1,options.japanese?121:108,65,{},gray,options.japanese?0:10);
    text(plan,Role::expense_header,period,-1,options.japanese?194:178,65,{},gray,options.japanese?0:10);
    for(int slot=0;slot<5;++slot) {
        const int y=90+18*slot;
        text(plan,Role::category,period,slot,22,y,{},brown);
        if(slot<4)
            plan.draws.emplace_back(SteamInformationLine{{22,y+14},{220,y+14},1,{199,223,148}});
        const auto &row=view->income->rows[static_cast<std::size_t>(slot)];
        text(plan,Role::income,period,slot,143,y,4,blue,0,row.income_text);
        text(plan,Role::expense,period,slot,216,y,4,red,0,row.expense_text);
    }
    plan.draws.emplace_back(SteamInformationLine{{22,180},{220,180},1,{146,184,247}});
    text(plan,Role::profit_label,period,-1,22,184,{},brown);
    text(plan,Role::profit_value,period,-1,216,184,4,view->income->profit<0?red:blue,0,
         view->income->profit_text);
    // 原Draw_btmMsg仍测宽但两个false跳过底色和手形，不产生触摸热区。
    text(plan,Role::description,period,-1,120,204,2,brown);
    return plan;
}

std::optional<std::vector<StartupSkinDraw>> steam_information_number_draws(
    const SteamInformationNumber &number) {
    using Kind=SteamInformationNumberKind;
    if(number.value<=0 || (number.kind!=Kind::inventory_count&&number.kind!=Kind::positive_attribute))return {};
    const bool inventory=number.kind==Kind::inventory_count;
    if(inventory&&number.value>999)return {};
    // 库存1..999使原Comma(p0,c0,anchor4)无逗号/负号，与8步宽普通数字逐项相同。
    // 仅复用其已核展开，不把这种有界等价扩称为两个原helper算法相同。
    const auto x=std::int64_t(number.position[0])-(inventory?10:0);
    if(x<std::numeric_limits<int>::min()||x>std::numeric_limits<int>::max())return {};
    const SteamFacilityNumber request{
        inventory?SteamFacilityNumberKind::number:SteamFacilityNumberKind::plus_value,
        SteamFacilityAsset::number05,number.value,{static_cast<int>(x),number.position[1]},0,4,-1};
    const auto digits=steam_facility_number_draws(request,8);
    if(!digits)return {};
    std::vector<StartupSkinDraw> result;
    for(const auto &digit:*digits)
        result.push_back({StartupSkinPackage::common,103,12,digit.frame,0,{},digit.position});
    if(inventory)result.push_back({StartupSkinPackage::common,85,76,2,0,{},
                                   {static_cast<int>(x),number.position[1]}});
    return result;
}

std::optional<SteamInformationSkinPlan> steam_item_information_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamInformationSkinOptions &options) {
    const auto view=inspect_startup_world_information_page(state,page);
    if(!view||view->raw!=37||!view->items||view->items->empty())return {};
    SteamInformationSkinPlan plan;
    if(!directory_frame(plan,options,37,0))return {};
    text(plan,Role::name_header,0,-1,30,69,{},brown);
    text(plan,Role::inventory_header,0,-1,options.japanese?185:175,69,{},brown);
    const int count=static_cast<int>(view->items->size());
    for(int row=view->first_visible;row<count&&row-view->first_visible<5;++row) {
        const auto &item=(*view->items)[static_cast<std::size_t>(row)];
        const int y=97+19*(row-view->first_visible);
        row_touch(plan,row,y);
        if(item.newly_unlocked)image(plan,147,20,9,10,y+2);
        if(row==view->selection)sprite(plan,70,21,-1,21,y+8);
        if(!icons(plan,startup_world_item_icon_draws(state,item.definition),30,y-2))return {};
        layout(plan,0,row,y,130,0,item.name);
        plan.draws.emplace_back(SteamInformationNumber{SteamInformationNumberKind::inventory_count,
                                                       item.inventory,{210,y+1}});
    }
    scroll(plan,count,view->first_visible,5,options.scroll_first_touch);
    text(plan,Role::item_description,0,view->selection,120,200,2,brown,0,
         (*view->items)[static_cast<std::size_t>(view->selection)].description);
    return plan;
}

std::optional<SteamInformationSkinPlan> steam_equipment_information_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamInformationSkinOptions &options) {
    const auto view=inspect_startup_world_information_page(state,page);
    if(!view||view->raw!=38||!view->equipment)return {};
    SteamInformationSkinPlan plan;
    const auto &equipment=*view->equipment;
    const int tab=view->selection_or_period;
    if(!directory_frame(plan,options,38,tab))return {};
    const int shift=3*((view->frame%16)/8);
    arrow(plan,26-shift,3,16,55);arrow(plan,214+shift,0,18,55);
    sprite(plan,128,88,tab+1,25,66);
    sprite(plan,37,98,equipment.attributes[0],137,67);
    sprite(plan,37,98,equipment.attributes[1],180,67);
    const int count=static_cast<int>(equipment.rows.size());
    if(count==0)text(plan,Role::empty_directory,tab,-1,120,97,2,brown);
    for(int row=view->first_visible;row<count&&row-view->first_visible<4;++row) {
        const auto &item=equipment.rows[static_cast<std::size_t>(row)];
        const int y=97+24*(row-view->first_visible);
        row_touch(plan,row,y);
        if(!item.visible)text(plan,Role::unknown_row,tab,row,30,y,{},gray);
        else {
            const auto &shown=*item.visible;
            const int kind=tab==0?1:tab==3?3:2;
            if(!icons(plan,startup_world_equipment_icon_draws(state,kind,item.definition),30,y-3))return {};
            if(options.japanese)text(plan,Role::row_name,tab,row,50,y,{},brown,0,shown.name);
            else layout(plan,tab,row,y,85,11,shown.name);
            for(int column=0;column<2;++column) {
                const auto value=shown.values[static_cast<std::size_t>(column)];
                if(value)plan.draws.emplace_back(SteamInformationNumber{
                    SteamInformationNumberKind::positive_attribute,*value,{164+43*column,y+1}});
                else text(plan,Role::attribute_placeholder,tab,row,146+43*column,y-1,{},brown,0,"--");
            }
            if(shown.newly_unlocked)image(plan,148,23,9,10,y+2);
        }
        if(row==view->selection)sprite(plan,70,21,-1,21,y+8);
    }
    scroll(plan,count,view->first_visible,4,options.scroll_first_touch);
    text(plan,Role::known_count,tab,-1,120,200,2,brown);
    auto &footer=std::get<SteamInformationText>(plan.draws.back());
    footer.mode=SteamInformationTextMode::rich_text;
    footer.extent={-1,-1}; // 原DrawString2点重载：无限定宽高，使用全局行距。
    footer.argument=static_cast<int>(equipment.known_count);
    return plan;
}

std::optional<std::string_view> steam_information_image(int image) {
    switch(image) {
    case 9:return "original/common/tresureIcon00.png";
    case 12:return "original/common/icon_weapon00.png";
    case 13:return "original/common/icon_star00.png";
    case 20:return "original/common/icon_armour00.png";
    case 21:return "original/common/icon_accessry00.png";
    case 24:return "original/common/icon_back00.png";
    case 28:return "original/common/wnd_back.png";
    case 29:return "original/common/wnd_bar.png";
    case 30:return "original/common/wnd_conner.png";
    case 31:return "steam-common/icon_result00.png";
    case 35:return "original/common/icon_medal00.png";
    case 37:return "steam-facility-common/original/common/icon_param00.png";
    case 38:return "original/common/wnd_lv.png";
    case 50:return "original/common/wnd_exp.png";
    case 70:return "original/common/finger_r.png";
    case 74:return "original/common/arrow02.png";
    case 85:return "steam-common/menuRT01.png";
    case 87:return "original/common/wnd_expBar.png";
    case 91:return "original/common/icon_tenantInfo.png";
    case 102:return "original/common/number03.png";
    case 103:return "steam-build-common/original/common/number05.png";
    case 104:return "original/common/number06.png";
    case 105:return "steam-facility-common/original/common/number08.png";
    case 108:return "original/common/number11.png";
    case 109:return "steam-common/number12.png";
    case 128:return "steam-common/icon_objRoots.png";
    case 129:return "original/common/wnd_max.png";
    case 147:return "original/common/wnd_new.png";
    case 148:return "original/common/wnd_get.png";
    default:return {};
    }
}
} // namespace dungeon_village_prototype
