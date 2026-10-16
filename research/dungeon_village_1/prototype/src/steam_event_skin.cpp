// Steam raw11 _draw VA10355E38–103565A9；只生成局部图元，不调用原代码或应用奖励。
#include "dungeon_village_prototype/steam_event_skin.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
// 构造写+144/+148与metadata默认数组独立核对；不是从截图拟合的运动。
constexpr std::array<int,27> happy_y{13,11,10,9,7,6,5,4,3,2,1,0,0,0,0,0,1,1,2,3,3,2,2,1,1,0,0};
constexpr std::array<int,25> sad_y{0,0,1,1,2,2,3,4,4,5,5,6,6,7,7,6,5,4,3,2,1,1,1,2,3};
constexpr std::array<int,3> actor_x{65,120,175};
bool clock(int value) {return value>=0&&value<std::numeric_limits<int>::max();}
void frame(SteamEventSkinPlan &plan,const SteamStartupFramePlan &source) {
    for(const auto &draw:source.draws)plan.draws.emplace_back(draw);
}
}
std::optional<SteamEventResource> steam_event_resource(StartupSkinPackage package,int image) {
    if(package==StartupSkinPackage::common) {
        if(image==72)return SteamEventResource{"original/common/arrow01.png","original/common/arrow01.seb",14,5,2};
        return {};
    }
    if(package!=StartupSkinPackage::event)return {};
    switch(image) {
    case 1:return SteamEventResource{"original/event/event_charaGlad00.png","original/event/event_charaGlad00.seb",51,58,0};
    case 2:return SteamEventResource{"original/event/event_charaGlad01.png","original/event/event_charaGlad01.seb",51,58,1};
    case 3:return SteamEventResource{"original/event/event_charaGlad02.png","original/event/event_charaGlad02.seb",51,60,2};
    case 4:return SteamEventResource{"original/event/event_charaSad00.png","original/event/event_charaSad00.seb",33,54,3};
    case 5:return SteamEventResource{"original/event/event_charaSad01.png","original/event/event_charaSad01.seb",27,51,4};
    case 6:return SteamEventResource{"original/event/event_charaSad02.png","original/event/event_charaSad02.seb",27,55,5};
    case 7:return SteamEventResource{"original/event/event_fireWork.png",nullptr,180,48,-1};
    case 9:return SteamEventResource{"original/event/event_backGlad.png",nullptr,180,80,-1};
    case 10:return SteamEventResource{"original/event/event_backSad.png",nullptr,180,80,-1};
    default:return {};
    }
}
std::optional<SteamEventSkinInput> steam_event_input(const StartupWorldRuntimeState &s,std::uint64_t id) {
    const ref::WorldScriptPage *page=nullptr;
    for(const auto &p:s.scripts.pages)if(p.id==id) {
        if(page||id==0)return {};
        page=&p;
    }
    if(!page||page->lifecycle<1||page->lifecycle>3||!valid_startup_world_event_page(s,*page))return {};
    const auto main=s.page_counters.find(id),phase=s.page_phases.find(id);
    if(main==s.page_counters.end()||phase==s.page_phases.end()||!clock(main->second)||phase->second<0||
       static_cast<std::size_t>(phase->second)>=page->message_commands.size()||
       page->message_commands.size()!=page->paragraphs.size())return {};
    const auto at=static_cast<std::size_t>(phase->second);
    return SteamEventSkinInput{main->second,page->message_commands[at],page->paragraphs[at]};
}
std::optional<SteamEventSkinPlan> steam_event_skin(const SteamEventSkinInput &in,const SteamEventSkinOptions &o) {
    if(!clock(in.main)||o.width<=0||o.height<=0||o.width>65536||o.height>65536||o.wait<0)return {};
    SteamEventSkinPlan plan;
    const bool centered=o.width>240||o.height>240;
    plan.origin={centered?(o.width-240)/2:0,centered?(o.height-240)/2:0};
    if(!o.on_top||o.until_active_hide||o.wait>0)return plan;
    if(in.text.size()>4096||o.title.size()>4096||!o.title_widths)return {};
    for(const auto value:*o.title_widths)if(value<0||value>4096)return {};
    const auto window=steam_startup_window_skin({222,170,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto box=steam_startup_box_skin({15,139,224,202},o.view_y);
    if(!window||!box)return {};
    plan.title=o.title;
    frame(plan,*window);frame(plan,*box);
    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::event,in.command==0?9:10,-1,0,0,
        {0,0,180,80},{30,67}});
    const int reveal=in.main<=5?0:in.main>=40?48:(in.main-5)*48/35;
    plan.draws.emplace_back(SteamEventClip{SteamEventClipKind::push_intersect,{0,159,240,reveal}});
    plan.draws.emplace_back(SteamEventTextLayout{in.text,{28,159,182,o.japanese?49:46},
        {92,51,31},o.japanese?2:34,6});
    plan.draws.emplace_back(SteamEventClip{SteamEventClipKind::pop,{}});
    if(in.main>=40&&in.main%40>20)
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,72,2,0,0,{},{213,204}});
    plan.draws.emplace_back(SteamEventClip{SteamEventClipKind::push_intersect,{0,67,240,79}});
    if(in.command==0||in.command==1) {
        const bool happy=in.command==0;
        const int index=std::min(in.main/2,happy?26:24);
        const int y=happy?148+happy_y[static_cast<std::size_t>(index)]:145+sad_y[static_cast<std::size_t>(index)];
        for(int actor=0;actor<3;++actor)
            plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::event,(happy?1:4)+actor,
                (happy?0:3)+actor,0,0,{},{actor_x[static_cast<std::size_t>(actor)],y}});
        if(happy)plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::event,7,-1,0,0,
            {0,in.main%20/10==0?0:24,180,24},{30,67}});
    }
    plan.draws.emplace_back(SteamEventClip{SteamEventClipKind::pop,{}});
    plan.draws.emplace_back(SteamStartupTouch{2,0,{},{},0});
    return plan;
}
} // namespace dungeon_village_prototype
