// PAGES page74: classify kind12 first, then detail g=1/4/5/6, kind2, ordinary. S017/S018/S019
// are cross-version visual references only; every numeric value below comes from the model.
#include "facility_page.hpp"
#include <algorithm>
namespace ark::desktop::ui {
namespace {
void money(const Skin &skin, std::int64_t value, Vector2 right) {
    skin.number(value, {right.x - 9, right.y});
    skin.text.draw("G", right.x - 8, right.y - 1, blue, 11);
}
void picture(const app::Game &game, const facilities::Definition &d, int orientation, Rectangle box,
             const Skin &skin) {
    skin.content(box, Color{226, 247, 212, 255});
    skin.sprites.thumbnail(game.display(d.display_id).sprite, orientation,
                           {box.x + 6, box.y + 4, box.width - 12, box.height - 15});
}
void effects(const facilities::Definition &d, Rectangle box, const Skin &skin) {
    skin.content(box, Color{255, 249, 217, 255});
    if (d.effect_icons.empty()) {
        skin.centered("---", box, blue);
        return;
    }
    // Only the currently unlocked ordinary facilities have this validated display pairing.
    // Keep other source lists independent; do not invent consumers for the extra A entries.
    for (std::size_t i = 0; i < d.effect_icons.size() && i < d.effect_markers.size() && i < 2;
         ++i) {
        const float y = box.y + 3 + i * 17;
        skin.sprites.image("icon_param00.png", {d.effect_icons[i] * 16.0F, 16, 16, 16},
                           {box.x + 9, y, 16, 16});
        for (int j = 0; j < d.effect_markers[i] && j < 5; ++j)
            skin.sprites.draw("number08.seb", 14, {box.x + 37 + j * 8, y + 3}, WHITE,
                              Sprites::Binding::common);
    }
}
void ordinary(const app::Game &game, const State &view, const facilities::Definition &d,
              Rectangle box, const Skin &skin) {
    const auto instance = view.page == Page::facility ? view.detail : std::nullopt;
    const auto values = game.facility_values(d.id, instance);
    const auto &progress = game.state().definition_progress.at(d.id);
    skin.text.draw("价格", box.x + 124, box.y + 25, blue);
    money(skin, instance ? values.instance[0] : values.definition[0], {box.x + 209, box.y + 27});
    skin.content({box.x + 8, box.y + 42, 204, 115});
    const auto orientation = instance ? game.state().facilities.at(*instance).orientation : 0;
    picture(game, d, orientation, {box.x + 13, box.y + 47, 97, 74}, skin);
    skin.sprites.image("wnd_lv.png", {0, 0, 17, 10}, {box.x + 15, box.y + 110, 17, 10});
    skin.number(progress.level, {box.x + 49, box.y + 110}, "number05.seb");
    skin.content({box.x + 113, box.y + 47, 94, 34}, Color{255, 249, 217, 255});
    const auto &attributes = instance ? values.instance : values.definition;
    skin.text.draw("品质", box.x + 120, box.y + 51, blue);
    skin.text.draw("魅力", box.x + 120, box.y + 66, blue);
    skin.number(attributes[1], {box.x + 201, box.y + 52});
    skin.number(attributes[2], {box.x + 201, box.y + 67});
    effects(d, {box.x + 113, box.y + 83, 94, 38}, skin);
    if (progress.level == 5) {
        skin.sprites.image("wnd_max.png", {0, 0, 20, 6}, {box.x + 99, box.y + 136, 20, 6});
    } else {
        const auto remaining =
            progress.completed_uses >= static_cast<std::uint64_t>(values.upgrade_uses)
                ? 0
                : values.upgrade_uses - static_cast<std::int64_t>(progress.completed_uses);
        skin.text.draw("距离下个等级还有", box.x + 25, box.y + 132, ink, 11);
        skin.number(remaining, {box.x + 188, box.y + 133}, "number05.seb");
        skin.text.draw("人", box.x + 192, box.y + 131);
    }
    // Definition preview has no instance actions. Item usage is not yet connected.
    if (instance)
        skin.centered("使用道具", {box.x + 60, box.y + 162, 100, 16}, GRAY);
}
void bonuses(const app::Game &game, const State &view, const facilities::Definition &d,
             Rectangle box, const Skin &skin) {
    const auto values = game.facility_values(d.id, view.detail);
    const auto neighbours = game.neighbourhood(*view.detail);
    skin.text.draw("设施奖励", box.x + 15, box.y + 25, blue);
    skin.text.draw("维护费", box.x + 123, box.y + 25, blue);
    money(skin, values.instance[3], {box.x + 209, box.y + 27});
    skin.content({box.x + 8, box.y + 42, 204, 115});
    if (neighbours.sources.empty()) {
        skin.centered("没有奖励", {box.x + 10, box.y + 66, 200, 30});
    } else {
        const int count = static_cast<int>(neighbours.sources.size());
        const int start = std::clamp(view.source_scroll, 0, std::max(0, count - 5));
        const char *names[] = {"价格", "品质", "魅力"};
        for (int i = start; i < std::min(count, start + 5); ++i) {
            const auto &source = neighbours.sources[i];
            const auto &definition = game.definition(source.definition_id);
            const float y = box.y + 49 + (i - start) * 19;
            skin.text.draw(
                definition.name + " Lv" +
                    std::to_string(game.state().definition_progress.at(definition.id).level),
                box.x + 15, y, ink, 10);
            std::string modifiers;
            for (const auto &m : definition.neighbours)
                modifiers += std::string(names[m.slot]) + (m.delta >= 0 ? "+" : "") +
                             std::to_string(m.delta) + " ";
            skin.right(modifiers.empty() ? "---" : modifiers, box.x + 205, y, blue, 9);
        }
    }
    skin.centered("周围设施的奖励", {box.x + 15, box.y + 161, 190, 17}, ink, 11);
}
void special(const app::Game &game, const State &view, const facilities::Definition &d,
             FacilityTemplate kind, Rectangle box, const Skin &skin) {
    skin.content({box.x + 8, box.y + 42, 204, 115});
    const auto instance = view.page == Page::facility ? view.detail : std::nullopt;
    picture(game, d, instance ? game.state().facilities.at(*instance).orientation : 0,
            {box.x + 62, box.y + 47, 97, 74}, skin);
    if (kind == FacilityTemplate::equipment) {
        const char *product = d.activity_detail == 1   ? "武器"
                              : d.activity_detail == 4 ? "防具"
                                                       : "饰品";
        skin.text.draw("种类", box.x + 128, box.y + 25, blue);
        skin.right("---", box.x + 209, box.y + 25, blue); // No unlocked-product snapshot yet.
        const bool active = instance && !game.state().facilities.at(*instance).remaining_ticks;
        skin.centered(active ? std::string("正在销售") + product : product,
                      {box.x + 10, box.y + 128, 200, 24});
        skin.centered("查看商品", {box.x + 60, box.y + 162, 100, 16}, GRAY);
    } else if (kind == FacilityTemplate::booster) {
        const char *labels[] = {"价格", "品质", "魅力"};
        float x = box.x + 30;
        for (const auto &m : d.neighbours) {
            skin.text.draw(std::string(labels[m.slot]) + (m.delta >= 0 ? "+" : "") +
                               std::to_string(m.delta),
                           x, box.y + 131, blue, 11);
            x += 82;
        }
        skin.centered("周围设施的奖励", {box.x + 15, box.y + 161, 190, 17}, ink, 11);
    } else if (kind == FacilityTemplate::recruitment || kind == FacilityTemplate::home) {
        skin.centered("---",
                      {box.x + 10, box.y + 128, 200, 24}); // Unknown applicant/resident state.
        if (kind == FacilityTemplate::recruitment)
            skin.centered("入住希望者", {box.x + 60, box.y + 162, 100, 16}, GRAY);
    } else {
        skin.centered("建设价格 " + std::to_string(d.price) + "G",
                      {box.x + 10, box.y + 128, 200, 24});
    }
}
} // namespace
void draw_facility_page(const app::Game &game, const State &view, const Layout &layout,
                        const Skin &skin) {
    const auto *d = shown_facility(game, view);
    if (!d)
        return;
    const auto box = layout.detail;
    const auto kind = facility_template(*d);
    const int count = facility_page_count(game, view);
    const int page = count == 2 ? view.facility_page : 0;
    skin.window(box, count == 2 ? "设施信息 " + std::to_string(page + 1) + "/2" : "设施信息");
    if (count == 2) {
        skin.sprites.draw("arrow02.seb", 1,
                          {layout.detail_previous.x + 4, layout.detail_previous.y + 9}, WHITE,
                          Sprites::Binding::common);
        skin.sprites.draw("arrow02.seb", 0, {layout.detail_next.x + 4, layout.detail_next.y + 9},
                          WHITE, Sprites::Binding::common);
    }
    if (page == 1) {
        bonuses(game, view, *d, box, skin);
        return;
    }
    skin.sprites.image("icon_tenantInfo.png", {d->icon * 16.0F, 0, 16, 16},
                       {box.x + 13, box.y + 22, 16, 16});
    skin.text.draw(d->name, box.x + 33, box.y + 24, ink, 13);
    if (kind == FacilityTemplate::ordinary)
        ordinary(game, view, *d, box, skin);
    else
        special(game, view, *d, kind, box, skin);
}
} // namespace ark::desktop::ui
