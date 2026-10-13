// Steam raw17通关计分：有序框/背景/演员/阶段文字/纪录/Touch；业务由应用Owner消费。
#include "dungeon_village_prototype/steam_clear_score_skin.hpp"
#include "dungeon_village_prototype/startup_application.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
constexpr std::array<int,8> durations{75,75,45,45,65,45,65,9999};
constexpr std::array<int,3> brown{92,51,31}, blue{0,100,255};
bool clock(int value) { return value>=0 && value<std::numeric_limits<int>::max(); }
bool measurement(std::optional<int> value) { return value && *value>=0 && *value<=4096; }
void text(SteamClearScorePlan &plan, std::string value, int x, int y, int anchor=2,
          std::array<int,3> color=brown) {
    plan.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        std::move(value),{x,y,0,0},anchor,0,false,color});
}
void frame(SteamClearScorePlan &plan, const SteamStartupFramePlan &source) {
    for(const auto &draw:source.draws)plan.operations.emplace_back(draw);
}
}
std::optional<std::string> steam_clear_score_number(std::int64_t value) {
    if(value<0)return {};
    std::string result=std::to_string(value);
    for(int at=static_cast<int>(result.size())-3;at>0;at-=3)
        result.insert(static_cast<std::size_t>(at),1,',');
    return result;
}
std::optional<SteamClearScoreInput> steam_clear_score_input(const StartupApplication &app,std::uint64_t id) {
    if(!app.world()||!app.clear_page()||!app.clear_rows())return {};
    const auto &s=app.world()->state();const auto &score=*app.clear_page();
    const auto top=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &p){return p.lifecycle!=4;});
    const auto phase=s.page_phases.find(id),counter=s.page_counters.find(id),main=s.page_secondary_counters.find(id);
    if(top==s.scripts.pages.rend()||top->id!=id||top->kind!=ref::WorldScriptPageKind::raw_page||
       top->legacy_page!=17||top->lifecycle!=2||!valid_startup_world_clear_page(s,*top)||score.finished||
       phase==s.page_phases.end()||counter==s.page_counters.end()||main==s.page_secondary_counters.end()||
       phase->second!=score.stage||counter->second!=score.counter||!clock(main->second)||
       validate_startup_clear_score_page(*app.clear_rows(),score)!=StartupClearScoreError::none)return {};
    return SteamClearScoreInput{score.stage,score.counter,main->second,score.row,score.sum,
        score.captured_high_score,*app.clear_rows()};
}
std::optional<SteamClearScorePlan> steam_clear_score_skin(
    const SteamClearScoreInput &in, const SteamClearScoreOptions &o) {
    // Draw_clearPoint VA1030B090–1030BEB6；与Owner奖励/系统文件事务无写入关系。
    if(in.stage<0||in.stage>7||!clock(in.stage_counter)||!clock(in.main_counter)||
       in.row<0||in.row>6||in.sum<0||in.captured_high_score<0||!o.title_widths||
       !measurement((*o.title_widths)[0])||!measurement((*o.title_widths)[1]))return {};
    const bool current_row=(in.stage==1||in.stage==3)&&in.stage_counter>=15;
    if(current_row&&(!in.rows||in.row>=6))return {};
    if(in.rows)for(const auto &row:*in.rows)if(row.count<0||row.score<0)return {};
    SteamClearScorePlan plan;plan.title="通关点数发表";plan.soft_labels={0,0};
    const auto window=steam_startup_window_skin({222,155,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto outer=steam_startup_box_skin({17,81,224,177},o.view_y);
    const auto inner=steam_startup_box_skin({21,85,218,137},o.view_y);
    if(!window||!outer||!inner)return {};
    frame(plan,*window);frame(plan,*outer);
    plan.operations.emplace_back(SteamHumanImage{SteamHumanAsset::ceremony_background,{30,105},0,
        std::array<int,4>{0,0,180,80}});
    frame(plan,*inner);
    const int actor_step=in.stage==4?2*(in.main_counter%8/4):0;
    // 原顺序是大臣后秘书；固定帧1/0，阶段4分别5/7与4/6。
    plan.operations.emplace_back(SteamHumanImage{SteamHumanAsset::president,{190,181},in.stage==4?5+actor_step:1,{}});
    plan.operations.emplace_back(SteamHumanImage{SteamHumanAsset::secretary,{51,181},in.stage==4?4+actor_step:0,{}});
    switch(in.stage) {
    case 0:
        if(in.stage_counter>=15)text(plan,"那么赶紧开始计算",120,105);
        if(in.stage_counter>=45)text(plan,"通关点数吧。",120,127);
        break;
    case 1:
        if(current_row) {
            constexpr const char *names[]{"街道人气","任务完成数","设施发现数",
                "冒险者全员LV合计","自宅数","努力度总和"};
            text(plan,names[in.row],120,105);
            // Steam CPOINT_STR六行单位均为空；数量不是当行得分，不补“个/人/栋”。
            if(in.stage_counter>=45)text(plan,*steam_clear_score_number((*in.rows)[in.row].count),120,127);
        }
        break;
    case 2:
        if(in.stage_counter>=15)text(plan,"通关点数…",120,117);
        break;
    case 3:
        if(current_row) {
            if(!measurement(o.score_widths[0])||!measurement(o.score_widths[1]))return {};
            const int left=120-(*o.score_widths[0]+48)/2;
            text(plan,*steam_clear_score_number((*in.rows)[in.row].score),left,117,0,blue);
            text(plan,"点追加!!",left+*o.score_widths[1],117,0);
        }
        break;
    case 4:text(plan,"排行榜更新！",120,117,2,blue);break;
    case 5:
        if(in.stage_counter>=15)text(plan,"接下来…",120,117);
        break;
    case 6:
        if(in.stage_counter>=15)text(plan,"通关点数的计算",120,105);
        if(in.stage_counter>=40)text(plan,"就是这样。",120,127);
        break;
    case 7:break; // 退休由表单Owner决定，活跃末段仍显示公共框和纪录。
    }
    if((in.stage==0||in.stage==3||in.stage==6)&&in.stage_counter==durations[in.stage]&&in.main_counter%40>20)
        plan.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{213,in.stage==6?167:179},0,{}});
    text(plan,"当前点数:",o.english?33:93,78,o.english?1:4);
    text(plan,*steam_clear_score_number(in.sum)+"点",213,78,4);
    text(plan,"最高分  :",93,192,4,blue);
    text(plan,*steam_clear_score_number(in.captured_high_score)+"点",213,192,4,blue);
    // _draw 17分支返回后VA10355D7F–10355DBB登记全局KEYCLICK，不伪造下方按钮。
    plan.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return plan;
}
} // namespace dungeon_village_prototype
