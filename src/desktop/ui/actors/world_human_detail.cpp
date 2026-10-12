#include "world_human_detail.hpp"
#include "../common/skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
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
        draw_world_human_skin(*v.detail_page_skin, {l.panel.x - 10, l.panel.y - 44}, s);
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
} // namespace ark::desktop::ui
