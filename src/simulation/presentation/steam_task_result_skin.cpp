// Steam SubForm._draw1_1：32 VA1032EFE1–1032F679，31 VA1032F67A–1032FDAD。
#include "ark/simulation/presentation/steam_task_result_skin.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
using Plan=SteamTaskResultPlan;
void text(Plan &p,std::string value,int x,int y,int anchor=0,int size=0,
          bool layout=false,int w=0,int h=0) {
    p.operations.emplace_back(SteamTaskSelectionText{std::move(value),{x,y,w,h},anchor,size,
        layout,{92,51,31}});
}
void number(Plan &p,SteamFacilityAsset a,int value,int x,int y,SteamFacilityNumberKind kind) {
    p.operations.emplace_back(SteamFacilityNumber{kind,a,value,{x,y},0,4,-1});
}
bool word(const std::string &s) {return !s.empty()&&s.size()<=4096;}
bool portrait(Plan &p,const SteamTaskResultHuman &h,int direction,
              std::array<int,2> at,std::array<int,4> clip) {
    if(h.identity<0)return false;
    const auto actor=startup_title_actor_skin(h.profession,h.sex,-1,0,direction,false);
    if(!actor)return false;
    p.operations.emplace_back(SteamHumanPortrait{*actor,at,clip,false});return true;
}
bool frame(Plan &p,const SteamTaskResultOptions &o) {
    if(!o.title_widths)return false;
    const auto w=steam_startup_window_skin({220,168,0,0,SteamStartupTextRole::message_title,0},
        o.view_y,o.title_widths);
    const auto b=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!w||!b)return false;
    for(const auto &d:w->draws)p.operations.emplace_back(d);
    for(const auto &d:b->draws)p.operations.emplace_back(d);return true;
}
void scroll(Plan &p,int count,int first,bool touched) {
    // 两页实参height110；DrawVerticalScroll2仅max+1与width-1，不把height改111。
    const int denominator=std::max(count,5),top=85+110*first/denominator,h=550/denominator;
    const std::array<int,4> track{221,85,3,110};
    p.operations.emplace_back(SteamHumanCatalogueScroll{
        std::array<StartupSkinRect,2>{{{{220,85,5,110},{7,5,78},false},
            {{220,top,5,h+1},touched&&count>5?std::array<int,3>{246,129,0}:
                std::array<int,3>{48,160,255},false}}},
        SteamStartupTouch{12,0x40000,track,{},0},{3,20,0,0},{count,5,0x20000},
        SteamStartupTouch{25,0,track,{},4}});
}
bool bottom(Plan &p,const SteamTaskResultInput &in) {
    const std::string label=in.raw==31?in.task_name+"完成!":"获得了宝物!";
    if(in.raw==31&&!in.options.japanese) {text(p,label,120,200,2,10);return true;}
    const auto measured=in.options.bottom_width;
    if(!measured||*measured<0||*measured>4096)return false;
    const int w=std::min(*measured,200)+16;
    // Draw_btmMsg(dy197,select=false,cursor=false,maxWidth200)，无底部按钮热区。
    text(p,label,120-w/2,197,34,0,true,w,16);return true;
}
bool icon(Plan *p,const SteamTaskResultReward &r,int y) {
    if(r.kind<0||r.kind>3||r.definition<0||r.value<0||!word(r.name)||r.icon.size()!=2)return false;
    const bool item=r.kind==0;
    const int expected=item?9:r.kind==1?12:r.kind==2?20:21;
    for(int i=0;i<2;++i) {
        const auto &d=r.icon[i];
        const int iw=i==0?144:item?240:180,ih=i==0?18:item?96:r.kind==1?72:r.kind==2?90:54;
        const int side=i==0?18:item?16:18;
        if(d.resource!=StartupVisualResource::common||d.sprite!=-1||d.frame!=0||d.layer!=0||d.record_index||
           d.image!=(i==0?24:expected)||d.offset!=(i==0&&item?std::array<int,2>{-1,-1}:std::array<int,2>{0,0})||
           d.crop[0]<0||d.crop[1]<0||d.crop[2]!=side||d.crop[3]!=side||
           std::int64_t(d.crop[0])+side>iw||std::int64_t(d.crop[1])+side>ih||
           d.crop[0]%side!=0||d.crop[1]%side!=0)return false;
        if(i==0&&!item&&d.crop[0]!=54)return false;
        const auto a=i==0?SteamHumanAsset::equipment_background:item?SteamHumanAsset::item_icon:
            r.kind==1?SteamHumanAsset::weapon_icon:r.kind==2?SteamHumanAsset::armour_icon:SteamHumanAsset::accessory_icon;
        if(p)p->operations.emplace_back(SteamHumanImage{a,{30+d.offset[0],y-3+d.offset[1]},0,d.crop});
    }
    return true;
}
}
std::optional<SteamTaskResultPlan> steam_task_result_skin(const SteamTaskResultInput &in) {
    const auto &o=in.options;
    if((in.raw!=31&&in.raw!=32)||!word(in.task_name)||in.crew.size()>65536||in.rewards.size()>65536||
       o.finger_frame<0||o.finger_frame>5||in.first<0)return {};
    const int count=static_cast<int>(in.raw==31?in.crew.size():in.rewards.size());
    if(in.raw==31) {
        if(!in.rewards.empty()||in.selection<0||
           (count==0?(in.selection!=0||in.first!=0):
            (in.selection>=count||in.first>in.selection||in.selection-in.first>=5)))return {};
        // 重复X是原合法输入；只核各行数据，不判重、不排序、不重新分配H。
        for(const auto &r:in.crew)if(r.kills<0||r.downs<0||r.human.identity<0||!word(r.human.name)||
            !startup_title_actor_skin(r.human.profession,r.human.sex,-1,0,1,false))return {};
    } else {
        if(!in.crew.empty()||in.selection!=0||in.difficulty<0||in.first>std::max(count-5,0))return {};
        for(const auto &r:in.rewards)if(!icon(nullptr,r,0))return {};
    }
    Plan p;p.title="成果";if(!frame(p,o))return {};
    if(in.raw==31) {
        text(p,"姓名",30,69);text(p,"打倒数",127,69);
        // Steam Graphics.DrawText(x185,y75,width40,anchor32)，不是APK点文字y69。
        // height0保留宽度限定重载，不能借16高TextLayout再垂直居中。
        text(p,"下降",185,75,32,0,true,40,0);
        for(int row=in.first;row<count&&row-in.first<5;++row) {
            const auto &r=in.crew[row];const int y=97+19*(row-in.first);
            p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{29,y-2,16,16},{196,236,169},false}});
            p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{29,y-2,16,16},{204,204,204},true}});
            if(!portrait(p,r.human,1,{37,y+21},{29,y-2,15,15}))return {};
            if(row==in.selection)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{21,y+8},o.finger_frame,{}});
            text(p,r.human.name,48,y);
            // Draw_count3 RVA252EA0：仅日文扣10并补menuRT01 frame1，非日文仅Comma。
            number(p,SteamFacilityAsset::number05,r.kills,o.japanese?152:162,y+1,SteamFacilityNumberKind::comma_number);
            if(o.japanese)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::inventory_unit,{152,y+1},1,{}});
            // Draw_count2 RVA252E10：所有语言先dx-9，再number05的frame19；不是count的frame2。
            number(p,SteamFacilityAsset::number05,r.downs,201,y+1,SteamFacilityNumberKind::comma_number);
            p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::number05,{201,y+1},19,{}});
        }
    } else {
        text(p,in.task_name,34,69);
        p.operations.emplace_back(SteamTaskSelectionImage{SteamTaskSelectionAsset::treasure,{186,81},2,{}});
        number(p,SteamFacilityAsset::number03,in.difficulty,208,69,SteamFacilityNumberKind::number);
        for(int row=in.first;row<count&&row-in.first<5;++row) {
            const auto &r=in.rewards[row];const int y=97+19*(row-in.first);
            if(!icon(&p,r,y))return {};
            text(p,r.name,50,y);
            number(p,SteamFacilityAsset::number08,r.value,210,y+1,SteamFacilityNumberKind::money);
        }
    }
    scroll(p,count,in.first,o.scroll_first_touch);if(!bottom(p,in))return {};
    // 原f/g独立于X与滚动，31/32两套锚点不能合并成同一对坐标。
    const int left=in.raw==31?21:42,right=in.raw==31?218:196;
    if(!portrait(p,in.footer[0],1,{left,220},{left-8,197,15,15})||
       !portrait(p,in.footer[1],2,{right,220},{right-8,197,15,15}))return {};
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return p;
}

