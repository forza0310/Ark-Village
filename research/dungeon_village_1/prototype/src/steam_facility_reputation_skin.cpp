#include "dungeon_village_prototype/steam_facility_reputation_skin.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"

#include <algorithm>
#include <set>

namespace dungeon_village_prototype {
std::optional<SteamHumanResource> steam_facility_reputation_resource(SteamFacilityReputationAsset asset) {
    if(asset!=SteamFacilityReputationAsset::secretary)return {};
    return SteamHumanResource{172,31,"original/common/chara_hishoko.png","original/common/chara_hisho.seb"};
}
std::optional<SteamFacilityReputationPlan> steam_facility_reputation_skin(const SteamFacilityReputationInput &in) {
    if(in.definition<0||in.name.empty()||in.phase<0||in.phase>1||in.frame<0||in.frame2<0||
       in.participants.empty()||in.participants.size()>3||!in.title_widths||
       (in.phase==1&&(!in.increase_suffix_width||*in.increase_suffix_width<0||*in.increase_suffix_width>4096)))return {};
    std::set<int> seen;
    for(const auto &p:in.participants)
        if(p.definition<0||!seen.insert(p.definition).second||p.body_image<0||p.body_image>37||p.body_image==32)return {};
    const auto &art=startup_evidence().displays;
    const auto binding=std::find_if(art.begin(),art.end(),[&](const auto &d){return d.id==in.mapchip;});
    if(binding==art.end()||binding->definition_id!=in.definition)return {};
    const auto map=steam_facility_mapchip2_draws({in.mapchip,{120,105},0});
    const auto window=steam_startup_window_skin({222,170,0,0,SteamStartupTextRole::message_title,0},in.view_y,in.title_widths);
    const auto box=steam_startup_box_skin({15,139,224,202},in.view_y);
    if(!map||!window||!box)return {};
    SteamFacilityReputationPlan out;
    for(const auto &op:window->draws)out.operations.emplace_back(op);
    for(const auto &op:box->draws)out.operations.emplace_back(op);
    using HA=SteamHumanAsset;
    out.operations.emplace_back(SteamHumanImage{HA::portrait_background,{30,66},0,std::array<int,4>{0,0,93,82}});
    out.operations.emplace_back(SteamHumanImage{HA::portrait_background,{109,66},0,std::array<int,4>{1,0,91,82}});
    out.operations.emplace_back(SteamHumanImage{HA::portrait_background,{189,66},0,std::array<int,4>{1,0,20,82}});
    // DrawRect把源末像素差179/81扩为180×82；不能填实心灰块遮住三段背景。
    out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,66,180,82},{148,148,148},true}});
    out.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::mini_background,{77,71},0,{}});
    out.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,{80,74,80,64}});
    out.operations.emplace_back(SteamFacilityMapchip2{in.mapchip,{120,105},0});
    out.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
    const int cycle=in.frame2%26;
    // Steam GetParabolaPos(8,16,t)：先有符号截断高度，再从143减；未取ceil或按帧数缩放。
    const int t=cycle-10;
    const float velocity=16.0f/7.0f,gravity=-16.0f/(7.0f*8.0f);
    const int dy=cycle<10?0:-static_cast<int>(velocity*t+gravity*t*(t+1)/2.0f);
    constexpr std::array<int,3> xs{62,44,80};
    for(std::size_t i=0;i<in.participants.size();++i) {
        StartupTitleActorSkin actor;
        actor.body={in.participants[i].body_image,cycle<10?1:9,0};
        // 阴影固定脚底143，不跟随跳起。保持源顺序：影子、无武器身体。
        StartupTitleActorSkin shadow;
        shadow.body=actor.body;
        shadow.shadow=StartupVisualDraw{StartupVisualResource::common,25,3,0,0,{}, {0,-dy},{}};
        out.operations.emplace_back(SteamHumanPortrait{shadow,{xs[i],143+dy},{},false});
    }
    const int special_frame=5+((in.frame2%8)/4)*2;
    out.operations.emplace_back(SteamHumanImage{HA::president,{176,144},special_frame,{}});
    out.operations.emplace_back(SteamFacilityReputationImage{SteamFacilityReputationAsset::secretary,{196,144},special_frame});
    const int clip_height=in.frame<=5?0:in.frame>=40?48:((in.frame-5)*48)/35;
    out.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,{0,158,240,clip_height}});
    const auto text=[&](const std::string &value,int x,int y,int anchor=2,int size=0,bool layout=false,int width=0) {
        out.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,value,
            {x,y,width,0},anchor,size,layout,{92,51,31},0});
    };
    if(in.phase==0) {
        if(in.japanese) {
            text("冒险者们中间",120,158);text(in.name+"的",120,176);text("话题传了开来!!",120,194);
        } else if(in.language8) {
            text("\""+in.name+"\"",120,164,2,11);
            // Steam独立语言8分支额外TextLayout；不冒充所有非日文语言均有此布局。
            text("冒险者们中间话题传开了!!",19,188,2,11,true,202);
        } else {
            text("\""+in.name+"\"",120,188,2,11);text("冒险者们中间话题传开了!!",120,164,2,11);
        }
    } else {
        text("街道人气",120,164);
        const int width=22+*in.increase_suffix_width,left=120-width/2;
        out.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,SteamFacilityAsset::number09,
            20,{left,188},0,1,-1});
        text("上升!",left+22,188,1);
    }
    out.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
    if(in.frame>=40&&(in.frame-40)%25<15)
        out.operations.emplace_back(SteamHumanImage{HA::continue_cursor,{212,204},0,{}});
    out.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return out;
}
std::optional<SteamFacilityReputationPlan> steam_facility_reputation_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamFacilityReputationOptions &o) {
    const auto view=inspect_startup_world_facility_catalog_page(s,id);
    const auto secondary=s.page_secondary_counters.find(id);
    if(!view||view->raw!=82||!s.rules||secondary==s.page_secondary_counters.end()||secondary->second<0)return {};
    const auto facility=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [&](const auto &d){return d.id==view->binding;});
    if(facility==s.rules->facilities.end())return {};
    SteamFacilityReputationInput in;
    in.definition=view->binding;in.mapchip=facility->display_id;in.name=facility->name;
    in.phase=view->phase;in.frame=view->counter;in.frame2=secondary->second;in.view_y=o.view_y;
    in.japanese=o.japanese;in.language8=o.language8;in.title_widths=o.title_widths;
    in.increase_suffix_width=o.increase_suffix_width;
    for(int human:view->entries) {
        const auto identity=std::find_if(s.rules->humans.begin(),s.rules->humans.end(),
            [=](const auto &h){return h.identity==human;});
        const auto growth=s.scene.world.world.ai.growth.find(human);
        if(identity==s.rules->humans.end()||identity->sex<0||identity->sex>1||
           growth==s.scene.world.world.ai.growth.end())return {};
        const int job=growth->second.definition.current_profession;
        if(job<0||job>=static_cast<int>(s.rules->jobs.size()))return {};
        in.participants.push_back({human,s.rules->jobs[static_cast<std::size_t>(job)].sprites[static_cast<std::size_t>(identity->sex)]});
    }
    return steam_facility_reputation_skin(in);
}
} // namespace dungeon_village_prototype
