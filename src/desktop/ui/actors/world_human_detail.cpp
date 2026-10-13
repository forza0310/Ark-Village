#include "world_human_detail.hpp"
#include "../common/skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
bool world_human_source_child(const WorldHumanView &v) {
    return v.raw == 62 || v.raw == 63 || v.raw == 66 || v.raw == 73 ||
           (v.raw == 65 && (!v.choice || v.choice->slot < 4));
}
Vector2 world_human_skin_origin(const WorldHumanLayout &l) {
    // Source helper: VIEW_Y23, vertical0. This also yields top44 for the 220x175 parent.
    return {l.panel.x - (120 - int(l.panel.width) / 2),
            l.panel.y - (131 - int(l.panel.height) / 2)};
}
WorldHumanLayout world_human_subwindow_layout(const WorldHumanLayout &base, int raw) {
    if (raw != 62 && raw != 63 && raw != 65 && raw != 66 && raw != 73)
        throw std::invalid_argument("Unsupported human source child");
    auto out = base;
    const float width = raw == 73 ? 170.F : raw == 63 || raw == 66 ? 222.F : 220.F;
    const float height = raw == 73 ? 125.F : raw == 62 ? 168.F : raw == 65 ? 136.F : 170.F;
    const auto center =
        Vector2{base.panel.x + base.panel.width / 2, base.panel.y + base.panel.height / 2};
    out.panel = {center.x - width / 2, center.y - height / 2, width, height};
    out.cancel = {raw == 73 ? out.panel.x : out.panel.x + width - 50, out.panel.y + height + 4, 50,
                  18};
    out.confirm =
        raw == 73 ? Rectangle{out.panel.x + width - 50, out.cancel.y, 50, 18} : Rectangle{};
    if (raw == 63 || raw == 66) {
        out.confirm = out.cancel; // Existing PC confirmation, separate from source Touch2.
        out.cancel = {};
    }
    out.previous = out.next = {}; // Real title arrows are measured from their SEB helper.
    return out;
}
WorldHumanLayout world_human_detail_layout(const WorldHumanLayout &base) {
    auto out = base;
    const auto center =
        Vector2{base.panel.x + base.panel.width / 2, base.panel.y + base.panel.height / 2};
    out.panel = {center.x - 110, center.y - 87.5F, 220, 175};
    const auto p = out.panel;
    out.body = {p.x + 15, p.y + 84, 185, 68}; // Source equipment rows128..179 minus window top44.
    // PC inline action row; source touch margins remain metadata in the formal plan.
    out.primary = {p.x + 15, p.y + 158, 185, 16};
    for (int n = 0; n < 4; ++n)
        out.tabs[n] = {p.x + n * 55, p.y - 19, 55, 17};
    out.cancel = {p.x + 170, p.y + 179, 50, 18};
    out.confirm = {p.x + 170, p.y + 179, 50, 18};
    out.professions = {p.x, p.y + 179, 50, 18};
    out.gifts = {p.x + 55, p.y + 179, 50, 18};
    out.track = {p.x + 110, p.y + 179, 50, 18};
    return out;
}
void draw_world_human_detail(const WorldHumanView &v, const WorldHumanLayout &l, const Skin &s,
                             bool enabled, const std::string &feedback) {
    if (!v.initialized) {
        s.window(l.panel, "冒险者信息");
    }
    constexpr const char *tabs[]{"概况", "属性", "装备", "魔法"};
    for (int n = 0; n < 4; ++n) {
        if (n == v.phase)
            DrawRectangleRec(l.tabs[n], {255, 185, 87, 255});
        s.centered(tabs[n], l.tabs[n], ink, 10);
    }
    if (v.initialized) {
        if (!v.detail_page_skin)
            throw std::runtime_error("Human four-page rendering requires an actual measured plan");
        // VIEW_Y23 makes the source window top44. Align that window once to the PC panel.
        draw_world_human_skin(*v.detail_page_skin, world_human_skin_origin(l), s);
    }
    // PC navigation stays explicit and separate from the unbound source soft-label lifecycle.
    s.button(l.cancel, "返回", enabled && v.initialized);
    s.choice(l.professions, "转职", enabled && v.initialized);
    s.choice(l.gifts, "赠送", enabled && v.initialized);
    if (v.can_track)
        s.choice(l.track, "追踪", enabled && v.initialized);
    if (!feedback.empty()) {
        const float size = 9.F * std::min(1.F, 185.F / std::max(1.F, s.text.width(feedback, 9.F)));
        s.text.draw(feedback, l.panel.x + 15, l.panel.y + 200, MAROON, size);
    }
}
void draw_world_human_subwindow(const WorldHumanView &v, const WorldHumanLayout &l, const Skin &s,
                                bool enabled, const std::string &feedback) {
    if (v.initialized) {
        if (!v.detail_page_skin)
            throw std::runtime_error("Human child needs its measured source plan");
        draw_world_human_skin(*v.detail_page_skin, world_human_skin_origin(l), s);
    } else
        s.window(l.panel, v.raw == 73 ? "装备信息" : "赠送礼物");
    if (v.can_cancel)
        s.button(l.cancel, "返回", enabled && v.initialized);
    if (v.raw == 73)
        s.button(l.confirm, "返回", enabled && v.initialized);
    if (v.raw == 63 || v.raw == 66)
        s.button(l.confirm, "确定", enabled && v.initialized && v.can_confirm);
    if (!feedback.empty()) {
        const float size =
            9.F * std::min(1.F, l.panel.width / std::max(1.F, s.text.width(feedback, 9.F)));
        s.text.draw(feedback, l.panel.x, l.panel.y + l.panel.height + 43, MAROON, size);
    }
}
} // namespace ark::desktop::ui
