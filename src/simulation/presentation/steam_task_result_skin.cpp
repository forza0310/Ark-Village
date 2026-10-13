// Steam SubForm._draw1_1：32 VA1032EFE1–1032F679，31 VA1032F67A–1032FDAD。
#include "ark/simulation/presentation/steam_task_result_skin.hpp"
#include <algorithm>

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
} // namespace ark::simulation
