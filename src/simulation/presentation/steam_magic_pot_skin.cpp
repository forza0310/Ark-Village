// 41—43只投影已初始化页面。资源/裁片与Steam helper独立交叉，不执行壶业务。
#include "ark/simulation/presentation/steam_magic_pot_skin.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
using Plan=SteamMagicPotPlan;using Asset=SteamMagicPotAsset;
bool fits(std::int64_t n) {return n>=std::numeric_limits<int>::min()&&n<=std::numeric_limits<int>::max();}
bool width(std::optional<int> w) {return w&&*w>=0&&*w<=4096;}
void text(Plan &p,std::string value,int x,int y,int anchor=0,int size=0,bool layout=false,
          int w=0,int h=0,std::array<int,3> rgb={92,51,31}) {
    p.operations.emplace_back(SteamMagicPotText{std::move(value),{x,y,w,h},anchor,size,layout,false,rgb});
}
void image(Plan &p,Asset asset,int x,int y,int frame=0,std::optional<std::array<int,4>> crop={}) {
    p.operations.emplace_back(SteamMagicPotImage{asset,{x,y},frame,crop});
}
void number(Plan &p,SteamFacilityAsset asset,int value,int x,int y,int anchor=4,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number) {
    p.operations.emplace_back(SteamFacilityNumber{kind,asset,value,{x,y},0,anchor,-1});
}
void element(Plan &p,int id,int x,int y) {image(p,Asset::element,x,y,0,std::array<int,4>{16*id,0,16,16});}
void rectangle(Plan &p,std::array<int,4> r,std::array<int,3> rgb) {
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{r,rgb,false}});
}
bool window(Plan &p,const SteamMagicPotOptions &o) {
    if(!o.title_widths)return false;
    const auto frame=steam_startup_window_skin({220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!frame||!box)return false;
    for(const auto &d:frame->draws)p.operations.emplace_back(d);
    for(const auto &d:box->draws)p.operations.emplace_back(d);return true;
}
bool cursor(int count,int selection,int first) {
    return count>0&&count<=65536&&selection>=0&&selection<count&&first>=0&&first<count&&selection>=first&&selection-first<5;
}
int figures(int n) {int count=1;for(n/=10;n>0;n/=10)++count;return count;}
void slash(Plan &p,int x,int y) {p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::inventory_unit,{x,y},3,{}});}
void capacity(Plan &p,int pending,int limit,int right,int y) {
    number(p,SteamFacilityAsset::number05,limit,right,y);
    const int split=right-figures(limit)*8-10;slash(p,split,y);
    number(p,SteamFacilityAsset::number05,pending,split,y);
}
bool icon(Plan &p,const std::vector<StartupVisualDraw> &draws,int x,int y) {
    if(draws.empty()||draws.size()>2)return false;
    for(const auto &d:draws) {
        if(d.resource!=StartupVisualResource::common||d.sprite!=-1||d.frame!=0||d.layer!=0||d.record_index||
           !fits(std::int64_t(x)+d.offset[0])||
           !fits(std::int64_t(y)+d.offset[1]))return false;
        // 固定common图标是直接裁片；错误裁框不能交给渲染器裁掉后假装合法。
        const int iw=d.image==9?240:d.image==24?144:d.image==91?112:180;
        const int ih=d.image==9?96:d.image==12?72:d.image==20?90:d.image==21?54:d.image==24?18:16;
        if(d.crop[0]<0||d.crop[1]<0||d.crop[2]<=0||d.crop[3]<=0||
           std::int64_t(d.crop[0])+d.crop[2]>iw||std::int64_t(d.crop[1])+d.crop[3]>ih)return false;
        const int dx=x+d.offset[0],dy=y+d.offset[1];
        if(d.image==91)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::facility_icon,{dx,dy},0,d.crop});
        else {
            const auto a=d.image==24?SteamHumanAsset::equipment_background:d.image==9?SteamHumanAsset::item_icon:
                d.image==12?SteamHumanAsset::weapon_icon:d.image==20?SteamHumanAsset::armour_icon:SteamHumanAsset::accessory_icon;
            if(d.image!=24&&d.image!=9&&d.image!=12&&d.image!=20&&d.image!=21)return false;
            p.operations.emplace_back(SteamHumanImage{a,{dx,dy},0,d.crop});
        }
    }
    return true;
}
void scroll(Plan &p,int count,int first,bool touched) {
    const int denominator=std::max(count,5),top=85+110*first/denominator,h=550/denominator;
    const std::array<int,4> track{221,85,3,110};
    p.operations.emplace_back(SteamHumanCatalogueScroll{
        std::array<StartupSkinRect,2>{{{{220,85,5,110},{7,5,78},false},
            {{220,top,5,h+1},touched&&count>5?std::array<int,3>{246,129,0}:
                std::array<int,3>{48,160,255},false}}},
        SteamStartupTouch{12,0x40000,track,{},0},{3,20,0,0},{count,5,0x20000},SteamStartupTouch{25,0,track,{},4}});
}
bool parameters(const std::array<int,13> &n) {
    if(n[11]<1||n[11]>3||n[0]<0||n[0]>999||n[1]<0||n[1]>10*n[11]||n[2]<0||n[2]>10*n[11]||n[12]<0)return false;
    for(int i=3;i<=10;++i)if(n[i]<0||(i<7&&n[i]>999))return false;
    return true;
}
}
std::optional<SteamHumanResource> steam_magic_pot_resource(Asset a) {
    switch(a) {
    case Asset::background:return SteamHumanResource{39,-1,"original/common/mp_back.png",nullptr};
    case Asset::character:return SteamHumanResource{40,47,"original/common/mp_chara.png","original/common/mp_chara.seb"};
    case Asset::pot:return SteamHumanResource{41,48,"original/common/mp_pot.png","original/common/mp_pot.seb"};
    case Asset::bar:return SteamHumanResource{42,49,"original/common/mp_bar.png","original/common/mp_bar.seb"};
    case Asset::fire:return SteamHumanResource{43,50,"original/common/mp_fire.png","original/common/mp_fire.seb"};
    case Asset::ladder:return SteamHumanResource{44,51,"original/common/mp_ladder.png","original/common/mp_ladder.seb"};
    case Asset::element:return SteamHumanResource{118,-1,"original/common/icon_magicpot.png",nullptr};
    }
    return {};
}
std::optional<SteamMagicPotPlan> steam_magic_pot_body_skin(int level,int frame,std::array<int,2> at,int mode) {
    if(level<1||level>3||frame<0||(mode!=0&&mode!=1)||!fits(std::int64_t(at[0])+93)||
       !fits(std::int64_t(at[1])+82))return {};
    // UserData.cctor的五数组分别static+134/138/13C/140/144，已独立核Steam值。
    constexpr std::array<std::array<int,2>,3> ladder{{{42,69},{43,70},{29,64}}},
        character{{{43,55},{44,46},{30,25}}},bar{{{47,40},{47,33},{35,10}}},
        pot{{{56,73},{57,77},{47,76}}},fire{{{57,81},{57,81},{46,81}}};
    Plan p;const int id=level-1,body=(frame%20)/10,flame=(frame%14)/7+(id>=2?2:0);
    if(mode==1) {
        image(p,Asset::pot,at[0]+46,at[1]+pot[id][1],id);
        image(p,Asset::fire,at[0]+46,at[1]+fire[id][1],flame);return p;
    }
    image(p,Asset::background,at[0],at[1]);
    image(p,Asset::ladder,at[0]+ladder[id][0],at[1]+ladder[id][1],id);
    image(p,Asset::character,at[0]+character[id][0],at[1]+character[id][1],body);
    image(p,Asset::pot,at[0]+pot[id][0],at[1]+pot[id][1],id);
    image(p,Asset::bar,at[0]+bar[id][0]+(body?5:0),at[1]+bar[id][1],body+(id>0?2:0));
    if(id==1) {
        p.operations.emplace_back(SteamMagicPotClip{SteamMagicPotClipKind::push_intersect,{at[0],at[1]+49,93,33}});
        image(p,Asset::pot,at[0]+pot[id][0],at[1]+pot[id][1],id);
        p.operations.emplace_back(SteamMagicPotClip{SteamMagicPotClipKind::pop,{}});
    }
    image(p,Asset::fire,at[0]+fire[id][0],at[1]+fire[id][1],flame);return p;
}
std::optional<SteamMagicPotPlan> steam_magic_pot_skin(const SteamMagicPotSkinInput &in) {
    const auto &o=in.options;const auto &n=in.parameters;
    if(in.raw<41||in.raw>43||in.counter<0||o.finger_frame<0||o.finger_frame>5||o.arrow_count<0||
       !parameters(n)||in.tab<0||in.tab>(in.raw==43?1:0))return {};
    Plan p;if(!window(p,o))return {};p.title=in.raw==43?"选择调和书 "+std::to_string(in.tab+1)+"/2":"魔法壶";
    const int limit=10*n[11];
    if(in.raw==41) {
        if(in.selection<0||in.selection>1||in.first!=0||!width(o.hint_width))return {};
        if(o.japanese) {text(p,"素材",26,70);capacity(p,n[1],limit,102,71);text(p,"知识",148,70);
            number(p,SteamFacilityAsset::number09,n[0],208,69);}
        else {text(p,"素材",23,71,0,10);number(p,SteamFacilityAsset::number05,limit,120,71);
            const int x=122-figures(limit)*8-10;slash(p,x-1,71);number(p,SteamFacilityAsset::number05,n[1],x,71);
            text(p,"知识",126,71,0,10);number(p,SteamFacilityAsset::number09,n[0],210,69);}
        const auto body=steam_magic_pot_body_skin(n[11],in.counter,{21,88},0);if(!body)return {};
        p.operations.insert(p.operations.end(),body->operations.begin(),body->operations.end());
        p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{23,158},0,{}});
        number(p,SteamFacilityAsset::number05,n[11],50,158);
        // Steam DrawBoxSmall helper，source(117,77)-(215,159)转半开框。
        const auto inner=steam_startup_box_skin({117,77,215,159},o.view_y,1);if(!inner)return {};
        for(const auto &d:inner->draws)p.operations.emplace_back(d);
        constexpr const char *names[]{"火","冰","雷","暗"};
        for(int i=0;i<4;++i) {
            const int y=97+18*i;element(p,i,o.japanese?127:124,y-2);
            text(p,names[i],o.japanese?146:143,y+(o.japanese?0:1),0,o.japanese?0:10,false,0,0,{0,100,255});
            number(p,SteamFacilityAsset::number05,n[3+i],o.japanese?205:207,y+1);
        }
        for(int side=0;side<2;++side) {
            if(!width(o.button_widths[side]))return {};
            const int w=std::min(*o.button_widths[side],60)+16,left=(side?166:74)-w/2;
            if(in.selection==side)rectangle(p,{left,174,w,16},{255,153,55});
            p.operations.emplace_back(SteamStartupTouch{3,0x20000|side,std::array<int,4>{left-20,154,w+40,56},{},0});
            text(p,side?"查看调和书":"放入道具",left,174,34,0,true,w,16);
            if(in.selection==side)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{left-4,183},o.finger_frame,{}});
        }
        const int w=std::min(*o.hint_width,200)+16;
        text(p,in.selection?"使用魔法点数生成道具":"放入道具获得魔法点数",120-w/2,197,34,0,true,w,16);
    } else if(in.raw==42) {
        if(in.items.size()>65536||!cursor(static_cast<int>(in.items.size()),in.selection,in.first))return {};
        std::set<int> ids;
        for(const auto &r:in.items) {
            if(r.identity<0||r.inventory<=0||r.inventory>999||r.name.empty()||r.name.size()>4096||!ids.insert(r.identity).second)return {};
            for(int e:r.elements)if(e<0)return {};
        }
        if(o.japanese) {
            text(p,"姓名",26,69);text(p,"素材",85,69);number(p,SteamFacilityAsset::number05,n[1],126,70);
            slash(p,129,70);number(p,SteamFacilityAsset::number05,limit,156,70);text(p,"剩余",185,69);
        } else {
            text(p,"姓名",26,70,0,10);text(p,o.english?"素材 Left":"素材",o.english?77:85,70,0,10);
            number(p,SteamFacilityAsset::number05,n[1],180,70);slash(p,181,70);number(p,SteamFacilityAsset::number05,limit,207,70);
        }
        for(int row=in.first;row<static_cast<int>(in.items.size())&&row-in.first<5;++row) {
            const auto &r=in.items[row];const int y=97+19*(row-in.first);
            if(row==in.selection)rectangle(p,{23,y-2,191,16},{255,153,55});
            p.operations.emplace_back(SteamFacilityEffectTouch{SteamStartupTouch{11,0x20000|row,std::array<int,4>{23,y-2,191,16},{},16},{29,0,0,0}});
            if(row==in.selection)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{21,y+8},o.finger_frame,{}});
            if(!icon(p,r.icon,30,y-3))return {};
            text(p,r.name,50,y-2,32,0,true,130,15);
            number(p,SteamFacilityAsset::number05,r.inventory,210,y+1,4,SteamFacilityNumberKind::count);
        }
        scroll(p,static_cast<int>(in.items.size()),in.first,o.scroll_first_touch);
        const auto &r=in.items[in.selection];
        for(int i=0;i<4;++i) {
            element(p,i,30+50*i,197);const int grade=r.elements[i]==0?0:r.elements[i]<5?1:r.elements[i]<10?2:3;
            for(int mark=0;mark<grade;++mark)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::number08,{48+50*i+8*mark,200},14,{}});
        }
        if(o.arrow_count==0)for(int y:{48,223})p.operations.emplace_back(SteamStartupTouch{2,786632,std::array<int,4>{90,y,70,30},{},0});
        else for(int side=0;side<2;++side) {
            const auto index=p.operations.size();p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::title_arrows,{120,side?200:85},side,{}});
            p.operations.emplace_back(SteamFacilityEffectTouch{SteamStartupTouch{23,side?19:17,{},index,0},{40,40,side?-15:45,side?60:0}});
        }
    } else {
        if(in.recipes.size()>65536||!cursor(static_cast<int>(in.recipes.size()),in.selection,in.first))return {};
        std::set<int> ids;
        for(const auto &r:in.recipes) {
            if(r.identity<0||r.reward_kind<0||r.reward_kind>4||r.reward_definition<0||r.inventory<0||r.inventory>999||
                r.name.size()>4096||!ids.insert(r.identity).second)return {};
            for(int c:r.cost)if(c<0)return {};
        }
        const int q=3*((in.counter%16)/8);
        p.operations.emplace_back(SteamFacilityArrowTouch{{26-q,55},3,16});
        p.operations.emplace_back(SteamFacilityArrowTouch{{214+q,55},0,18});
        text(p,"作成物",26,69,32,11,true,75,11);
        if(in.tab==0)text(p,"持有数",168,69,0,11);
        else for(int i=0;i<4;++i)element(p,i,107+28*i,67);
        for(int row=in.first;row<static_cast<int>(in.recipes.size())&&row-in.first<5;++row) {
            const auto &r=in.recipes[row];const int y=97+19*(row-in.first);
            if(row==in.selection)rectangle(p,{o.japanese?19:21,y-2,o.japanese?191:194,16},{255,153,55});
            p.operations.emplace_back(SteamFacilityEffectTouch{SteamStartupTouch{11,0x20000|row,std::array<int,4>{-1,y-2,231,16},{},0},{0,-20,0,0}});
            if(row==in.selection)p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{17,y+8},o.finger_frame,{}});
            if(!r.discovered) {text(p,"？？？？？",26,y,1,0,false,0,0,{156,155,155});continue;}
            if(r.name.empty()||!icon(p,r.icon,26,y-2))return {};
            if(in.tab==0) {
                // 原对普通道具也以当前数恰1判定已持有，不用inventory>0改颜色。
                text(p,r.name,46,y,32,0,true,110,12,r.affordable||r.inventory==1?std::array<int,3>{92,51,31}:std::array<int,3>{255,14,1});
                if(r.reward_kind==0)number(p,SteamFacilityAsset::number05,r.inventory,206,y+1,4,SteamFacilityNumberKind::multi_value);
                else if(o.japanese)text(p,r.owned?"获得":"未获得",186,y,2,0,false,0,0,r.owned?std::array<int,3>{92,51,31}:std::array<int,3>{156,155,155});
                else if(r.owned)text(p,"获得",158,y,34,10,true,50,10);
                else text(p,"未获得",182,y,2,10,false,0,0,{156,155,155});
            } else for(int i=0;i<4;++i)number(p,n[3+i]>=r.cost[i]?SteamFacilityAsset::number08:SteamFacilityAsset::number12,r.cost[i],123+28*i,y+1);
        }
        scroll(p,static_cast<int>(in.recipes.size()),in.first,o.scroll_first_touch);
        for(int i=0;i<4;++i) {element(p,i,19+50*i,197);number(p,SteamFacilityAsset::number08,n[3+i],61+50*i,201);}
    }
    return p;
}
std::optional<SteamMagicPotPlan> steam_magic_pot_skin(const StartupWorldRuntimeState &s,std::uint64_t page,const SteamMagicPotOptions &o) {
    if(!s.rules)return {};
    const auto view=inspect_startup_world_magic_pot_page(s,page);if(!view||view->raw<41||view->raw>43)return {};
    SteamMagicPotSkinInput in;in.raw=view->raw;in.selection=view->selection;in.first=view->first_visible;
    in.tab=view->phase;in.counter=view->counter;in.parameters=s.legacy_n;in.options=o;
    if(in.raw==42)for(int id:view->entries) {
        const auto d=std::find_if(s.rules->items.begin(),s.rules->items.end(),[=](const auto &r){return r.identity==id;});
        const auto held=s.items.find(id);const auto draws=startup_world_item_icon_draws(s,id);
        if(d==s.rules->items.end()||held==s.items.end()||!draws)return {};
        in.items.push_back({id,held->second.inventory,d->name,*draws,d->magic_pot_elements});
    }
    if(in.raw==43)for(int id:view->entries) {
        const auto d=std::find_if(s.rules->magic_pot_recipes.begin(),s.rules->magic_pot_recipes.end(),[=](const auto &r){return r.identity==id;});
        const auto progress=s.magic_pot_recipes.find(id);
        if(d==s.rules->magic_pot_recipes.end()||progress==s.magic_pot_recipes.end())return {};
        SteamMagicPotRecipeRow r;r.identity=id;r.reward_kind=d->reward_kind;r.reward_definition=d->reward_definition;
        r.discovered=progress->second.status==1;r.cost=d->costs;r.affordable=true;
        for(int i=0;i<4;++i)r.affordable=r.affordable&&s.legacy_n[3+i]>=r.cost[i];
        if(r.reward_kind<=3) {
            const auto state=s.catalog.find({r.reward_kind,r.reward_definition});if(state==s.catalog.end())return {};
            if(r.reward_kind==0) {
                const auto item=std::find_if(s.rules->items.begin(),s.rules->items.end(),[&](const auto &v){return v.identity==r.reward_definition;});
                const auto icons=startup_world_item_icon_draws(s,r.reward_definition);if(item==s.rules->items.end()||!icons)return {};
                r.name=item->name;r.inventory=state->second.inventory;r.icon=*icons;
            } else {
                const auto equipment=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[&](const auto &v){return v.shop.kind==r.reward_kind&&v.shop.id==r.reward_definition;});
                const auto icons=startup_world_equipment_icon_draws(s,r.reward_kind,r.reward_definition);
                if(equipment==s.rules->equipment.end()||!icons)return {};
                r.name=equipment->name;r.owned=state->second.status==1;r.inventory=r.owned?1:0;r.icon=*icons;
            }
        } else if(r.reward_kind==4) {
            const auto dfn=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &v){return v.id==r.reward_definition;});
            const auto state=s.facility_presence.find(r.reward_definition);const auto draw=startup_world_facility_icon_draw(s,r.reward_definition);
            if(dfn==s.rules->facilities.end()||state==s.facility_presence.end()||!draw)return {};
            r.name=dfn->name;r.owned=state->second==2;r.inventory=r.owned?1:0;r.icon={*draw};
        } else return {};
        in.recipes.push_back(std::move(r));
    }
    return steam_magic_pot_skin(in);
}
} // namespace ark::simulation
