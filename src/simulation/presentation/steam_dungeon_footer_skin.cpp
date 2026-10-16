// Steam探索底栏有序只读计划；源调用、裁剪及版本差异见ui/DUNGEON_RENDER.md。
#include "ark/simulation/presentation/steam_dungeon_footer_skin.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include <algorithm>

namespace ark::simulation {
namespace {
int rate(int v,int a,int b,int c,int d) {
    if(a==b)return c;
    return c+static_cast<int>(std::int64_t(std::clamp(v,a,b)-a)*(d-c)/(b-a));
}
bool coordinate(int v) {return v>=-65536&&v<=65536;}
void image(SteamDungeonFooterPlan &p,int id,int seb,int frame,int x,int y,
           std::array<int,4> crop={}) {
    p.operations.emplace_back(StartupSkinDraw{StartupSkinPackage::common,id,seb,frame,0,crop,{x,y}});
}
void clip(SteamDungeonFooterPlan &p,int x,int y,int w,int h) {
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{x,y,w,h}});
}
void pop(SteamDungeonFooterPlan &p) {
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
}
bool monster_image(int v) {
    constexpr std::array<int,4> counts{9,12,8,6};
    return v>=0&&v/30<4&&v%30<counts[static_cast<std::size_t>(v/30)];
}
int face_sprite(int image) {
    // Steam MONSTER_FACE_SEB四行独立核值；不是身体朝向SEB或按截图选小图。
    constexpr std::array<int,4> first{42,30,16,24};
    return first[static_cast<std::size_t>(image/30)]+image%30;
}
}
std::optional<SteamDungeonFooterPlan> steam_dungeon_footer_skin(const SteamDungeonFooterInput &in) {
    SteamDungeonFooterPlan p;
    if(!in.task||in.task->kind!=0)return p;
    const auto &t=*in.task;
    const auto [left,top,width,height]=in.view;
    if(!coordinate(left)||!coordinate(top)||width<=0||width>65536||height<18||height>65536||
       in.side_width<0||in.side_width>width/2||!in.page||!t.facility||
       !in.presentation.ordinal||in.presentation.random_after<in.presentation.random_before||
       in.presentation.random_after-in.presentation.random_before!=in.presentation.dungeon_jitters.size()||
       std::count(in.presentation.pages.begin(),in.presentation.pages.end(),in.page)!=1||
       in.presentation.cleared_task||t.background_row<0||t.background_row>5||
       t.counter<0||t.progress<0||t.maximum<0||t.challenges.size()>4096||
       t.humans.size()>4096||t.effects.size()>4096)return {};
    // 探索底栏不消费其它页的随机事实；保留原逆序，不按monster ID排序。
    std::vector<const StartupDungeonJitter*> jitters;
    for(const auto &j:in.presentation.dungeon_jitters) {
        if(j.page!=in.page)continue;
        if(j.facility!=t.facility||j.offset<-1||j.offset>1)return {};
        jitters.push_back(&j);
    }
    std::size_t next_jitter{};
    for(std::size_t n=t.challenges.size();n>0;--n) {
        const auto &c=t.challenges[n-1];const auto &r=c.record;
        if(r[0]<0||r[1]<0||r[1]>1||r[2]<0||r[2]>3||r[3]<0||r[4]<0||
           (r[1]==0&&(r[4]>3||r[5]<0))||
           (r[1]==1&&(!monster_image(c.monster_image)||c.monster_sprite!=face_sprite(c.monster_image))))return {};
        if(r[1]==1&&r[2]==1&&r[3]>=10) {
            if(next_jitter==jitters.size()||jitters[next_jitter]->challenge!=n-1)return {};
            ++next_jitter;
        }
    }
    if(next_jitter!=jitters.size())return {};
    for(const auto &h:t.humans)
        if(h.progress<0||h.state<0||h.counter<0||h.body_image<0||h.body_image>37||h.body_image==32)return {};
    for(const auto &e:t.effects)if(e[0]<0||!coordinate(e[1])||!coordinate(e[2]))return {};
    const int base=(left+width)/2-127;
    const int y=top+height+rate(t.counter,0,18,0,-18);
    const int u=in.side_width,crop_width=base+177-u;
    // 负/空宽为不适用视口，不能调用后端制造未核请求；大于原图宽仍保留。
    if(crop_width<=0)return {};
    clip(p,u,top+height-18,width-2*u,18);
    image(p,127,-1,0,u,y,{217-crop_width,t.background_row*18,crop_width,18});
    const auto treasure=[&](const auto &r,int dy,int frame) {
        const int x=base+16+rate(r[0],0,100,-8,153);
        image(p,r[4]==0?6:97,27,frame,x+7,y+17+dy);
    };
    for(auto it=t.challenges.rbegin();it!=t.challenges.rend();++it)
        if(it->record[1]==0&&it->record[2]==3)treasure(it->record,0,5);
    int dy{};next_jitter=0;
    for(auto it=t.challenges.rbegin();it!=t.challenges.rend();++it) {
        const auto &r=it->record;
        if(r[1]==0) {
            dy=r[2]==1?rate(r[3],0,5,0,-18):0;
            if(r[2]!=3)treasure(r,dy,r[2]==2?5:3);
        } else {
            int jitter{};
            if(r[2]==1&&r[3]>=10) {
                dy=rate(r[3],10,40,0,18);jitter=jitters[next_jitter++]->offset;
            }
            const int x=base+16+rate(r[0],0,100,-8,153)-1+jitter;
            image(p,24,-1,0,x,y+dy,{0,0,18,18});
            p.operations.emplace_back(SteamMonsterBody{it->monster_image,it->monster_sprite,0,{x,y+dy+1}});
        }
    }
    clip(p,u,y,crop_width,18);
    for(auto it=t.humans.rbegin();it!=t.humans.rend();++it) {
        const int x=base+rate(it->progress,0,100,-8,153);
        const int hy=y+1+(it->state==1?rate(it->counter,10,15,0,-18):0);
        p.operations.emplace_back(StartupSkinRect{{x,hy,16,16},{196,236,169},false});
        p.operations.emplace_back(StartupSkinRect{{x,hy,16,16},{204,204,204},true});
        StartupTitleActorSkin actor;actor.body={it->body_image,1,0};
        p.operations.emplace_back(SteamHumanPortrait{actor,{x+8,hy+22},
            std::array<int,4>{x,hy+1,15,14},true});
    }
    pop(p);
    for(auto it=t.effects.rbegin();it!=t.effects.rend();++it)
        image(p,141,93,(*it)[0]<4?0:1,base+(*it)[1],(*it)[2]);
    image(p,86,-1,0,base+177,y,{0,0,77,18});
    const int extra=std::max(0,width-u-(77+base+177));
    for(int offset=0;offset<extra;) {
        const int part=std::min(13,extra-offset);
        image(p,86,-1,0,base+254+offset,y,{64,0,part,18});offset+=part;
    }
    const int k=extra/2+3;
    image(p,84,75,0,base+177+k,y+2);
    const int number_x=base+177+k+47+(in.japanese?0:6);
    p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,
        SteamFacilityAsset::number03_hud,rate(t.progress,0,t.maximum,0,100),{number_x,y+3},0,4,-1});
    image(p,85,76,0,number_x+1,y+6);
    pop(p);
    return p;
}
std::optional<SteamFacilityResource> steam_dungeon_footer_resource(int image) {
    switch(image) {
    case 6:return SteamFacilityResource{"common",6,27,"original/common/treasure00.png","original/common/treasure00.seb"};
    case 24:return SteamFacilityResource{"common",24,-1,"original/common/icon_back00.png",nullptr};
    case 84:return SteamFacilityResource{"common",84,75,"steam-common/menuRT00.png","steam-common/menuRT00.seb"};
    case 85:return SteamFacilityResource{"common",85,76,"steam-common/menuRT01.png","original/common/menuRT01.seb"};
    case 86:return SteamFacilityResource{"common",86,-1,"original/common/menuRT02.png",nullptr};
    case 97:return SteamFacilityResource{"common",97,27,"original/common/treasure01.png","original/common/treasure00.seb"};
    case 102:return SteamFacilityResource{"common",102,10,"original/common/number03.png","original/common/number03.seb"};
    case 127:return SteamFacilityResource{"common",127,-1,"original/common/dungeonProgressBarBack.png",nullptr};
    case 141:return SteamFacilityResource{"common",141,93,"original/common/eff_dungeonSearch.png","original/common/eff_dungeonSearch.seb"};
    default:return {};
    }
}
std::optional<SteamDungeonFooterResourceRequest> steam_dungeon_footer_resource(
    int image,const SteamDungeonFooterLanguageProfile &profile) {
    using Base=SteamDungeonFooterBaseLanguage;
    using Language=SteamDungeonFooterResourceLanguage;
    if(profile.base!=Base::root&&profile.base!=Base::english)return {};
    switch(profile.resource) {
    case Language::root:case Language::english:case Language::simplified_chinese:
    case Language::traditional_chinese:break;
    default:return {};
    }
    const auto original=steam_dungeon_footer_resource(image);
    if(!original)return {};
    SteamDungeonFooterResourceRequest out{*original,{},{}};
    // 首命中：允许资源语言才先试包目录，随后基础目录，最后根。
    const auto language=profile.use_resource_language?profile.resource:Language::root;
    const bool english=profile.base==Base::english;
    switch(image) {
    case 6:case 97:out.physical_size={22,95};break;
    case 24:out.physical_size={144,18};break;
    case 84:
        out.physical_size={59,45};
        if(language==Language::simplified_chinese) {
            out.resource.published_image="steam-common/zh-CN/menuRT00.png";
            out.resource.published_sprite="steam-common/zh-CN/menuRT00.seb";
            out.physical_size={118,90};
        } else if(language==Language::traditional_chinese) {
            out.resource.published_image="steam-common/zh/menuRT00.png";
            out.resource.published_sprite="steam-common/zh/menuRT00.seb";
            out.physical_size={118,90};
        } else if(language==Language::english||english) {
            out.resource.published_image="steam-common/English.lproj/menuRT00.png";
            out.resource.published_sprite="steam-common/English.lproj/menuRT00.seb";
        }
        if(english)out.standard_size=std::array<int,2>{59,45};
        break;
    case 85:
        out.physical_size={57,10};
        // 中文目录没有85；不能跟随84选择虚构的中文百分号。
        if(language==Language::english||english)
            out.resource.published_image="steam-common/English.lproj/menuRT01.png";
        break;
    case 86:out.physical_size={77,18};break;
    case 102:out.physical_size={80,24};break;
    case 127:out.physical_size={217,108};break;
    case 141:out.physical_size={34,18};break;
    default:return {};
    }
    return out;
}
std::optional<std::array<float,4>> steam_dungeon_footer_physical_source(
    const SteamDungeonFooterResourceRequest &request,std::array<int,4> source) {
    if(request.physical_size[0]<=0||request.physical_size[1]<=0||source[2]<=0||source[3]<=0)return {};
    float x=1,y=1;
    if(request.standard_size) {
        const auto standard=*request.standard_size;
        if(standard[0]<=0||standard[1]<=0)return {};
        x=float(request.physical_size[0])/standard[0];
        y=float(request.physical_size[1])/standard[1];
    }
    return std::array<float,4>{source[0]*x,source[1]*y,source[2]*x,source[3]*y};
}
std::optional<SteamDungeonFooterPlan> steam_dungeon_footer_skin(
    const StartupWorldRuntimeState &s,std::uint64_t page,const StartupPresentationPlan &presentation,
    std::array<int,4> view,int side_width,bool japanese) {
    const auto source=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),
        [=](const auto &p){return p.id==page;});
    if(!s.rules||!page||page>=s.scripts.next_page_id||s.scripts.executing_page||
       s.scripts.page_mutations_locked||source==s.scripts.pages.end()||
       std::count_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==page;})!=1||
       source->kind!=ref::WorldScriptPageKind::scene||
       source->lifecycle<1||source->lifecycle>3||!presentation.ordinal||
       presentation.random_after!=s.scene.random.draws()||
       std::count(presentation.pages.begin(),presentation.pages.end(),page)!=1)return {};
    SteamDungeonFooterInput in;in.view=view;in.side_width=side_width;in.japanese=japanese;
    in.page=page;in.presentation=presentation;
    if(!s.active_task)return steam_dungeon_footer_skin(in);
    const auto task=s.tasks.find(*s.active_task);
    if(task==s.tasks.end()||task->second.identity!=task->first)return {};
    const auto definition=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),
        [&](const auto &d){return d.factory.identity==task->second.definition;});
    if(definition==s.rules->tasks.end())return {};
    if(definition->factory.kind!=0)return steam_dungeon_footer_skin(in);
    if(!task->second.facility)return {};
    const auto facility=s.scene.world.world.facilities.find(*task->second.facility);
    const auto progress=s.dungeon_facilities.find(*task->second.facility);
    if(facility==s.scene.world.world.facilities.end()||progress==s.dungeon_facilities.end()||
       progress->second.challenges.size()>4096||facility->second.occupants.size()>4096||
       s.scene.world.floating_notes.size()>4096)return {};
    SteamDungeonFooterTask t;t.facility=*task->second.facility;
    t.background_row=definition->dungeon_background_row;t.counter=s.scene.world.updates;
    t.progress=progress->second.progress;t.maximum=progress->second.extent;
    for(const auto &r:progress->second.challenges) {
        SteamDungeonFooterChallenge c;c.record=r;
        if(r[1]==1) {
            const auto m=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),
                [&](const auto &d){return d.identity==r[4];});
            if(m==s.rules->monsters.end()||m->initial.body<0||m->initial.body>3||
               m->initial.sprite_variant<0||m->initial.sprite_variant>=30)return {};
            c.monster_image=m->initial.body*30+m->initial.sprite_variant;
            if(!monster_image(c.monster_image))return {};
            c.monster_sprite=face_sprite(c.monster_image);
        }
        t.challenges.push_back(c);
    }
    for(const auto id:facility->second.occupants) {
        const auto &ai=s.scene.world.world.ai;
        const auto active=ai.battle.actors.find(id),retained=ai.retired_actors.find(id);
        if((active!=ai.battle.actors.end())==(retained!=ai.retired_actors.end()))return {};
        const auto &actor=active!=ai.battle.actors.end()?active->second:retained->second;
        const auto state=s.dungeon_actors.find(id);
        if(!(actor.id==id)||actor.kind!=ref::ActorKind::human||
           state==s.dungeon_actors.end())return {};
        const auto human=startup_world_human_details(s,actor.definition);
        if(!human)return {};
        const auto portrait=startup_title_actor_skin(human->profession,human->sex,-1,0,1,false);
        if(!portrait)return {};
        t.humans.push_back({state->second.percent,state->second.retreat_state,
            state->second.retreat_updates,portrait->body.image});
    }
    for(const auto &e:s.scene.world.floating_notes) {
        if(e.size()<3)return {};
        t.effects.push_back({e[0],e[1],e[2]}); // 源其它尾槽不修改、不截回Owner。
    }
    in.task=std::move(t);
    return steam_dungeon_footer_skin(in);
}
} // namespace ark::simulation
