// Steam raw87：冻结授勋目录与实时人物行的纯表现；不排序、不授奖、不安装询问页。
#include "dungeon_village_prototype/steam_human_skin.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace dungeon_village_prototype {
std::optional<SteamHumanAwardCursor> steam_human_award_cursor(
    SteamHumanAwardCursor cursor,int count,bool up,bool down,bool left,bool right) {
    if(count<=0||count>65536||cursor.selection<0||cursor.selection>=count||
       cursor.first<0||cursor.first>=count||cursor.tab<0||cursor.tab>2)return {};
    // Update2 VA10319368..1031945B：上→下→五行首行→左→右；两个方向都读，不合成一步。
    if(up)cursor.selection=(cursor.selection+count-1)%count;
    if(down)cursor.selection=(cursor.selection+1)%count;
    if(cursor.first>cursor.selection)cursor.first=cursor.selection;
    if(cursor.first+5<=cursor.selection)cursor.first=cursor.selection-4;
    if(left)cursor.tab=(cursor.tab+2)%3;
    if(right)cursor.tab=(cursor.tab+1)%3;
    return cursor;
}
namespace {
using Plan=SteamHumanOverviewPlan;
void text(Plan &p,std::string value,int x,int y,int anchor=0,int font=0) {
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::equipment_name,
        std::move(value),{x,y,0,0},anchor,font,false});
}
void number(Plan &p,SteamFacilityAsset asset,int value,int x,int y,
            SteamFacilityNumberKind kind=SteamFacilityNumberKind::number) {
    p.operations.emplace_back(SteamFacilityNumber{kind,asset,value,{x,y},0,4,-1});
}
bool append_medals(Plan &p,int count,int x,int y) {
    const auto medal=steam_human_medal_skin(count,{x,y});if(!medal)return false;
    for(const auto &op:medal->draws) {
        // 已核Draw_medal254F70只返回图元；不把其它helper文字绑定为勋章列。
        const auto *image=std::get_if<SteamHumanImage>(&op);if(!image)return false;
        p.operations.emplace_back(*image);
    }
    return true;
}
}
std::optional<SteamHumanOverviewPlan> steam_human_award_skin(const SteamHumanAwardInput &in) {
    const auto &o=in.options;
    if(in.rank_order.empty()||in.rank_order.size()>65536||in.humans.size()>65536||
       in.frame<0||in.frame==std::numeric_limits<int>::max()||in.medals_left<0||
       o.view_y< -65536||o.view_y>65536||o.finger_frame<0||o.finger_frame>5||!o.title_widths||
       (!o.japanese&&(!o.footer_widths||std::any_of(o.footer_widths->begin(),o.footer_widths->end(),
           [](int w){return w<0||w>4096;}))))return {};
    const int count=static_cast<int>(in.rank_order.size());
    const auto cursor=steam_human_award_cursor(in.cursor,count,false,false,false,false);
    if(!cursor||cursor->selection!=in.cursor.selection||cursor->first!=in.cursor.first)return {};
    std::map<int,const SteamHumanAwardHuman *> humans;
    for(const auto &h:in.humans) {
        if(h.identity<0||h.contribution<0||h.effort<0||h.year_acquired<0||h.year_spent<0||
           h.residence<0||h.residence>2||h.medals<0||h.name.empty()||h.name.size()>4096||
           !startup_title_actor_skin(h.profession,h.sex,-1,0,1,false)||!humans.emplace(h.identity,&h).second)return {};
    }
    std::set<int> seen;
    for(int id:in.rank_order)if(!humans.count(id)||!seen.insert(id).second)return {};
    const auto window=steam_startup_window_skin({220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!window||!box)return {};
    Plan out;out.title="勋章授予式 "+std::to_string(cursor->tab+1)+"/3";out.soft_labels={7,10};
    for(const auto &op:window->draws)out.operations.emplace_back(op);
    for(const auto &op:box->draws)out.operations.emplace_back(op);
    const int shift=3*((in.frame%16)/8);
    out.operations.emplace_back(SteamFacilityArrowTouch{{26-shift,55},3,16});
    out.operations.emplace_back(SteamFacilityArrowTouch{{214+shift,55},0,18});
    text(out,"姓名",30,69);
    const int header_y=o.japanese?69:70,header_font=o.japanese?0:10;
    if(cursor->tab==0) {
        text(out,"贡献",149,header_y,2,header_font);text(out,"努力",o.japanese?196:201,header_y,2,header_font);
    } else if(cursor->tab==1) {
        out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::result_icon,{o.japanese?128:116,67},4,{}});
        text(out,"获得",o.japanese?146:134,header_y,1,header_font);
        text(out,"消费",o.japanese?196:201,header_y,2,header_font);
    } else {
        text(out,"居住",132,header_y,0,header_font);text(out,"勋章",196,header_y,2,header_font);
    }
    for(int row=cursor->first;row<count&&row-cursor->first<5;++row) {
        const auto &h=*humans.find(in.rank_order[row])->second;const int y=97+19*(row-cursor->first);
        if(row==cursor->selection)out.operations.emplace_back(SteamStartupFrameDraw{
            StartupSkinRect{{23,y-2,191,16},{255,153,55},false}});
        // VA1034C1C1..1034C1F0普通重载，无marker16或80的margin29。
        out.operations.emplace_back(SteamStartupTouch{11,0x20000|row,std::array<int,4>{3,y-2,231,16},{},0});
        out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{29,y-2,16,16},{196,236,169},false}});
        out.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{29,y-2,16,16},{204,204,204},true}});
        const auto actor=startup_title_actor_skin(h.profession,h.sex,-1,0,1,false);if(!actor)return {};
        out.operations.emplace_back(SteamHumanPortrait{*actor,{37,y+20},std::array<int,4>{29,y-1,15,14},false});
        if(row==cursor->selection)out.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,{21,y+8},o.finger_frame,{}});
        text(out,h.name,48,y,0,11);
        if(cursor->tab==0) {
            number(out,SteamFacilityAsset::number05,h.contribution,154,y+1);
            out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::contribution_unit,{155,y+(o.japanese?0:2)},13,{}});
            number(out,SteamFacilityAsset::number05,h.effort,212,y+1);
        } else if(cursor->tab==1) {
            number(out,SteamFacilityAsset::number11,h.year_acquired,o.japanese?154:158,y+1);
            number(out,SteamFacilityAsset::number08,h.year_spent,212,y+1,SteamFacilityNumberKind::money);
        } else {
            if(h.residence==1)out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::residence_icon,{137,y-2},0,std::array<int,4>{80,0,16,16}});
            if(!append_medals(out,h.medals,180,y))return {};
        }
    }
    const int denominator=std::max(count,5),top=85+111*cursor->first/denominator,height=555/denominator;
    const std::array<int,4> track{221,85,3,111};
    out.operations.emplace_back(SteamHumanCatalogueScroll{
        {StartupSkinRect{{220,85,5,111},{7,5,78},false},StartupSkinRect{{220,top,5,height+1},
            o.scroll_first_touch&&count>5?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255},false}},
        SteamStartupTouch{12,0x40000,track,{},0},{3,20,0,0},{count,5,0x20000},SteamStartupTouch{25,0,track,{},4}});
    const std::string quantity=std::to_string(in.medals_left)+"个";
    if(o.japanese) {
        text(out,"还有",81,200);
        out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::medal,{107,198},0,std::array<int,4>{14,0,14,14}});
        text(out,quantity,153,200,4);
    } else {
        const int half=((*o.footer_widths)[0]+(*o.footer_widths)[1]+28)/2;
        out.operations.emplace_back(SteamHumanImage{SteamHumanAsset::medal,{120-half,198},0,std::array<int,4>{14,0,14,14}});
        text(out,quantity,140-half,200);text(out,"还有",120+half,200,4);
    }
    return out;
}
} // namespace dungeon_village_prototype
