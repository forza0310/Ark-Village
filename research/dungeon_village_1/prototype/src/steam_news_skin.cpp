// 固定Steam MakeNews315750 / Update32630F / Draw354F9F；世界/布局冻结由调用方负责。
#include "dungeon_village_prototype/steam_news_skin.hpp"
#include <cstdint>
#include <limits>

namespace dungeon_village_prototype {
namespace {
constexpr int maximum=std::numeric_limits<int>::max();
bool signed_int(std::int64_t value) { return value>=std::numeric_limits<int>::min()&&value<=maximum; }
int common_tick(int value) { return value==maximum-1?0:value+1; }
}
std::optional<int> steam_news_rows(bool thai,std::optional<int> size) {
    if(thai)return 7;
    if(!size||*size < -1||*size>4096)return {};
    return *size<=10?7:6;
}
std::optional<int> steam_news_page_count(int height,int step,int rows) {
    if(height<0||step<=0||(rows!=6&&rows!=7))return {};
    const auto h=std::int64_t(height)+6;
    if(!signed_int(h))return {};
    const auto lines=h/step;
    const auto numerator=lines-1+rows;
    if(!signed_int(numerator))return {};
    return static_cast<int>(numerator/rows);
}
std::optional<SteamNewsUpdatePlan> steam_news_update(const SteamNewsUpdateInput &in) {
    if(in.pages<0||in.page<0||(in.pages==0?in.page!=0:in.page>=in.pages)||in.main<0||in.main==maximum||
       in.secondary<0||in.secondary==maximum)return {};
    SteamNewsUpdatePlan out;
    out.page=in.page;out.main=(common_tick(in.main)+1)%20;out.secondary=common_tick(in.secondary);
    out.checked_keys[0]=true;
    if(in.left_repeat) {
        if(in.pages==0)return {};
        const auto n=std::int64_t(out.page)-1+in.pages;
        if(!signed_int(n))return {};
        out.page=static_cast<int>(n%in.pages);
    }
    out.checked_keys[1]=true;
    if(in.right_repeat) {
        if(in.pages==0)return {};
        const auto n=std::int64_t(out.page)+1+in.pages;
        if(!signed_int(n))return {};
        out.page=static_cast<int>(n%in.pages);
    }
    out.checked_keys[2]=true;
    bool confirm=in.confirm_pulse;
    if(!confirm&&in.debug_command) {out.checked_keys[3]=true;confirm=in.debug_held;}
    if(confirm) {
        ++out.page; // 入参page<pages<=INT_MAX，故此步不溢出。
        if(out.page>=in.pages) {out.request_pop=true;return out;}
    }
    out.checked_keys[4]=true;out.request_pop=in.back_soft_label;
    return out;
}
std::optional<SteamNewsSkinPlan> steam_news_skin(const SteamNewsSkinInput &in) {
    if(in.width<=0||in.width>65536||in.height<=0||in.height>65536||in.wait<0||
       in.pages<0||in.page<0||in.page==maximum||in.main<0||in.main==maximum)return {};
    SteamNewsSkinPlan out;
    if(in.width>240||in.height>240)out.origin={(in.width-240)/2,(in.height-240)/2};
    if(!in.on_top||in.until_active_hide||in.wait>0)return out;
    const auto rows=steam_news_rows(in.thai,in.rendering_font_size);
    if(!rows||!in.configured_text_layout||!in.line_step||*in.line_step<=0||!in.name||in.name->size()>4096)return {};
    const auto line_offset=std::int64_t(in.page)* *in.line_step;
    if(!signed_int(line_offset))return {};
    const auto scroll=line_offset* *rows;
    if(!signed_int(scroll))return {};
    if(in.japanese||in.english) {
        if(!in.window_title||in.window_title->size()>4096||!in.title_widths)return {};
        for(int w:*in.title_widths)if(w<0||w>4096)return {};
        const auto frame=steam_startup_window_skin({194,186,0,0,SteamStartupTextRole::message_title,0},
                                                  in.view_y,in.title_widths);
        if(!frame)return {};
        out.title=*in.window_title;
        for(const auto &draw:frame->draws)out.draws.emplace_back(draw);
    }
    out.draws.emplace_back(SteamNewsLogoDraw{});
    out.draws.emplace_back(SteamNewsColor{});
    // 非JP确实调用这两个helper；helper仅English为真时才产生实际字号修改。
    if(!in.japanese&&in.english)out.draws.emplace_back(SteamNewsFont{SteamNewsFontAction::set_size11});
    out.draws.emplace_back(SteamNewsNameDraw{*in.name});
    if(!in.japanese&&in.english)out.draws.emplace_back(SteamNewsFont{SteamNewsFontAction::reset});
    if(in.pages>1) {
        const int shift=(in.main%20)/5;
        for(int side=0;side<2;++side) {
            const auto index=out.draws.size();
            out.draws.emplace_back(SteamManualArrowRequest{{side?193+shift:44-shift,46},side?0:3,in.arrow_selected[side]});
            out.draws.emplace_back(SteamStartupTouch{1,side?18:16,{},index,0});
        }
    }
    const auto box=steam_startup_box_skin({31,82,210,209},in.view_y);
    if(!box)return {};
    for(const auto &draw:box->draws)out.draws.emplace_back(draw);
    out.draws.emplace_back(SteamNewsBodyDraw{{41,106},static_cast<int>(scroll)});
    out.draws.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return out;
}
} // namespace dungeon_village_prototype
