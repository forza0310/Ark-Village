// 来源：ui/STEAM_INTERACTIONS.md虚拟屏幕光标；仅维护平台绘制边界，不持世界状态。
#include "ark/simulation/presentation/steam_cursor_skin.hpp"
#include <algorithm>
#include <cmath>

namespace ark::simulation {
namespace {
// Graph.Easing中间单精度阶段不能化简成一次double三角式；Sin内部才升double。
float recovery_ratio(int frame) {
    const float t=static_cast<float>(frame)/12.F;
    constexpr float pi=3.1415927410125732421875F;
    const float angle=((t*pi)*180.F)/pi;
    const double radians=(static_cast<double>(angle)*3.14159265358979323846)/180.0;
    const float wave=static_cast<float>(std::sin(radians));
    const float strength=-80.F/314.159271240234375F;
    return wave*strength+t;
}
}
bool steam_cursor_pressed(const SteamCursorPressInput &in) {
    const bool primary=in.reverse?(in.b_key==8&&in.b_pressed):(in.a_key==271&&in.a_pressed);
    return primary||(((in.gui_mode==0&&in.keyboard_y)||in.steam_y)&&in.y_key==271);
}
std::optional<SteamCursorSkinPlan> steam_cursor_skin(const SteamCursorSkinInput &in) {
    if(in.draw_frame<0||in.draw_frame>=192)return {};
    SteamCursorSkinPlan plan;
    plan.next_draw_frame=in.draw_frame==191?0:in.draw_frame+1;
    if(in.joystick||!in.position)return plan;
    if(in.recover<0||in.recover>12)return {};
    SteamCursorDraw draw;
    for(int axis=0;axis<2;++axis) {
        const float value=(*in.position)[axis];
        if(!std::isfinite(value)||value<-65536.F||value>65536.F)return {};
        draw.anchor[axis]=static_cast<int>(value);
        if(in.application_type==4) {
            const int screen=in.screen_size[axis],canvas=in.canvas_size[axis];
            if(screen<=0||screen>65536||canvas<=0||canvas>65536)return {};
            draw.anchor[axis]+=(screen-canvas)/2;
        }
    }
    draw.frame=in.pressed?1:0;
    plan.operations.emplace_back(draw);
    if(in.recover>0) {
        // 原端点直接返回v1；中间float乘加与int向零截断、范围夹限保持原次序。
        const float ratio=in.recover==12?1.F:recovery_ratio(in.recover);
        const float scale=in.recover==12?12.F:std::clamp(ratio*11.F+1.F,1.F,12.F);
        const int alpha=in.recover==12?160:std::clamp(static_cast<int>(ratio*160.F),0,160);
        plan.operations.emplace_back(SteamCursorPushScale{scale,scale,{0,0}});
        plan.operations.emplace_back(SteamCursorRenderMode{1,alpha,255-alpha});
        plan.operations.emplace_back(draw);
        // 原无参重置写默认值，不恢复先前的ambient混合；正常Pop仅弹本次scale。
        plan.operations.emplace_back(SteamCursorRenderMode{0,255,0});
        plan.operations.emplace_back(SteamCursorPopMatrix{});
    }
    return plan;
}
} // namespace ark::simulation
