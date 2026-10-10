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
void scroll(SteamInformationSkinPlan &plan,int count,int first,int visible,bool first_touch) {
    const std::array<int,4> rectangle{221,85,3,110};
    plan.touches.push_back({{12,0x40000,rectangle,{},0},{3,20,0,0},
                           std::array<int,3>{count,visible,0x20000}});
    // _addTouch的组件12同步调用GameView.DrawVerticalScroll；type0原轨道扩大1。
    const int denominator=std::max(count,visible);
    const int top=85+static_cast<int>(std::int64_t(110)*first/denominator);
    const int height=110*visible/denominator;
    plan.draws.emplace_back(StartupSkinRect{{220,85,5,110},{7,5,78},false});
    plan.draws.emplace_back(StartupSkinRect{{220,top,5,height+1},
        first_touch&&count>visible?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false});
    plan.touches.push_back({{25,0,rectangle,{},4},{},{}});
}
bool directory_frame(SteamInformationSkinPlan &plan,const SteamInformationSkinOptions &options,
                     int raw,int tab) {
    if(!options.title_widths)return false;
    for(const int width:*options.title_widths)if(width<0)return false;
    const auto outer=steam_startup_window_skin(
        {220,168,0,0,SteamStartupTextRole::message_title,raw==38?tab+1:0},options.view_y,options.title_widths);
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
    case 20:return "original/common/icon_armour00.png";
    case 21:return "original/common/icon_accessry00.png";
    case 24:return "original/common/icon_back00.png";
    case 28:return "original/common/wnd_back.png";
    case 29:return "original/common/wnd_bar.png";
    case 30:return "original/common/wnd_conner.png";
    case 37:return "steam-facility-common/original/common/icon_param00.png";
    case 70:return "original/common/finger_r.png";
    case 74:return "original/common/arrow02.png";
    case 85:return "steam-common/menuRT01.png";
    case 103:return "steam-build-common/original/common/number05.png";
    case 128:return "steam-common/icon_objRoots.png";
    case 147:return "original/common/wnd_new.png";
    case 148:return "original/common/wnd_get.png";
    default:return {};
    }
}
} // namespace dungeon_village_prototype
