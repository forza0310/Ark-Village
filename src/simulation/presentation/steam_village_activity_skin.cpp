// 村办51—54只读表现。提交、抽签、声音和页面计时只由世界Owner处理。
#include "ark/simulation/presentation/steam_village_activity_skin.hpp"
#include "ark/simulation/village/startup_world_village_activity.hpp"
#include "ark/simulation/map/startup_world_projection.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
using Plan=SteamVillageActivityPlan;
constexpr const char *attributes[]{"体力","力","器用","丈夫","魔力","运"};
bool fits(std::int64_t n) {return n>=std::numeric_limits<int>::min()&&n<=std::numeric_limits<int>::max();}
bool width(std::optional<int> n) {return n&&*n>=0&&*n<=4096;}
void text(Plan &p,std::string s,int x,int y,int anchor=0,int size=0,
          bool layout=false,int w=0,int h=0,bool rich=false,std::array<int,3> rgb={92,51,31}) {
    p.operations.emplace_back(SteamVillageActivityText{std::move(s),{x,y,w,h},anchor,size,layout,rich,rgb});
}
void image(Plan &p,SteamFacilityAsset a,int x,int y,int f=0) {
    p.operations.emplace_back(SteamFacilityImage{a,{x,y},f,{}});
}
void rectangle(Plan &p,std::array<int,4> r,std::array<int,3> color,bool outline=false) {
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{r,color,outline}});
}
void number(Plan &p,SteamFacilityAsset a,int value,int x,int y,int anchor=4,bool plus=false) {
    p.operations.emplace_back(SteamFacilityNumber{plus?SteamFacilityNumberKind::plus_value:
        SteamFacilityNumberKind::number,a,value,{x,y},0,anchor,-1});
}
void attribute(Plan &p,int index,int x,int y) {
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::combat_icon,{x,y},0,
        std::array<int,4>{16*(index==5?6:index),16,16,16}});
}
bool window(Plan &p,int w,int h,std::array<int,4> b,const SteamVillageActivityOptions &o) {
    if(!o.title_widths)return false;
    const auto outer=steam_startup_window_skin({w,h,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto inner=steam_startup_box_skin({b[0],b[1],b[2],b[3]},o.view_y);
    if(!outer||!inner)return false;
    for(const auto &d:outer->draws)p.operations.emplace_back(d);
    for(const auto &d:inner->draws)p.operations.emplace_back(d);
    return true;
}
bool actor(Plan &p,const SteamVillageActivityHuman &h,std::array<int,2> at,
           std::array<int,4> clip,int direction) {
    const auto skin=startup_title_actor_skin(h.profession,h.sex,-1,0,direction,false);
    if(!skin)return false;
    p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,clip});
    p.operations.emplace_back(SteamHumanPortrait{*skin,at,{},false});
    p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
    return true;
}
bool definition(const SteamVillageActivityDefinition &d) {
    return d.identity>=0&&d.picture>=0&&d.picture<16&&d.icon>=0&&d.icon<7&&d.name.size()<=4096&&!d.name.empty()&&
        d.kind>=0&&d.kind<=6&&d.kind!=4&&(d.kind!=1||(d.attribute>=0&&d.attribute<6))&&d.cost>=0&&
        d.magnitude>=0&&d.detail.size()<=4096&&d.description.size()<=4096;
}
void picture(Plan &p,const SteamVillageActivityDefinition &d) {
    p.operations.emplace_back(SteamVillageActivityImage{SteamVillageActivityAsset::picture_background,{78,66},0,{}});
    p.operations.emplace_back(SteamVillageActivityImage{SteamVillageActivityAsset::activity_picture,{98,79},0,
        std::array<int,4>{40*(d.picture%4),55*(d.picture/4),40,55}});
}
// 与Steam GameUtil同float32运算顺序；演出只看本page counter。
float product(float a,float b) {volatile float n=a*b;return n;}
float sum(float a,float b) {volatile float n=a+b;return n;}
float quotient(float a,float b) {volatile float n=a/b;return n;}
int parabola(int tick) {
    const float v=quotient(12.F,9.F),a=quotient(-12.F,90.F);
    return static_cast<int>(std::max(0.F,sum(product(float(tick),v),
        product(product(product(float(tick),a),float(tick+1)),0.5F))));
}
void mini(Plan &p,int counter) {
    const int phase=counter<70?20:(counter-70)%30;
    const bool jumping=phase<20;
    const int rise=jumping?parabola(phase):0;
    for(int side=0;side<2;++side) {
        const int x=side?192:48;
        image(p,SteamFacilityAsset::mini,x,140,jumping?1:0);
        image(p,SteamFacilityAsset::mini,x,140-rise,(side?2:3)+(jumping?2:0));
    }
}
void scroll(Plan &p,int count,int first,bool touched) {
    const int divisor=std::max(count,5),top=85+111*first/divisor,h=555/divisor;
    const std::array<int,4> track{221,85,3,111};
    p.operations.emplace_back(SteamHumanCatalogueScroll{
        std::array<StartupSkinRect,2>{{{{220,85,5,111},{7,5,78},false},
            {{220,top,5,h+1},touched&&count>5?std::array<int,3>{246,129,0}:
                std::array<int,3>{48,160,255},false}}},
        SteamStartupTouch{12,0x40000,track,{},0},{3,20,0,0},{count,5,0x20000},
        SteamStartupTouch{25,0,track,{},4}});
}
bool cursor(int count,int selection,int first) {
    return count>0&&count<=65536&&selection>=0&&selection<count&&first>=0&&first<count&&
        selection>=first&&selection-first<5;
}
bool attr_message(Plan &p,const SteamVillageActivityDefinition &d,const SteamVillageActivityOptions &o,bool result) {
    if(!result&&!o.original_language) {text(p,d.detail,120,199,2);return true;}
    for(int i=0;i<(result&&o.japanese?2:3);++i)if(!width(o.attribute_widths[i]))return false;
    const int prefix=*o.attribute_widths[0],name=*o.attribute_widths[1],suffix=o.attribute_widths[2].value_or(0);
    const int total=result?(o.japanese?prefix+name+34:prefix+name+suffix+20):prefix+name+suffix+22;
    const int left=120-total/2,y=result?164:199;
    text(p,result?"冒险者们的":"冒险者的",left,y,result?1:0,result?0:11);
    attribute(p,d.attribute,left+prefix+2,y-2);
    text(p,attributes[d.attribute],left+prefix+20,y,result?0:1,result?0:11,false,0,0,false,
        result?std::array<int,3>{92,51,31}:std::array<int,3>{0,101,255});
    text(p,result?"":"上升",result?120+total/2-(o.japanese?12:30):120+total/2,y,result?0:4,result?0:11);
    if(result)text(p,"上升了",120,188,2);
    return true;
}
bool footer(Plan &p,const SteamVillageActivityDefinition &d,const SteamVillageActivityOptions &o) {
    if(d.kind==0||d.kind==2)text(p,d.kind==0?"冒险者的<co=0065FF>满足度</co>上升":
        "街道的<co=0065FF>人气</co>上升",120,199,2,11,false,0,0,true);
    else if(d.kind==1)return attr_message(p,d,o,false);
    else text(p,d.detail,120,199,2);
    return true;
}
}
std::optional<SteamHumanResource> steam_village_activity_resource(SteamVillageActivityAsset a) {
    switch(a) {
    case SteamVillageActivityAsset::season:return SteamHumanResource{95,24,"original/common/icon_season.png","original/common/icon_season.seb"};
    case SteamVillageActivityAsset::activity_icon:return SteamHumanResource{131,-1,"original/common/icon_asevent.png",nullptr};
    case SteamVillageActivityAsset::activity_picture:return SteamHumanResource{117,-1,"original/common/asEventImg.png",nullptr};
    case SteamVillageActivityAsset::picture_background:return SteamHumanResource{135,-1,"original/common/asEventImg_back.png",nullptr};
    }
    return {};
}
std::optional<SteamVillageActivityPlan> steam_village_activity_skin(const SteamVillageActivitySkinInput &in) {
    const auto &o=in.options;
    if(in.raw<51||in.raw>54||in.counter<0||o.finger_frame<0||o.finger_frame>5||o.arrow_count<0||
        in.quarter_remaining<0||in.points<0||in.season<0||in.season>3||in.display_page<0)return {};
    Plan p;
    if(in.raw==51||in.raw==54) {
        if(!window(p,220,168,{17,74,219,184},o))return {};
    } else if(!window(p,222,170,{15,139,224,202},o))return {};
    if(in.raw!=51&&(!in.activity||!definition(*in.activity)))return {};
    if(in.raw==51) {
        p.title="开展活动";
        if(in.activities.size()>65536||!cursor(static_cast<int>(in.activities.size()),in.selection,in.first))return {};
        std::set<int> ids;for(const auto &d:in.activities)if(!definition(d)||!ids.insert(d.identity).second)return {};
        p.operations.emplace_back(SteamVillageActivityImage{SteamVillageActivityAsset::season,{45,65},in.season,{}});
        constexpr const char *seasons[]{"春","夏","秋","冬"};text(p,seasons[in.season],66,69);
        if(o.japanese) {text(p,"还有",79,69);number(p,SteamFacilityAsset::number09,in.quarter_remaining,105,68,1);
            text(p,"次",118,69);text(p,"价格",162,69);}
        else {text(p,"还有",o.thai?105:114,69,o.thai?0:32,0,!o.thai,o.thai?0:35,o.thai?0:12);
            number(p,SteamFacilityAsset::number09,in.quarter_remaining,o.thai?129:100,68,1);
            text(p,"价格",158,69,32,0,true,30,12);}
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::result_icon,{190,67},4,{}});
        for(int row=in.first;row<static_cast<int>(in.activities.size())&&row-in.first<5;++row) {
            const auto &d=in.activities[row];const int y=97+19*(row-in.first);
            if(row==in.selection)rectangle(p,{23,y-2,191,16},{255,153,55});
            p.operations.emplace_back(SteamHumanEquipmentRow{SteamStartupTouch{11,0x20000|row,
                std::array<int,4>{3,y-2,231,16},{},0},{0,-20,0,0}});
            if(d.newly_unlocked)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::catalogue_new,{10,y+2},0,{}});
            p.operations.emplace_back(SteamVillageActivityImage{SteamVillageActivityAsset::activity_icon,{30,y-2},0,
                std::array<int,4>{16*d.icon,0,16,16}});
            if(row==in.selection)image(p,SteamFacilityAsset::finger,21,y+8,o.finger_frame);
            text(p,d.name,50,y);number(p,in.points>=d.cost?SteamFacilityAsset::number11:SteamFacilityAsset::number12,d.cost,210,y);
        }
        scroll(p,static_cast<int>(in.activities.size()),in.first,o.scroll_first_touch);
        if(!footer(p,in.activities[in.selection],o)||
           !actor(p,in.display_humans[0],{26,220},{18,197,15,15},1)||
           !actor(p,in.display_humans[1],{213,220},{205,197,15,15},2))return {};
        if(o.arrow_count==0)for(int y:{48,223})p.operations.emplace_back(SteamStartupTouch{2,786632,std::array<int,4>{90,y,70,30},{},2});
        else for(int side=0;side<2;++side) {
            const auto node=p.operations.size();image(p,SteamFacilityAsset::title_arrows,120,side?200:85,side);
            p.operations.emplace_back(SteamFacilityEffectTouch{SteamStartupTouch{23,side?19:17,{},node,2},
                {40,40,side?-15:45,side?60:0}});
        }
    } else if(in.raw==52) {
        p.title=in.activity->name;if(in.selection<0||in.selection>1||in.first!=0||!width(o.description_width))return {};
        picture(p,*in.activity);mini(p,0);
        const int dw=std::min(*o.description_width,190)+16;
        text(p,in.activity->description,120-dw/2,162,34,0,true,dw,16);
        for(int side=0;side<2;++side) {
            if(!width(o.button_widths[side]))return {};
            const int w=std::min(*o.button_widths[side],60)+16,left=(side?160:80)-w/2;
            if(in.selection==side)rectangle(p,{left,186,w,16},{255,153,55});
            // BTMT_MSG_MARGIN六行均[-20,-20,40,40]；独立于可视选中底。
            p.operations.emplace_back(SteamStartupTouch{3,0x20000|side,std::array<int,4>{left-20,166,w+40,56},{},0});
            const bool execute=(in.activity->flags&8)!=0;
            text(p,side?(execute?"不实施":"不开展"):(execute?"实施":"开展"),left,186,34,0,true,w,16);
            if(in.selection==side)image(p,SteamFacilityAsset::finger,left-4,195,o.finger_frame);
        }
    } else if(in.raw==53) {
        p.title=in.activity->name;p.soft_labels={0,0};if(in.selection!=0||in.first!=0)return {};
        if(in.counter<70) {
            rectangle(p,{70,165,100,14},{46,145,26},true);rectangle(p,{71,166,98,12},{30,75,22},true);
            rectangle(p,{72,167,96,10},{38,45,30});rectangle(p,{72,167,std::min(in.counter,60)*96/60,10},{76,233,68});
            text(p,in.activity->name+"实施中",120,190,2);
        } else {
            p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::push_intersect,{0,158,240,std::min(in.counter-70,40)*48/40}});
            switch(in.activity->kind) {
            case 0:text(p,"冒险者的满足度",120,164,2);text(p,"上升了！",120,188,2);break;
            case 1:if(!attr_message(p,*in.activity,o,true))return {};break;
            case 2: {
                if(!width(o.increase_suffix_width))return {};
                int figures=1;for(int n=in.activity->magnitude;n>=10;n/=10)++figures;
                const int left=120-(figures*10+2+*o.increase_suffix_width)/2;
                text(p,"街道的人气",120,164,2);number(p,SteamFacilityAsset::number09,in.activity->magnitude,left,188,1);
                text(p,"上升了!",left+figures*10+2,188,1);break;
            }
            case 3:text(p,"街道扩张完成!",120,176,2);break;
            case 5:text(p,"魔法壶的扩张",120,164,2);text(p,"结束!",120,188,2);break;
            case 6:text(p,"魔法壶导入",120,164,2);text(p,"完毕!",120,188,2);break;
            }
            p.operations.emplace_back(SteamFacilityClip{SteamFacilityClipKind::pop,{}});
            if(in.counter>=120&&in.counter%25<15)p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
        }
        picture(p,*in.activity);mini(p,in.counter);p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    } else {
        p.title="活动结束";
        if(in.activity->kind>1||in.humans.size()>65536||!cursor(static_cast<int>(in.humans.size()),in.selection,in.first))return {};
        const int y=o.japanese?69:70,size=o.japanese?0:10;
        if(in.display_page==0) {text(p,"姓名",30,y,0,size);text(p,"上升",o.japanese?139:129,y,0,size);
            if(in.activity->kind==0)text(p,"满足",o.japanese?186:194,y,0,size);
            else {attribute(p,in.activity->attribute,o.japanese?172:182,67);text(p,attributes[in.activity->attribute],o.japanese?190:200,y,0,size);}}
        std::set<int> ids;
        for(const auto &h:in.humans)if(h.identity<0||h.name.size()>4096||!ids.insert(h.identity).second||
            !fits(std::int64_t(h.current)-h.previous))return {};
        for(int row=in.first;row<static_cast<int>(in.humans.size())&&row-in.first<5;++row) {
            const auto &h=in.humans[row];const int dy=97+19*(row-in.first);
            if(row==in.selection)rectangle(p,{23,dy-3,191,18},{255,153,55});
            // Steam54沿无TouchOption重载，不能照51追加margin。
            p.operations.emplace_back(SteamStartupTouch{11,0x20000|row,std::array<int,4>{3,dy-3,231,18},{},0});
            rectangle(p,{29,dy-2,16,16},{196,236,169});rectangle(p,{29,dy-2,16,16},{204,204,204},true);
            if(!actor(p,h,{37,dy+21},{29,dy-2,15,15},1))return {};
            if(row==in.selection)image(p,SteamFacilityAsset::finger,21,dy+8,o.finger_frame);
            if(in.display_page==0)text(p,h.name,48,dy);
            number(p,SteamFacilityAsset::number05,h.current-h.previous,161,dy+1,0,true);
            number(p,SteamFacilityAsset::number05,h.current,208,dy+1);
        }
        scroll(p,static_cast<int>(in.humans.size()),in.first,o.scroll_first_touch);text(p,"辛苦了",120,200,2);
        if(!actor(p,in.display_humans[0],{42,220},{34,197,15,15},1)||
           !actor(p,in.display_humans[1],{196,220},{188,197,15,15},2))return {};
        p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    }
    return p;
}
std::optional<SteamVillageActivityPlan> steam_village_activity_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamVillageActivityOptions &o) {
    if(!s.rules)return {};
    const auto view=inspect_startup_world_village_activity_page(s,id);if(!view)return {};
    SteamVillageActivitySkinInput in;in.raw=view->raw;in.selection=view->selection;in.first=view->first_visible;
    in.counter=view->counter;in.options=o;in.quarter_remaining=s.quarter_counter;in.points=s.village_points;
    const int month=s.scene.calendar.month;if(month<0||month>=12)return {};
    in.season=month>=2&&month<=4?0:month>=5&&month<=7?1:month>=8&&month<=10?2:3;
    auto def=[&](int key)->std::optional<SteamVillageActivityDefinition> {
        const auto d=std::find_if(s.rules->activities.begin(),s.rules->activities.end(),[=](const auto &a){return a.identity==key;});
        const auto status=s.scripts.activities.find(key);
        const auto flags=s.activity_flags.find(key);
        if(d==s.rules->activities.end()||status==s.scripts.activities.end()||flags==s.activity_flags.end())return {};
        return SteamVillageActivityDefinition{key,d->parameters[0],d->parameters[1],d->parameters[2],d->parameters[3],
            d->parameters[4],d->parameters[5],d->name,d->detail,d->description,flags->second,status->second.pending_notice};
    };
    auto human=[&](int key,bool result)->std::optional<SteamVillageActivityHuman> {
        const auto profile=startup_world_human_profile(s,key);
        const auto growth=s.scene.world.world.ai.growth.find(key);
        if(!profile||growth==s.scene.world.world.ai.growth.end())return {};
        SteamVillageActivityHuman h{key,growth->second.definition.current_profession,profile->sex,0,0,profile->name};
        if(result) {const auto old=s.human_activity_previous.find(key);
            const auto shop=s.shop_humans.find(key);
            if(old==s.human_activity_previous.end()||shop==s.shop_humans.end()||!in.activity)return {};
            h.previous=old->second;h.current=in.activity->kind==0?shop->second.satisfaction:
                growth->second.derived.attributes[in.activity->attribute];}
        return h;
    };
    if(view->activity) {in.activity=def(*view->activity);if(!in.activity)return {};}
    if(in.raw==51)for(int key:view->entries) {const auto d=def(key);if(!d)return {};in.activities.push_back(*d);}
    if(in.raw==54)for(int key:view->entries) {const auto h=human(key,true);if(!h)return {};in.humans.push_back(*h);}
    if(in.raw==51||in.raw==54)for(int i=0;i<2;++i) {const auto h=human(view->display_humans[i],false);if(!h)return {};in.display_humans[i]=*h;}
    return steam_village_activity_skin(in);
}
} // namespace ark::simulation
