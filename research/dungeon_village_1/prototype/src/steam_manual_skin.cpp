// Steam SubForm._draw VA103554E5..10355DC9；Init VA10314BF0..10314FF2。
// 与APK b/g:9447..9482交叉；原字体/翻译及scratch消费由调用方独立接入。
#include "dungeon_village_prototype/steam_manual_skin.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <cmath>

namespace dungeon_village_prototype {
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
