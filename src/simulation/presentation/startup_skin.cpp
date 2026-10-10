#include "ark/simulation/presentation/startup_skin.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace ark::simulation {
namespace {
bool integer_coordinate(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
}
std::optional<StartupSkinDraw> startup_skin_image(StartupSkinImage part) {
    using Package = StartupSkinPackage;
    // title/img.inf及b/h绘制；event图9在纪录与91使用不同源裁片。
    switch (part) {
    case StartupSkinImage::title_background:
        return StartupSkinDraw{Package::title, 0, -1, 0, 0, {0, 0, 240, 330}, {}};
    case StartupSkinImage::title_menu:
        return StartupSkinDraw{Package::title, 1, -1, 0, 0, {0, 0, 98, 68}, {}};
    case StartupSkinImage::title_logo:
        return StartupSkinDraw{Package::title, 2, -1, 0, 0, {0, 0, 236, 115}, {}};
    case StartupSkinImage::title_cursor:
        return StartupSkinDraw{Package::title, 3, -1, 0, 0, {0, 0, 28, 23}, {}};
    case StartupSkinImage::title_grass:
        return StartupSkinDraw{Package::title, 4, -1, 0, 0, {0, 0, 240, 18}, {}};
    case StartupSkinImage::record_background:
        return StartupSkinDraw{Package::event, 9, -1, 0, 0, {0, 0, 180, 80}, {30, 78}};
    case StartupSkinImage::new_game_background:
        return StartupSkinDraw{Package::event, 9, -1, 0, 0, {0, 9, 180, 63}, {30, 65}};
    case StartupSkinImage::clear_background:
        return StartupSkinDraw{Package::event, 13, -1, 0, 0, {0, 0, 180, 80}, {30, 105}};
    }
    return {};
}
std::optional<StartupWindowSkin> startup_window_skin(int width, int height, int scene_top,
    int vertical_offset, int style_offset, std::optional<int> measured_title_width) {
    // 维护接口拒绝不可裁片尺寸/溢出，不把这种保护声称为原Java行为。
    if (width < 3 || width > 240 || height < 17 || height > 240 ||
        (measured_title_width && *measured_title_width < 0)) return {};
    const std::int64_t inset = height > 216 ? 0 :
        static_cast<std::int64_t>(scene_top) + 2LL * style_offset;
    if (!integer_coordinate(inset) || !integer_coordinate(inset + 240)) return {};
    const std::int64_t top64 = (inset + 240) / 2 - height / 2 + vertical_offset;
    if (!integer_coordinate(top64 - 2) || !integer_coordinate(top64 + height + 1)) return {};
    const int left = 120 - width / 2, top = static_cast<int>(top64);
    StartupWindowSkin result;
    // o.d(x,y,w,h)真正覆盖w+1/h+1；这里统一为半开像素大小。
    result.borders = {{{{left - 1, top - 2, width + 2, height + 3}, {89,103,91}, true},
                       {{left, top - 1, width, height + 1}, {239,239,221}, true}}};
    result.images.reserve(static_cast<std::size_t>((width + 39) / 40 + 1));
    for (int x = 0; x < width; x += 40)
        result.images.push_back({StartupSkinPackage::common,28,-1,0,0,
                                 {0,0,std::min(40,width-x),height},{left+x,top}});
    result.images.push_back({StartupSkinPackage::common,29,-1,0,0,
                             {left+1,0,width-2,17},{left+1,top}});
    if (measured_title_width) {
        const int x = 120 - *measured_title_width / 2;
        result.title = std::array<StartupSkinTextAnchor,2>{{
            {{x,top+3},{44,54,105}}, {{x,top+2},{247,253,247}}}};
    }
    return result;
}
std::optional<StartupContentSkin> startup_content_skin(int left, int top, int right,
    int bottom, int scene_top) {
    const std::int64_t width = static_cast<std::int64_t>(right) - left;
    const std::int64_t height = static_cast<std::int64_t>(bottom) - top;
    const std::int64_t y0 = static_cast<std::int64_t>(top) + scene_top / 2;
    const std::int64_t y1 = static_cast<std::int64_t>(bottom) + scene_top / 2;
    if (width < 4 || height < 4 || !integer_coordinate(width) || !integer_coordinate(height) ||
        !integer_coordinate(y0) || !integer_coordinate(y1)) return {};
    // SEB四角4x4允许窄框互相覆盖，但拒绝比单角更窄的维护输入。
    const int w = static_cast<int>(width), h = static_cast<int>(height);
    const int y = static_cast<int>(y0), end_y = static_cast<int>(y1);
    StartupContentSkin result;
    result.rectangles = {{{{left,y,w,h},{247,253,247},false},
                          {{left,y,w,h},{172,202,179},true},
                          {{left+1,y+1,w-2,h-2},{222,234,225},true}}};
    result.corners = {{{StartupSkinPackage::common,30,6,0,0,{}, {left,y}},
                       {StartupSkinPackage::common,30,6,1,0,{}, {right,y}},
                       {StartupSkinPackage::common,30,6,2,0,{}, {right,end_y}},
                       {StartupSkinPackage::common,30,6,3,0,{}, {left,end_y}}}};
    return result;
}
std::optional<StartupClearSkin> startup_clear_skin(int stage, int counter, int page_counter) {
    constexpr std::array<int, 8> thresholds{75, 75, 45, 45, 65, 45, 65, 9999};
    if (stage < 0 || stage >= static_cast<int>(thresholds.size()) || counter < 0 ||
        counter > thresholds[stage] || page_counter < 0)
        return {};
    // b/g绘制中的f124d为页面计数；阶段4只请求SEB合法子集4/6与5/7。
    const int left_frame = stage == 4 ? 4 + 2 * ((page_counter % 8) / 4) : 0;
    StartupClearSkin result;
    result.background = *startup_skin_image(StartupSkinImage::clear_background);
    result.actors = {{
        {StartupSkinPackage::common, 171, 46, left_frame + 1, 0, {}, {190, 181}},
        {StartupSkinPackage::common, 172, 31, left_frame, 0, {}, {51, 181}}
    }};
    // 只表达源Draw条件；stage6满门槛是否实际进入绘制另由框架/页退休控制。
    if ((stage == 0 || stage == 3 || stage == 6) && counter == thresholds[stage] &&
        page_counter % 40 > 20) {
        result.continue_marker = StartupSkinDraw{StartupSkinPackage::common, 72, 2, 0, 0,
                                                 {}, {213, stage == 6 ? 167 : 179}};
    }
    return result;
}
} // namespace ark::simulation
