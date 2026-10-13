// Steam SubForm._draw1_2 VA10336228–103375CC；固定APK b/g.java:1457–1662交叉。
#include "dungeon_village_prototype/steam_task_deadline_skin.hpp"
#include "dungeon_village_prototype/steam_task_result_skin.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_reference/world_encounters.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
using Plan=SteamTaskDeadlinePlan;
constexpr std::array<int,3> brown{92,51,31},blue{0,100,255};
bool width(std::optional<int> value) {return value&&*value>=0&&*value<=4096;}
void text(Plan &p,std::string value,int x,int y,int anchor=0,int size=0,
          std::array<int,3> color=brown) {
    p.operations.emplace_back(SteamTaskSelectionText{std::move(value),{x,y,0,0},anchor,size,false,color});
}
void number(Plan &p,SteamFacilityAsset asset,int value,int x,int y,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number) {
    p.operations.emplace_back(SteamFacilityNumber{kind,asset,value,{x,y},0,4,-1});
}
void small_box(Plan &p,int x,int y) {
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{x,y,16,16},{196,236,169},false}});
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{x,y,16,16},{204,204,204},true}});
}
bool bubble(Plan &p,const std::string &value,std::optional<int> measured) {
    if(!width(measured))return false;
    const auto shape=steam_human_down_bubble_skin(*measured,{69,96});if(!shape)return false;
    // 复用已核Draw_fukidashi图元，替换该helper唯一的“倒下”文案，不复制另一套切片算法。
    for(const auto &d:shape->draws) {
        if(const auto *r=std::get_if<StartupSkinRect>(&d))p.operations.emplace_back(SteamStartupFrameDraw{*r});
        else if(const auto *r=std::get_if<SteamHumanImage>(&d))p.operations.emplace_back(*r);
    }
    text(p,value,69,99,2,0,{0,0,0});return true;
}
std::optional<StartupTitleActorSkin> actor(const SteamTaskDeadlineHuman &h,int action,int step) {
    // 原JobData.GetBodyImg()无参版本固定男性图；不是人物sex或随机性别。
    auto out=startup_title_actor_skin(h.profession,0,action==7?-1:h.weapon,step,1,false);
    if(!out)return {};
    if(action==7) {out->body.sprite=13;out->body.frame=0;}
    else if(action==4) {
        out->body.sprite=21;out->body.frame=0;
        if(out->weapon) {
            const auto &weapons=startup_world_rules().equipment;
            const auto found=std::find_if(weapons.begin(),weapons.end(),[&](const auto &w){return w.shop.kind==1&&w.shop.id==h.weapon;});
            if(found==weapons.end()||found->render_style<0||found->render_style>3)return {};
            // GetWeponOffset RVA283D40的4／6／11共用WEAPON_MAGIC_POS，方向1、step0。
            constexpr std::array<std::array<int,2>,4> raised{{{-4,-36},{-4,-35},{-4,-36},{-8,-46}}};
            out->weapon->offset=raised[found->render_style];out->weapon->frame=0;
        }
    } else if(action!=0)return {};
    return out;
}
bool map(Plan &p,int mapchip) {
    const auto &catalog=startup_evidence().displays;
    const auto art=std::find_if(catalog.begin(),catalog.end(),[=](const auto &v){return v.id==mapchip;});
    if(art==catalog.end()||art->sprite.empty())return false;
    const auto &definitions=startup_world_rules().facilities;
    const auto d=std::find_if(definitions.begin(),definitions.end(),[&](const auto &v){return v.id==art->definition_id;});
    if(d==definitions.end()||d->display_id!=mapchip||d->shape<0||d->shape>2)return false;
    // 同普通建设消费者的ah/ai分片与kind6帧覆盖；无DrawMapchip2中心补偿。
    const auto pieces=ref::facility_footprint(static_cast<ref::FacilityShape>(d->shape),
        ref::FacilityOrientation::first,{1,1},3,3);
    if(pieces.error!=ref::GeometryError::none)return false;
    p.operations.emplace_back(SteamTaskSelectionClip{true,{17,85,202,110}});
    for(const auto &part:pieces.cells) {
        const int x=part.position.x-1,y=part.position.y-1;
        p.operations.emplace_back(SteamFacilityMapchipDraw{art->sprite,d->kind==6?11:part.fragment_index,
            {130+30*(x+y),121+15*(x-y)}});
    }
    p.operations.emplace_back(SteamTaskSelectionClip{false,{}});return true;
}
}
std::optional<SteamFacilityResource> steam_task_deadline_resource(SteamTaskDeadlineAsset asset) {
    if(asset==SteamTaskDeadlineAsset::progress_label)
        return SteamFacilityResource{"common",84,75,"steam-common/menuRT00.png","steam-common/menuRT00.seb"};
    return {};
}
std::optional<Plan> steam_task_deadline_skin(const SteamTaskDeadlineInput &in) {
    const auto &o=in.options;
    if(in.phase<0||in.phase>1||in.selection<0||in.selection>1||in.counter<0||
       in.counter>=std::numeric_limits<int>::max()||in.fee<0||in.grade<0||in.grade>4||in.kind<0||in.kind>1||
       in.task_name.empty()||in.task_name.size()>4096||in.participants.size()>9||in.humans.size()>65536||
       in.active_humans.size()>65536||in.monsters.size()>65536||in.encounter_monsters.size()>65536||
       o.finger_frame<0||o.finger_frame>5||!o.title_widths||!width(o.down_number_width)||
       !width(o.prediction_width)||!width(o.button_widths[0])||!width(o.button_widths[1])||
       (in.phase==1&&(!in.answer||*in.answer<0||*in.answer>1||*in.answer!=in.selection)))return {};
    std::set<int> identities;
    for(const auto &h:in.humans)if(h.identity<0||h.downs<0||!identities.insert(h.identity).second||
        !startup_title_actor_skin(h.profession,0,h.weapon,0,1,false))return {};
    // HP_AFTER可为负或高于新最大HP；不在只读查询夹值。只有源100*HP的int32溢出拒绝。
    for(const auto &h:in.active_humans)if(h.definition<0||h.action<0||h.action>11||h.hp_maximum<=0||
        std::int64_t(h.hp_after)*100<std::numeric_limits<int>::min()||
        std::int64_t(h.hp_after)*100>std::numeric_limits<int>::max())return {};
    const auto human=[&](int id)->const SteamTaskDeadlineHuman * {
        const auto it=std::find_if(in.humans.begin(),in.humans.end(),[=](const auto &h){return h.identity==id;});
        return it==in.humans.end()?nullptr:&*it;
    };
    std::int64_t downs=0;
    for(int id:in.participants) {const auto *h=human(id);if(!h)return {};downs+=h->downs;}
    if(downs>std::numeric_limits<int>::max())return {};
    if(in.kind==0) {
        if(!in.progress||*in.progress<0||*in.progress>100||in.remaining||in.main_monster||
           !in.encounter_monsters.empty()||!in.monsters.empty())return {};
    } else {
        if(in.mapchip||in.progress||!in.remaining||*in.remaining<0||!in.main_monster)return {};
        identities.clear();
        constexpr std::array<int,4> image_counts{9,12,8,6}; // 原monster/img.inf真实槽，拒绝缺图。
        for(const auto &m:in.monsters) {
            if(m.identity<0||m.body<0||m.body>3||m.variant<0||m.variant>=image_counts[m.body]||
                m.shadow<0||m.shadow>3||!identities.insert(m.identity).second)return {};
        }
        if(!identities.count(*in.main_monster))return {};
        for(int id:in.encounter_monsters)if(!identities.count(id))return {};
        if(*in.remaining>=2&&in.encounter_monsters.empty())return {};
    }
    Plan p;p.title="任务继续确认";
    const auto outer=steam_startup_window_skin({220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto inner=steam_startup_box_skin({17,74,219,184},o.view_y);if(!outer||!inner)return {};
    for(const auto &d:outer->draws)p.operations.emplace_back(d);
    for(const auto &d:inner->draws)p.operations.emplace_back(d);
    text(p,in.task_name,22,o.japanese?69:71,1,o.japanese?0:9);
    text(p,"费用",139,69,0,o.japanese?0:11,blue);
    number(p,in.affordable?SteamFacilityAsset::number08:SteamFacilityAsset::number12,in.fee,215,70,SteamFacilityNumberKind::money);
    int last_step=0;
    for(int i=0;i<static_cast<int>(std::min<std::size_t>(3,in.participants.size()));++i) {
        const auto *h=human(in.participants[i]);if(!h)return {};
        int action=0,step=(in.counter%16)/4;
        if(in.phase==1) {
            if(*in.answer==1)step=0;
            else if((in.counter>=2&&in.counter<8)||(in.counter>=12&&in.counter<18)||
                    (in.counter>=22&&in.counter<36)) {action=4;step=0;}
        }
        const auto live=std::find_if(in.active_humans.begin(),in.active_humans.end(),
            [&](const auto &v){return v.definition==h->identity;});
        if(live!=in.active_humans.end()&&live->action==7) {action=7;step=0;}
        const auto skin=actor(*h,action,step);if(!skin)return {};
        p.operations.emplace_back(SteamHumanPortrait{*skin,{49+20*i,145},{},false});last_step=step;
        if(live!=in.active_humans.end()&&(in.counter%12)/6==0&&
           std::int64_t(live->hp_after)*100/live->hp_maximum<=30)
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::pinch,{35+20*i,117},0,{}});
    }
    const int down_width=*o.down_number_width*8/6+60;
    text(p,"下降次数",69-down_width/2,157);
    number(p,SteamFacilityAsset::number05,static_cast<int>(downs),69+down_width/2-9,158,SteamFacilityNumberKind::comma_number);
    p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::number05,{69+down_width/2-9,158},19,{}});
    if(in.phase==1&&!bubble(p,*in.answer==0?"加油哦!":"遗憾…",o.bubble_width))return {};
    if(in.kind==0) {
        if(in.mapchip&&!map(p,*in.mapchip))return {};
        p.operations.emplace_back(SteamTaskDeadlineImage{SteamTaskDeadlineAsset::progress_label,{129,154},0});
        number(p,SteamFacilityAsset::number03_hud,*in.progress,178,155);
        p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::inventory_unit,{180,157},0,{}});
    } else {
        p.operations.emplace_back(SteamTaskDeadlineImage{SteamTaskDeadlineAsset::progress_label,{130,154},1});
        number(p,SteamFacilityAsset::number03_hud,*in.remaining,187,155);
        p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::inventory_unit,{189,158},1,{}});
        const auto monster=[&](int id)->const SteamTaskDeadlineMonsterDefinition * {
            const auto it=std::find_if(in.monsters.begin(),in.monsters.end(),[=](const auto &m){return m.identity==id;});
            return it==in.monsters.end()?nullptr:&*it;
        };
        const auto *main=monster(*in.main_monster);if(!main)return {};
        for(int i=2;i>=0;--i)if(i<*in.remaining) {
            const int id=i==0?*in.main_monster:in.encounter_monsters[i==1||in.encounter_monsters.size()<2?0:1];
            const auto *m=monster(id);if(!m)return {};
            const int x=i==0?165:165+(i==1?-1:1)*(main->body*3+15);
            p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::shadow,{x,146},{},m->shadow});
            // DrawDispMonster忽略外层animeIndex；SetDrawParam的action0按主钟与怪物速度重算。
            constexpr std::array<int,4> speed{6,6,8,10};
            p.operations.emplace_back(SteamTaskDeadlineMonster{m->body*30+m->variant,m->body*4+2,
                (in.counter%(speed[m->body]*4))/speed[m->body],{x,145},last_step});
        }
    }
    constexpr const char *predictions[]{"再加把劲!!","再加加油!!","需要补充战力!","重新来过比较好","应该撤退…"};
    const int prediction_width=*o.prediction_width+20,left=120-prediction_width/2;
    small_box(p,left-5,173);
    p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::secretary_face,{left-5,173},std::array<int,4>{0,0,16,16}});
    // VA10337406读取SC_WINDOW_BLACK(+68)，不是费用使用的SC_WINDOW_BLUE。
    text(p,predictions[in.grade],120+prediction_width/2,176,4);
    constexpr const char *buttons[]{"继续","中止"};
    for(int i=0;i<2;++i) {
        const int w=*o.button_widths[i]+16,left=80+80*i-w/2;
        if(in.selection==i)p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{left,197,w,16},{255,153,55},false}});
        text(p,buttons[i],80+80*i,199,2);
        if(in.selection==i)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{left-4,206},o.finger_frame,{}});
    }
    // phase1仍绘制并注册；是否接受方向／确认由Owner Update决定，纯绘制不提前退休。
    for(int i=0;i<2;++i)p.operations.emplace_back(SteamStartupTouch{3,0x20000|i,std::array<int,4>{31+90*i,175,98,56},{},0});
    return p;
}
std::optional<SteamTaskDeadlineInput> steam_task_deadline_input(const StartupWorldRuntimeState &s,
    std::uint64_t id) {
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
        [](const auto &v){return v.lifecycle!=4;});
    if(!s.rules||page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||page->legacy_page!=33||
       !valid_startup_world_runtime_task_summary_ui_page(s,*page)||!page->task_definition)return {};
    const auto ui=s.task_page_ui.find(id),end=s.task_page_ui.end();
    const auto phase=s.page_phases.find(id),clock=s.page_counters.find(id),grade=s.deadline_grades.find(id),
        answer=s.deadline_returns.find(id);
    if(ui==end||phase==s.page_phases.end()||clock==s.page_counters.end()||grade==s.deadline_grades.end()||
       answer==s.deadline_returns.end())return {};
    const auto definition=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),
        [&](const auto &v){return v.factory.identity==*page->task_definition;});
    if(definition==s.rules->tasks.end())return {};
    const auto name=steam_task_display_name(s,*definition,false);if(!name)return {};
    SteamTaskDeadlineInput in;in.phase=phase->second;in.selection=ui->second.selection;in.counter=clock->second;
    in.grade=grade->second;in.fee=page->legacy_f;in.affordable=s.scene.world.world.ai.accounting.funds()>=in.fee;
    in.task_name=*name;in.kind=definition->factory.kind;if(in.phase==1)in.answer=answer->second;
    const ref::DungeonFinishTask *task=nullptr;
    if(page->task_identity) {
        const auto found=s.tasks.find(*page->task_identity);
        if(found==s.tasks.end()||found->second.definition!=*page->task_definition)return {};
        task=&found->second;
    }
    in.participants=s.participants;std::set<int> seen;
    for(int identity:in.participants)if(seen.insert(identity).second) {
        const auto h=startup_world_human_details(s,identity);
        const auto b=s.scene.world.world.ai.battle.humans.find(identity);
        if(!h||!h->equipment[0]||b==s.scene.world.world.ai.battle.humans.end())return {};
        in.humans.push_back({identity,h->profession,*h->equipment[0],b->second.participant_downs});
    }
    // bl原序，仅查询其首同定义实例；不把活跃实例改为定义map排序。
    for(const auto actor_id:s.scene.world.world.ai.human_order) {
        const auto a=s.scene.world.world.ai.battle.actors.find(actor_id);
        if(a==s.scene.world.world.ai.battle.actors.end())return {};
        if(seen.count(a->second.definition))in.active_humans.push_back({a->second.definition,
            a->second.control.action,a->second.hp.target,a->second.capacity});
    }
    if(in.kind==0) {
        if(!definition->factory.facilities.empty()) {
            const auto f=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
                [&](const auto &v){return v.id==definition->factory.facilities.front();});
            if(f==s.rules->facilities.end())return {};in.mapchip=f->display_id;
        }
        if(!task)in.progress=70; // 原aa为空的明确展示分支，不用于修补悬空绑定。
        else {
            if(!task->facility)return {};
            const auto f=s.dungeon_facilities.find(*task->facility);
            if(f==s.dungeon_facilities.end())return {};
            // Dungeon消费者维护的原g→extent百分比；绘制不再次推进设施进度。
            if(f->second.percent<0||f->second.percent>100)return {};in.progress=f->second.percent;
        }
    } else if(in.kind==1) {
        const auto progress=s.task_progress.definitions.find(*page->task_definition);
        if(progress==s.task_progress.definitions.end())return {};
        const auto quota=ref::prepare_task_encounter_quota(definition->encounter_quota,
            progress->second.completed,progress->second.flags);if(!quota)return {};
        std::int64_t remaining=task?*quota:std::min(3,*quota);
        if(task&&s.task.encounter) {
            const auto event=s.scene.world.world.ai.encounters.find(*s.task.encounter);
            if(event==s.scene.world.world.ai.encounters.end())return {};
            remaining-=std::int64_t(event->second.runtime.spawned)-event->second.linked_monsters;
        }
        if(remaining<0||remaining>std::numeric_limits<int>::max())return {};
        in.remaining=static_cast<int>(remaining);in.main_monster=definition->factory.monster;
        in.encounter_monsters=definition->encounter_monsters;seen.clear();
        const auto monster=[&](int identity)->bool {
            if(!seen.insert(identity).second)return true;
            const auto d=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),
                [&](const auto &v){return v.identity==identity;});
            if(d==s.rules->monsters.end())return false;
            in.monsters.push_back({identity,d->initial.body,d->initial.sprite_variant,d->shadow});return true;
        };
        if(!monster(*in.main_monster))return {};
        for(int identity:in.encounter_monsters)if(!monster(identity))return {};
    } else return {};
    return in;
}
std::optional<SteamTaskDeadlinePlan> steam_task_deadline_skin(const StartupWorldRuntimeState &s,
    std::uint64_t id,const SteamTaskDeadlineOptions &options) {
    auto in=steam_task_deadline_input(s,id);if(!in)return {};in->options=options;
    return steam_task_deadline_skin(*in);
}
} // namespace dungeon_village_prototype
