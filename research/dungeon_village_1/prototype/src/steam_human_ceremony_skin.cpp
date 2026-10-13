// Steam人物结果：努力67、授勋88、住宅96。仅展开源绘制请求，不写共享scratch。
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
bool bubble(SteamHumanOverviewPlan &out,const char *value,std::array<int,2> position,
            const std::optional<int> &width) {
    if(!width)return false;
    const auto plan=steam_human_down_bubble_skin(*width,position);if(!plan)return false;
    for(const auto &draw:plan->draws) {
        if(const auto *i=std::get_if<SteamHumanImage>(&draw))out.operations.emplace_back(*i);
        else if(const auto *r=std::get_if<StartupSkinRect>(&draw))out.operations.emplace_back(SteamStartupFrameDraw{*r});
    }
    out.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,value,
        {position[0],position[1]+3,0,0},2,0,false,{0,0,0}});return true;
}
}
std::optional<SteamHumanOverviewPlan> steam_human_award_result_skin(
    const SteamHumanAwardResultInput &in,const SteamHumanOverviewOptions &o) {
    // SubForm._draw2_4 VA1034B396–1034BC4B；门槛数组由Steam cctor独立核为30/60/75/95/135。
    if(in.phase<0||in.phase>1||!clock(in.counter)||!o.title_widths)return {};
    const int local=in.counter-in.phase*135;
    auto reward_input=in.human;reward_input.phase=in.phase;reward_input.counter=local;
    const auto reward=steam_human_award_mental_skin(reward_input,o);if(!reward)return {};
    SteamHumanOverviewPlan out;out.title="勋章授予式";out.soft_labels={0,0};
    const auto window=steam_startup_window_skin({222,170,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({15,139,224,202},o.view_y);if(!window||!box)return {};
    for(const auto &d:window->draws)out.operations.emplace_back(d);
    for(const auto &d:box->draws)out.operations.emplace_back(d);
    image(out,SteamHumanAsset::ceremony_background,30,67,std::array<int,4>{0,0,180,80});
    // spChara2复用总统common171/SEB46。方向0：站立0、授予8、庆祝4/6。
    const int minister_frame=in.phase==1||(local>=95&&local<135)?4+2*(in.counter%8/4):
        local>=30&&local<60?8:0;
    out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::president,{108,139},minister_frame,{}});
    if(in.phase==0&&local>=30&&local<60&&
       !bubble(out,"恭喜!",{112,96},o.ceremony_message_widths[0]))return {};
    auto actor=startup_title_actor_skin(in.human.profession,in.human.sex,-1,0,2,false);if(!actor)return {};
    // SetDispPlayerData(action,body,animeIndex0,dir2,main)→human SetDrawParam；三种动作帧均0。
    if(in.phase==1||(local>=75&&local<135))actor->body.sprite=22; // action4、dir2。
    else if(local>=60&&local<75)actor->body.sprite=26; // action10、dir2。
    out.operations.emplace_back(SteamHumanPortrait{*actor,{136,139},{},false});
    if(in.phase==0&&local>=95&&local<135&&
       !bubble(out,"太好了!",{138,85},o.ceremony_message_widths[1]))return {};
    // Draw_icon mode6/index1直接裁common35；原先在人／气泡之后绘制奖章。
    if(in.phase==1||local>=30) {
        const std::array<int,2> pos=in.phase==0&&local<60?std::array<int,2>{111,123}:
            in.phase==0&&local<75?std::array<int,2>{121,123}:std::array<int,2>{128,108};
        out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::medal,pos,0,std::array<int,4>{14,0,14,14}});
    }
    const int frame=local>=95||in.phase==1?in.counter%8/4:0;
    constexpr std::array<std::array<int,2>,3> positions{{{158,134},{176,135},{192,140}}};
    for(int i=0;i<3;++i)out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::award_crowd,positions[i],i*2+frame,{}});
    out.operations.insert(out.operations.end(),reward->operations.begin(),reward->operations.end());
    out.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return out;
}
std::optional<SteamHumanOverviewPlan> steam_human_award_result_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamHumanOverviewOptions &o) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    if(page==s.scripts.pages.end()||page->lifecycle!=2||s.scripts.pages.empty()||s.scripts.pages.back().id!=id||
       !valid_startup_world_runtime_award_result_page(s,*page))return {};
    const auto bound=s.page_human_bindings.find(id),counter=s.page_counters.find(id),phase=s.page_phases.find(id);
    if(bound==s.page_human_bindings.end()||counter==s.page_counters.end()||phase==s.page_phases.end())return {};
    const auto human=startup_world_human_details(s,bound->second);if(!human)return {};
    SteamHumanAwardResultInput in;in.phase=phase->second;in.counter=counter->second;
    in.human={human->profession,human->sex,human->equipment[0].value_or(-1),in.phase,in.counter,human->name,s.reward_display};
    return steam_human_award_result_skin(in,o);
}
std::optional<SteamHumanOverviewPlan> steam_human_effort_result_skin(
    const SteamHumanEffortResultInput &in,const SteamHumanOverviewOptions &o) {
    // 真正AddPutUp跨十门槛才创建67；显示冻结旧／新努力，不从现人物缓存重造。
    if(!clock(in.counter)||in.old_effort<0||in.old_effort>100||in.new_effort<0||in.new_effort>100||
       in.new_effort/10<=in.old_effort/10||in.name.empty()||in.name.size()>4096||!o.title_widths)return {};
    const auto portrait=startup_title_actor_skin(in.profession,in.sex,-1,0,1,false);
    const auto rows=steam_human_combat_result_rows(in.attributes,in.counter);if(!portrait||!rows)return {};
    SteamHumanOverviewPlan out;out.title="努力奖励";out.soft_labels={0,0};
    const auto window=steam_startup_window_skin({220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},o.view_y);if(!window||!box)return {};
    for(const auto &d:window->draws)out.operations.emplace_back(d);
    for(const auto &d:box->draws)out.operations.emplace_back(d);
    out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,67,16,16},{196,236,169},false}});
    out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,67,16,16},{204,204,204},true}});
    out.operations.emplace_back(SteamHumanPortrait{*portrait,{38,89},std::array<int,4>{30,68,15,14},false});
    out.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        in.name+"的能力上升",{50,70,0,0},0,0,false,{92,51,31}});
    out.operations.insert(out.operations.end(),rows->operations.begin(),rows->operations.end());
    out.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,"基础能力",
        {o.japanese?33:27,o.japanese?199:200,0,0},0,o.japanese?0:9,false,{92,51,31}});
    for(int column=0;column<2;++column) {
        if(column==1)out.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
            "→",{149,199,0,0},0,0,false,{92,51,31}});
        const int value=((column?in.new_effort:in.old_effort)/10)*10+100,x=column?190:126;
        out.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,SteamFacilityAsset::number05,value,{x,200},0,4,-1});
        out.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::inventory_unit,{x+1,200},0,{}});
    }
    out.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return out;
}
std::optional<SteamHumanOverviewPlan> steam_human_effort_result_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamHumanOverviewOptions &o) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    if(page==s.scripts.pages.end()||page->lifecycle!=2||s.scripts.pages.empty()||s.scripts.pages.back().id!=id||
       !valid_startup_world_runtime_effort_result_page(s,*page))return {};
    const auto human_binding=s.page_human_bindings.find(id),counter=s.page_counters.find(id);
    if(human_binding==s.page_human_bindings.end()||counter==s.page_counters.end())return {};
    const auto h=startup_world_human_details(s,human_binding->second);if(!h)return {};
    return steam_human_effort_result_skin(SteamHumanEffortResultInput{h->profession,h->sex,counter->second,
        page->legacy_f,page->legacy_g,h->name,s.effort_display},o);
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
