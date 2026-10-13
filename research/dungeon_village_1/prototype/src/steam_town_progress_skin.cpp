// 原版Steam的进度小窗。规则值来自Owner；本模块仅展开图元，不执行领取或演出计数。
#include "dungeon_village_prototype/steam_town_progress_skin.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_reference/world_calendar_tasks.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
constexpr std::array<int,3> brown{92,51,31}, blue{0,100,255}, red{255,14,1};
bool width(std::optional<int> w) { return w&&*w>=0&&*w<=4096; }
void image(SteamTownProgressPlan &p,SteamTownAsset a,int x,int y,int f=0,
           std::optional<std::array<int,4>> crop={}) {p.operations.emplace_back(SteamTownImage{a,{x,y},f,crop});}
void text(SteamTownProgressPlan &p,std::string s,int x,int y,int anchor=0,int size=0,
          std::array<int,3> color=brown,bool rich=false) {
    p.operations.emplace_back(SteamTownText{std::move(s),{x,y,0,0},anchor,size,rich,false,color});
}
void number(SteamTownProgressPlan &p,SteamFacilityAsset a,int value,int x,int y,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number,int anchor=4) {
    p.operations.emplace_back(SteamFacilityNumber{kind,a,value,{x,y},0,anchor,-1});
}
bool window(SteamTownProgressPlan &p,int w,int h,std::array<int,4> box,const SteamTownProgressOptions &o) {
    if(!o.title_widths||o.finger_frame<0)return false;
    auto outer=steam_startup_window_skin({w,h,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    auto inner=steam_startup_box_skin({box[0],box[1],box[2],box[3]},o.view_y);
    if(!outer||!inner)return false;
    for(const auto &d:outer->draws)p.operations.emplace_back(d);
    for(const auto &d:inner->draws)p.operations.emplace_back(d);
    return true;
}
// Steam Draw_fukidashi RVA253230：中段裁片→两端SEB→黑字；>60时27像素切片并加中央尾片。
bool bubble(SteamTownProgressPlan &p,const std::string &value,int x,int y,std::optional<int> measured) {
    if(!width(measured))return false;
    const int w=*measured;
    if(w<=60)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-w/2,y},0,
        std::array<int,4>{36-w/2,0,w,21}});
    else {
        for(int done=0;done<w;) {
            const int n=std::min(27,w-done);
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-w/2+done,y},0,
                std::array<int,4>{6,0,n,21}});done+=n;
        }
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-2,y},0,
            std::array<int,4>{33,0,5,21}});
    }
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-w/2-6,y},0,{}});
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x+w/2,y},1,{}});
    text(p,value,x,y+3,2,0,{0,0,0});return true;
}
// Steam Draw_spChara RVA30D8C0的本批type2/type4分支，裁片由已核SEB展开。
void award(SteamTownProgressPlan &p,int action,int direct,int frame,int x,int y) {
    int base=0;
    if(action==1)base=4+2*((frame%8)/4);
    else if(action==3)base=2;
    else if(action==4) {const int step=(frame%16)/4;base=step==1?8:step==3?10:0;}
    image(p,SteamTownAsset::award,x,y,base+direct);
}
void president(SteamTownProgressPlan &p,int action,int direct,int frame,int x,int y) {
    const int base=action==3?8:action==1?4+2*((frame%8)/4):action==2?2*((frame%12)/6):0;
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::president,{x,y},base+direct,{}});
}
int rate(int frame,int from,int to,int start,int end) {
    if(frame<=from)return start;if(frame>=to)return end;
    return start+(end-start)*(frame-from)/(to-from);
}
float product(float a,float b) {volatile float r=a*b;return r;}
float sum(float a,float b) {volatile float r=a+b;return r;}
float quotient(float a,float b) {volatile float r=a/b;return r;}
// 原GetParabolaValue各步单精度，调用点向零取整；不是圆弧/三角函数近似。
int parabola(int distance,int time,int tick) {
    const int half=time/2;
    const float v=quotient(sum(float(distance),float(distance)),float(half-1));
    const float a=quotient(product(float(distance),-2.F),float(half*(half-1)));
    return static_cast<int>(std::max(0.F,sum(product(float(tick),v),
        product(product(product(float(tick),a),float(tick+1)),0.5F))));
}
bool participants(SteamTownProgressPlan &p,const SteamTownCelebrationInput &in,int layer) {
    p.operations.emplace_back(SteamTownClip{SteamTownClipKind::push_intersect,{30,67,180,80}});
    for(const auto &h:in.participants) {
        if(h.frozen[3]!=layer)continue;
        int x=h.frozen[0],y=h.frozen[1],direction=2,step=0,action=0;
        if(in.frame>=10&&in.frame<50) {x=rate(in.frame,10,50,x,x-150);step=(in.frame2%16)/4;}
        else if(in.frame>=50) {
            x-=150;direction=h.frozen[2];
            if(in.frame>=70&&(in.frame-70)%40>=14) {
                action=8;if(direction>=2)--x;
            }
        }
        const auto skin=startup_title_actor_skin(h.profession,h.sex,-1,step,direction,false);
        if(!skin)return false;
        auto actor=*skin;
        if(action==8)actor.body={actor.body.image,16+direction,0};
        p.operations.emplace_back(SteamHumanPortrait{actor,{x,y},{},false});
    }
    p.operations.emplace_back(SteamTownClip{SteamTownClipKind::pop,{}});return true;
}
const ref::WorldScriptPage *page(const StartupWorldRuntimeState &s,std::uint64_t id,int raw) {
    const auto top=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &p){return p.lifecycle!=4;});
    if(!s.rules||top==s.scripts.pages.rend()||top->id!=id||top->lifecycle==0||
       top->kind!=ref::WorldScriptPageKind::raw_page||top->legacy_page!=raw||
       !valid_startup_world_town_progress_page(s,*top))return nullptr;
    return &*top;
}
}
std::optional<SteamTownResource> steam_town_resource(SteamTownAsset a) {
    switch(a) {
    case SteamTownAsset::star:return SteamTownResource{"common",13,33,"original/common/icon_star00.png","original/common/icon_star00.seb"};
    case SteamTownAsset::rank_star:return SteamTownResource{"common",132,89,"original/common/rankup_star.png","original/common/rankup_star.seb"};
    case SteamTownAsset::rank_frame:return SteamTownResource{"common",133,-1,"original/common/rankup_frame.png",nullptr};
    case SteamTownAsset::award:return SteamTownResource{"common",174,72,"original/common/chara_award.png","original/common/chara_award.seb"};
    case SteamTownAsset::president_carry:return SteamTownResource{"common",175,90,"original/common/chara_presidentCurry.png","original/common/chara_presidentCurry.seb"};
    case SteamTownAsset::shadow:return SteamTownResource{"common",3,25,"original/common/shadow00.png","original/common/shadow00.seb"};
    case SteamTownAsset::celebration_background:return SteamTownResource{"event",11,-1,"original/event/event_medelCelemony_back.png",nullptr};
    case SteamTownAsset::reward_background:return SteamTownResource{"event",14,-1,"original/event/event_getItem_back.png",nullptr};
    case SteamTownAsset::mini1:return SteamTownResource{"event",15,-1,"original/event/event_BackMini01.png",nullptr};
    case SteamTownAsset::mini2:return SteamTownResource{"event",16,-1,"original/event/event_BackMini02.png",nullptr};
    case SteamTownAsset::move_facility:return SteamTownResource{"common",119,-1,"original/common/moveTenant.png",nullptr};
    case SteamTownAsset::residence_icon:return SteamTownResource{"common",91,-1,"original/common/icon_tenantInfo.png",nullptr};
    case SteamTownAsset::activity_icon:return SteamTownResource{"common",117,-1,"original/common/asEventImg.png",nullptr};
    }
    return {};
}
std::optional<int> steam_town_conditions_cursor(int raw,int selection,bool up,bool down) {
    if((raw!=48&&raw!=49)||selection<0||selection>4||(raw==49&&selection!=0))return {};
    if(raw==48) {if(up)selection=(selection+4)%5;if(down)selection=(selection+1)%5;}
    return selection;
}
std::optional<SteamTownProgressPlan> steam_town_conditions_skin(const SteamTownConditionsInput &in) {
    const auto &o=in.options;
    if(in.rank<0||in.rank>=5||!steam_town_conditions_cursor(in.raw_page,in.selection,false,false)||
       !width(o.condition_suffix_width)||!width(o.bottom_width))return {};
    for(const auto &r:in.rows)if(r.type<0||r.type>6||r.required<0||(r.type==3&&r.facility_name.empty()))return {};
    SteamTownProgressPlan p;p.title="街道升级";
    if(!window(p,220,168,{17,74,219,184},o))return {};
    text(p,in.village_name,30,69);
    // Steam Draw_star RVA255600从dx-60起，starMax5每格12；不是APK固定位置helper。
    for(int i=0;i<5;++i)image(p,SteamTownAsset::star,147+12*i,69,i<in.rank?0:1);
    const int w=*o.condition_suffix_width+27;
    image(p,SteamTownAsset::star,120-w/2,95);
    number(p,SteamFacilityAsset::number03,in.rank+1,133-w/2,95,SteamFacilityNumberKind::number,1);
    text(p,"的升级条件",120+w/2,95,4);
    constexpr const char *labels[]{"月收入","设施数","居住数","建设","任务完成数","街道人气","举办活动数"};
    for(int i=0;i<4;++i) {
        const auto &r=in.rows[i];const int y=116+19*i;
        if(in.selection==i+1)p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{22,y-2,192,16},{255,153,55},false}});
        if(in.raw_page==48)p.operations.emplace_back(SteamStartupTouch{11,0x20000|(i+1),std::array<int,4>{2,y-2,232,16},{},0});
        text(p,std::string(labels[r.type])+(r.type==3?r.facility_name:""),26,y,1,11,blue);
        if(r.type==2)image(p,SteamTownAsset::residence_icon,o.japanese?64:67,y-2,0,std::array<int,4>{80,0,16,16});
        if(in.selection==i+1)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{20,y+8},o.finger_frame,{}});
        if(r.type!=3) {
            const bool ordinary=r.type==5;
            const auto kind=ordinary?SteamFacilityNumberKind::number:SteamFacilityNumberKind::money;
            number(p,SteamFacilityAsset::number08,r.current,r.type==0?133:132,y+1,kind);
            number(p,SteamFacilityAsset::number05,r.required,r.type==0?197:196,y+1,kind);
            text(p,"/",136,y);
        }
        text(p,r.met?"○":"×",208,y,2,0,r.met?blue:red);
    }
    const bool ready=std::all_of(in.rows.begin(),in.rows.end(),[](const auto &r){return r.met;});
    const int wbottom=(in.raw_page==49?std::min(*o.bottom_width,200):*o.bottom_width)+16,left=120-wbottom/2;
    if(in.selection==0)p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{left,197,wbottom,16},{255,153,55},false}});
    if(in.raw_page==48)p.operations.emplace_back(SteamFacilityEffectTouch{SteamStartupTouch{3,0x20000,
        std::array<int,4>{left-20,177,wbottom+40,56},{},0},{0,0,ready?-20:-10,0}});
    else if(in.selection==0)p.operations.emplace_back(SteamStartupTouch{4,20,std::array<int,4>{left-20,177,wbottom+40,56},{},0});
    // Steam Draw_btmMsg_touch使用TextLayout(anchor34)，不是APK字符串点重载。
    p.operations.emplace_back(SteamTownText{in.raw_page==49?"了解":ready?"升级!":"申请",
        {left,197,wbottom,16},34,0,false,true,brown});
    if(in.selection==0)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{left-4,206},o.finger_frame,{}});
    return p;
}
std::optional<SteamTownProgressPlan> steam_town_celebration_skin(const SteamTownCelebrationInput &in) {
    const auto &o=in.options;
    if(in.rank<1||in.rank>5||in.phase<0||in.phase>2||in.frame<0||in.frame2<0||in.participants.size()>10)return {};
    std::set<int> seen;
    for(const auto &h:in.participants)if(h.definition<0||!seen.insert(h.definition).second||
        h.frozen[0]<std::numeric_limits<int>::min()+151||
        h.frozen[2]<0||h.frozen[2]>3||h.frozen[3]<0||h.frozen[3]>1||
        !startup_title_actor_skin(h.profession,h.sex,-1,0,2,false))return {};
    SteamTownProgressPlan p;p.title="街道等级上升";p.soft_labels={0,0};
    if(!window(p,222,170,{15,139,224,202},o))return {};
    image(p,SteamTownAsset::celebration_background,30,67);
    const int x=in.phase==2?rate(in.frame,0,30,108,78):108;
    const int action=in.phase==0?(in.frame<30?0:in.frame<60?3:in.frame<95?0:1):in.phase==1?1:in.frame<30?4:0;
    award(p,action,in.phase==2&&in.frame<30?1:0,in.frame,x,139);
    if(in.phase==0&&in.frame>=30&&in.frame<60&&!bubble(p,"恭喜!",x+4,96,o.bubble_widths[0]))return {};
    if(in.phase==2&&!participants(p,in,1))return {};
    if(in.phase!=2||in.frame<70) {
        president(p,in.phase==0&&in.frame>=60&&in.frame<75?3:0,1,in.frame,136,139);
        if(in.phase==0&&in.frame>=95&&in.frame<135&&!bubble(p,"太好了!",138,96,o.bubble_widths[1]))return {};
        int sx=131,sy=130;
        if(in.phase==0) {if(in.frame<30)sx=115;else if(in.frame<60){sx=119;sy=131;}else if(in.frame<75){sx=129;sy=131;}}
        image(p,SteamTownAsset::rank_star,sx,sy);
    } else {
        const int cycle=(in.frame-70)%40;
        const int y=cycle<14?130:130-parabola(10+5*in.rank,26,cycle-14);
        if(cycle>=14&&!bubble(p,"万岁",136,68,o.bubble_widths[2]))return {};
        image(p,SteamTownAsset::president_carry,136,y,cycle<14?0:1);
    }
    image(p,SteamTownAsset::rank_frame,78,157);
    constexpr int flashes[]{0,1,2,1,0};
    for(int i=0;i<in.rank-(in.phase==0?1:0);++i)
        image(p,SteamTownAsset::rank_star,94+13*i,172,
            in.phase==1&&i==in.rank-1&&in.frame<=10?flashes[(in.frame%10)/2]:0);
    if((in.phase==1&&in.frame>=10)||in.phase==2) {
        if(!o.japanese&&!width(o.promoted_suffix_width))return {};
        const int left=o.japanese?36:120-(*o.promoted_suffix_width+26)/2-5;
        image(p,SteamTownAsset::rank_star,left+5,199);
        number(p,SteamFacilityAsset::number09,in.rank,left+15,193,SteamFacilityNumberKind::number,1);
        text(p,"升级！",left+29,193);
    }
    if(in.phase==2) {
        if(!participants(p,in,0))return {};
        if(in.frame>=140&&(in.frame-140)%25<15)
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
    }
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return p;
}
std::optional<SteamTownProgressPlan> steam_town_gift_skin(const SteamTownGiftInput &in) {
    const auto &o=in.options;
    if(in.frame<0||in.value<0||(in.kind!=0&&in.kind!=1&&in.kind!=3&&in.kind!=4&&in.kind!=9&&in.kind!=10&&in.kind!=11)||
       ((in.kind==3||in.kind==4||in.kind==11)&&in.name.empty()))return {};
    SteamTownProgressPlan p;p.title="活动";p.soft_labels={0,0};
    if(!window(p,222,170,{15,139,224,202},o))return {};
    image(p,SteamTownAsset::reward_background,30,68);
    const int y=(in.kind==4||in.kind==10)?83:73;
    image(p,y==83?SteamTownAsset::mini1:SteamTownAsset::mini2,y==83?96:77,y);
    if(in.kind==0||in.kind==1) {
        if(!width(o.reward_number_width))return {};
        if(in.kind==0) {
            number(p,SteamFacilityAsset::number05,in.value,120+(*o.reward_number_width*8/6+10)/2,y+23,SteamFacilityNumberKind::money);
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::item_icon,{112,y+33},0,std::array<int,4>{160,32,16,16}});
        } else {
            const int w=*o.reward_number_width*7/6+20;
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::result_icon,{120-w/2,y+26},4,{}});
            number(p,SteamFacilityAsset::number11,in.value,120+w/2,y+30);
        }
    } else if(in.kind==3) {
        if(!steam_facility_mapchip2_draws({in.mapchip,{120,113},0}))return {};
        p.operations.emplace_back(SteamTownClip{SteamTownClipKind::push_intersect,{80,76,80,64}});
        p.operations.emplace_back(SteamFacilityMapchip2{in.mapchip,{120,113},0});
        p.operations.emplace_back(SteamTownClip{SteamTownClipKind::pop,{}});
    } else if(in.kind==4) {
        const auto actor=startup_title_actor_skin(in.value,in.sex,-1,0,2,false);if(!actor)return {};
        image(p,SteamTownAsset::shadow,120,119);
        p.operations.emplace_back(SteamHumanPortrait{*actor,{120,119},{},false});
    } else if(in.kind==9)image(p,SteamTownAsset::move_facility,89,93);
    else if(in.kind==10)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::medal,{113,100},0,std::array<int,4>{14,0,14,14}});
    else {
        if(in.activity_icon<0||in.activity_icon>=16)return {};
        image(p,SteamTownAsset::activity_icon,100,80,0,std::array<int,4>{in.activity_icon%4*40,in.activity_icon/4*55,40,55});
    }
    image(p,SteamTownAsset::shadow,175,145);award(p,0,1,in.frame,175,145);
    if(in.frame>=40&&!bubble(p,"请",175,102,o.bubble_widths[3]))return {};
    p.operations.emplace_back(SteamTownClip{SteamTownClipKind::replace,{0,152,240,rate(in.frame,5,40,0,58)}});
    if(in.kind==0) {text(p,"<co=0065FF>"+std::to_string(in.value)+"Ｇ</co>",120,166,2,0,brown,true);text(p,"获得。",120,184,2);}
    else if(in.kind==1) {text(p,"获得街道点数 <co=0065FF>"+std::to_string(in.value)+"</co> ",120,166,2,0,brown,true);text(p,"",120,184,2);}
    else if(in.kind==3||in.kind==4) {
        text(p,"",120,158,2);text(p,"「<co=0065FF>"+in.name+"</co>」",120,175,2,0,brown,true);
        text(p,in.kind==3?"可以建设了。":"可转职了。",120,192,2);
    } else if(in.kind==9) {text(p,"「<co=0065FF>配置更替</co>」",120,166,2,0,brown,true);text(p,"可实施了。",120,184,2);}
    else if(in.kind==10) {
        text(p,o.japanese?"「<co=0065FF>勋章</co>」":"<co=0065FF>勋章</co> 获得。",120,o.japanese?166:176,2,0,brown,true);
        if(o.japanese)text(p,"获得。",120,184,2);
    } else {
        text(p,"「<co=0065FF>"+in.name+"</co>」",120,166,2,0,brown,true);
        // Steam10348B3D两分支择一；不重复JADX误落流后的公共句。
        text(p,(in.activity_flags&8U)?"可实施了。":"可以开展了。",120,184,2);
    }
    p.operations.emplace_back(SteamTownClip{SteamTownClipKind::pop,{}});
    if(in.frame>=40&&in.frame%25<15)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return p;
}
std::optional<SteamTownProgressPlan> steam_town_conditions_skin(const StartupWorldRuntimeState &s,std::uint64_t id,const SteamTownProgressOptions &o) {
    const auto *p=page(s,id,48);if(!p)p=page(s,id,49);if(!p)return {};
    const auto clock=s.page_counters.find(id),selection=s.page_phases.find(id);
    if(clock==s.page_counters.end()||clock->second<0||selection==s.page_phases.end()||s.rank<0||s.rank>=5)return {};
    const auto table=ref::fixed_calendar_task_rank_terms();const auto terms=table.find(s.rank);
    if(terms==table.end()||terms->second.size()!=4)return {};
    SteamTownConditionsInput in;in.raw_page=p->legacy_page;in.rank=s.rank;in.selection=selection->second;
    in.village_name=s.scripts.village_name;in.options=o;
    for(int i=0;i<4;++i) {
        const auto &t=terms->second[i];in.rows[i]={t.type,s.rank_values[i],t.threshold,s.rank_met[i],{}};
        if(t.type==3) {
            const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &r){return r.id==t.threshold;});
            if(d==s.rules->facilities.end())return {};in.rows[i].facility_name=d->name;
        }
    }
    return steam_town_conditions_skin(in);
}
std::optional<SteamTownProgressPlan> steam_town_celebration_skin(const StartupWorldRuntimeState &s,std::uint64_t id,const SteamTownProgressOptions &o) {
    if(!page(s,id,50))return {};
    const auto frame=s.page_counters.find(id),phase=s.page_phases.find(id),frame2=s.page_secondary_counters.find(id);
    const auto list=s.rank_celebration_participants.find(id);
    if(frame==s.page_counters.end()||phase==s.page_phases.end()||frame2==s.page_secondary_counters.end()||list==s.rank_celebration_participants.end())return {};
    SteamTownCelebrationInput in;in.rank=s.rank;in.phase=phase->second;in.frame=frame->second;in.frame2=frame2->second;in.options=o;
    for(const auto &h:list->second) {
        const auto definition=std::find_if(s.rules->humans.begin(),s.rules->humans.end(),[&](const auto &r){return r.identity==h[0];});
        const auto growth=s.scene.world.world.ai.growth.find(h[0]);
        const auto profile=startup_world_human_profile(s,h[0]);
        if(definition==s.rules->humans.end()||growth==s.scene.world.world.ai.growth.end()||!profile)return {};
        in.participants.push_back({h[0],growth->second.definition.current_profession,profile->sex,{h[1],h[2],h[3],h[4]}});
    }
    return steam_town_celebration_skin(in);
}
std::optional<SteamTownProgressPlan> steam_town_gift_skin(const StartupWorldRuntimeState &s,std::uint64_t id,const SteamTownProgressOptions &o) {
    const auto *p=page(s,id,95);if(!p)return {};
    const auto frame=s.page_counters.find(id);if(frame==s.page_counters.end())return {};
    SteamTownGiftInput in;in.kind=p->legacy_r;in.value=p->legacy_s;in.sex=p->legacy_t;in.frame=frame->second;in.options=o;
    if(in.kind==3) {
        const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &r){return r.id==in.value;});
        if(d==s.rules->facilities.end())return {};in.name=d->name;in.mapchip=d->display_id;
    } else if(in.kind==4) {
        if(in.value<0||in.value>=static_cast<int>(s.rules->jobs.size()))return {};in.name=s.rules->jobs[in.value].name;
    } else if(in.kind==11) {
        const auto d=std::find_if(s.rules->activities.begin(),s.rules->activities.end(),[&](const auto &r){return r.identity==in.value;});
        if(d==s.rules->activities.end())return {};in.name=d->name;in.activity_icon=d->parameters[0];in.activity_flags=d->flags;
    }
    return steam_town_gift_skin(in);
}
} // namespace dungeon_village_prototype
