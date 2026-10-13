// SteamInputManager.DrawButton/DrawKeyboardButton；来源合同见ui/STEAM_INTERACTIONS.md。
#include "dungeon_village_prototype/steam_platform_button_skin.hpp"
#include <cmath>
#include <limits>

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
std::optional<SteamControllerButtonFallbackPlan> steam_controller_button_fallback(
    const SteamControllerButtonFallbackInput &in) {
    if(in.frame<0||in.frame>=44)return {};
    int frame=in.frame;
    if(in.auto_key_reverse&&(frame==0||frame==1)) {
        const auto &mapped=frame==0?in.okay_frame:in.cancel_frame;
        if(!mapped||*mapped<0||*mapped>=44)return {};
        frame=*mapped;
    }
    if(in.layer==1&&frame>=16&&frame<=19)frame+=16;
    SteamControllerButtonResource resource{"steam-platform/buttons_switch.png",
        "steam-platform/buttons_switch.seb",15,13};
    if(in.input_type==5||in.input_type==13)
        resource={"steam-platform/buttons_ps4.png","steam-platform/buttons_ps4.seb",22,19};
    else if(in.input_type==1||in.input_type==2||in.input_type==3)
        resource={"steam-platform/buttons_xbox.png","steam-platform/buttons_xbox.seb",33,29};
    return SteamControllerButtonFallbackPlan{resource,frame,0};
}
std::optional<SteamButtonImagePlan> steam_button_image_layout(const SteamButtonImageInput &in) {
    if(in.width<=0||in.width>65536||in.height<=0||in.height>65536||
       in.anchor[0]<-65536||in.anchor[0]>65536||in.anchor[1]<-65536||in.anchor[1]>65536)return {};
    float maximum{};
    switch(in.kind) {
    case SteamButtonImageKind::dynamic_glyph: maximum=60.F;break;
    case SteamButtonImageKind::right_click: maximum=75.F;break;
    default: return {};
    }
    // 原单精度除法、乘法分别发生；保留非整数尺寸，直到无矩阵分支才截断偏移。
    const float width=static_cast<float>(in.width),height=static_cast<float>(in.height);
    const float ratio=maximum/(width>height?width:height);
    SteamButtonImagePlan plan;
    plan.scaled_size={width*ratio,height*ratio};
    const std::array<float,2> offset{plan.scaled_size[0]*-0.5F,plan.scaled_size[1]*-0.5F};
    plan.draw_anchor=in.anchor;
    if(in.local_matrix)plan.translation=offset;
    else for(int axis=0;axis<2;++axis)plan.draw_anchor[axis]+=static_cast<int>(offset[axis]);
    // 原尾段无论先前是否有矩阵都重新CheckLocalMatrix，消费端依实际结果决定Pop。
    plan.recheck_matrix_before_pop=true;
    return plan;
}
std::optional<SteamLanguageButtonPlan> steam_language_button_skin(const SteamLanguageButtonInput &in) {
    SteamLanguageButtonPlan plan;
    if(!in.switch_enabled)return plan;
    if(in.width<=0||in.width>65536||in.height<=0||in.height>65536||!in.scale_ratio||
       !std::isfinite(*in.scale_ratio)||*in.scale_ratio<=0.F||*in.scale_ratio>10000.F)return {};
    if(in.new_state==2&&(!in.new_frame||*in.new_frame<0||*in.new_frame>=16))return {};
    // 源anchor以居中优先于右/下，其余位不改变本helper分支。
    const int x=(in.anchor&2)?in.width/2:(in.anchor&4)?in.width-20:20;
    const int y=(in.anchor&32)?in.height/2:(in.anchor&64)?in.height-20:20;
    const float scale=in.touched?1.1F*0.5F:0.5F;
    SteamPlatformButtonBody body;
    body.resource={"steam-platform/button_language.png","steam-platform/button_language.seb",
                   0,0,{0,0,60,60},{-30,-30}};
    body.destination={float(x)-30.F*scale,float(y)-30.F*scale,60.F*scale,60.F*scale};
    if(in.touched)body.blend=SteamPlatformBlend{1,{255,255,255,128}};
    body.component=3;body.value=0x133A066;
    body.reset_origin=false; // steps显式持有两层原点，按钮图元不再自行清原点。
    body.touch_rectangle={x-15,y-15,30,30};
    body.margin={16,16,8,8}; // 默认TOUCHPARAMS[3]；不是平台键盘helper的显式四边15。
    body.joystick=SteamPlatformJoystickRequest{9,2,100,
        {int(float(x+14)*(*in.scale_ratio)/100.F),int(float(y-14)*(*in.scale_ratio)/100.F)}};
    plan.body=body;
    using Step=SteamLanguageButtonStep;
    plan.steps={Step::push_origin_zero,Step::replace_callback,Step::push_origin_zero,Step::draw_button};
    if(in.new_state==2) {
        const int next=(*in.new_frame+1)%16;
        plan.new_text=SteamLanguageNewText{{float(x)-0.5F,float(y-next/8+16)},next,8,-1,0.75F};
        plan.steps.push_back(Step::draw_new);
    }
    plan.steps.insert(plan.steps.end(),{Step::register_touch,Step::register_joystick,Step::pop_origin,Step::pop_origin});
    return plan;
}
SteamLanguageTouchResult steam_language_button_touch(int component,int value,int event_type) {
    if(component!=3||value!=0x133A066)return {true,false};
    return {false,event_type==1};
}
std::optional<SteamTitlePlatformPlan> steam_title_platform_skin(const SteamTitlePlatformInput &in) {
    if(in.title_frame<0||in.title_frame>std::numeric_limits<int>::max()-1||in.state<0||in.state>1)return {};
    SteamTitlePlatformPlan plan;
    if(!in.on_top||in.state!=0)return plan;
    const auto language=steam_language_button_skin({in.language_switch_enabled,in.language_touched,
        in.width,in.height,20,in.scale_ratio,in.new_state,in.new_frame});
    if(!language)return {};
    plan.language=*language;
    // 原75门槛位于语言按钮之后；APPLI_TYPE是调用者资格，Platform在helper里另查。
    if(in.title_frame<75||in.application_type!=4)return plan;
    for(const auto kind:{SteamPlatformButtonKind::controller_binding,SteamPlatformButtonKind::keyboard_guide}) {
        const bool keyboard=kind==SteamPlatformButtonKind::keyboard_guide;
        const auto button=steam_platform_button_skin({kind,keyboard?std::array<int,2>{21,51}:
            std::array<int,2>{20,20},2,in.desktop,in.xbox,
            keyboard?in.keyboard_touched:in.controller_touched,in.scale_ratio});
        if(!button)return {};
        if(button->body)plan.after_menu.push_back(*button->body);
    }
    return plan;
}
std::optional<SteamPlatformButtonEventPlan> steam_platform_button_event(const SteamPlatformButtonEventInput &in) {
    if(in.kind!=SteamPlatformButtonKind::controller_binding&&in.kind!=SteamPlatformButtonKind::keyboard_guide)return {};
    SteamPlatformButtonEventPlan plan;
    if(in.debug&&in.event_type==2) {
        plan.actions.emplace_back(SteamCaptureSharedButtonTimeRequest{});
    } else if(in.debug&&in.event_type==13) {
        if(!in.now_millis||!in.shared_down_millis)return {};
        // 固定x86依次sub低/sbb高、js、jg、低位cmp/jbe；jg仍读取sbb的OF/ZF。
        // 极端跨符号值不能简化成回卷后的int64比较，以下保留原flags且无C++有符号溢出。
        const auto now=static_cast<std::uint64_t>(*in.now_millis),down=static_cast<std::uint64_t>(*in.shared_down_millis);
        const auto now_low=static_cast<std::uint32_t>(now),down_low=static_cast<std::uint32_t>(down);
        const auto now_high=static_cast<std::uint32_t>(now>>32),down_high=static_cast<std::uint32_t>(down>>32);
        const std::uint32_t low=now_low-down_low,high=now_high-down_high-std::uint32_t(now_low<down_low);
        const bool overflow=((now_high^down_high)&(now_high^high)&0x80000000U)!=0;
        if((high&0x80000000U)==0&&((high!=0&&!overflow)||low>3000)) {
            plan.actions.emplace_back(SteamDebugTrophyRequest{});
            // 原Push正常返回false也继续读钟；异常流程不由此纯候选执行或吞掉。
            plan.actions.emplace_back(SteamCaptureSharedButtonTimeRequest{});
        }
    }
    if(in.event_type!=5)return plan;
    if(in.kind==SteamPlatformButtonKind::controller_binding) {
        if(!in.use_steam||!in.initialized)return plan;
        if(!in.controller_handle)return {};
        plan.actions.emplace_back(SteamBindingPanelRequest{*in.controller_handle});
    } else {
        if(!in.language_code_resolved||(in.language_code&&in.language_code->size()>4096))return {};
        // 原三串Concat将null视为空；不擅自编码/裁剪/改写语言码，也不实际打开外链。
        plan.actions.emplace_back(SteamControlMapRequest{
            "https://kairosoft.net/steam/?_lh="+in.language_code.value_or("")+"#controlmap"});
    }
    return plan;
}
} // namespace dungeon_village_prototype
