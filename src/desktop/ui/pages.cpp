// S001/S002 and PAGES layout: original menu sprites and field order, independent of rule values.
#include "pages.hpp"
#include "facility_page.hpp"
#include <algorithm>
namespace ark::desktop::ui {
namespace {
const char *error_text(app::Error error) {
    switch (error) {
    case app::Error::none:
        return "";
    case app::Error::insufficient_funds:
        return "金钱不足";
    case app::Error::outside_map:
    case app::Error::outside_town:
        return "请选择街道内地域";
    case app::Error::occupied:
        return "有建筑物";
    case app::Error::unavailable:
        return "不可用";
    default:
        return "状态异常";
    }
}
void menu(const app::Game &game, const State &view, const Layout &layout, const Skin &skin) {
    const char *names[] = {"建造", "冒险", "办公室", "信息", "系统"};
    const int icons[] = {0, 1, 2, 5, 6};
    for (int i = 0; i < 5; ++i) {
        const auto r = layout.menu_rows[i];
        // menu frames2/3 are the 89-wide highlighted/ordinary rows, not image index2/3.
        skin.sprites.draw("menu.seb", i == view.menu_row ? 2 : 3, {r.x, r.y}, WHITE,
                          Sprites::Binding::common);
        skin.sprites.draw("wnd_menuIcon.seb", icons[i], {r.x + 4, r.y + 5}, WHITE,
                          Sprites::Binding::common);
        skin.text.draw(names[i], r.x + 26, r.y + 8,
                       ((i == 0 && !game.ai_preview_enabled()) || i == 3)
                           ? (i == view.menu_row ? ink : WHITE)
                           : Color{182, 174, 147, 255});
        if (i == view.menu_row)
            skin.sprites.draw("finger_r.seb", 0, {r.x + r.width - 1, r.y + 11}, WHITE,
                              Sprites::Binding::common);
    }
}
void catalog(const app::Game &game, const State &view, const Layout &layout, const Skin &skin) {
    const auto box = layout.catalog;
    DrawRectangleRec(box, Color{232, 240, 183, 255});
    DrawRectangleLinesEx(box, 1, Color{64, 103, 36, 255});
    DrawRectangleLinesEx({box.x + 2, box.y + 2, box.width - 4, box.height - 4}, 1, green);
    const char *names[] = {"设备", "一般", "饮食"};
    for (int i = 0; i < 3; ++i) {
        skin.sprites.image("buildCategoryBack.png", {0, i == view.tab ? 17.0F : 0, 57, 17},
                           layout.tabs[i], Sprites::Binding::common2);
        skin.centered(names[i], layout.tabs[i], Color{90, 111, 39, 255}, 11);
    }
    skin.sprites.image("arrow00.png", {0, 0, 8, 12}, {box.x - 8, box.y + 4, 8, 12});
    skin.sprites.image("arrow00.png", {8, 0, 8, 12}, {box.x + box.width, box.y + 4, 8, 12});
    const auto items = catalog_items(view.tab);
    for (int i = 0; i < 4; ++i) {
        const auto r = layout.rows[i];
        DrawRectangleRec(r, Color{250, 254, 252, 255});
        DrawRectangleLinesEx(r, 1, green);
        const auto index = view.scroll + i;
        if (index >= static_cast<int>(items.size()))
            continue;
        const auto &item = *items[index];
        const bool disabled = item.kind == 6 || item.kind == 13;
        DrawRectangleRec({r.x + 6, r.y + 2, 63, 32}, Color{209, 244, 225, 255});
        BeginScissorMode(static_cast<int>(r.x + 6), static_cast<int>(r.y + 2), 63, 32);
        skin.sprites.thumbnail(game.display(item.display_id).sprite, 0, {r.x + 6, r.y + 2, 63, 32},
                               disabled ? Color{165, 165, 165, 255} : WHITE);
        EndScissorMode();
        if (index == view.row) {
            const float name_width = skin.text.width(item.name, 11);
            DrawRectangleRec({r.x + 74, r.y + 3, name_width + 3, 13}, Color{255, 211, 139, 255});
            skin.sprites.draw("finger_r.seb", 0, {r.x + 69, r.y + 9}, WHITE,
                              Sprites::Binding::common);
        }
        skin.text.draw(item.name, r.x + 75, r.y + 3, disabled ? GRAY : ink, 11);
        skin.right(std::to_string(item.price) + "G", r.x + r.width - 4, r.y + 23,
                   game.state().money < item.price ? RED : Color{104, 106, 106, 255}, 11);
    }
    DrawRectangleRec({box.x + 3, box.y + 165, box.width - 6, box.height - 168},
                     Color{250, 254, 252, 255});
    skin.centered("---", {box.x + 4, box.y + 168, box.width - 8, 25}, GRAY);
    if (items.size() > 4) {
        const float thumb_height = 144 * 4.0F / items.size();
        DrawRectangleRec({box.x + box.width - 3, box.y + 21, 2, 144}, Color{201, 225, 181, 255});
        DrawRectangleRec({box.x + box.width - 3, box.y + 21 + view.scroll * 144.0F / items.size(),
                          2, thumb_height},
                         Color{56, 149, 208, 255});
    }
}
} // namespace
void draw_pages(const app::Game &game, const State &view, const Layout &layout, const Skin &skin) {
    const auto &s = game.state();
    if (view.page == Page::menu) {
        menu(game, view, layout, skin);
    } else if (view.page == Page::facility || view.page == Page::definition) {
        draw_facility_page(game, view, layout, skin);
    } else if (view.page == Page::roster) {
        const auto box = layout.detail;
        skin.window(box, "冒险者一览");
        skin.content({box.x + 8, box.y + 24, box.width - 16, box.height - 34});
        if (!s.adventurer) {
            skin.centered("尚无到访者", {box.x, box.y + 60, box.width, 40});
        } else {
            const auto &a = *s.adventurer;
            skin.sprites.draw("walk00.seb", 0, {box.x + 27, box.y + 57}, WHITE,
                              Sprites::Binding::farmer);
            skin.text.draw(a.name, box.x + 49, box.y + 32);
            skin.text.draw(TextFormat("农家  等级 %d", a.level), box.x + 49, box.y + 52);
            skin.text.draw(TextFormat("体力 %d  攻击 %d", a.combat[0], a.combat[1]), box.x + 26,
                           box.y + 86);
            skin.text.draw(TextFormat("防御 %d  魔法 %d", a.combat[2], a.combat[3]), box.x + 26,
                           box.y + 109);
        }
    } else if (s.mode == app::Mode::catalog) {
        catalog(game, view, layout, skin);
    }
    if (s.mode == app::Mode::tutorial) {
        const auto box = layout.dialogue;
        skin.window(box, "");
        skin.sprites.draw("chara_hishoko01.seb", 0, {box.x + 13, box.y + 20}, WHITE,
                          Sprites::Binding::secretary);
        skin.content({box.x + 9, box.y + 27, box.width - 18, box.height - 54});
        skin.text.paragraph(app::startup_data().first_talk.at(s.talk_line), box.x + 15, box.y + 33,
                            box.width - 30);
        skin.centered("确定", {box.x + 143, box.y + 96, 46, 20});
    } else if (s.mode == app::Mode::research_boundary) {
        const auto box = layout.dialogue;
        skin.window(box, "本轮结束");
        skin.button({box.x + 48, box.y + 66, 106, 29}, "重新开始");
    }
    if (view.error != app::Error::none) {
        const auto box = layout.message;
        skin.content(box);
        skin.centered(error_text(view.error), box, Color{175, 49, 30, 255}, 11);
    }
}
} // namespace ark::desktop::ui
