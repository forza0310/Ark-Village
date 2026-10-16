// 已核GameView通用叶分支，不复制surface命中、平台键盘或具体页面业务。
#include "dungeon_village_prototype/steam_touch_commands.hpp"

namespace dungeon_village_prototype {
namespace {
std::int32_t low_short(std::int32_t packed) {
    const auto low=static_cast<std::uint32_t>(packed)&0xffffU;
    return low<0x8000U?static_cast<std::int32_t>(low):static_cast<std::int32_t>(low)-0x10000;
}
bool set_values(SteamTouchCommandPlan &plan,std::uint32_t type,std::int32_t value,
                const std::vector<std::int32_t> &extended) {
    if(extended.size()>4096)return false;
    // Set(accessor,type,value,c)也先掩类型；轨条传入的低位不是属性类型。
    type&=0xffff0000U;
    if(type)plan.commands.emplace_back(SteamTouchValueWrite{type,value});
    for(const auto packed:extended)
        plan.commands.emplace_back(SteamTouchValueWrite{
            static_cast<std::uint32_t>(packed)&0xffff0000U,low_short(packed)});
    return true;
}
bool packed_set(SteamTouchCommandPlan &plan,std::int32_t packed,const SteamTouchCommandInput &in) {
    return set_values(plan,static_cast<std::uint32_t>(packed)&0xffff0000U,
                      low_short(packed),in.extended_values);
}
}
std::optional<SteamTouchCommandPlan> steam_touch_commands(const SteamTouchCommandInput &in) {
    SteamTouchCommandPlan plan;
    if(in.admission==SteamTouchAdmission::intercepted)return plan;
    if(in.admission!=SteamTouchAdmission::ready||in.event_type<0||in.event_type>13)return {};
    const int id=in.component,event=in.event_type;
    if(id==0) {
        // 标题ENTER写完整选择值；UP只发键，不重复Set或消费扩展。
        if(event==2&&!set_values(plan,0x30000U,in.value,in.extended_values))return {};
        if(event==1)plan.commands.emplace_back(SteamTouchKeyClick{in.first_key.value_or(0x100000U)});
        return plan;
    }
    if(id==1||id==18) {
        if(event!=0&&event!=6&&event!=10)return plan;
        const auto bits=in.first_key.value_or(std::uint32_t{1}<<(static_cast<std::uint32_t>(in.value)&31U));
        if(event!=0)plan.commands.emplace_back(SteamTouchKeyUp{bits});
        else {
            if(!in.arrow_paint)return {};
            plan.commands.emplace_back(SteamTouchKeyDown{bits,true});
            plan.commands.emplace_back(SteamTouchArrowEffect{*in.arrow_paint,
                in.arrow_paint->frame==3?2:3,in.value==16?-10.f:10.f});
        }
        return plan;
    }
    if(id==19) {
        if(event==9&&!set_values(plan,static_cast<std::uint32_t>(in.value),in.delta_x,in.extended_values))return {};
        return plan;
    }
    if(id==2||id==9||id==22||id==24) {
        const bool set=id==22?event==13:event==2||event==1;
        if(set&&!packed_set(plan,in.value,in))return {};
        if(id!=22&&event==1)
            plan.commands.emplace_back(SteamTouchKeyClick{id==24?0x100000U:in.first_key.value_or(0x100000U)});
        return plan;
    }
    if(id!=4&&id!=5&&id!=6&&id!=7&&id!=20&&id!=21)return {};
    // 20/21实际路由到4。名字KEYUP指触摸触发边沿，不代表Canvas.KeyUp。
    const bool click=id==4||id==20||id==21;
    const bool press=id==5;
    const bool trigger=click?event==5:((press||id==6)?event==0:event==1);
    const bool release=press&&(event==6||event==10);
    if(!trigger&&!release)return plan;
    const auto value=in.value==0?20:in.value;
    if(static_cast<std::uint32_t>(value)&0xffff0000U) {
        if(trigger&&!packed_set(plan,value,in))return {};
        return plan;
    }
    const auto bits=in.first_key.value_or(std::uint32_t{1}<<(static_cast<std::uint32_t>(value)&31U));
    if(release)plan.commands.emplace_back(SteamTouchKeyUp{bits});
    else if(click)plan.commands.emplace_back(SteamTouchKeyClick{bits});
    else plan.commands.emplace_back(SteamTouchKeyDown{bits,!press});
    return plan;
}

std::optional<SteamTouchMainMenuPlan> steam_touch_main_menu(const SteamTouchMainMenuInput &in) {
    SteamTouchMainMenuPlan plan;
    if(in.admission==SteamTouchAdmission::intercepted)return plan;
    if(in.admission!=SteamTouchAdmission::ready||in.event_type<0||in.event_type>13||!in.ordered_forms)return {};
    // OnTouchMainMenu RVA23D340先扫描，再判断事件，且按原表单顺序绑定第一个raw3。
    for(const auto &form:*in.ordered_forms) {
        if(plan.probes.size()==4096||!form.identity)return {};
        plan.probes.push_back({SteamTouchMainMenuProbeKind::scan_form,form.identity,0});
        if(form.form_id!=4)continue;
        if(!form.subform_type)return {};
        if(*form.subform_type==3) { plan.main_menu=form.identity;break; }
    }
    if(in.event_type!=1&&in.event_type!=2)return plan;
    plan.probes.push_back({SteamTouchMainMenuProbeKind::submenu_component,0,0});
    if(!in.has_submenu_component)return {};
    const auto type=static_cast<std::uint32_t>(in.value)&0xffff0000U;
    std::uint64_t accessor{};
    if(*in.has_submenu_component) {
        if(!plan.main_menu)return {};
        plan.probes.push_back({SteamTouchMainMenuProbeKind::main_menu_value,*plan.main_menu,type});
        if(!in.main_menu_value)return {};
        // 原比较低16位无符号；真正Set却按short有符号扩展，二者不能统一。
        if(*in.main_menu_value==static_cast<std::int32_t>(static_cast<std::uint32_t>(in.value)&0xffffU))return plan;
        accessor=*plan.main_menu;
    }
    if(!in.top_form)return {};
    plan.probes.push_back({SteamTouchMainMenuProbeKind::top_form,*in.top_form,0});
    SteamTouchCommandPlan values;
    if(!set_values(values,type,low_short(in.value),in.extended_values))return {};
    if(*in.has_submenu_component) {
        // 原体无null检查即请求Pop；不据此承诺其异步后端成功，写目标仍是已绑定父菜单。
        plan.commands.emplace_back(SteamTouchFormPop{*in.top_form});
    } else accessor=*in.top_form;
    if(!accessor&&!values.commands.empty())return {};
    for(const auto &command:values.commands)
        plan.commands.emplace_back(SteamTouchAccessorWrite{accessor,std::get<SteamTouchValueWrite>(command)});
    if(!*in.has_submenu_component&&in.event_type==1)
        plan.commands.emplace_back(SteamTouchKeyClick{0x100000U});
    return plan;
}

std::optional<SteamTouchUnmarkerPlan> steam_touch_unmarker(const SteamTouchUnmarkerInput &in) {
    SteamTouchUnmarkerPlan plan;
    if(in.admission==SteamTouchAdmission::intercepted)return plan;
    if(in.admission!=SteamTouchAdmission::ready||in.event_type<0||in.event_type>13)return {};
    const int event=in.event_type;
    // RVA23D6C0：DOWN重置先于任何查询；UP/DRAG在key=-1时什么也不查询。
    if(event==0)plan.commands.emplace_back(SteamTouchMarkerFrameReset{});
    else if((event!=1&&event!=7)||in.marker_key==-1)return plan;
    if(event!=7)for(std::size_t i=0;i<3;++i) {
        constexpr std::array<int,3> ids{12,14,15};
        plan.probes.push_back({SteamTouchUnmarkerProbeKind::component,ids[i],event,0});
        if(!in.scrollbar_events[i])return {};
        if(*in.scrollbar_events[i])return plan;
    }
    if(in.marker_key!=-1) {
        plan.probes.push_back({SteamTouchUnmarkerProbeKind::marker_key,0,event,in.marker_key});
        if(!in.marker_event)return {};
        if(*in.marker_event)return plan;
    }
    // DOWN旧key=-1也经1023D743直接跳至1023D7EF/D805调用ClearMarker，不能合并成仅Reset。
    plan.commands.emplace_back(SteamTouchMarkerClear{});
    return plan;
}
} // namespace dungeon_village_prototype
