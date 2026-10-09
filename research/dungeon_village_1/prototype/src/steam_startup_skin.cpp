#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using Role = SteamStartupTextRole;
using Asset = SteamStartupAsset;
constexpr std::array<int,3> brown{{92,51,31}};
bool dimensions(int width, int height) { return width > 0 && height > 0; }
bool measurement(double width) {
    return std::isfinite(width) && width >= 0 && width <= std::numeric_limits<int>::max()-220.0;
}
void text(SteamStartupSkinPlan &p, Role role, int row, double x, double y,
          double w, double h, std::optional<int> anchor, std::optional<std::array<int,3>> rgb, int spacing=0, int size=0,
          SteamStartupTextPlacement placement=SteamStartupTextPlacement::source_point) {
    p.draws.emplace_back(SteamStartupText{role,row,{x,y,w,h},anchor,rgb,spacing,size,placement});
}
void image(SteamStartupSkinPlan &p, Asset asset, int x, int y, int frame=0, int scale=1000,
           std::optional<std::array<int,4>> crop={}) {
    p.draws.emplace_back(SteamStartupImage{asset,{x,y},crop,frame,scale});
}
}
std::optional<SteamStartupResource> steam_startup_resource(SteamStartupAsset asset) {
    switch(asset) {
    case Asset::title_menu: return SteamStartupResource{"title","title_window.png",nullptr,1,-1,false};
    case Asset::title_cursor: return SteamStartupResource{"title","title_cursor.png",nullptr,3,-1,false};
    case Asset::save_icon: return SteamStartupResource{"common","saveload.png",nullptr,177,-1,true};
    case Asset::arrows: return SteamStartupResource{"common","arrow02.png","arrow02.seb",74,3,false};
    case Asset::menu: return SteamStartupResource{"common","menu.png","menu.seb",25,0,false};
    case Asset::finger_left: return SteamStartupResource{"common","finger_r.png","finger_l.seb",70,22,false};
    case Asset::finger_right: return SteamStartupResource{"common","finger_r.png","finger_r.seb",70,21,false};
    }
    return {};
}
std::optional<SteamStartupSkinPlan> steam_title_menu_skin(int width, int height,
    int title_frame, int selection) {
    if(!dimensions(width,height) || title_frame<0 || selection<0 || selection>1) return {};
    SteamStartupSkinPlan p;
    p.origin={(width-240)/2,0};
    if(title_frame<75) return p;
    image(p,Asset::title_menu,137,height-72);
    for(int row=0;row<2;++row) {
        const int y=height-60+22*row;
        if(row==selection)
            p.draws.emplace_back(SteamStartupFill{{152.0,static_cast<double>(y),73,16},{255,150,107}});
        text(p,Role::title_menu,row,157,y,63,16,0x22,std::array<int,3>{96,87,0},0,0,SteamStartupTextPlacement::layout);
        if(row==selection) image(p,Asset::title_cursor,127,y-13);
        p.touches.push_back({0,row,std::array<int,4>{138,y-3,100,22},{},0});
    }
    return p;
}
std::optional<SteamStartupSkinPlan> steam_save_selector_skin(const SteamSaveSelectorSkinInput &in) {
    if(!dimensions(in.width,in.height) || in.frame<0 || in.title_frame<0 || in.slot<0 || in.slot>1 ||
       in.row<0 || in.row>1 || (in.row==0 && !in.present[0])) return {};
    SteamStartupSkinPlan p;
    p.origin={(in.width-240)/2,0};
    if(in.covered_by_dialog || in.title_frame<75) return p;
    const int v=(in.height-240)/2;
    p.draws.emplace_back(SteamStartupWindow{198,146,v,-12,Role::slot_title,in.slot+1});
    image(p,Asset::arrows,34-(in.frame%20)/5,v+54,3);
    if(in.title_on_top && in.focused) p.touches.push_back({1,16,{},p.draws.size()-1,0});
    image(p,Asset::arrows,203+(in.frame%20)/5,v+54,0);
    if(in.title_on_top && in.focused) p.touches.push_back({1,18,{},p.draws.size()-1,0});
    // 箭头AddTouch的最终尺寸由SEB helper注册；本计划不以图片大小猜命中框。
    text(p,Role::slot_type,in.row,120,v+69,0,0,{},{},0,0,SteamStartupTextPlacement::centered);
    for(int row=0;row<2;++row) {
        const int r=v+84+47*row;
        p.draws.emplace_back(SteamStartupBox{29,r-12,210,r+29});
        if(in.present[row]) {
            image(p,Asset::save_icon,39+(row==0?-4:0),r+6+(row==0?-2:0),0,1000,
                  row==0?std::array<int,4>{2,23,25,20}:std::array<int,4>{6,3,16,16});
            if(row==in.row)
                p.draws.emplace_back(SteamStartupFill{{in.japanese?79.0:75.0,
                    static_cast<double>(r+6),in.japanese?94.0:102.0,14},{200,200,255}});
            text(p,Role::slot_name,row,75,r+6,102,14,0x22,{},0,0,SteamStartupTextPlacement::layout);
            text(p,Role::slot_date,row,34,r+24,0,0,{},{});
            text(p,Role::slot_cash,row,201,r+24,0,0,{},{},0,0,SteamStartupTextPlacement::right);
            if(row==in.row && in.title_on_top) image(p,Asset::finger_left,181,r+12,-1);
        } else {
            image(p,Asset::save_icon,39+(row==0?-4:0),r+15+(row==0?-2:0),0,1000,
                  row==0?std::array<int,4>{2,23,25,20}:std::array<int,4>{6,3,16,16});
            text(p,Role::empty_slot,row,126,r+15,0,0,{},{},0,0,SteamStartupTextPlacement::centered);
            if(row==in.row && in.title_on_top) image(p,Asset::finger_left,177,r+21,-1);
        }
        if(in.present[row] || row==1)
            p.touches.push_back({3,0x20000|row,std::array<int,4>{0,r-5,240,51},{},0});
        else
            p.draws.emplace_back(SteamStartupFill{{29.0,static_cast<double>(r),181,41},{0,0,0},128});
    }
    text(p,Role::choose_data,0,120,v+176,0,0,{},std::array<int,3>{125,158,116},0,0,SteamStartupTextPlacement::centered);
    return p;
}
std::optional<SteamStartupSkinPlan> steam_save_menu_skin(const SteamSaveMenuSkinInput &in) {
    if(!dimensions(in.width,in.height) || in.frame<0 || in.selection<0 || in.selection>2 ||
       !std::all_of(in.measured_text_widths.begin(),in.measured_text_widths.end(),[](int value){return value>=0;})) return {};
    SteamStartupSkinPlan p;
    // 标题根页的SubForm：任一维大于240即同时加两个偏移，窄的另一维可为负。
    const bool centered=in.width>240 || in.height>240;
    p.origin={136+(centered?(in.width-240)/2:0),144+(centered?(in.height-240)/2:0)};
    if(in.covered_by_dialog) return p;
    const int m=in.japanese?84:90;
    const int dx=p.origin[0]>in.width-m?in.width-m-p.origin[0]:0;
    const int dy=p.origin[1]>in.height-89?in.height-89-p.origin[1]:0;
    // DrawMenu2本身未夹frame；保留imul32再有符号除3，不将FrameMenu更新政策当查询前提。
    const auto product=static_cast<std::uint32_t>(in.frame)*1000U;
    const auto signed_product=product<=static_cast<std::uint32_t>(std::numeric_limits<int>::max())?
        static_cast<std::int64_t>(product):static_cast<std::int64_t>(product)-4294967296LL;
    const int scale=static_cast<int>(signed_product/3);
    for(int row=0;row<3;++row) {
        const int y=dy+28*row;
        image(p,Asset::menu,dx+1,y+1,row==in.selection?2:3,scale);
        if(in.frame>=3) {
            text(p,Role::save_menu,row,dx+7,y+9,0,0,{},
                 row==in.selection?std::array<int,3>{76,58,50}:std::array<int,3>{255,242,220},
                 0,in.measured_text_widths[row]>m-10?11:0);
            if(row==in.selection && in.on_top)
                image(p,dx<120?Asset::finger_left:Asset::finger_right,
                      dx<120?dx+m+6:dx-2,y+13,-1);
        }
        p.touches.push_back({9,0x20000|row,std::array<int,4>{dx+40,y,m-40,28},{},0});
    }
    // 原全局KEYCLICK helper范围/重叠由surface处理，不能展开成任意鼠标按下即返回。
    p.touches.push_back({4,22,{},{},2});
    return p;
}
std::optional<SteamStartupSkinPlan> steam_save_confirmation_skin(int width, int height,
    int selection, const std::array<float,2> &widths) {
    if(!dimensions(width,height) || selection<0 || selection>1 ||
       !std::all_of(widths.begin(),widths.end(),measurement)) return {};
    // 原StringWidthF及后续运算为float；先验转换边界，不将double近似反写成原字宽。
    const float b=std::max(widths[0],widths[1])+2.0F;
    const float touch_width=b+210.0F;
    if(!std::isfinite(touch_width) || touch_width>=static_cast<float>(std::numeric_limits<int>::max())) return {};
    SteamStartupSkinPlan p;
    const bool centered=width>240 || height>240;
    p.origin={centered?(width-240)/2:0,centered?(height-240)/2:0};
    p.draws.emplace_back(SteamStartupWindow{210,110,0,0,Role::message_title,0});
    p.draws.emplace_back(SteamStartupBox{26,90,213,148});
    text(p,Role::message_body,0,26,115,187,32,0x22,brown,6,0,SteamStartupTextPlacement::layout);
    for(int row=0;row<2;++row) {
        const int center=84+72*row;
        if(row==selection)
            p.draws.emplace_back(SteamStartupFill{{center-b/2-5,164,b+10,17},{255,153,55}});
        text(p,Role::answer,row,center,167,0,0,2,{});
        p.touches.push_back({3,0x20000|row,
            std::array<int,4>{static_cast<int>(center-b/2-105),64,static_cast<int>(touch_width),217},{},0});
    }
    return p;
}
} // namespace dungeon_village_prototype
