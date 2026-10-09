#pragma once

#include <array>
#include <optional>

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
