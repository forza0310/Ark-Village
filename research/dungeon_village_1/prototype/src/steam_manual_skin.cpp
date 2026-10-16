// Steam SubForm._draw VA103554E5..10355DC9；Init VA10314BF0..10314FF2。
// 与APK b/g:9447..9482交叉；原字体/翻译及scratch消费由调用方独立接入。
#include "dungeon_village_prototype/steam_manual_skin.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <cmath>
#include <type_traits>
#include <utility>

namespace dungeon_village_prototype {
namespace {
static_assert(std::is_trivially_copyable_v<SteamTouchArrowState>);
std::int32_t low_age(std::int64_t now,std::int64_t then) {
    const auto bits=static_cast<std::uint32_t>(now)-static_cast<std::uint32_t>(then);
    return static_cast<std::int32_t>(bits<=0x7fffffffU ? static_cast<std::int64_t>(bits) :
                                     static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int64_t long_age(std::int64_t now,std::int64_t then) {
    const auto bits=static_cast<std::uint64_t>(now)-static_cast<std::uint64_t>(then);
    return bits<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ?
        static_cast<std::int64_t>(bits) : -static_cast<std::int64_t>(~bits)-1;
}
bool valid_arrow_effect(const SteamTouchArrowState &s) {
    if(!s.surface || (s.texture_index!=2 && s.texture_index!=3) ||
       !std::isfinite(s.x) || !std::isfinite(s.y) ||
       (s.velocity_x!=10.F && s.velocity_x!=-10.F))return false;
    for(const auto n:s.arguments)if(!std::isfinite(n))return false;
    return s.arguments[0]>0 && s.arguments[1]>0 && s.arguments[4]>0 && s.arguments[5]>0;
}
}
std::optional<SteamTouchArrowState> prepare_steam_touch_arrow_effect(
    const SteamTouchArrowEffect &request,std::uint64_t surface,int image_width,
    const SteamTouchEffectClock &clock) {
    if(!surface || image_width<=0 || !clock ||
       request.texture_row!=(request.paint.frame==3?2:3) ||
       (request.velocity_x!=10.F && request.velocity_x!=-10.F))return {};
    try {
        SteamTouchArrowState s;
        s.surface=surface;s.component_key=request.paint.component_key;s.texture_index=request.texture_row;
        s.x=float(request.paint.x)+(request.texture_row==2?-9.F:-6.F)*0.75F;
        s.y=float(request.paint.y)-12.F*0.75F;
        const float density=float(image_width)/128.F;
        s.arguments={20.F*0.75F,26.F*0.75F,
            (request.texture_row==2?31.F:52.F)*density,57.F*density,20.F*density,26.F*density};
        const auto now=clock();if(!now)return {};
        s.created=s.last_frame=*now;s.velocity_x=request.velocity_x;
        return s;
    } catch(...) { return {}; }
}
std::optional<SteamTouchArrowFrame> advance_steam_touch_arrow_effect(
    const SteamTouchArrowState &s,const SteamTouchEffectClock &clock) {
    if(!valid_arrow_effect(s) || !clock)return {};
    try {
        const auto age_clock=clock();if(!age_clock)return {};
        if(low_age(*age_clock,s.created)>=400)return SteamTouchArrowFrame{s,true};
        const auto integrate_clock=clock();if(!integrate_clock)return {};
        SteamTouchArrowFrame next{s,false};
        const double dt=static_cast<double>(long_age(*integrate_clock,s.last_frame))/1000.0;
        next.state.x=s.x+static_cast<float>(static_cast<double>(s.velocity_x)*dt);
        // 本入口id0的vy/ax/ay恒0；不扩大成其它TouchEffect类型或对称修复其vy分支。
        const auto last_clock=clock();if(!last_clock)return {};
        next.state.last_frame=*last_clock;
        return next;
    } catch(...) { return {}; }
}
std::optional<SteamTouchArrowDraw> steam_touch_arrow_effect_skin(
    const SteamTouchArrowState &s,const SteamTouchArrowSurface &surface,
    const SteamTouchEffectClock &clock) {
    if(!valid_arrow_effect(s) || !clock)return {};
    try {
        const auto now=clock();if(!now)return {};
        const int k=low_age(*now,s.created)/50,source=160-18*k;
        SteamTouchArrowDraw plan{s.surface,{}};
        if(source<=0)return plan;
        SteamTouchArrowImage image;
        for(int n=0;n<4;++n) {
            const float value=s.arguments[n+2]+0.5F;
            if(double(value)<double(std::numeric_limits<int>::min()) ||
               double(value)>double(std::numeric_limits<int>::max()))return {};
            image.crop[n]=static_cast<int>(value);
        }
        const float q=float(surface.scale_ratio)/100.F;
        const float growth=(float(k)*2.5F)*q;
        image.destination={(s.x*q+float(surface.x))-growth*0.5F,
            (s.y*q+float(surface.y))-growth*0.5F,s.arguments[0]*q+growth,s.arguments[1]*q+growth};
        plan.operations.emplace_back(SteamTouchArrowBlend{1,source,255-source});
        plan.operations.emplace_back(image);
        plan.operations.emplace_back(SteamTouchArrowBlend{0,255,0});
        return plan;
    } catch(...) { return {}; }
}
std::optional<std::size_t> SteamTouchArrowEffects::add(const SteamTouchArrowEffect &request,
    std::uint64_t surface,int image_width,const SteamTouchEffectClock &clock) {
    try {
        std::size_t slot=0;
        while(slot<buffer_.size() && std::find(active_.begin(),active_.end(),slot)!=active_.end())++slot;
        // 所有可能分配先完成；Init读钟失败不能留下半初始化的活动槽。
        if(active_.size()==active_.max_size() || (slot==buffer_.size() && buffer_.size()==buffer_.max_size()))return {};
        const auto reserve_one=[](auto &items) {
            if(items.size()==items.capacity()) {
                const auto growth=std::min(items.capacity()?items.capacity():std::size_t{1},
                                           items.max_size()-items.capacity());
                items.reserve(items.capacity()+growth);
            }
        };
        reserve_one(active_);
        if(slot==buffer_.size())reserve_one(buffer_);
        const auto initial=prepare_steam_touch_arrow_effect(request,surface,image_width,clock);
        if(!initial)return {};
        if(slot==buffer_.size())buffer_.push_back(*initial);
        else buffer_[slot]=*initial;
        active_.push_back(slot);
        return slot;
    } catch(...) { return {}; }
}
bool SteamTouchArrowEffects::frame(const SteamTouchEffectClock &clock) {
    try {
        std::vector<SteamTouchArrowFrame> staged;
        std::vector<std::size_t> next_active;
        staged.reserve(active_.size());next_active.reserve(active_.size());
        for(auto i=active_.rbegin();i!=active_.rend();++i) {
            const auto next=advance_steam_touch_arrow_effect(buffer_[*i],clock);
            if(!next)return false;
            staged.push_back(*next);
        }
        // 按原顺序保留存活active；以下只有不抛异常的标量赋值及已预留容量写入。
        for(std::size_t i=0;i<active_.size();++i) {
            const auto &next=staged[active_.size()-1-i];
            buffer_[active_[i]]=next.state;
            if(!next.retire)next_active.push_back(active_[i]);
        }
        active_.swap(next_active);
        return true;
    } catch(...) { return false; }
}
std::optional<std::vector<SteamTouchArrowDraw>> SteamTouchArrowEffects::draw(
    const SurfaceQuery &surface,const SteamTouchEffectClock &clock) const {
    if(!active_.empty() && (!surface || !clock))return {};
    try {
        std::vector<SteamTouchArrowDraw> plans;plans.reserve(active_.size());
        for(const auto slot:active_) {
            const auto &s=buffer_[slot];const auto observed=surface(s.surface);
            if(!observed)return {};
            auto plan=steam_touch_arrow_effect_skin(s,*observed,clock);
            if(!plan)return {};
            plans.push_back(std::move(*plan));
        }
        return plans;
    } catch(...) { return {}; }
}
bool SteamTouchArrowEffects::remove(std::size_t slot) noexcept {
    const auto found=std::find(active_.begin(),active_.end(),slot);
    if(found==active_.end())return false;
    active_.erase(found);
    return true;
}
std::optional<SteamTouchArrowState> SteamTouchArrowEffects::state(std::size_t slot) const {
    if(slot>=buffer_.size())return {};
    return buffer_[slot];
}
std::optional<SteamManualArrowPlan> steam_manual_arrow_skin(
    const SteamManualArrowRequest &in,std::array<int,4> bounds,int image_width) {
    if((in.bounds_frame!=0 && in.bounds_frame!=3) || image_width<=0 ||
       bounds[2]<=0 || bounds[3]<=0 ||
       !std::all_of(bounds.begin(),bounds.end(),[](int n){return n>=-32768 && n<=32767;}))return {};
    // GameView.IMAGE_DATA的前四行；字段为image/offsetX/offsetY/srcX/srcY/width/height。
    constexpr std::array<std::array<int,7>,4> data{{
        {0,-5,-8,0,57,15,20},{0,-4,-8,15,57,15,20},
        {0,-9,-12,31,57,20,26},{0,-6,-12,52,57,20,26}}};
    SteamManualArrowPlan out;
    out.texture_index=(in.bounds_frame==3?0:1)+(in.selected?2:0);
    const auto &row=data[out.texture_index];
    const float ox=float(row[1])*0.75F,oy=float(row[2])*0.75F;
    const float w=float(row[5])*0.75F,h=float(row[6])*0.75F;
    const auto integer=[](float value)->std::optional<int> {
        if(!std::isfinite(value) || double(value)<double(std::numeric_limits<int>::min()) ||
           double(value)>double(std::numeric_limits<int>::max()))return {};
        return static_cast<int>(value);
    };
    const std::array<float,2> shift{
        float(bounds[0]+bounds[2]/2)-(ox+w*0.5F),
        float(bounds[1]+bounds[3]/2)-(oy+h*0.5F)};
    for(int axis=0;axis!=2;++axis) {
        const auto move=integer(shift[axis]);if(!move)return {};
        const auto n=static_cast<std::int64_t>(in.position[axis])+*move;
        if(n<std::numeric_limits<int>::min() || n>std::numeric_limits<int>::max())return {};
        out.paint_origin[axis]=static_cast<int>(n);
    }
    out.paint_frame=in.bounds_frame;
    out.destination={float(out.paint_origin[0])+ox,float(out.paint_origin[1])+oy,w,h};
    for(int n=0;n!=4;++n) {
        const auto value=integer(out.destination[n]);if(!value)return {};
        out.touch_rectangle[n]=*value;
    }
    const float density=float(image_width)/128.F;
    for(int n=0;n!=4;++n) {
        const auto value=integer(float(row[n+3])*density+0.5F);if(!value)return {};
        out.crop[n]=*value;
    }
    if(out.crop[2]<=0 || out.crop[3]<=0 ||
       static_cast<std::int64_t>(out.crop[0])+out.crop[2]>image_width)return {};
    return out;
}
std::optional<SteamManualSkinPlan> steam_manual_skin(const SteamManualSkinInput &in) {
    if(in.width<=0 || in.height<=0 || in.body_pages<=0 ||
       in.body_pages==std::numeric_limits<int>::max() || in.page<0 || in.page>in.body_pages ||
       in.frame<0 || in.wait<0) return {};
    SteamManualSkinPlan plan;
    const bool centered=in.width>240 || in.height>240;
    for(int axis=0;axis!=2;++axis) {
        const auto value=static_cast<std::int64_t>(in.origin[axis])+
            (centered?((axis==0?in.width:in.height)-240)/2:0);
        if(value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max())return {};
        plan.origin[axis]=static_cast<int>(value);
    }
    if(!in.on_top || in.until_active_hide || in.wait>0)return plan;
    const bool about=in.page==in.body_pages;
    if(!about && !in.body_text)return {};
    if(about) {
        if(in.definition_count<0 || in.frozen_definitions.size()>4)return {};
        for(std::size_t n=0;n<in.frozen_definitions.size();++n) {
            const int id=in.frozen_definitions[n];
            if(id<0 || id>=in.definition_count ||
               std::find(in.frozen_definitions.begin(),in.frozen_definitions.begin()+n,id)!=
                   in.frozen_definitions.begin()+n)return {};
        }
    }
    const int extra=in.japanese?0:40, half=extra/2, arrow=(in.frame%20)/5;
    plan.draws.emplace_back(SteamManualWindow{
        {180+extra,166,0,0,SteamStartupTextRole::message_title,0},in.page+1,in.body_pages+1});
    const auto add_arrow=[&](int x,int frame,int value,bool selected) {
        const auto image_index=plan.draws.size();
        plan.draws.emplace_back(SteamManualArrowRequest{{x,56},frame,selected});
        plan.draws.emplace_back(SteamStartupTouch{1,value,{},image_index,0});
    };
    add_arrow(44-arrow-half,3,16,in.arrow_selected[0]);
    add_arrow(193+arrow+half,0,18,in.arrow_selected[1]);
    plan.draws.emplace_back(SteamStartupBox{37-half,60,203+half,197});
    if(!about) {
        plan.draws.emplace_back(SteamManualText{SteamManualTextRole::body,*in.body_text,
            {48-half,80,in.japanese?146:186,124},{30,30,30},{},in.japanese?0:10,
            in.japanese?6:8,in.localize_text,false});
    } else {
        plan.draws.emplace_back(SteamManualText{SteamManualTextRole::about,{},
            {120,80,0,0},{30,30,30},2});
        plan.draws.emplace_back(SteamManualText{SteamManualTextRole::game_name,{},
            {120,166,0,0},{30,30,30},2,0,0,true,in.trial_version});
        plan.draws.emplace_back(SteamManualText{SteamManualTextRole::copyright,{},
            {120,182,0,0},{30,30,30},2});
        plan.draws.emplace_back(SteamManualClip{true,{43,101,154,56}});
        // 后两片sx=1是源裁剪，不是水平翻转。三片依次铺底，clip保持至人物画完。
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,39,-1,0,0,{0,0,93,82},{30,90}});
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,39,-1,0,0,{1,0,91,82},{109,90}});
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,39,-1,0,0,{1,0,20,82},{189,90}});
        for(std::size_t n=0;n<in.frozen_definitions.size();++n)
            plan.draws.emplace_back(SteamManualActorRequest{in.frozen_definitions[n],
                {80+static_cast<int>(n)*80/3,148},0,(in.frame%16)/4,2,in.frame});
        plan.draws.emplace_back(SteamManualClip{false,{}});
        // Steam Graphics.DrawRect端点合同：这里保留调用153×55，不套APK的+1转换。
        plan.draws.emplace_back(StartupSkinRect{{43,101,153,55},{43,116,190},true});
    }
    plan.draws.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return plan;
}
} // namespace dungeon_village_prototype
