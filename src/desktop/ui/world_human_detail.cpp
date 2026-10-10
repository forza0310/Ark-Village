#include "world_human_detail.hpp"
#include "skin.hpp"
#include <algorithm>
#include <cstdint>

namespace ark::desktop::ui {
WorldHumanLayout world_human_detail_layout(const WorldHumanLayout &base) {
    auto out = base;
    const auto center =
        Vector2{base.panel.x + base.panel.width / 2, base.panel.y + base.panel.height / 2};
    out.panel = {center.x - 110, center.y - 87.5F, 220, 175};
    const auto p = out.panel;
    out.body = {p.x + 15, p.y + 95, 185, 68};
    for (int n = 0; n < 4; ++n)
        out.tabs[n] = {p.x + n * 55, p.y - 19, 55, 17};
    out.cancel = {p.x + 170, p.y + 179, 50, 18};
    out.confirm = {p.x + 170, p.y + 179, 50, 18};
    out.professions = {p.x, p.y + 179, 50, 18};
    out.gifts = {p.x + 55, p.y + 179, 50, 18};
    return out;
}
void draw_world_human_detail(const WorldHumanView &v, const WorldHumanLayout &l, const Skin &s,
                             bool enabled, const std::string &feedback) {
    const Vector2 o{l.panel.x - 10, l.panel.y - 33};
    const auto box = [&](float x, float y, float w, float h) {
        return Rectangle{o.x + x, o.y + y, w, h};
    };
    const auto point = [&](float x, float y) { return Vector2{o.x + x, o.y + y}; };
    const auto label = [&](const std::string &value, float x, float y, float width, float size = 11,
                           Color c = ink) {
        const float fit = size * std::min(1.F, width / std::max(1.F, s.text.width(value, size)));
        s.text.draw(value, o.x + x, o.y + y, c, fit);
    };
    const auto number = [&](int value, float x, float y, const char *sprite = "number08.seb") {
        s.number(value, point(x, y), sprite, Sprites::Binding::steam_common);
    };
    const auto image = [&](const char *name, Rectangle crop, float x, float y,
                           Sprites::Binding group = Sprites::Binding::common) {
        s.sprites.image(name, crop, box(x, y, crop.width, crop.height), group);
    };
    const auto icon = [&](int slot, float x, float y) {
        const int id = v.equipment_icons[slot];
        // Existing catalog backing color is a desktop adaptation; actual icon identity is
        // separate from weapon render-image ID, and empty armour/accessories use slot7.
        s.sprites.indexed_image(Sprites::Binding::common, 24, {id < 0 ? 126.F : 54.F, 0, 18, 18},
                                box(x, y, 18, 18));
        if (id >= 0)
            s.sprites.indexed_image(Sprites::Binding::common,
                                    slot == 0   ? 12
                                    : slot == 3 ? 21
                                                : 20,
                                    {18.F * (id % 10), 18.F * (id / 10), 18, 18},
                                    box(x, y, 18, 18));
    };
    s.window(l.panel, "冒险者信息");
    s.content(box(17, 52, 202, 138));
    constexpr const char *tabs[]{"概况", "属性", "装备", "魔法"};
    for (int n = 0; n < 4; ++n) {
        if (n == v.phase)
            DrawRectangleRec(l.tabs[n], {255, 185, 87, 255});
        s.centered(tabs[n], l.tabs[n], ink, 10);
    }
    if (v.initialized) {
        label(v.name, 124, 71, 86);
        DrawRectangleRec(box(119, 84, 91, 25), {215, 249, 214, 255});
        DrawRectangleRec(box(119, 109, 91, 13), {172, 235, 169, 255});
        const float job_size = s.text.width(v.profession) >= 53 ? 7.F : 9.F;
        label(v.profession, 123, job_size == 9 ? 88 : 89, 53, job_size);
        image("wnd_lv.png", {0, 0, 17, 10}, 177, 88);
        if (v.details.level == 10)
            image("wnd_max.png", {0, 0, 20, 6}, 195, 90);
        else
            number(v.details.level, 208, 88, "number05.seb");
        image("wnd_expBar.png", {1, 0, 81, 5}, 123, 102);
        const int fill =
            v.details.level == 10 ? 79
            : v.details.threshold > 0
                ? static_cast<int>(std::clamp<std::int64_t>(
                      std::int64_t(v.details.experience) * 79 / v.details.threshold, 0, 79))
                : 0;
        if (fill > 0)
            image("wnd_expBar.png", {2, 5, float(fill), 3}, 124, 103);
        s.sprites.draw("wnd_exp.seb", 0, point(130, 112), WHITE, Sprites::Binding::common);
        image("wnd_ato.png", {0, 0, 10, 7}, 149, 113, Sprites::Binding::steam_common);
        if (v.details.level != 10)
            number(v.details.threshold - v.details.experience, 201, 110);
        image("mp_back.png", {0, 5, 90, 57}, 25, 69);
        DrawRectangleLinesEx(box(25, 69, 89, 56), 1, {148, 148, 148, 255});
        if (v.details.resident)
            image("myHomeBack.png", {0, 0, 47, 32}, 27, 82);
        // Static source walk01 frame0 preview only. The full Steam motion/weapon/HP helper
        // remains unpublished, so none of these counters is advanced by the desktop renderer.
        s.sprites.actor(false, 1, v.portrait_image, 0, point(75, 121));
        label("勋章 " + std::to_string(v.details.medals), 82, 70, 31, 8);
        if (v.phase == 0) {
            DrawRectangleRec(box(25, 153, 89, 36), {193, 238, 247, 255});
            label("满意", 30, 157, 40, 10);
            number(v.details.satisfaction, 108, 158, "number05.seb");
            label("努力", 30, 175, 40, 10);
            number(v.details.effort, 108, 176, "number05.seb");
            for (int i = 0; i < 4; ++i)
                icon(i, 32 + 18 * i, 129);
            constexpr const char *labels[]{"HP", "攻击", "防御", "魔法"};
            for (int i = 0; i < 4; ++i) {
                const int y = 123 + 18 * i;
                DrawRectangleRec(box(119, y, 87, 18), {173, 209, 221, 255});
                label(labels[i], 122, y + 4, 41, 10);
                number(v.details.combat[i], 203, y + 5);
            }
        } else if (v.phase == 1) {
            constexpr const char *names[]{"体力", "力量", "灵巧", "结实", "魔力", "运气"};
            for (int i = 0; i < 6; ++i) {
                const int x = 29 + 92 * (i / 3), y = 134 + 19 * (i % 3);
                DrawRectangleRec(box(x - 4, y - 2, 93, 20), {208, 242, 249, 255});
                DrawRectangleLinesEx(box(x - 4, y - 2, 92, 19), 1, {149, 206, 217, 255});
                image("icon_param00.png", {float(i == 5 ? 96 : 16 * i), 16, 16, 16}, float(x),
                      float(y), Sprites::Binding::steam_common);
                label(names[i], float(x + 18), float(y + 2), 38, 10);
                number(v.details.attributes[i], float(x + 83), float(y + 3));
            }
        } else if (v.phase == 2) {
            constexpr const char *names[]{"武器", "防具1", "防具2", "饰品"};
            for (int i = 0; i < 4; ++i) {
                const int y = 128 + 17 * i;
                DrawRectangleRec(box(25, y, 185, 18), {208, 242, 249, 255});
                DrawRectangleLinesEx(box(25, y, 184, 17), 1, {149, 206, 217, 255});
                label(names[i], 29, float(y + 3), 45, 10);
                icon(i, 79, float(y));
                label(v.equipment_names[i], 112, float(y + 3), 90, 10);
            }
            const int selected = std::clamp(v.selection, 0, 3);
            s.sprites.draw("finger_r.seb", 0, point(24, 138 + 17 * selected), WHITE,
                           Sprites::Binding::common);
            if (v.equipment_icons[selected] >= 0) {
                const auto &values = v.equipment_values[selected];
                const int first = selected == 0 ? 1 : 0, second = selected == 0 ? 3 : 2;
                constexpr const char *parameters[]{"HP", "攻击", "防御", "魔法"};
                if (values[first] > 0)
                    label(std::string(parameters[first]) + " +" + std::to_string(values[first]), 31,
                          202, 82, 10, blue);
                if (values[second] > 0)
                    label(std::string(parameters[second]) + " +" + std::to_string(values[second]),
                          132, 202, 77, 10, blue);
            }
        } else {
            label("已学魔法", 94, 134, 100, 11);
            constexpr const char *names[]{"火魔法", "冰魔法", "雷魔法", "回复魔法"};
            for (int i = 0; i < 4; ++i) {
                const int x = 29 + 92 * (i / 2), y = 153 + 19 * (i % 2);
                DrawRectangleRec(box(x - 4, y - 2, 93, 20), {208, 242, 249, 255});
                DrawRectangleLinesEx(box(x - 4, y - 2, 92, 19), 1, {149, 206, 217, 255});
                if (v.details.spells[i]) {
                    image("icon_magicpot.png", {float((i == 3 ? 4 : i) * 16), 0, 16, 16}, float(x),
                          float(y));
                    label(names[i], float(x + 18), float(y + 3), 65, 9);
                } else
                    label("---", float(x + 33), float(y + 3), 40, 10);
            }
            const auto count = std::count(v.details.spells.begin(), v.details.spells.end(), true);
            label(count == 0 ? "精通特定职业后可以习得"
                             : "可以使用 " + std::to_string(count) + " 种魔法",
                  38, 202, 164, 10);
        }
    }
    // PC navigation stays explicit and separate from the unbound source soft-label lifecycle.
    s.button(l.cancel, "返回", enabled && v.initialized);
    s.choice(l.professions, "转职", enabled && v.initialized);
    s.choice(l.gifts, "赠送", enabled && v.initialized);
    if (!feedback.empty())
        label(feedback, 25, 197, 180, 9, MAROON);
}
} // namespace ark::desktop::ui
