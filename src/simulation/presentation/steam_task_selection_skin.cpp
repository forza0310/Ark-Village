// Steam SubForm._draw raw22与_draw1_1 raw23／24的局部皮肤，保留原调用序。
#include "ark/simulation/presentation/steam_task_selection_skin.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
using Plan=SteamTaskSelectionPlan;
void text(Plan &p,std::string v,int x,int y,int anchor=0,int size=0,bool layout=false,
          int w=0,int h=0,std::array<int,3> rgb={92,51,31}) {
    p.operations.emplace_back(SteamTaskSelectionText{std::move(v),{x,y,w,h},anchor,size,layout,rgb});
}
void image(Plan &p,SteamTaskSelectionAsset a,int x,int y,int frame=0,
           std::optional<std::array<int,4>> crop={}) {
    p.operations.emplace_back(SteamTaskSelectionImage{a,{x,y},frame,crop});
}
void number(Plan &p,SteamFacilityAsset a,int n,int x,int y,int anchor=4,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number) {
    p.operations.emplace_back(SteamFacilityNumber{kind,a,n,{x,y},0,anchor,-1});
}
bool width(std::optional<int> w) {return w&&*w>=0&&*w<=4096;}
std::optional<std::string> task_name(const StartupWorldRuntimeState &s,const StartupWorldTask &d) {
    // QuestData.GetName(false) RVA21D6B0／21D6D0，复发名称不冻结成原表基础串。
    if(d.factory.flags&2U) {
        const auto g=s.scene.world.world.ai.monster_growth.find(d.factory.monster);
        if(g==s.scene.world.world.ai.monster_growth.end()||g->second.growth<0)return {};
        if(g->second.growth>0) {
            const auto next=std::int64_t(g->second.growth)+1;
            if(next>std::numeric_limits<int>::max())return {};
            return d.name+std::to_string(next);
        }
    } else if(d.factory.flags&4U) {
        const auto progress=s.task_progress.definitions.find(d.factory.identity);
        if(progress==s.task_progress.definitions.end()||progress->second.completed<0)return {};
        if(progress->second.completed>0)return d.name+"+"+
            std::to_string(std::min(std::int64_t(progress->second.completed)*2,std::int64_t(10)));
    }
    return d.name;
}
bool frame(Plan &p,const SteamTaskSelectionInput &in) {
    if(!in.options.title_widths)return false;
    const bool list=in.raw==22;
    const auto w=steam_startup_window_skin({list?220:222,list?168:170,0,0,
        SteamStartupTextRole::message_title,0},in.options.view_y,in.options.title_widths);
    const auto b=steam_startup_box_skin(list?SteamStartupBox{17,74,219,184}:
        SteamStartupBox{15,139,224,202},in.options.view_y);
    if(!w||!b)return false;
    for(const auto &d:w->draws)p.operations.emplace_back(d);
    for(const auto &d:b->draws)p.operations.emplace_back(d);
    return true;
}
bool bottom(Plan &p,const SteamTaskSelectionInput &in) {
    if(!width(in.options.bottom_width))return false;
    const int w=std::min(*in.options.bottom_width,200)+16,left=120-w/2,y=in.raw==22?197:184;
    if(in.raw==23) {
        p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{left,y,w,16},{255,153,55},false}});
        // Draw_btmMsg的BTMT首行[-20,-20,40,40]，组件4确认值20，无marker。
        p.operations.emplace_back(SteamStartupTouch{4,20,std::array<int,4>{left-20,y-20,w+40,56},{},0});
    }
    text(p,in.raw==22?"请选择任务":"募集冒险者",left,y,34,0,true,w,16);
    if(in.raw==23)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{left-4,y+9},in.options.finger_frame,{}});
    return true;
}
void scroll(Plan &p,int count,int first,bool touched) {
    const int denom=std::max(count,5),top=85+111*first/denom,height=555/denom;
    const std::array<int,4> track{221,85,3,111};
    p.operations.emplace_back(SteamHumanCatalogueScroll{
        std::array<StartupSkinRect,2>{{{{220,85,5,111},{7,5,78},false},
            {{220,top,5,height+1},touched&&count>5?std::array<int,3>{246,129,0}:
                std::array<int,3>{48,160,255},false}}},
        SteamStartupTouch{12,0x40000,track,{},0},{3,20,0,0},{count,5,0x20000},SteamStartupTouch{25,0,track,{},4}});
}
int rate(int n,int lo,int hi,int first,int last) {
    if(n<=lo)return first;if(n>=hi)return last;
    return first+static_cast<int>(std::int64_t(last-first)*(n-lo)/(hi-lo));
}
bool actor(Plan &p,const SteamTaskRecruitmentHuman &h,int step,int direction,int x,int y) {
    const auto skin=startup_title_actor_skin(h.profession,h.sex,h.weapon,step,direction,false);
    if(!skin)return false;
    p.operations.emplace_back(SteamHumanPortrait{*skin,{x,y},{},false});return true;
}
bool phrase(Plan &p,int id,int x,int y,const SteamTaskSelectionOptions &o) {
    constexpr const char *words[]{"交给我!","多指教","来了!","我会加油的!","我要参加!","好期待"};
    if(id<0||id>=6||!width(o.phrase_widths[id]))return false;
    const int w=*o.phrase_widths[id],left=x-w/2;
    if(w<=60)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{left,y},0,std::array<int,4>{36-w/2,0,w,21}});
    else {
        for(int used=0;used<w;) {const int part=std::min(27,w-used);
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{left+used,y},0,std::array<int,4>{6,0,part,21}});used+=part;}
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-2,y},0,std::array<int,4>{33,0,5,21}});
    }
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{left-6,y},0,{}});
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x+w/2,y},1,{}});
    text(p,words[id],x,y+3,2,0,false,0,0,{0,0,0});return true;
}
void medals(Plan &p,int n) {
    // Steam Draw_medal(x69,y157)只处理小枚数，五枚起改乘号＋右展开数字。
    if(n==0)return;
    if(n<5)for(int i=0;i<n;++i)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::medal,{69+10*i,157},0,std::array<int,4>{14,0,14,14}});
    else {
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::medal,{69,157},0,std::array<int,4>{14,0,14,14}});
        p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::number05,{81,161},16,{}});
        number(p,SteamFacilityAsset::number05,n,91,161,1);
    }
}
}
std::optional<SteamFacilityResource> steam_task_selection_resource(SteamTaskSelectionAsset a) {
    switch(a) {
    case SteamTaskSelectionAsset::recruitment_background:return SteamFacilityResource{"event",0,-1,"original/event/event_backNormal.png",nullptr};
    case SteamTaskSelectionAsset::treasure:return SteamFacilityResource{"common",6,27,"original/common/treasure00.png","original/common/treasure00.seb"};
    }
    return {};
}
std::optional<Plan> steam_task_selection_skin(const SteamTaskSelectionInput &in) {
    const auto &o=in.options;
    if(in.raw<22||in.raw>24||in.counter<0||o.finger_frame<0||o.finger_frame>5)return {};
    Plan p;p.title=in.raw==22?"任务选择":"寻求！冒险者";
    p.soft_labels=in.raw==22?std::array<int,2>{6,2}:in.raw==23?std::array<int,2>{0,2}:std::array<int,2>{0,0};
    if(!frame(p,in))return {};
    if(in.raw==22) {
        if(in.rows.empty()||in.rows.size()>65536||in.selection<0||in.selection>=static_cast<int>(in.rows.size())||
           in.first<0||in.first>in.selection||in.selection-in.first>=5)return {};
        text(p,"姓名",30,69);text(p,"费用",185,69);
        for(int row=in.first;row<static_cast<int>(in.rows.size())&&row-in.first<5;++row) {
            const auto &r=in.rows[row];const int y=97+19*(row-in.first);
            if(r.identity==0||r.kind<0||r.kind>1||r.difficulty<0||r.fee<0||r.name.empty()||r.name.size()>4096)return {};
            if(row==in.selection)p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{23,y-2,191,16},{255,153,55},false}});
            p.operations.emplace_back(SteamStartupTouch{11,0x20000|row,std::array<int,4>{10,y,230,16},{},0});
            if(r.newly_available)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::catalogue_new,{10,y+2},0,{}});
            if(row==in.selection)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{21,y+8},o.finger_frame,{}});
            text(p,r.name,29,y+(o.japanese?1:2),0,o.japanese?0:9);
            if(r.kind==0) {image(p,SteamTaskSelectionAsset::treasure,136,y+12,2);number(p,SteamFacilityAsset::number03,r.difficulty,156,y);}
            number(p,r.affordable?SteamFacilityAsset::number08:SteamFacilityAsset::number12,r.fee,214,y+1,4,SteamFacilityNumberKind::money);
        }
        scroll(p,static_cast<int>(in.rows.size()),in.first,o.scroll_first_touch);
        if(!bottom(p,in))return {};
    } else {
        if(in.first!=0||in.selection<0||in.selection>(in.raw==23?1:0))return {};
        image(p,SteamTaskSelectionAsset::recruitment_background,30,67);
        if(in.raw==23) {
            if(in.fee<0)return {};
            text(p,"费用",46,164,0,0,false,0,0,{0,100,255});
            number(p,in.affordable?SteamFacilityAsset::number08:SteamFacilityAsset::number12,in.fee,194,165,4,SteamFacilityNumberKind::money);
            if(!bottom(p,in))return {};
        } else {
            if(!in.recruitment)return {};const auto &a=*in.recruitment;
            if(a.entries.size()>9||a.portraits.size()>9||in.humans.size()>65536||a.displayed_count<0||a.displayed_count>9)return {};
            std::set<int> ids;
            for(const auto &h:in.humans)if(h.identity<0||h.level<0||h.level>10||h.medals<0||h.name.empty()||
                h.name.size()>4096||!ids.insert(h.identity).second)return {};
            const auto human=[&](int id)->const SteamTaskRecruitmentHuman * {
                const auto it=std::find_if(in.humans.begin(),in.humans.end(),[=](const auto &h){return h.identity==id;});
                return it==in.humans.end()?nullptr:&*it;
            };
            p.operations.emplace_back(SteamTaskSelectionClip{true,{30,67,180,80}});
            const int step=(in.counter%16)/4;
            for(const auto &entry:a.entries) {
                if(entry.phrase< -1||entry.phrase>=6)return {};
                if(entry.counter<0||entry.counter>=130)continue;
                const auto *h=human(entry.human);if(!h)return {};
                const int x=entry.counter<80?rate(entry.counter,0,80,220,80):entry.counter<110?
                    rate(entry.counter,80,110,80,61):rate(entry.counter,110,130,61,20);
                const int y=entry.counter<80?142:entry.counter<110?rate(entry.counter,80,110,142,121):121;
                if(!actor(p,*h,step,2,x,y))return {};
                if(entry.phrase!=-1&&entry.counter>=20&&entry.counter<55&&!phrase(p,entry.phrase,x,y-45,o))return {};
            }
            image(p,SteamTaskSelectionAsset::recruitment_background,30,105,0,std::array<int,4>{0,38,19,22});
            p.operations.emplace_back(SteamTaskSelectionClip{false,{}});
            if(!a.portraits.empty()) {
                const auto *h=human(a.portraits.front());if(!h||!actor(p,*h,step,1,48,183))return {};
                text(p,h->name,69,191,2);
                p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{71,174},0,{}});
                if(h->level==10)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::maximum,{90,176},0,{}});
                else number(p,SteamFacilityAsset::number05,h->level,105,174);
                medals(p,h->medals);
            }
            number(p,SteamFacilityAsset::number03_hud,a.displayed_count,188,174);
            text(p,"合计",132,175);text(p,"人",190,175);
            // 原AddTouch无矩形的组件5/value20；按住确认由原页Update消费。
            p.operations.emplace_back(SteamStartupTouch{5,20,{},{},2});
        }
    }
    return p;
}
std::optional<Plan> steam_task_selection_skin(const StartupWorldRuntimeState &s,std::uint64_t id,
                                               const SteamTaskSelectionOptions &o) {
    if(!s.rules)return {};
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &p){return p.lifecycle!=4;});
    const auto clock=s.page_counters.find(id);
    if(page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||
       page->kind!=ref::WorldScriptPageKind::raw_page||page->legacy_page<22||page->legacy_page>24||clock==s.page_counters.end()||!valid_startup_world_runtime_task_page(s,*page))return {};
    SteamTaskSelectionInput in;in.raw=page->legacy_page;in.counter=clock->second;in.options=o;
    if(in.raw==22) {
        const auto list=s.task_page_lists.find(id);
        const auto selection=s.page_phases.find(id),first=s.page_secondary_counters.find(id);
        if(list==s.task_page_lists.end()||selection==s.page_phases.end()||first==s.page_secondary_counters.end())return {};
        in.selection=selection->second;in.first=first->second;
        for(const auto taskid:list->second) {
            const auto t=s.tasks.find(taskid);if(t==s.tasks.end()||t->second.identity!=taskid)return {};
            const auto d=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[&](const auto &v){return v.factory.identity==t->second.definition;});
            if(d==s.rules->tasks.end())return {};
            const auto name=task_name(s,*d);if(!name)return {};
            in.rows.push_back({taskid,d->factory.kind,t->second.difficulty,d->recruitment_fee,
                t->second.newly_available,s.scene.world.world.ai.accounting.funds()>=d->recruitment_fee,*name});
        }
    } else {
        if(!page->task_identity)return {};const auto task=s.tasks.find(*page->task_identity);
        if(task==s.tasks.end()||task->second.identity!=*page->task_identity||
           !page->task_definition||*page->task_definition!=task->second.definition)return {};
        const auto d=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[&](const auto &v){return v.factory.identity==task->second.definition;});
        if(d==s.rules->tasks.end())return {};
        in.fee=d->recruitment_fee;in.affordable=s.scene.world.world.ai.accounting.funds()>=in.fee;
        if(in.raw==23) {const auto selection=s.page_phases.find(id);if(selection==s.page_phases.end())return {};in.selection=selection->second;}
        else {
            const auto a=s.task_recruitment_pages.find(id);if(a==s.task_recruitment_pages.end()||!s.page_secondary_counters.count(id))return {};
            in.recruitment=a->second;std::set<int> needed;
            for(const auto &entry:a->second.entries)if(entry.counter>=0&&entry.counter<130)needed.insert(entry.human);
            if(!a->second.portraits.empty())needed.insert(a->second.portraits.front());
            for(int human:needed) {const auto h=startup_world_human_details(s,human);if(!h)return {};
                in.humans.push_back({human,h->profession,h->sex,h->equipment[0].value_or(-1),h->level,h->medals,h->name});}
        }
    }
    return steam_task_selection_skin(in);
}
} // namespace ark::simulation
