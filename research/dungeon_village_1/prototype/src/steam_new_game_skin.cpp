// Steam raw91 _draw2_4 VA10349D52–1034A925；顺序与当前草稿只读，业务留应用Owner。
#include "dungeon_village_prototype/steam_new_game_skin.hpp"

#include <limits>
#include <utility>

namespace dungeon_village_prototype {
namespace {
constexpr std::array<int,3> brown{92,51,31},blue{0,100,255},red{255,14,1},orange{255,153,55};
bool width(std::optional<int> value) {return value&&*value>=0&&*value<=4096;}
void text(SteamNewGamePlan &plan,std::string value,int x,int y,int anchor,int size,
          std::array<int,3> color) {
    plan.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        std::move(value),{x,y,0,0},anchor,size,false,color});
}
void frame(SteamNewGamePlan &plan,const SteamStartupFramePlan &source) {
    for(const auto &draw:source.draws)plan.operations.emplace_back(draw);
}
}
std::optional<SteamNewGamePlan> steam_new_game_skin(const SteamNewGameInput &in,const SteamNewGameOptions &o) {
    const auto &rules=startup_world_rules();
    // 草稿沿现profile合同允许空串；绘制保留空字段，不补名字或新增非空业务门槛。
    if(in.village.size()>4096||in.name.size()>4096||
       in.sex<0||in.sex>1||in.selection<0||in.selection>3||in.main<0||in.main==std::numeric_limits<int>::max()||
       in.adventure_profession<0||in.adventure_profession>=static_cast<int>(rules.jobs.size())||
       (rules.jobs[in.adventure_profession].flags&4U)==0||o.finger_frame<0||o.finger_frame>5||
       (o.english&&o.japanese)||!o.title_widths||!width((*o.title_widths)[0])||!width((*o.title_widths)[1])||
       !width(o.start_width))return {};
    // 原正常人类DrawDispPlayer传weapon0，而非-1；基础consumer保留武器→身体，无标题阴影。
    const auto actor=startup_title_actor_skin(in.adventure_profession,in.sex,0,in.main%16/4,2,false);
    if(!actor||!actor->weapon||actor->shadow)return {};
    const auto window=steam_startup_window_skin({220,186,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({17,48,219,193},o.view_y);
    if(!window||!box)return {};
    SteamNewGamePlan plan;plan.title="重新开始";
    frame(plan,*window);frame(plan,*box);
    plan.operations.emplace_back(StartupSkinDraw{StartupSkinPackage::event,9,-1,0,0,
        {0,9,180,63},{30,65}});
    // Steam Graphics.DrawRect RVA763590：原179×62转换为半开180×63，消费侧不得再加1。
    plan.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,65,180,63},{52,133,255},true}});
    const int shift=o.english?1:0,label_size=o.english?9:0,value_size=o.english?11:0;
    text(plan,"城镇名称",27,135+shift,0,label_size,blue);
    text(plan,"冒险者姓名",27,157+shift,0,label_size,blue);
    text(plan,"性别",27,178+shift,0,label_size,blue);
    text(plan,in.sex==0?"男":"女",o.japanese?62:70,178,0,0,in.sex==0?brown:red);
    if(in.selection<2)plan.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{
        in.selection==0?std::array<int,4>{116,133,94,16}:std::array<int,4>{122,155,82,16},orange,false}});
    if(in.selection<3) {
        constexpr std::array<std::array<int,2>,3> positions{{{112,143},{118,165},{136,186}}};
        plan.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,positions[in.selection],o.finger_frame,{}});
    }
    text(plan,in.village,163,135,2,value_size,brown);
    text(plan,in.name,163,157,2,value_size,brown);
    plan.operations.emplace_back(SteamHumanPortrait{*actor,{163,195},{},false});
    const int button_width=*o.start_width+16,left=120-button_width/2;
    if(in.selection==3)plan.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{left,206,button_width,16},orange,false}});
    plan.operations.emplace_back(SteamStartupTouch{3,0x20003,std::array<int,4>{left-20,186,*o.start_width+56,56},{},0});
    plan.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        "开始游戏",{left,206,button_width,16},34,0,true,brown});
    if(in.selection==3)plan.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{left-4,215},o.finger_frame,{}});
    constexpr std::array<std::array<int,4>,3> rectangles{{{106,128,114,26},{112,150,102,26},{112,171,102,26}}};
    for(int row=0;row<3;++row)
        plan.operations.emplace_back(SteamStartupTouch{11,0x20000|row,rectangles[row],{},0});
    plan.operations.emplace_back(SteamNewGameArrow{{{118,185},3,o.arrow_selected[0]},18,16,{30,5,20,4}});
    plan.operations.emplace_back(SteamNewGameArrow{{{210,185},0,o.arrow_selected[1]},18,18,{5,20,20,4}});
    return plan;
}
} // namespace dungeon_village_prototype
