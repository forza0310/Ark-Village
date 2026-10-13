// SteamInputManager.DrawButton/DrawKeyboardButton；来源合同见ui/STEAM_INTERACTIONS.md。
#include "dungeon_village_prototype/steam_platform_button_skin.hpp"
#include <cmath>

namespace dungeon_village_prototype {
std::optional<SteamPlatformButtonResource> steam_platform_button_resource(SteamPlatformButtonKind kind) {
    switch(kind) {
    case SteamPlatformButtonKind::controller_binding:
        return SteamPlatformButtonResource{"steam-platform/button_gamepad.png","steam-platform/button_gamepad.seb",
            27,23,{0,0,64,60},{-32,-30}};
    case SteamPlatformButtonKind::keyboard_guide:
        return SteamPlatformButtonResource{"steam-platform/button_keyboard.png","steam-platform/button_keyboard.seb",
            29,25,{0,0,64,42},{-32,-21}};
    }
    return {};
}
std::optional<SteamPlatformButtonPlan> steam_platform_button_skin(const SteamPlatformButtonInput &in) {
    const auto resource=steam_platform_button_resource(in.kind);
    if(!resource||in.component<0||in.component>65535||in.anchor[0]<-65536||in.anchor[0]>65536||
       in.anchor[1]<-65536||in.anchor[1]>65536)return {};
    SteamPlatformButtonPlan plan;
    const bool keyboard=in.kind==SteamPlatformButtonKind::keyboard_guide;
    if(!in.desktop||(keyboard&&in.xbox))return plan;
    if(!keyboard&&(!in.scale_ratio||!std::isfinite(*in.scale_ratio)||*in.scale_ratio<=0||*in.scale_ratio>10000))return {};
    const float scale=in.touched?1.1F*0.5F:0.5F;
    SteamPlatformButtonBody body;
    body.resource=*resource;
    // 原T(anchor)*S(scale)*T(SEB offset)：锚不缩放；不把外层表单origin加回来。
    body.destination={float(in.anchor[0])+float(resource->offset[0])*scale,
        float(in.anchor[1])+float(resource->offset[1])*scale,
        float(resource->crop[2])*scale,float(resource->crop[3])*scale};
    if(in.touched)body.blend=SteamPlatformBlend{1,{255,255,255,128}};
    body.component=in.component;
    body.value=keyboard?0x13489A9:0x13465BA;
    // 两种按钮均以SEB23的64×60构造普通0.5尺度热区，键盘图高与高亮无权改它。
    body.touch_rectangle={in.anchor[0]-16,in.anchor[1]-15,32,30};
    if(!keyboard)body.joystick=SteamPlatformJoystickRequest{8,24,100,
        {int(float(in.anchor[0]-14)*(*in.scale_ratio)/100.F),
         int(float(in.anchor[1]-14)*(*in.scale_ratio)/100.F)}};
    plan.body=body;
    return plan;
}
} // namespace dungeon_village_prototype
