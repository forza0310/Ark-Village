#pragma once
#include "world_list_controls.hpp"
#include <algorithm>
#include <cmath>

namespace ark::desktop::ui {
struct WorldFacilityGesture {
    std::uint64_t page{};
    std::optional<int> marked_row;
    bool scrolling{}, consumed{};
    float start_y{};
    int start_first{};
};
inline bool world_facility_hit(std::optional<Vector2> p, Rectangle r) {
    return p && r.width > 0 && r.height > 0 && p->x >= r.x && p->y >= r.y && p->x < r.x + r.width &&
           p->y < r.y + r.height;
}
struct WorldFacilityScrollInput {
    std::optional<int> absolute;
    bool consume_click{};
};
// PC motion becomes an absolute source request. Crossing a page/pending gate cannot turn
// a consumed drag release into a row confirmation; this state never enters the Owner.
inline WorldFacilityScrollInput
world_facility_scroll_input(WorldFacilityGesture &g, const WorldFacilityPaintedControls *p,
                            bool usable, std::optional<Vector2> pointer, bool pressed, bool down,
                            bool released) {
    WorldFacilityScrollInput out;
    if (!usable)
        g.scrolling = false;
    if (usable && p->scroll && p->count > p->scroll_visible && pressed &&
        world_facility_hit(pointer, *p->scroll)) {
        g.scrolling = g.consumed = true;
        g.start_y = pointer->y;
        g.start_first = p->first;
        g.marked_row.reset();
    }
    out.consume_click = g.consumed;
    if (usable && g.scrolling && pointer && (down || released)) {
        const float grid = p->scroll->height / p->count;
        if (grid > 0)
            out.absolute = static_cast<int>(std::clamp(
                double(g.start_first) + std::trunc(double(pointer->y - g.start_y) / grid), 0.0,
                double(std::max(0, p->count - p->scroll_visible))));
    }
    if (released || !pointer)
        g.scrolling = g.consumed = false;
    return out;
}
} // namespace ark::desktop::ui
