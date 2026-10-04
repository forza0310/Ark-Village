// S005 top bar/date/money, town points and fan popularity meter. Values come only from Game.
#include "hud.hpp"
#include <algorithm>
namespace ark::desktop::ui {
void draw_hud(const app::Game &game, const State &view, const Layout &layout, const Skin &skin) {
    const auto &s = game.state();
    const float w = static_cast<float>(layout.extent.width),
                h = static_cast<float>(layout.extent.height);
    skin.tile("top_bar.png", {22, 0, 90, 24}, {22, 0, w - 150, 24});
    skin.sprites.image("top_bar.png", {0, 0, 22, 24}, {0, 0, 22, 24});
    skin.sprites.image("top_bar.png", {112, 0, 128, 24}, {w - 128, 0, 128, 24});
    // This slice is April/May; S001/S005 identify the spring flower. Annual icon switching
    // remains outside the finite pre-report run, rather than guessing the four-frame order.
    skin.sprites.draw("icon_season.seb", 0, {2, 3}, WHITE, Sprites::Binding::common);
    const int values[] = {s.calendar[0] + 1, s.calendar[1] + 1, s.calendar[2] + 1};
    float x = 29;
    for (int i = 0; i < 3; ++i) {
        for (char c : std::to_string(values[i])) {
            skin.sprites.draw("number01.seb", c - '0', {x, 8}, WHITE, Sprites::Binding::common);
            x += 8;
        }
        skin.sprites.draw("number01.seb", 10 + i, {x, 8}, WHITE, Sprites::Binding::common);
        x += 13;
    }
    skin.right(std::to_string(s.money) + "G", w - 7, 6, Color{85, 87, 87, 255});
    skin.sprites.image("townPointbar.png", {0, 0, 55, 15}, {w - 55, 24, 55, 15});
    skin.number(s.points, {w - 3, 27});
    skin.tile("btmbar.png", {116, 1, 4, 20}, {0, h - 21, w, 20});
    DrawRectangleLinesEx({0, h - 22, w, 22}, 1, Color{65, 62, 47, 255});
    const float meter_x = w - 137;
    skin.sprites.image("btmbar_popular00.png", {0, 0, 78, 41}, {meter_x, h - 41, 78, 41});
    skin.sprites.image("btmbar_popular01.png", {0, 0, 77, 14}, {meter_x + 1, h - 16, 77, 14});
    skin.number(s.popularity, {meter_x + 74, h - 20});
    DrawRectangle(static_cast<int>(meter_x + 26), static_cast<int>(h - 4),
                  static_cast<int>(std::min(s.popularity, 100) * 0.48F), 2,
                  Color{240, 82, 139, 255});
    const bool catalog = s.mode == app::Mode::catalog;
    const bool placement = s.mode == app::Mode::placement;
    const bool normal = s.mode == app::Mode::normal && view.page == Page::village;
    // Explicit desktop pause is separate from the original modal update restrictions.
    if (normal)
        skin.button(layout.pause_button, s.paused ? "已暂停 · 继续" : "运行中 · 暂停");
    if (normal && game.life_state() && game.life_state()->error != app::InitialAiError::none) {
        const auto &life = *game.life_state();
        std::string activity = "后续活动";
        if (life.handoff == app::LifeHandoff::encounter_creation)
            activity = "野外遭遇";
        else if (life.pending_definition)
            activity = game.definition(*life.pending_definition).name;
        else if (life.pending_category == 3)
            activity = "住所/出口";
        else if (life.pending_category == 4)
            activity = "野外";
        const auto label = life.error == app::InitialAiError::unsupported_branch
                               ? "人物活动待接入：" + activity
                               : "人物更新失败";
        skin.centered(label, {4, 68, std::min(230.0F, w - 8), 20}, Color{142, 42, 31, 255}, 10);
    }
    skin.button(layout.left_button,
                placement                 ? "旋转"
                : catalog                 ? "信息"
                : view.page == Page::menu ? "网站"
                                          : "保存",
                placement || catalog);
    skin.button(layout.right_button, normal ? "菜单" : "返回",
                s.mode != app::Mode::tutorial && s.mode != app::Mode::research_boundary);
    if (placement) {
        const auto r = layout.message;
        skin.sprites.image("hisho_talk.png", {0, 0, 176, 22}, r);
        skin.centered(view.error != app::Error::none ? ""
                      : view.notice_frames           ? "建设完毕"
                                                     : "要建造在哪里",
                      layout.message);
    }
}
} // namespace ark::desktop::ui
