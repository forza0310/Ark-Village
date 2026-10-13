// Steam44—47原序表现；投入/结算已由Owner提交，绘制禁止再扣元素、抽评语或发声。
#include "dungeon_village_prototype/steam_magic_pot_effect_skin.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
constexpr std::array<int,3> brown{92,51,31},blue{0,100,255};
bool fits(std::int64_t v) {return v>=std::numeric_limits<int>::min()&&v<=std::numeric_limits<int>::max();}
bool measured(std::optional<int> v) {return v&&*v>=0&&*v<=4096;}
void label(SteamMagicPotPlan &p,std::string s,int x,int y,int anchor=0,int size=0,
           std::array<int,3> color=brown) {
    p.operations.emplace_back(SteamMagicPotText{std::move(s),{x,y,0,0},anchor,size,false,false,color});
}
void image(SteamMagicPotPlan &p,SteamMagicPotAsset a,int x,int y,int frame=0,
           std::optional<std::array<int,4>> crop={}) {
    p.operations.emplace_back(SteamMagicPotImage{a,{x,y},frame,crop});
}
void number(SteamMagicPotPlan &p,SteamFacilityAsset a,int n,int x,int y,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number,int anchor=4) {
    p.operations.emplace_back(SteamFacilityNumber{kind,a,n,{x,y},0,anchor,-1});
}
bool window(SteamMagicPotPlan &p,int w,int h,std::array<int,4> box,const SteamMagicPotOptions &o) {
    if(!o.title_widths||o.finger_frame<0)return false;
    const auto outer=steam_startup_window_skin({w,h,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto inner=steam_startup_box_skin({box[0],box[1],box[2],box[3]},o.view_y);
    if(!outer||!inner)return false;
    for(const auto &d:outer->draws)p.operations.emplace_back(d);
    for(const auto &d:inner->draws)p.operations.emplace_back(d);
    return true;
}
bool body(SteamMagicPotPlan &p,int level,int frame,std::array<int,2> at,int mode) {
    const auto skin=steam_magic_pot_body_skin(level,frame,at,mode);if(!skin)return false;
    p.operations.insert(p.operations.end(),skin->operations.begin(),skin->operations.end());return true;
}
// Steam DrawMagicIconName RVA2CD450：元素裁片再TextLayout(anchor33)，不是APK的点文字。
void element(SteamMagicPotPlan &p,int index,int x,int y) {
    constexpr const char *names[]{"火","冰","雷","暗"};
    image(p,SteamMagicPotAsset::element,x,y,0,std::array<int,4>{index*16,0,16,16});
    p.operations.emplace_back(SteamMagicPotText{names[index],{x+18,y+1,14,14},33,0,true,false,blue});
}
bool icon(SteamMagicPotPlan &p,const std::vector<StartupVisualDraw> &draws,int x,int y,int kind) {
    if(draws.size()!=(kind==4?1U:2U))return false;
    for(const auto &d:draws) {
        if(d.resource!=StartupVisualResource::common||d.sprite!=-1||!fits(std::int64_t(x)+d.offset[0])||
           !fits(std::int64_t(y)+d.offset[1]))return false;
        const std::array<int,2> at{x+d.offset[0],y+d.offset[1]};
        if(d.image==91&&kind==4)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::facility_icon,at,0,d.crop});
        else {
            const auto asset=d.image==24?SteamHumanAsset::equipment_background:d.image==9?SteamHumanAsset::item_icon:
                d.image==12?SteamHumanAsset::weapon_icon:d.image==20?SteamHumanAsset::armour_icon:SteamHumanAsset::accessory_icon;
            if(d.image!=24&&d.image!=(kind==0?9:kind==1?12:kind==2?20:kind==3?21:-1))return false;
            p.operations.emplace_back(SteamHumanImage{asset,at,0,d.crop});
        }
    }
    return true;
}
int rate(int frame,int start,int end,int before,int after) {
    if(frame<start)return before;if(frame>end)return after;
    return before+static_cast<int>(std::int64_t(after-before)*(frame-start)/(end-start));
}
// GameUtil.RateConvert_countAnime RVA2A4620；abs(first-after)==0与end==start均回旧值。
int count_anime(int frame,int start,int end,int before,int after) {
    if(frame<start)return before;if(frame>end)return after;
    const int first=before+(after>before?1:after<before?-1:0);
    if(first==after)return before;
    const int finish=std::clamp(start+3*(after-first),start,end);
    if(finish==start)return before;
    return frame>finish?after:rate(frame,start,finish,first,after);
}
float mul(float a,float b) {volatile float v=a*b;return v;}
float add(float a,float b) {volatile float v=a+b;return v;}
float div(float a,float b) {volatile float v=a/b;return v;}
int parabola(int distance,int time,int tick) {
    const int half=time/2;
    const float velocity=div(add(float(distance),float(distance)),float(half-1));
    const float acceleration=div(mul(float(distance),-2.F),float(half*(half-1)));
    return static_cast<int>(std::max(0.F,add(mul(float(tick),velocity),mul(mul(mul(float(tick),acceleration),float(tick+1)),0.5F))));
}
bool bubble(SteamMagicPotPlan &p,const std::string &s,int x,int y,std::optional<int> w) {
    if(!measured(w))return false;
    if(*w<=60)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-*w/2,y},0,std::array<int,4>{36-*w/2,0,*w,21}});
    else {
        for(int done=0;done<*w;) {const int n=std::min(27,*w-done);
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-*w/2+done,y},0,std::array<int,4>{6,0,n,21}});done+=n;}
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-2,y},0,std::array<int,4>{33,0,5,21}});
    }
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x-*w/2-6,y},0,{}});
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::bubble,{x+*w/2,y},1,{}});
    label(p,s,x,y+3,2,0,{0,0,0});return true;
}
bool parameters(const SteamMagicPotEffectInput &in,int columns) {
    for(int i=0;i<columns;++i) {
        const int before=in.attributes[0][i],after=in.attributes[1][i],delta=in.attributes[2][i];
        if(before<0||before>999||after<0||after>999||delta<0||
           std::int64_t(after)!=std::min<std::int64_t>(999,std::int64_t(before)+delta))return false;
    }
    return true;
}
void result(SteamMagicPotPlan &p,const SteamMagicPotEffectInput &in,bool report) {
    const int frame=report?in.frame:std::max(0,in.frame-36);
    const int start=report?12:2,bounce_end=report?52:32,move_start=report?62:34,move_end=report?77:49,finish=report?83:55;
    for(int i=0;i<(report?5:4);++i) {
        int x=report?129:24+103*(i/2),y=report?98+18*i:165+23*(i%2);
        if(report&&i==4) {x=68;y=177;label(p,"知识",in.options.common.japanese?68:55,177,0,in.options.common.japanese?0:11,blue);}
        else element(p,i,report?120:x+4,report?y-4:y-2);
        int value=in.attributes[0][i],dy=-1;
        if(frame>=start&&frame<bounce_end) {
            const int age=frame-start-6*i;if(age>=0)dy=y-parabola(8,12,age);
        } else if(frame>=bounce_end)dy=y;
        if(frame>=move_end)value=in.attributes[1][i];
        else if(frame>=move_start)value=count_anime(frame,move_start,move_end,value,in.attributes[1][i]);
        number(p,SteamFacilityAsset::number08,value,report?(i==4?x+66+(in.options.common.japanese?0:9):x+49):x+62,report?y:y+1);
        if(dy!=-1&&in.attributes[2][i]!=0)
            number(p,SteamFacilityAsset::number05,in.attributes[2][i],report?(i==4?x+104+(in.options.common.japanese?0:9):x+83):x+89,
                   report?dy:dy+1,SteamFacilityNumberKind::plus_value);
    }
    if(frame>=finish&&(frame-finish)%25<15)p.operations.emplace_back(SteamHumanImage{
        SteamHumanAsset::continue_cursor,report?std::array<int,2>{208,187}:std::array<int,2>{212,204},0,{}});
}
std::optional<SteamMagicPotPlan> deposit(const SteamMagicPotEffectInput &in) {
    const auto &o=in.options.common;SteamMagicPotPlan p;p.title="魔法壶";
    if(in.frame2<0||in.comment.empty()||!parameters(in,4)||!window(p,220,168,{15,139,224,202},o))return {};
    image(p,SteamMagicPotAsset::background,30,66,0,std::array<int,4>{0,0,93,82});
    image(p,SteamMagicPotAsset::background,109,66,0,std::array<int,4>{1,0,91,82});
    image(p,SteamMagicPotAsset::background,189,66,0,std::array<int,4>{1,0,20,82});
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,66,180,82},{148,148,148},true}});
    if(!body(p,in.level,in.frame2,{74,66},1))return {};
    constexpr int positions[]{152,150,170},distances[]{34,34,48},targets[]{112,112,108};
    const int x=positions[in.level-1];int item_x=x,item_y=129,action=8;
    if(in.frame>=15&&in.frame<36) {
        const int age=in.frame-15;action=in.frame<27?10:0;
        item_x=rate(age,0,21,x,targets[in.level-1]);item_y=130-parabola(distances[in.level-1],21,age);
    } else if(in.frame>=36)action=0;
    auto actor=startup_title_actor_skin(in.profession,in.sex,-1,0,2,false);if(!actor)return {};
    if(action==8)actor->body.sprite=18;else if(action==10)actor->body.sprite=26;
    p.operations.emplace_back(SteamHumanPortrait{*actor,{x,146},{},false});
    if(in.frame<33&&!icon(p,in.item_icon,item_x-8,item_y-16,0))return {};
    if(in.level==3) {
        p.operations.emplace_back(SteamMagicPotClip{SteamMagicPotClipKind::push_intersect,{97,97,46,51}});
        if(!body(p,in.level,in.frame2,{74,66},1))return {};
        p.operations.emplace_back(SteamMagicPotClip{SteamMagicPotClipKind::pop,{}});
    }
    if(in.frame>=97&&!bubble(p,in.comment,x,97,in.options.comment_width))return {};
    if(in.frame>=35&&in.frame<47)p.operations.emplace_back(SteamFacilityEffectImage{
        SteamFacilityEffectAsset::throw_flash,{120,in.level==3?100:116},(in.frame-35)/4});
    result(p,in,false);p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return p;
}
std::optional<SteamMagicPotPlan> report(const SteamMagicPotEffectInput &in) {
    const auto &o=in.options.common;SteamMagicPotPlan p;p.title="研究报告";p.soft_labels={0,0};
    if(in.processed<0||in.processed>30||!parameters(in,5)||!window(p,220,168,{17,74,219,184},o))return {};
    if(o.japanese) {label(p,"素材",94,70,2);label(p,"消费",146,70,2);number(p,SteamFacilityAsset::number12,in.processed,120,71,SteamFacilityNumberKind::number,2);}
    else {
        if(!in.options.report_header_widths||std::any_of(in.options.report_header_widths->begin(),in.options.report_header_widths->end(),
            [](int w){return w<0||w>4096;}))return {};
        const auto &w=*in.options.report_header_widths;const int total=w[1]+5+w[2]+5+w[0]*8/6,left=120-total/2;
        label(p,"素材",left,69,1,11);label(p,"消费",left+w[3]+5,69,1,11);
        number(p,SteamFacilityAsset::number12,in.processed,120+total/2,70);
    }
    if(!body(p,in.level,in.frame,{21,88},0))return {};
    p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{23,158},0,{}});
    number(p,SteamFacilityAsset::number05,in.level,50,158);
    const auto box=steam_startup_box_skin({117,77,215,159},o.view_y,1);if(!box)return {};
    for(const auto &d:box->draws)p.operations.emplace_back(d);
    result(p,in,true);label(p,"好好煮一煮",120,199,2);
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});return p;
}
std::optional<SteamMagicPotPlan> recipe(const SteamMagicPotEffectInput &in) {
    const auto &o=in.options.common;SteamMagicPotPlan p;p.title="调和";
    if(in.reward_kind<0||in.reward_kind>4||in.reward_definition<0||in.reward_name.empty()||
       !measured(in.options.reward_name_width)||!measured(in.options.confirm_width)||
       !window(p,220,135,{17,73,219,169},o))return {};
    for(int i=0;i<4;++i)if(in.costs[i]<0||in.costs[i]>999||in.elements[i]<0||in.elements[i]>999)return {};
    bool affordable=true;for(int i=0;i<4;++i)affordable=affordable&&in.elements[i]>=in.costs[i];
    const bool yes=in.raw==46||affordable;
    const int width=*in.options.reward_name_width+34-(yes?0:14),left=120-width/2;
    if(!icon(p,in.reward_icon,left,90,in.reward_kind))return {};
    label(p,in.reward_name,left+20,93);
    if(yes)label(p,"",120+width/2-12,93);
    label(p,in.raw==46?"可以制作了!":affordable?"要制作吗":"魔法点数不足",120,113,2);
    constexpr const char *names[]{"火","冰","雷","暗"};
    for(int i=0;i<4;++i) {
        const int x=42+97*(i%2),y=139+18*(i/2);
        image(p,SteamMagicPotAsset::element,x-8,y-4,0,std::array<int,4>{16*i,0,16,16});
        label(p,names[i],x+10,y-2,0,0,blue);
        if(in.costs[i]>0)number(p,in.elements[i]>=in.costs[i]?SteamFacilityAsset::number08:SteamFacilityAsset::number12,in.costs[i],x+60,y-1);
    }
    const int button_width=std::min(*in.options.confirm_width,200)+16,left_button=120-button_width/2;
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{left_button,181,button_width,16},{255,153,55},false}});
    p.operations.emplace_back(SteamStartupTouch{4,20,std::array<int,4>{left_button-20,161,button_width+40,56},{},0});
    p.operations.emplace_back(SteamMagicPotText{in.raw==47&&affordable?"制作!":"了解",{left_button,181,button_width,16},34,0,true,false,brown});
    p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{left_button-4,190},o.finger_frame,{}});return p;
}
}
std::optional<SteamMagicPotPlan> steam_magic_pot_effect_skin(const SteamMagicPotEffectInput &in) {
    if(in.raw<44||in.raw>47||in.frame<0||in.level<1||in.level>3)return {};
    return in.raw==44?deposit(in):in.raw==45?report(in):recipe(in);
}
std::optional<SteamMagicPotPlan> steam_magic_pot_effect_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamMagicPotEffectOptions &o) {
    const auto view=inspect_startup_world_magic_pot_page(s,id);
    if(!s.rules||!view||view->raw<44||view->raw>47||!s.magic_pot_pages_initialized.count(id))return {};
    const auto source=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    if(source==s.scripts.pages.end()||source->lifecycle==0||source->lifecycle==4)return {};
    SteamMagicPotEffectInput in;in.raw=view->raw;in.frame=view->counter;in.level=s.legacy_n[11];in.processed=s.legacy_n[2];
    in.attributes=s.magic_pot_display;in.comment=s.magic_pot_comment;in.options=o;
    if(in.raw==44) {
        const auto second=s.page_secondary_counters.find(id);const auto draws=startup_world_item_icon_draws(s,view->binding);
        // 原JobData.special1按定义序首flags2，GetImgId()取男图，不从主角职业/性别猜。
        const auto job=std::find_if(s.rules->jobs.begin(),s.rules->jobs.end(),[](const auto &j){return (j.flags&2U)!=0;});
        if(second==s.page_secondary_counters.end()||!draws||job==s.rules->jobs.end())return {};
        in.frame2=second->second;in.item_icon=*draws;in.profession=static_cast<int>(job-s.rules->jobs.begin());in.sex=0;
    } else if(in.raw>=46) {
        const auto r=std::find_if(s.rules->magic_pot_recipes.begin(),s.rules->magic_pot_recipes.end(),[&](const auto &v){return v.identity==view->binding;});
        if(r==s.rules->magic_pot_recipes.end())return {};
        in.reward_kind=r->reward_kind;in.reward_definition=r->reward_definition;in.costs=r->costs;
        for(int i=0;i<4;++i)in.elements[i]=s.legacy_n[i+3];
        if(in.reward_kind==0) {
            const auto d=std::find_if(s.rules->items.begin(),s.rules->items.end(),[&](const auto &v){return v.identity==in.reward_definition;});
            const auto icons=startup_world_item_icon_draws(s,in.reward_definition);if(d==s.rules->items.end()||!icons)return {};
            in.reward_name=d->name;in.reward_icon=*icons;
        } else if(in.reward_kind>=1&&in.reward_kind<=3) {
            const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[&](const auto &v){return v.shop.kind==in.reward_kind&&v.shop.id==in.reward_definition;});
            const auto icons=startup_world_equipment_icon_draws(s,in.reward_kind,in.reward_definition);if(d==s.rules->equipment.end()||!icons)return {};
            in.reward_name=d->name;in.reward_icon=*icons;
        } else if(in.reward_kind==4) {
            const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &v){return v.id==in.reward_definition;});
            const auto icons=startup_world_facility_icon_draw(s,in.reward_definition);if(d==s.rules->facilities.end()||!icons)return {};
            in.reward_name=d->name;in.reward_icon={*icons};
        } else return {};
    }
    return steam_magic_pot_effect_skin(in);
}
} // namespace dungeon_village_prototype
