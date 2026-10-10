// Steam raw36具名局部合同：ui/INFORMATION_MENU.md末节；不是APK坐标的同名替代。
#include "dungeon_village_prototype/steam_information_skin.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
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
void arrow(SteamInformationSkinPlan &plan,int x,int frame,int value) {
    const auto index=plan.draws.size();
    // Steam已核common image74/SEB3；SEB自身offset(0,-3)留给消费者且只加一次。
    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,74,3,frame,0,{}, {x,51}});
    plan.touches.push_back({1,value,{},index,0});
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
} // namespace dungeon_village_prototype
