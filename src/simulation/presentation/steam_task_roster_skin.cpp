// Steam SubForm._draw1_1：25/26 RVA331996，27 RVA3311DE，28 RVA330630。
#include "ark/simulation/presentation/steam_task_roster_skin.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
using Plan=SteamTaskRosterPlan;
void text(Plan &p,std::string v,int x,int y,int anchor=0,bool layout=false,int w=0,int h=0) {
    p.operations.emplace_back(SteamTaskSelectionText{std::move(v),{x,y,w,h},anchor,0,layout,{92,51,31}});
}
void number(Plan &p,SteamFacilityAsset a,int value,int x,int y,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number) {
    p.operations.emplace_back(SteamFacilityNumber{kind,a,value,{x,y},0,4,-1});
}
void small_box(Plan &p,int x,int y) {
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{x,y,16,16},{196,236,169},false}});
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{x,y,16,16},{204,204,204},true}});
}
bool portrait(Plan &p,const SteamTaskRosterHuman &h,int y,bool roster) {
    const auto actor=startup_title_actor_skin(h.profession,h.sex,-1,0,1,false);
    if(!actor)return false;
    // 25/26用Draw_chara2的y-1 clip；27用Draw_chara的原y clip，锚点相差一行。
    p.operations.emplace_back(SteamHumanPortrait{*actor,{37,y+(roster?21:20)},
        std::array<int,4>{29,y-1,15,14},false});return true;
}
void finger(Plan &p,int y,const SteamTaskRosterOptions &o) {
    p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{21,y+8},o.finger_frame,{}});
}
void level(Plan &p,int value,int x,int y) {
    if(value==10)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::maximum,{x-15,y+2},0,{}});
    else number(p,SteamFacilityAsset::number03,value,x,y);
}
void scroll(Plan &p,int count,int first,bool touched) {
    // 原X.size-1为最大行号，helper转换成真实count；不能减去25的追加委托行。
    const int denominator=std::max(count,5),top=85+111*first/denominator,height=555/denominator;
    const std::array<int,4> track{221,85,3,111};
    p.operations.emplace_back(SteamHumanCatalogueScroll{
        std::array<StartupSkinRect,2>{{{{220,85,5,111},{7,5,78},false},
            {{220,top,5,height+1},touched&&count>5?std::array<int,3>{246,129,0}:
                std::array<int,3>{48,160,255},false}}},
        SteamStartupTouch{12,0x40000,track,{},0},{3,20,0,0},{count,5,0x20000},
        SteamStartupTouch{25,0,track,{},4}});
}
int rate(int n,int lo,int hi,int a,int b) {
    // 原整数Rate在同端点时返回左值，单个人物不能做除零或补中心位置。
    if(n<=lo)return a;if(n>=hi)return b;
    return a+static_cast<int>(std::int64_t(b-a)*(n-lo)/(hi-lo));
}
bool measured(std::optional<int> w) {return w&&*w>=0&&*w<=4096;}
bool frame(Plan &p,const SteamTaskRosterInput &in) {
    if(!in.options.title_widths)return false;
    const bool departure=in.raw==28;
    const auto w=steam_startup_window_skin({departure?222:220,departure?170:168,0,0,
        SteamStartupTextRole::message_title,0},in.options.view_y,in.options.title_widths);
    const auto b=steam_startup_box_skin(departure?SteamStartupBox{15,139,224,202}:
        SteamStartupBox{17,74,219,184},in.options.view_y);
    if(!w||!b)return false;
    for(const auto &d:w->draws)p.operations.emplace_back(d);
    for(const auto &d:b->draws)p.operations.emplace_back(d);
    return true;
}
}
std::optional<SteamFacilityResource> steam_task_roster_resource(SteamTaskRosterAsset a) {
    switch(a) {
    // EXE resources.assets:1148/common两图片与固定APK已独立核字节／像素相同。
    case SteamTaskRosterAsset::departure_background:return SteamFacilityResource{"common",39,-1,"original/common/mp_back.png",nullptr};
    case SteamTaskRosterAsset::secretary_face:return SteamFacilityResource{"common",126,-1,"original/common/chara_hishoko01.png",nullptr};
    case SteamTaskRosterAsset::shadow:return SteamFacilityResource{"common",3,25,"original/common/shadow00.png","original/common/shadow00.seb"};
    }
    return {};
}
std::optional<StartupTitleActorSkin> steam_task_departure_actor_skin(const SteamTaskDepartureActor &r) {
    if(r.action==0)return startup_title_actor_skin(r.profession,r.sex,r.weapon,r.step,2,false);
    if(r.action!=4||r.step!=0)return {};
    auto out=startup_title_actor_skin(r.profession,r.sex,r.weapon,0,2,false);if(!out)return {};
    // c/b.bd[4]+direction2、be[4][0]；Steam cctor294B8C写HUMAN_ANIME_SEB，
    // 294D79将新2槽[0,1]装入SEB_F[4]；不拿walk身体帧绘举手出发。
    out->body.sprite=22;out->body.frame=0;
    if(out->weapon) {
        const auto &rules=startup_world_rules();
        const auto weapon=std::find_if(rules.equipment.begin(),rules.equipment.end(),
            [&](const auto &v){return v.shop.kind==1&&v.shop.id==r.weapon;});
        if(weapon==rules.equipment.end())return {};
        // a/p.A与Steam WeaponData.cctor22F650..230263的32对完全相同；
        // Character2.cctor295164/29539C/2955F2/295845的四新数组均保留slot0=0。
        constexpr std::array<std::array<int,2>,4> raised{{{-19,-37},{-17,-35},{-22,-37},{-24,-46}}};
        out->weapon->offset=raised[weapon->render_style];out->weapon->frame=0;
    }
    return out;
}
std::optional<Plan> steam_task_roster_skin(const SteamTaskRosterInput &in) {
    const auto &o=in.options;
    if(in.raw<25||in.raw>28||in.counter<0||in.secondary<0||o.finger_frame<0||o.finger_frame>5||
       in.humans.size()>65536||in.frozen_humans.size()>65536||in.participant_count<0||in.participant_count>9)return {};
    std::set<int> ids;
    for(const auto &h:in.humans)if(h.identity<0||h.level<0||h.level>10||h.effort<0||h.medals<0||h.fee<0||
        h.name.empty()||h.name.size()>4096||!ids.insert(h.identity).second||
        std::any_of(h.combat.begin(),h.combat.end(),[](int v){return v<0;}))return {};
    const auto human=[&](int id)->const SteamTaskRosterHuman * {
        const auto it=std::find_if(in.humans.begin(),in.humans.end(),[=](const auto &h){return h.identity==id;});
        return it==in.humans.end()?nullptr:&*it;
    };
    std::set<int> frozen;
    for(int id:in.frozen_humans) {
        if(!frozen.insert(id).second||(id!=-1&&!human(id)))return {};
        if(id==-1&&(in.raw>=27||in.frozen_humans.back()!=-1))return {};
    }
    if(in.raw<=26&&(in.frozen_humans.empty()||in.frozen_humans.back()!=-1||in.frozen_humans.size()>10))return {};
    if(in.raw>=27&&in.frozen_humans.empty())return {};
    if(in.raw==28&&(in.frozen_humans.size()>9||in.participant_count!=static_cast<int>(in.frozen_humans.size())))return {};
    if(in.raw!=28&&(in.selection<0||in.selection>=static_cast<int>(in.frozen_humans.size())||
        in.first<0||in.first>in.selection||in.selection-in.first>=5))return {};
    if(in.tab<0||in.tab>(in.raw==27?1:0)||in.phase<0||in.phase>(in.raw==28?1:0))return {};
    Plan p;p.title=in.raw<=26?"参加者名单":in.raw==27?"参加委托 "+std::to_string(in.tab+1)+"/2":"任务开始";
    p.soft_labels=in.raw==25?std::array<int,2>{7,12}:in.raw==26?std::array<int,2>{7,2}:
        in.raw==27?std::array<int,2>{7,2}:std::array<int,2>{0,2};
    if(!frame(p,in))return {};
    if(in.raw<=27) {
        if(in.raw<=26) {
            text(p,std::to_string(in.participant_count)+"名参加",74,69,2);
            p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{140,70},0,{}});
            // Steam与APK中心字符串不同：TextLayout161,69,60,11，anchor34。
            text(p,"加油",161,69,34,true,60,11);
        } else {
            const int shift=3*((in.counter%16)/8);
            p.operations.emplace_back(SteamFacilityArrowTouch{{26-shift,55},3,16});
            p.operations.emplace_back(SteamFacilityArrowTouch{{214+shift,55},0,18});
            if(in.tab==0) {
                text(p,"姓名",30,69);text(p,"参加费用",184,69,2);
                p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{129,70},0,{}});
            } else {
                for(int i=0;i<4;++i)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::combat_icon,{53+36*i,67},i,{}});
                p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::medal,{199,67},0,std::array<int,4>{14,0,14,14}});
            }
        }
        for(int row=in.first;row<static_cast<int>(in.frozen_humans.size())&&row-in.first<5;++row) {
            const int y=97+19*(row-in.first),id=in.frozen_humans[row];
            if(in.raw==27&&row==in.selection)p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{23,y-2,191,16},{255,153,55},false}});
            p.operations.emplace_back(SteamTaskRosterTouch{SteamStartupTouch{11,0x20000|row,
                in.raw==27?std::array<int,4>{23,y-2,191,16}:std::array<int,4>{10,y,230,16},{},in.raw==27?16:0},
                in.raw==27?std::array<int,4>{29,29,0,0}:std::array<int,4>{0,-22,0,0}});
            if(id==-1) {text(p,"追加参加委托",29,y);if(row==in.selection)finger(p,y,o);continue;}
            const auto *h=human(id);if(!h)return {};
            small_box(p,29,y-2);if(!portrait(p,*h,y,in.raw<=26))return {};
            if(row==in.selection)finger(p,y,o);
            if(in.raw<=26) {text(p,h->name,48,y);level(p,h->level,155,y);number(p,SteamFacilityAsset::number05,h->effort,207,y);}
            else if(in.tab==0) {
                text(p,h->name,48,y);level(p,h->level,145,y);
                number(p,h->affordable?SteamFacilityAsset::number08:SteamFacilityAsset::number12,h->fee,212,y+1,SteamFacilityNumberKind::money);
            } else {
                for(int i=0;i<4;++i)number(p,SteamFacilityAsset::number08,h->combat[i],80+36*i,y+2);
                number(p,SteamFacilityAsset::number05,h->medals,211,y+1);
            }
        }
        scroll(p,static_cast<int>(in.frozen_humans.size()),in.first,o.scroll_first_touch);
        if(in.raw==25)text(p,"点[开始]出发！",120,200,2);
        else if(in.raw==27)text(p,"请选择冒险者",120,200,2);
        else if(in.ongoing) {if(in.task_name.empty()||in.task_name.size()>4096)return {};text(p,in.task_name+"实施中",120,200,2);}
    } else {
        if(in.selection<0||in.selection>1||in.first!=0||in.prediction<0||in.prediction>6||
           in.task_name.empty()||in.task_name.size()>4096)return {};
        p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::departure_background,{30,66},std::array<int,4>{0,0,93,82}});
        p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::departure_background,{109,66},std::array<int,4>{1,0,91,82}});
        p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::departure_background,{189,66},std::array<int,4>{1,0,20,82}});
        p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,66,180,82},{148,148,148},true}});
        p.operations.emplace_back(SteamTaskSelectionClip{true,{30,66,180,82}});
        const int count=static_cast<int>(in.frozen_humans.size()),left=rate(count,3,10,80,50),right=rate(count,3,10,160,190);
        for(int row=0;row<count;++row) {
            const auto *h=human(in.frozen_humans[row]);if(!h)return {};
            int x=rate(row,0,count-1,left,right),action=0,step=(in.secondary%16)/4;
            if(in.phase==1&&in.counter>=2) {
                if(in.counter<40) {
                    const int local=in.counter-2;
                    if(local<6||(local>=10&&local<16)||(local>=20&&local<34)){action=4;step=0;}
                } else if(in.counter>=50)x+=rate(in.counter,50,90,0,-180);
            }
            // 原Draw_shadow先于SetDispPlayerData与当前武器/身体，仍在同一clip内。
            p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::shadow,{x,134},{},0});
            const SteamTaskDepartureActor request{h->profession,h->sex,h->weapon,action,step,{x,134}};
            if(!steam_task_departure_actor_skin(request))return {};p.operations.emplace_back(request);
        }
        p.operations.emplace_back(SteamTaskSelectionClip{false,{}});
        if(in.phase==1) {
            text(p,in.task_name,120,165,2);text(p,"出发！",120,185,2);
            p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
        } else {
            constexpr const char *predictions[]{"有些困难吧","有点担心","这些人真是了不起","我觉得会好的。","值得期待！","能轻松通关！","他们能做到!"};
            if(!measured(o.prediction_width))return {};const int w=*o.prediction_width+20,left=120-w/2;
            small_box(p,left-5,161);
            p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::secretary_face,{left-5,161},std::array<int,4>{0,0,16,16}});
            text(p,predictions[in.prediction],120+w/2,164,4);
            constexpr const char *buttons[]{"立即开始","重新考虑"};
            for(int i=0;i<2;++i) {
                if(!measured(o.button_widths[i]))return {};const int bw=*o.button_widths[i]+16,bx=80+80*i-bw/2;
                if(in.selection==i)p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{bx,187,bw,16},{255,153,55},false}});
                text(p,buttons[i],80+80*i,189,2);
                if(in.selection==i)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{bx-4,196},o.finger_frame,{}});
            }
            // 原Touch3两热区固定，不随实际按钮字宽缩放，沿bk各[-20,-20,40,40]。
            for(int i=0;i<2;++i)p.operations.emplace_back(SteamStartupTouch{3,0x20000|i,std::array<int,4>{31+90*i,167,98,56},{},0});
        }
    }
    return p;
}
std::optional<Plan> steam_task_roster_skin(const StartupWorldRuntimeState &s,std::uint64_t id,
                                          const SteamTaskRosterOptions &o) {
    if(!s.rules)return {};
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &p){return p.lifecycle!=4;});
    const auto ui=s.task_page_ui.find(id);const auto counter=s.page_counters.find(id);
    if(page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||page->legacy_page<25||page->legacy_page>28||
       ui==s.task_page_ui.end()||counter==s.page_counters.end()||!valid_startup_world_runtime_task_page(s,*page)||
       !page->task_identity||!page->task_definition)return {};
    const auto task=s.tasks.find(*page->task_identity);
    if(task==s.tasks.end()||task->second.definition!=*page->task_definition)return {};
    const auto d=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[&](const auto &v){return v.factory.identity==task->second.definition;});
    if(d==s.rules->tasks.end()||s.participants.size()>9)return {};
    SteamTaskRosterInput in;in.raw=page->legacy_page;in.selection=ui->second.selection;in.first=ui->second.first;
    in.tab=ui->second.tab;in.counter=counter->second;in.participant_count=static_cast<int>(s.participants.size());in.options=o;
    // 上方列表／出发均调用GetName(false)，active26 IsDungeon为实际kind0谓词。
    in.task_name=d->name;
    if(d->factory.flags&2U) {
        const auto growth=s.scene.world.world.ai.monster_growth.find(d->factory.monster);
        if(growth==s.scene.world.world.ai.monster_growth.end()||growth->second.growth<0||
           growth->second.growth==std::numeric_limits<int>::max())return {};
        if(growth->second.growth)in.task_name+=std::to_string(growth->second.growth+1);
    } else if(d->factory.flags&4U) {
        const auto progress=s.task_progress.definitions.find(d->factory.identity);
        if(progress==s.task_progress.definitions.end()||progress->second.completed<0)return {};
        if(progress->second.completed)in.task_name+="+"+std::to_string(std::min(std::int64_t(progress->second.completed)*2,std::int64_t(10)));
    }
    if(in.raw<=26)in.frozen_humans=ui->second.member_rows;
    else if(in.raw==27) {
        const auto extra=s.task_extra_pages.find(id);if(extra==s.task_extra_pages.end())return {};in.frozen_humans=extra->second;
    } else {
        const auto phase=s.page_phases.find(id),secondary=s.page_secondary_counters.find(id),prediction=s.task_page_predictions.find(id);
        if(phase==s.page_phases.end()||secondary==s.page_secondary_counters.end()||prediction==s.task_page_predictions.end())return {};
        in.phase=phase->second;in.secondary=secondary->second;in.prediction=prediction->second;in.frozen_humans=s.participants;
    }
    if(in.raw==26) {
        if(!s.active_task||*s.active_task!=*page->task_identity)return {};
        const auto progress=s.task_progress.definitions.find(d->factory.identity);if(progress==s.task_progress.definitions.end())return {};
        in.ongoing=progress->second.kind==0;
    }
    std::set<int> needed;
    for(int id:in.frozen_humans)if(id!=-1)needed.insert(id);
    for(int definition:needed) {
        const auto h=startup_world_human_details(s,definition);const auto calendar=s.human_calendar.find(definition);
        if(!h||calendar==s.human_calendar.end())return {};
        const int fee=in.raw==27?calendar->second.continuation_cost:0;
        in.humans.push_back({definition,h->profession,h->sex,h->equipment[0].value_or(-1),h->level,h->effort,h->medals,fee,
            h->name,h->combat,s.scene.world.world.ai.accounting.funds()>=fee});
    }
    return steam_task_roster_skin(in);
}
} // namespace ark::simulation
