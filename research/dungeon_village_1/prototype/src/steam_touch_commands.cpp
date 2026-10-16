// 已核GameView通用叶分支，不复制surface命中、平台键盘或具体页面业务。
#include "dungeon_village_prototype/steam_touch_commands.hpp"
#include <algorithm>
#include <limits>

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
std::int32_t signed_bits(std::uint32_t bits) {
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t add32(std::int32_t a,std::int32_t b) {
    return signed_bits(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));
}
std::int32_t sub32(std::int32_t a,std::int32_t b) {
    return signed_bits(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));
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

void SteamTouchFlow::emit(Phase phase,SteamTouchFlowOperation op,std::int32_t type,
                         std::int32_t value,std::uint64_t key,std::uint64_t binding) {
    phase_=phase;
    request_={op,input_.event_identity,request_.sequence+1,binding,
        static_cast<std::uint32_t>(type)&0xffff0000U,value,key};
}
void SteamTouchFlow::get(Phase phase,std::int32_t type) {
    emit(phase,SteamTouchFlowOperation::get_current,type);
}
void SteamTouchFlow::set(Phase phase,std::int32_t type,std::int32_t value) {
    emit(phase,SteamTouchFlowOperation::set_current,type,value,0,
        phase==Phase::list_set?input_.component_binding:0);
}
void SteamTouchFlow::done() { emit(Phase::done,SteamTouchFlowOperation::done); }
std::optional<SteamTouchFlow> SteamTouchFlow::begin(const SteamTouchFlowInput &in) {
    SteamTouchFlow flow;flow.input_=in;
    if(in.admission==SteamTouchAdmission::intercepted) { flow.done();return flow; }
    if(in.admission!=SteamTouchAdmission::ready||in.event_type<0||in.event_type>13||!in.event_identity)return {};
    const bool list=in.component==3||in.component==10||in.component==11;
    if(!list&&(in.component<12||in.component>15))return {};
    if(list) {
        if(in.event_type!=1&&in.event_type!=2)flow.done();
        else {
            if(!in.component_binding)return {};
            flow.set(Phase::list_set,in.value,low_short(in.value));
        }
    } else {
        if(!in.component_binding)return {};
        // 两种滚动入口均在事件类型判断前读args；13与其它三类的null资格不同。
        flow.emit(Phase::arguments,SteamTouchFlowOperation::arguments,0,0,0,in.component_binding);
    }
    return flow;
}
bool SteamTouchFlow::resume(const SteamTouchFlowReply &reply) {
    if(phase_==Phase::done||reply.event_identity!=input_.event_identity||
       reply.sequence!=request_.sequence||request_.sequence==std::numeric_limits<std::uint64_t>::max())return false;
    auto candidate=*this;
    if(!candidate.consume(reply.result))return false;
    *this=candidate;
    return true;
}
void SteamTouchFlow::bar_candidate(std::int32_t value) {
    if(value==std::numeric_limits<std::int32_t>::max()) { done();return; }
    wanted_=std::max<std::int32_t>(0,std::min(value,sub32(count_,visible_)));
    get(Phase::bar_compare,input_.value);
}
bool SteamTouchFlow::consume(const SteamTouchFlowResult &result) {
    using Op=SteamTouchFlowOperation;
    const auto *ack=std::get_if<SteamTouchFlowCompleted>(&result);
    const auto *value=std::get_if<SteamTouchFlowValue>(&result);
    const auto *key=std::get_if<SteamTouchFlowKey>(&result);
    const auto move_to_bottom=[&] { get(Phase::bottom_selection,selection_type_); };
    switch(phase_) {
    case Phase::done:return false;
    case Phase::list_set:
        if(!ack||!ack->success)return false;
        if(input_.event_type==2)done();
        else emit(Phase::list_flag,Op::marker_flag,0,0,0,input_.component_binding);
        return true;
    case Phase::list_flag: {
        const auto *flag=std::get_if<SteamTouchFlowFlag>(&result);
        if(!flag)return false;
        if(flag->enabled)emit(Phase::list_marker,Op::marker_key);
        else emit(Phase::list_first_key,Op::event_first_key);
        return true;
    }
    case Phase::list_marker:
        if(!key)return false;
        old_marker_=key->bits;
        emit(Phase::list_compare,Op::component_key,0,0,0,input_.component_binding);
        return true;
    case Phase::list_compare:
        if(!key)return false;
        if(key->bits==old_marker_)emit(Phase::list_first_key,Op::event_first_key);
        else emit(Phase::list_replace,Op::event_component_key);
        return true;
    case Phase::list_replace:
        if(!key)return false;
        emit(Phase::list_write,Op::write_marker,0,0,key->bits);
        return true;
    case Phase::list_first_key: {
        const auto *first=std::get_if<SteamTouchFlowFirstKey>(&result);
        if(!first)return false;
        emit(Phase::list_click,Op::click_key,0,0,first->bits.value_or(0x100000U));
        return true;
    }
    case Phase::list_write:case Phase::list_click:case Phase::finish_write:
        if(!ack||!ack->success)return false;
        done();return true;
    case Phase::arguments: {
        const auto *args=std::get_if<SteamTouchFlowArguments>(&result);
        if(!args)return false;
        if(!args->values) {
            if(input_.component==13)return false;
            done();return true;
        }
        if(args->values->size()<2)return false;
        count_=(*args->values)[0];visible_=(*args->values)[1];
        selection_type_=args->values->size()>2?(*args->values)[2]:-1;
        if(input_.component==13) {
            if(selection_type_==-1||input_.event_type!=13)done();
            else if((static_cast<std::uint32_t>(input_.value)&0xffffU)==0)get(Phase::button_dec_check,selection_type_);
            else if((static_cast<std::uint32_t>(input_.value)&0xffffU)==1)get(Phase::button_inc_check,selection_type_);
            else move_to_bottom();
        } else if(input_.event_type==8)get(Phase::bar_candidate,input_.value);
        else if(input_.event_type==9)emit(Phase::bar_absolute_dy,Op::event_delta_y);
        else done();
        return true;
    }
    case Phase::bar_candidate:
        if(!value)return false;
        wanted_=value->value;
        emit(Phase::bar_relative_dy,Op::event_delta_y);return true;
    case Phase::bar_relative_dy:
        if(!value)return false;
        bar_candidate(add32(wanted_,value->value));return true;
    case Phase::bar_absolute_dy:
        if(!value)return false;
        bar_candidate(value->value);return true;
    case Phase::bar_compare:
        if(!value)return false;
        if(wanted_==value->value)done();
        else get(Phase::bar_delta,input_.value);
        return true;
    case Phase::bar_delta:
        if(!value)return false;
        delta_=sub32(wanted_,value->value);
        set(Phase::bar_write,input_.value,wanted_);return true;
    case Phase::bar_write:
        if(!ack||!ack->success)return false;
        if(selection_type_==-1)done();else get(Phase::bar_selection,selection_type_);
        return true;
    case Phase::bar_selection:
        if(!value)return false;
        selection_=value->value;
        emit(Phase::bar_component_id,Op::component_id,0,0,0,input_.component_binding);return true;
    case Phase::bar_component_id:
        if(!value)return false;
        selection_=std::max<std::int32_t>(0,add32(selection_,
            value->value==14?add32(delta_,delta_):delta_));
        if(value->value!=14&&selection_>=count_)selection_=sub32(count_,1);
        set(Phase::finish_write,selection_type_,selection_);return true;
    case Phase::button_dec_check:case Phase::button_inc_check:
        if(!value)return false;
        if(phase_==Phase::button_dec_check&&value->value>0)get(Phase::button_dec_read,selection_type_);
        else if(phase_==Phase::button_inc_check&&value->value<sub32(count_,1))get(Phase::button_inc_read,selection_type_);
        else move_to_bottom();
        return true;
    case Phase::button_dec_read:case Phase::button_inc_read:
        if(!value)return false;
        set(Phase::button_move_write,selection_type_,phase_==Phase::button_dec_read?
            sub32(value->value,1):add32(value->value,1));return true;
    case Phase::button_move_write:
        if(!ack||!ack->success)return false;
        move_to_bottom();return true;
    case Phase::bottom_selection:case Phase::top_selection:
        if(!value)return false;
        selection_=value->value;
        get(phase_==Phase::bottom_selection?Phase::bottom_first:Phase::top_first,input_.value);
        return true;
    case Phase::bottom_first:
        if(!value)return false;
        if(selection_>=add32(value->value,visible_))get(Phase::bottom_read,selection_type_);
        else get(Phase::top_selection,selection_type_);
        return true;
    case Phase::bottom_read:
        if(!value)return false;
        set(Phase::bottom_write,input_.value,add32(sub32(value->value,visible_),1));return true;
    case Phase::bottom_write:
        if(!ack||!ack->success)return false;
        get(Phase::top_selection,selection_type_);return true;
    case Phase::top_first:
        if(!value)return false;
        if(selection_<value->value)get(Phase::top_read,selection_type_);
        else get(Phase::tail_positive,input_.value);
        return true;
    case Phase::top_read:
        if(!value)return false;
        set(Phase::top_write,input_.value,value->value);return true;
    case Phase::top_write:case Phase::tail_write:
        if(!ack||!ack->success)return false;
        get(Phase::tail_positive,input_.value);return true;
    case Phase::tail_positive:
        if(!value)return false;
        if(value->value<=0)done();else get(Phase::tail_fits,input_.value);
        return true;
    case Phase::tail_fits:
        if(!value)return false;
        if(count_>=add32(value->value,visible_))done();else get(Phase::tail_read,input_.value);
        return true;
    case Phase::tail_read:
        if(!value)return false;
        set(Phase::tail_write,input_.value,sub32(value->value,1));return true;
    }
    return false;
}
} // namespace dungeon_village_prototype
