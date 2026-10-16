// Canvas登记/重绘/释放协议；来源ui/STEAM_INTERACTIONS.md，不发现设备或持有SDK图片。
#include "dungeon_village_prototype/steam_joystick_prompts.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
bool valid(const SteamJoystickPromptState &s) {
    if(s.entries.size()>4096||(!s.table_present&&!s.entries.empty()))return false;
    std::set<std::pair<int,int>> keys;
    for(const auto &e:s.entries)
        if(e.button<0||e.button>=44||e.registration<-1||e.life<0||e.life>4||e.scale<0||e.scale>10000||
           !keys.emplace(e.button,e.registration).second)return false;
    return true;
}
int right_label(const std::string &value) {
    return value=="戻る"?0:(value=="メニュー"||value=="ﾒﾆｭｰ")?1:-1;
}
bool dimensions(const std::optional<std::array<int,2>> &size) {
    return size&&(*size)[0]>0&&(*size)[0]<=65536&&(*size)[1]>0&&(*size)[1]<=65536;
}
}
std::optional<bool> steam_joystick_is_shown(
        bool unity_editor,const std::optional<std::int32_t> &last_input_device) {
    if(unity_editor)return true; // 原此分支不取Canvas；固定平台4也不问语言模拟。
    if(!last_input_device)return {};
    return *last_input_device==0||*last_input_device==4;
}
std::optional<SteamJoystickRegisterResult> steam_joystick_register(
    const SteamJoystickPromptState &s,const SteamJoystickRegisterInput &in) {
    if(!valid(s))return {};
    SteamJoystickRegisterResult result{s,false,{}};
    if(!s.table_present)return result;
    const auto &request=in.request;
    if(in.application_type==4&&!in.has_controller) {
        if(request.button!=2&&request.button!=3) {
            if(request.button!=7)return result;
            if(!in.right_soft_label)return {};
            if(right_label(*in.right_soft_label)<0)return result;
        }
    }
    auto anchor=request.anchor;
    if(in.application_type==4) {
        // 原此分支先取四个尺寸，随后才检查offset开关；不用游戏逻辑宽代替应用宽。
        if(!dimensions(in.screen_size)||!dimensions(in.application_size))return {};
        if(in.steam_offset_enabled)for(int axis=0;axis<2;++axis) {
            const auto shifted=std::int64_t(anchor[axis])+((*in.screen_size)[axis]-(*in.application_size)[axis])/2;
            if(shifted<std::numeric_limits<int>::min()||shifted>std::numeric_limits<int>::max())return {};
            anchor[axis]=static_cast<int>(shifted);
        }
    }
    if(request.button<0||request.button>=44||request.registration<-1)return {};
    const auto found=std::find_if(result.state.entries.begin(),result.state.entries.end(),[&](const auto &e){
        return e.button==request.button&&e.registration==request.registration;});
    if(found!=result.state.entries.end()) {
        found->displayed=true;found->anchor=anchor; // reuse不消费传入scale，不复活life。
    } else {
        if(s.entries.size()==4096||request.scale<0||request.scale>10000)return {};
        result.state.entries.push_back({request.button,request.registration,true,anchor,4,request.scale});
        if(in.top_form_present&&request.button<4)result.auto_cancel_button=request.button;
    }
    result.registered=true;
    return result;
}
std::optional<SteamJoystickPromptState> steam_joystick_reset(const SteamJoystickPromptState &s) {
    if(!valid(s))return {};
    auto result=s;for(auto &e:result.entries)e.displayed=false;return result;
}
std::optional<SteamJoystickPromptState> steam_joystick_raise(const SteamJoystickPromptState &s) {
    if(!valid(s))return {};
    auto result=s;for(auto &e:result.entries)e.life=4;return result;
}
std::optional<SteamJoystickPromptState> steam_joystick_release(
    const SteamJoystickPromptState &s,const std::array<bool,24> &released) {
    if(!valid(s))return {};
    auto result=s;
    for(auto &e:result.entries)if(e.displayed&&e.button<24&&released[e.button])e.life=std::max(0,e.life-1);
    return result;
}
std::optional<SteamJoystickDrawPlan> steam_joystick_draw(
    const SteamJoystickPromptState &s,const SteamJoystickDrawInput &in) {
    if(!valid(s))return {};
    SteamJoystickDrawPlan plan;
    if(!s.table_present)return plan;
    if(in.cursor_draw_frame<0||in.cursor_draw_frame>=192)return {};
    float base=1.F;
    if(in.application_type==4) {
        if(!in.scale_ratio||!std::isfinite(*in.scale_ratio)||*in.scale_ratio<=0.F||*in.scale_ratio>10000.F)return {};
        base=((*in.scale_ratio/100.F)*240.F)/1080.F;
    }
    if(in.cursor_draw_frame>=168)base*=1.1F;
    if(base!=1.F)plan.operations.emplace_back(SteamCursorPushScale{base,base,{0,0}});
    for(const auto &e:s.entries) {
        if(!e.displayed)continue;
        int life=e.life;
        if((in.use_steam||in.xbox)&&!in.has_controller&&e.button==7) {
            if(!in.right_soft_label)return {};
            const int index=right_label(*in.right_soft_label);
            if(index>=0) {
                if(!in.right_click_counts)return {};
                life=(*in.right_click_counts)[index];
            }
        }
        if(life<=0)continue;
        if(e.scale!=100) {
            const float scale=float(e.scale)/100.F;
            plan.operations.emplace_back(SteamCursorPushScale{scale,scale,{0,0}});
        }
        // Graph.Easing(0,255,4,life,0)：端点夹255；1/2/3独立结果63/127/191。
        const int alpha=life>=4?255:static_cast<int>((float(life)/4.F)*255.F);
        plan.operations.emplace_back(SteamCursorRenderMode{1,alpha,255-alpha});
        plan.operations.emplace_back(SteamJoystickDrawButton{e.anchor,e.button,0,true});
        plan.operations.emplace_back(SteamCursorRenderMode{0,255,0});
        if(e.scale!=100)plan.operations.emplace_back(SteamCursorPopMatrix{});
    }
    if(base!=1.F)plan.operations.emplace_back(SteamCursorPopMatrix{});
    return plan;
}
} // namespace dungeon_village_prototype
