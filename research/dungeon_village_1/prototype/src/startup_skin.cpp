#include "dungeon_village_prototype/startup_skin.hpp"

namespace dungeon_village_prototype {
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
} // namespace dungeon_village_prototype
