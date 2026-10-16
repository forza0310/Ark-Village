// ?????setter??????Owner????????????????????
#include "dungeon_village_prototype/steam_config_skin.hpp"

namespace dungeon_village_prototype {
std::optional<SteamConfigUpdatePlan> steam_config_english_update(const SteamConfigUpdateInput &in) {
    if(in.state.selection<0||in.state.selection>=4)return {};
    for(int row=0;row<4;++row)
        if(in.state.values[row]<0||in.state.values[row]>(row<2?1:5))return {};
    SteamConfigUpdatePlan plan;plan.candidate=in.state;
    auto &state=plan.candidate;
    plan.checked_keys[0]=true;
    if(in.up_repeat)state.selection=(state.selection+3)%4;
    plan.checked_keys[1]=true;
    if(in.down_repeat)state.selection=(state.selection+1)%4;
    const int row=state.selection,before=state.values[row],count=row<2?2:6;
    int display=row<2?(before+1)%2:before;
    // 键盘禁改整个未解锁速度行，区别于Draw只禁高速选项的Touch注册。
    if(row!=1||state.speed_unlocked) {
        plan.checked_keys[2]=true;
        if(in.left_repeat)display=(display+count-1)%count;
        plan.checked_keys[3]=true;
        if(in.right_repeat)display=(display+1)%count;
        const int after=row<2?(display+1)%2:display;
        if(after!=before) {
            state.values[row]=after;
            plan.operations.emplace_back(SteamConfigSettingRequest{row,before,after});
        }
    }
    plan.checked_keys[4]=true;
    if(in.back_soft_label) {
        plan.operations.emplace_back(SteamConfigReturnRequest::save_system);
        plan.operations.emplace_back(SteamConfigReturnRequest::reset_offscreen);
        plan.operations.emplace_back(SteamConfigReturnRequest::pop);
    }
    return plan;
}
// Steam MyFormBase.SetTouchValue RVA200FC0；APK b/d.java:191–244独立交叉。
// 拆分为只读请求供唯一执行者消费，不在计划生成时重复setter或真实平台调用。
std::optional<SteamConfigSetterPlan> steam_config_setting_effects(const SteamConfigSettingRequest &request) {
    if(request.row<0 || request.row>=4)return {};
    const int maximum=request.row<2?1:5;
    if(request.before<0 || request.before>maximum || request.after<0 || request.after>maximum)return {};
    SteamConfigSetterPlan plan;
    if(request.row<2) {
        plan.operations.emplace_back(SteamConfigPropertyWrite{
            SteamConfigProperty::world,request.row==0?12:13,request.after});
        if(request.row==0) {
            plan.operations.emplace_back(SteamConfigLinearFilter{request.after==1});
            plan.operations.emplace_back(SteamConfigSetterAction::draw_all_forms);
        }
    } else if(request.before!=request.after) {
        const int channel=request.row-2;
        plan.operations.emplace_back(SteamConfigPropertyWrite{
            SteamConfigProperty::system,channel==0?3:4,request.after});
        plan.operations.emplace_back(SteamConfigVolume{channel,request.after*51});
        plan.operations.emplace_back(SteamConfigMute{channel,request.after==0});
        plan.operations.emplace_back(SteamConfigSetterAction::reflect_audio);
        if(channel==1)plan.operations.emplace_back(SteamConfigResourceSound{7});
    }
    return plan;
}
} // namespace dungeon_village_prototype
