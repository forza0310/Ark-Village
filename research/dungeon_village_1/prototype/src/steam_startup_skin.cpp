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
constexpr std::array<int,3> slot_text{{30,30,30}};
bool dimensions(int width, int height) { return width > 0 && height > 0; }
bool measurement(double width) {
    return std::isfinite(width) && width >= 0 && width <= std::numeric_limits<int>::max()-220.0;
}
bool coordinate(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
std::optional<int> window_top(int height, int vertical, int view_y, int style) {
    const std::int64_t inset = height > 216 ? 0 : static_cast<std::int64_t>(view_y) + 2LL * style;
    // 安全接口拒绝源int中间值溢出，不复刻有符号回绕。
    if (!coordinate(inset) || !coordinate(inset + 240)) return {};
    const auto top = (inset + 240) / 2 - height / 2 + vertical;
    if (!coordinate(top - 2) || !coordinate(top + height + 2)) return {};
    return static_cast<int>(top);
}
void wood(SteamStartupFramePlan &plan, int width, int height, int left, int top) {
    // Steam循环包含W/40项；整40宽时最后一次为0宽，保留调用事实而不伪造像素。
    for (int n = 0; n <= width / 40; ++n)
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,28,-1,0,0,
            {0,0,std::min(40,width-40*n),height},{left+40*n,top}});
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
std::optional<SteamStartupFramePlan> steam_startup_window_skin(const SteamStartupWindow &window,
    int view_y, std::optional<std::array<int, 2>> measured_title_widths) {
    const int width = window.width, height = window.height;
    if (width < 3 || width > 240 || height < 17 || height > 240 ||
        (measured_title_widths && ((*measured_title_widths)[0] < 0 || (*measured_title_widths)[1] < 0)))
        return {};
    const auto y = window_top(height, window.vertical, view_y, window.style);
    if (!y) return {};
    const int left = 120 - width / 2, top = *y;
    SteamStartupFramePlan result;
    result.draws.emplace_back(StartupSkinRect{{left-1,top-2,width+2,height+3},{89,103,91},true});
    result.draws.emplace_back(StartupSkinRect{{left,top-1,width,height+1},{239,239,221},true});
    wood(result,width,height,left,top);
    result.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,29,-1,0,0,
        {left+1,0,width-2,17},{left+1,top}});
    if (measured_title_widths) {
        for (int pass = 0; pass < 2; ++pass)
            result.draws.emplace_back(SteamStartupText{window.title,window.title_value,
                {static_cast<double>(120-(*measured_title_widths)[pass]/2),
                 static_cast<double>(top+3-pass),0,0},{},
                pass == 0 ? std::array<int,3>{44,54,105} : std::array<int,3>{247,253,247}});
    }
    return result;
}
std::optional<SteamStartupFramePlan> steam_startup_box_skin(const SteamStartupBox &box,
    int view_y, int mode) {
    const auto width = static_cast<std::int64_t>(box.right) - box.left;
    const auto height = static_cast<std::int64_t>(box.bottom) - box.top;
    const auto top64 = static_cast<std::int64_t>(box.top) + view_y/2;
    const auto bottom64 = static_cast<std::int64_t>(box.bottom) + view_y/2;
    if (width < 4 || height < 4 || !coordinate(width) || !coordinate(height) ||
        !coordinate(top64) || !coordinate(bottom64)) return {};
    const int left=box.left, top=static_cast<int>(top64), bottom=static_cast<int>(bottom64);
    const int w=static_cast<int>(width), h=static_cast<int>(height);
    SteamStartupFramePlan result;
    result.draws.emplace_back(StartupSkinRect{{left,top,w,h},{247,253,247},false});
    result.draws.emplace_back(StartupSkinRect{{left,top,w,h},{172,202,179},true});
    result.draws.emplace_back(StartupSkinRect{{left+1,top+1,w-2,h-2},{222,234,225},true});
    if (mode == 0 || mode == 1) {
        const std::array<std::array<int,2>,4> anchors{{{left,top},{box.right,top},
                                                     {box.right,bottom},{left,bottom}}};
        for (int frame=0; frame<4; ++frame)
            result.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,
                mode == 0 ? 30 : 121,6,frame,0,{},anchors[frame]});
    }
    return result;
}
std::optional<SteamStartupFramePlan> steam_startup_window2_skin(int width, int height,
    int vertical, int view_y) {
    if (width < 1 || width > 240 || height < 1 || height > 240) return {};
    const auto top=window_top(height,vertical,view_y,0);
    if (!top) return {};
    SteamStartupFramePlan result;
    wood(result,width,height,120-width/2,*top);
    return result;
}
std::optional<SteamStartupFramePlan> steam_startup_window3_skin(int width, int height,
    int left, int top) {
    if (width < 3 || width > 240 || height < 1 || height > 239 ||
        !coordinate(static_cast<std::int64_t>(left)-1) ||
        !coordinate(static_cast<std::int64_t>(left)+width+1) ||
        !coordinate(static_cast<std::int64_t>(top)-1) ||
        !coordinate(static_cast<std::int64_t>(top)+height+2)) return {};
    SteamStartupFramePlan result;
    result.draws.emplace_back(StartupSkinRect{{left-1,top-1,width+2,height+3},{89,103,91},true});
    result.draws.emplace_back(StartupSkinRect{{left,top,width,height+1},{239,239,221},true});
    wood(result,width,height+1,left,top);
    return result;
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
    text(p,Role::slot_type,in.row,120,v+69,0,0,2,std::array<int,3>{60,100,200},0,0,SteamStartupTextPlacement::centered);
    for(int row=0;row<2;++row) {
        const int r=v+84+47*row;
        p.draws.emplace_back(SteamStartupBox{29,r-12,210,r+29});
        if(in.present[row]) {
            image(p,Asset::save_icon,39+(row==0?-4:0),r+6+(row==0?-2:0),0,1000,
                  row==0?std::array<int,4>{2,23,25,20}:std::array<int,4>{6,3,16,16});
            if(row==in.row)
                p.draws.emplace_back(SteamStartupFill{{in.japanese?79.0:75.0,
                    static_cast<double>(r+6),in.japanese?94.0:102.0,14},{200,200,255}});
            text(p,Role::slot_name,row,75,r+6,102,14,0x22,slot_text,0,0,SteamStartupTextPlacement::layout);
            text(p,Role::slot_date,row,34,r+24,0,0,{},slot_text);
            text(p,Role::slot_cash,row,201,r+24,0,0,4,slot_text,0,0,SteamStartupTextPlacement::right);
            if(row==in.row && in.title_on_top) image(p,Asset::finger_left,181,r+12,-1);
        } else {
            // Steam空手动栏有独立选中底色，不复用非空村名的裁片位置。
            if(row==1 && row==in.row)
                p.draws.emplace_back(SteamStartupFill{{85.0,static_cast<double>(r+14),82,14},{200,200,255}});
            image(p,Asset::save_icon,39+(row==0?-4:0),r+15+(row==0?-2:0),0,1000,
                  row==0?std::array<int,4>{2,23,25,20}:std::array<int,4>{6,3,16,16});
            text(p,Role::empty_slot,row,126,r+15,0,0,2,slot_text,0,0,SteamStartupTextPlacement::centered);
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
        text(p,Role::answer,row,center,167,0,0,2,brown);
        p.touches.push_back({3,0x20000|row,
            std::array<int,4>{static_cast<int>(center-b/2-105),64,static_cast<int>(touch_width),217},{},0});
    }
    return p;
}
} // namespace dungeon_village_prototype
