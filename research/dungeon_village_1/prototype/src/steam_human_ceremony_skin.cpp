// Steam住宅完成raw96：SubForm._draw2_4 VA10347B1D–10347FC5。
#include "dungeon_village_prototype/steam_human_skin.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
bool clock(int value) {return value>=0&&value<std::numeric_limits<int>::max();}
void image(SteamHumanOverviewPlan &p,SteamHumanAsset asset,int x,int y,
            std::optional<std::array<int,4>> crop={}) {
    p.operations.emplace_back(SteamHumanImage{asset,{x,y},0,crop});
}
void text(SteamHumanOverviewPlan &p,std::string value,int x,int y,bool rich=false) {
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        std::move(value),{x,y,0,0},2,0,false,{92,51,31},0,rich});
}
int jump(int tick) {
    // 原Math2.GetParabolaPos(8,16,t)：每步float32，禁FMA／double曲线替换。
    volatile float velocity=16.F/7.F,acceleration=-16.F/56.F;
    volatile float linear=float(tick)*velocity,curve=float(tick)*acceleration;
    curve=curve*float(tick+1);curve=curve*.5F;
    volatile float total=float(curve)+float(linear);return static_cast<int>(total);
}
}
std::optional<SteamHumanOverviewPlan> steam_human_residence_result_skin(
    const SteamHumanResidenceResultInput &in,const SteamHumanOverviewOptions &o) {
    if(in.phase<0||in.phase>1||!clock(in.counter)||!clock(in.secondary)||!o.title_widths||
       in.human.name.empty()||in.human.name.size()>4096||
       !startup_title_actor_skin(in.human.profession,in.human.sex,in.human.weapon,0,2,false)||
       !steam_facility_mapchip2_draws({in.mapchip,{120,113},0}))return {};
    // 原源不使用请求差校验after-before；请求量可受封顶影响。只核必需的三行合法数值。
    for(const auto &row:in.human.rewards)for(int n:row)if(n<0)return {};
    SteamHumanOverviewPlan out;out.title="自宅完成";out.soft_labels={0,0};
    const auto window=steam_startup_window_skin({222,170,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({15,139,224,202},o.view_y);
    if(!window||!box)return {};
    for(const auto &d:window->draws)out.operations.emplace_back(d);
    for(const auto &d:box->draws)out.operations.emplace_back(d);
    if(in.phase==0) {
        const int height=in.counter<=5?0:in.counter>=40?48:48*(in.counter-5)/35;
        out.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{0,158,240,height}});
        text(out,in.human.name+"的",120,165,true);text(out,"自宅完成!",120,187);
        out.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
        // Draw_btmCursor(g)读取完整主钟，不减50后再取余。
        if(in.counter>=50&&in.counter%25<15)
            out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
    } else {
        auto reward_input=in.human;reward_input.phase=1;reward_input.counter=in.counter;
        const auto reward=steam_human_reward_skin(reward_input,o);if(!reward)return {};
        out.operations.insert(out.operations.end(),reward->operations.begin(),reward->operations.end());
    }
    // 原有序绘制先底部说明／mentalUp，再顶部插画；不要为排序方便重排分层。
    image(out,SteamHumanAsset::residence_background,30,66,std::array<int,4>{0,0,93,82});
    image(out,SteamHumanAsset::residence_background,109,66,std::array<int,4>{1,0,91,82});
    image(out,SteamHumanAsset::residence_background,189,66,std::array<int,4>{1,0,20,82});
    // 原DrawRect的右下端点包含像素，计划保存现后端半开宽高。
    out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,66,180,82},{148,148,148},true}});
    out.operations.emplace_back(SteamFacilityMapchip2{in.mapchip,{120,113},0});
    image(out,SteamHumanAsset::shadow,175,142);
    auto actor=startup_title_actor_skin(in.human.profession,in.human.sex,-1,0,2,false);
    if(!actor)return {};
    const int phase=in.secondary%26,height=phase<10?0:jump(phase-10);
    if(phase>=10) {actor->body.sprite=10;actor->body.frame=0;}
    // DrawDispPlayer(g,x,y)无武器重载；阴影固定，不随跳跃人物上移。
    out.operations.emplace_back(SteamHumanPortrait{*actor,{175,142-height},{},false});
    out.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return out;
}
std::optional<SteamHumanOverviewPlan> steam_human_residence_result_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamHumanOverviewOptions &o) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    if(page==s.scripts.pages.end()||page->lifecycle!=2||s.scripts.pages.empty()||
       s.scripts.pages.back().id!=id||!valid_startup_world_runtime_residence_result_page(s,*page))return {};
    const auto bound=s.page_human_bindings.find(id),counter=s.page_counters.find(id),phase=s.page_phases.find(id),
        secondary=s.page_secondary_counters.find(id);
    if(bound==s.page_human_bindings.end()||counter==s.page_counters.end()||phase==s.page_phases.end()||
       secondary==s.page_secondary_counters.end())return {};
    const auto human=startup_world_human_details(s,bound->second);if(!human)return {};
    const auto facility=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [&](const auto &d){return d.id==page->legacy_f;});
    if(facility==s.rules->facilities.end())return {};
    SteamHumanResidenceResultInput in;
    in.phase=phase->second;in.counter=counter->second;in.secondary=secondary->second;in.mapchip=facility->display_id;
    in.human={human->profession,human->sex,human->equipment[0].value_or(-1),in.phase,in.counter,human->name,s.reward_display};
    return steam_human_residence_result_skin(in,o);
}
} // namespace dungeon_village_prototype
