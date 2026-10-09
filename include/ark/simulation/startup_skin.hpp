#pragma once

#include <array>
#include <optional>
#include <vector>

namespace ark::simulation {
// 固定APK的源包身份，不复用世界举物的common/weapon二分适配器。
enum class StartupSkinPackage { title, event, common };
enum class StartupSkinImage {
    title_background, title_menu, title_logo, title_cursor, title_grass,
    record_background, new_game_background, clear_background
};
struct StartupSkinDraw {
    StartupSkinPackage package{StartupSkinPackage::common};
    int image{-1}, sprite{-1}, frame{}, layer{};
    std::array<int, 4> crop{}; // sprite<0时为原PNG裁片；否则由原SEB解析。
    std::array<int, 2> offset{}; // 页面逻辑锚点，SEB内部偏移由适配器另加一次。
};
// title五图只返回资源/完整裁片，offset=0；不是按目录顺序绘制的标题场景。
// 三个页面背景带原局部锚点，调用方另加原页面变换；不推断Steam/DPI映射。
std::optional<StartupSkinDraw> startup_skin_image(StartupSkinImage image);

// rect统一为实际像素的半开区间(x,y,width,height)。outline为1像素内描边；
// 已把原o.d的“末像素差值”转换为大小，消费者不可再加1。
struct StartupSkinRect {
    std::array<int, 4> rect{};
    std::array<int, 3> rgb{};
    bool outline{};
};
struct StartupSkinTextAnchor {
    std::array<int, 2> offset{}; // 原文字入口的左上锚；不是Canvas基线。
    std::array<int, 3> rgb{};
};
struct StartupWindowSkin {
    std::array<StartupSkinRect, 2> borders;
    std::vector<StartupSkinDraw> images; // 边线后：横向木纹块→标题带。
    std::optional<std::array<StartupSkinTextAnchor, 2>> title; // 阴影→正文。
};
// 固定APK d/a标题窗。尺寸只接受可从原PNG无缩放取样的3..240、17..240。
// measured_title_width由真实字体/语言适配器提供；空值代表不请求标题文字。
// 0宽末木纹块是原调用空操作，本计划只保留非空图块；不含外层页面变换。
std::optional<StartupWindowSkin> startup_window_skin(int width, int height, int scene_top,
    int vertical_offset, int style_offset, std::optional<int> measured_title_width = {});
struct StartupContentSkin {
    std::array<StartupSkinRect, 3> rectangles; // 填充→外线→内线。
    std::array<StartupSkinDraw, 4> corners; // 左上→右上→右下→左下。
};
// 只覆盖标准内容框(style0/common30)，不把蓝框或image121替换分支混入。
std::optional<StartupContentSkin> startup_content_skin(int left, int top, int right,
    int bottom, int scene_top);

struct StartupClearSkin {
    StartupSkinDraw background;
    std::array<StartupSkinDraw, 2> actors; // 原右大臣→左秘书顺序。
    std::optional<StartupSkinDraw> continue_marker;
};
// 原bo/bp/f124d三者分开；page_counter不是stage_counter。
// 只投影原图片层，背景之后的内框/文字由调用方插入，不冒充完整皮肤。
// 查询无Owner、文件、声音或随机写入；坏阶段/计数显式拒绝，不重算游戏分数。
std::optional<StartupClearSkin> startup_clear_skin(int stage, int stage_counter, int page_counter);
} // namespace ark::simulation