namespace {
int victory_rate(int n,int lo,int hi,int left,int right) {
    if(n<=lo)return left;if(n>=hi)return right;
    return left+static_cast<int>(std::int64_t(right-left)*(n-lo)/(hi-lo));
}
int figures(int value) {
    // Steam AppData.GetFig(long) RVA2578C0；零也占一位，不沿APK字体测宽估数字。
    int count=1;while(value>=10){value/=10;++count;}return count;
}
float victory_product(float a,float b) {volatile float v=a*b;return v;}
float victory_sum(float a,float b) {volatile float v=a+b;return v;}
float victory_quotient(float a,float b) {volatile float v=a/b;return v;}
int victory_jump(int tick) {
    // Math2.GetParabolaPos RVA2A4330：内部逐步float32，返回后截断成int；不改为double/FMA。
    const float velocity=victory_quotient(16.F,7.F);
    const float acceleration=victory_quotient(-16.F,56.F);
    const float linear=victory_product(float(tick),velocity);
    const float curve=victory_product(victory_product(victory_product(float(tick),acceleration),float(tick+1)),0.5F);
    return static_cast<int>(victory_sum(curve,linear));
}
void victory_text(SteamTaskVictoryPlan &p,std::string value,int x,int y,int anchor) {
    p.operations.emplace_back(SteamTaskSelectionText{std::move(value),{x,y,0,0},anchor,0,false,{92,51,31}});
}
}
std::optional<std::string> steam_task_display_name(const StartupWorldRuntimeState &s,
    const StartupWorldTask &d,bool after_completion) {
    // QuestData.GetName(bool) RVA21D6D0：成果用已增加进度，期限用当前任务后缀。
    if(d.factory.flags&2U) {
        const auto g=s.scene.world.world.ai.monster_growth.find(d.factory.monster);
        if(g==s.scene.world.world.ai.monster_growth.end()||g->second.growth<0)return {};
        if(g->second.growth>0) {
            const auto value=std::int64_t(g->second.growth)+(after_completion?0:1);
            if(value>std::numeric_limits<int>::max())return {};
            return d.name+std::to_string(value);
        }
    } else if(d.factory.flags&4U) {
        const auto progress=s.task_progress.definitions.find(d.factory.identity);
        if(progress==s.task_progress.definitions.end()||progress->second.completed<0)return {};
        const auto done=std::int64_t(progress->second.completed)-(after_completion?1:0);
        if(done>0)return d.name+"+"+std::to_string(std::min(done*2,std::int64_t(10)));
    }
    return d.name;
}
std::optional<SteamTaskResultInput> steam_task_result_input(const StartupWorldRuntimeState &s,
    std::uint64_t id) {
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
        [](const auto &v){return v.lifecycle!=4;});
    if(!s.rules||page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||
       (page->legacy_page!=31&&page->legacy_page!=32)||
       !valid_startup_world_runtime_task_summary_ui_page(s,*page))return {};
    const auto ui=s.task_page_ui.find(id);if(ui==s.task_page_ui.end())return {};
    const auto human=[&](int identity)->std::optional<SteamTaskResultHuman> {
        const auto value=startup_world_human_details(s,identity);if(!value)return {};
        return SteamTaskResultHuman{identity,value->profession,value->sex,value->name};
    };
    const auto left=human(page->legacy_f),right=human(page->legacy_g);if(!left||!right)return {};
    SteamTaskResultInput in;in.raw=page->legacy_page;in.selection=ui->second.selection;
    in.first=ui->second.first;in.footer={*left,*right};
    std::optional<std::uint64_t> task_id=page->task_identity;
    std::optional<int> definition=page->task_definition;
    if(in.raw==32) {
        const auto summary=s.exploration_summaries.find(id);if(summary==s.exploration_summaries.end())return {};
        task_id=summary->second.task;definition=summary->second.definition;
        for(const auto &row:summary->second.rewards) {
            SteamTaskResultReward reward;reward.kind=row[0];reward.definition=row[1];
            if(reward.kind==0) {
                const auto item=std::find_if(s.rules->items.begin(),s.rules->items.end(),
                    [&](const auto &v){return v.identity==reward.definition;});
                const auto draws=startup_world_item_icon_draws(s,reward.definition);
                if(item==s.rules->items.end()||!draws)return {};
                reward.value=item->commerce_price;reward.name=item->name;reward.icon=*draws;
            } else {
                const auto equipment=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),
                    [&](const auto &v){return v.shop.kind==reward.kind&&v.shop.id==reward.definition;});
                const auto draws=startup_world_equipment_icon_draws(s,reward.kind,reward.definition);
                if(equipment==s.rules->equipment.end()||!draws)return {};
                reward.value=equipment->shop.price;reward.name=equipment->name;reward.icon=*draws;
            }
            in.rewards.push_back(std::move(reward));
        }
    } else {
        const auto rows=s.crew_summaries.find(id);if(rows==s.crew_summaries.end())return {};
        for(int identity:rows->second) {
            const auto value=human(identity);const auto stats=s.scene.world.world.ai.battle.humans.find(identity);
            if(!value||stats==s.scene.world.world.ai.battle.humans.end())return {};
            in.crew.push_back({*value,stats->second.task_kills,stats->second.participant_downs});
        }
    }
    if(!task_id||!definition)return {};
    const auto task=s.tasks.find(*task_id);const auto source=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),
        [&](const auto &v){return v.factory.identity==*definition;});
    if(task==s.tasks.end()||task->second.definition!=*definition||source==s.rules->tasks.end())return {};
    const auto name=steam_task_display_name(s,*source,true);if(!name)return {};
    in.task_name=*name;in.difficulty=task->second.difficulty;
    return in;
}
std::optional<SteamTaskResultPlan> steam_task_result_skin(const StartupWorldRuntimeState &s,
    std::uint64_t id,const SteamTaskResultOptions &options) {
    auto in=steam_task_result_input(s,id);if(!in)return {};in->options=options;
    return steam_task_result_skin(*in);
}
std::optional<StartupTitleActorSkin> steam_task_victory_actor_skin(const SteamTaskVictoryActor &r) {
    if(r.action!=0&&r.action!=6)return {};
    auto out=startup_title_actor_skin(r.profession,r.sex,r.weapon,0,2,false);if(!out)return {};
    if(r.action==6) {
        // Character2.cctor VA10294B8C元数据12槽[0,4,4,4,20,4,8,12,16,20,24,20]，
        // VA10294DD5..10294E12安装SEB_F[6]新单槽零数组：喜悦body=8+2/frame0。
        out->body.sprite=10;out->body.frame=0;
        if(out->weapon) {
            const auto &rules=startup_world_rules();
            const auto weapon=std::find_if(rules.equipment.begin(),rules.equipment.end(),
                [&](const auto &v){return v.shop.kind==1&&v.shop.id==r.weapon;});
            if(weapon==rules.equipment.end()||weapon->render_style<0||weapon->render_style>3)return {};
            // GetWeponOffset RVA283D40的6走magic A[][dir][step]；RVA283C80将6映到walk0，
            // 四weapon slot0(frame0)和WeaponData.cctor的A偏移已分别交叉，不能沿walk偏移。
            constexpr std::array<std::array<int,2>,4> anchors{{{-19,-37},{-17,-35},{-22,-37},{-24,-46}}};
            out->weapon->offset=anchors[weapon->render_style];out->weapon->frame=0;
        }
    }
    return out;
}
std::optional<SteamTaskVictoryPlan> steam_task_victory_skin(const SteamTaskVictoryInput &in) {
    const auto &o=in.options;
    if(in.phase<0||in.phase>1||in.counter<0||in.secondary<0||
       in.counter==std::numeric_limits<int>::max()||in.secondary==std::numeric_limits<int>::max()||
       in.experience<0||in.reputation<0||!word(in.task_name)||in.humans.size()>9||
       o.view_y< -65536||o.view_y>65536||!o.title_widths||
       (in.phase==1&&(!o.reputation_suffix_width||*o.reputation_suffix_width<0||*o.reputation_suffix_width>4096)))return {};
    for(const auto &h:in.humans)if(h.identity<0||
        !steam_task_victory_actor_skin({h.profession,h.sex,h.weapon,0,{}}))return {};
    SteamTaskVictoryPlan p;p.title="胜利！"; // Steam metadata slot110F5924原字面「勝利！」。
    const auto w=steam_startup_window_skin({222,170,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto b=steam_startup_box_skin({15,139,224,202},o.view_y);
    if(!w||!b)return {};for(const auto &d:w->draws)p.operations.emplace_back(d);
    for(const auto &d:b->draws)p.operations.emplace_back(d);
    p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::departure_background,{30,66},std::array<int,4>{0,0,93,82},0});
    p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::departure_background,{109,66},std::array<int,4>{1,0,91,82},0});
    p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::departure_background,{189,66},std::array<int,4>{1,0,20,82},0});
    // Steam1032FF65 DrawRect(30,66,179,81)含右下端点，平台半开矩形各加1。
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,66,180,82},{148,148,148},true}});
    const int exp_left=120-(8*figures(in.experience)+22)/2;
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::experience_label,{exp_left,74},0,{}});
    p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,SteamFacilityAsset::number05,in.experience,{exp_left+22,73},0,1,-1});
    const int count=static_cast<int>(in.humans.size());
    const int left=victory_rate(count,3,10,80,50),right=victory_rate(count,3,10,160,190);
    const int local=in.secondary%26,action=local<10?0:6,y=134-(local<10?0:victory_jump(local-10));
    for(int i=0;i<count;++i) {
        const auto &h=in.humans[i];const int x=count==1?left:victory_rate(i,0,count-1,left,right);
        p.operations.emplace_back(SteamTaskRosterImage{SteamTaskRosterAsset::shadow,{x,134},{},0});
        p.operations.emplace_back(SteamTaskVictoryActor{h.profession,h.sex,h.weapon,action,{x,y}});
    }
    p.operations.emplace_back(SteamTaskSelectionClip{true,{0,158,240,victory_rate(in.counter,5,40,0,48)}});
    if(in.phase==0) {
        victory_text(p,in.task_name,120,164,2);victory_text(p,"完成!!",120,188,2);
    } else {
        victory_text(p,"因为完成任务",120,158,2);victory_text(p,"街道的人气",120,176,2);
        const int digits=figures(in.reputation)*10,start=120-(digits+2+*o.reputation_suffix_width)/2;
        p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::number,SteamFacilityAsset::number09,in.reputation,{start,194},0,1,-1});
        victory_text(p,"上升了!",start+digits+2,194,1);
    }
    p.operations.emplace_back(SteamTaskSelectionClip{false,{}});
    if(in.counter>=40&&in.counter%25<15)
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
    // Steam30汇合VA10330EAA..10330EE6 AddTouchHelper(2,option2)，无源显式矩形。
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return p;
}
std::optional<SteamTaskVictoryPlan> steam_task_victory_skin(const StartupWorldRuntimeState &s,
    std::uint64_t id,const SteamTaskVictoryOptions &o) {
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &v){return v.lifecycle!=4;});
    if(page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||
       !valid_startup_world_runtime_task_victory_page(s,*page))return {};
    // 探索producer原有summary绑定，不要求它补encounter专有的optional task字段。
    const auto summary=s.exploration_summaries.find(id);
    if(summary==s.exploration_summaries.end())return {};
    const auto task=s.tasks.find(summary->second.task);
    const auto definition=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),
        [&](const auto &d){return d.factory.identity==summary->second.definition;});
    if(task==s.tasks.end()||definition==s.rules->tasks.end())return {};
    const auto name=steam_task_display_name(s,*definition,true);if(!name)return {};
    const auto phase=s.page_phases.find(id),counter=s.page_counters.find(id),secondary=s.page_secondary_counters.find(id);
    if(phase==s.page_phases.end()||counter==s.page_counters.end()||secondary==s.page_secondary_counters.end())return {};
    SteamTaskVictoryInput in;in.phase=phase->second;in.counter=counter->second;in.secondary=secondary->second;
    in.experience=page->legacy_f;in.reputation=definition->factory.pending_completion_value;in.task_name=*name;in.options=o;
    if(s.participants.size()>9)return {};
    for(int human:s.participants) {
        const auto h=startup_world_human_details(s,human);
        if(!h||!h->equipment[0])return {};
        in.humans.push_back({human,h->profession,h->sex,*h->equipment[0]});
    }
    return steam_task_victory_skin(in);
}
} // namespace ark::simulation
