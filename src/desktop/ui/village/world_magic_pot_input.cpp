#include "world_magic_pot.hpp"

namespace ark::desktop::ui {
std::optional<simulation::StartupMagicPotInput>
world_magic_pot_input(const WorldMagicPotView &v, const WorldMagicPotLayout &l,
                      const WorldMagicPotInput &i, bool blocked,
                      const std::optional<WorldFacilityPaintedControls> &painted,
                      WorldFacilityGesture *g) {
    if (g && g->page != v.page) {
        g->page = v.page;
        g->marked_row.reset();
        g->scrolling = false;
    }
    if (g && (i.keyboard_event || i.enter || i.escape || i.up || i.down || i.left || i.right ||
              i.wheel_rows))
        g->marked_row.reset();
    const bool fresh = painted && painted->page == v.page && painted->raw == v.raw &&
                       painted->phase == v.phase && painted->selection == v.selection &&
                       painted->first == v.first_visible && painted->count == int(v.rows.size()) &&
                       painted->choice == v.binding && painted->extent.width == l.extent.width &&
                       painted->extent.height == l.extent.height;
    const auto drag =
        g ? world_facility_scroll_input(
                *g, fresh ? &*painted : nullptr,
                fresh && !blocked && v.initialized && v.interactive && (v.raw == 42 || v.raw == 43),
                i.pointer, i.pointer_pressed, i.pointer_down, i.pointer_released)
          : WorldFacilityScrollInput{};
    if (blocked || !v.initialized || !v.interactive)
        return {};
    simulation::StartupMagicPotInput f;
    f.up = (v.raw == 42 || v.raw == 43) && i.up;
    f.down = (v.raw == 42 || v.raw == 43) && i.down;
    f.left = (v.raw == 41 || v.raw == 43) && i.left;
    f.right = (v.raw == 41 || v.raw == 43) && i.right;
    // Source43 resolves directions before eligibility; an unknown old row cannot swallow Enter.
    f.confirm = (v.raw <= 43 || v.can_confirm) && i.enter;
    f.cancel = v.can_cancel && i.escape;
    if ((v.raw == 42 || v.raw == 43) && i.wheel_rows)
        f.scroll = simulation::StartupMagicPotInput::Scroll{false, i.wheel_rows};
    if (drag.absolute)
        f.scroll = simulation::StartupMagicPotInput::Scroll{true, *drag.absolute};
    if (fresh && !drag.consume_click) {
        f.confirm |= v.can_confirm && world_facility_hit(i.click, painted->confirm);
        f.cancel |= v.can_cancel && world_facility_hit(i.click, painted->cancel);
        if (v.raw == 44 || v.raw == 45)
            f.confirm |= v.can_confirm && world_facility_hit(i.click, l.panel);
        if (v.raw == 43) {
            f.left |= world_facility_hit(i.click, painted->previous);
            f.right |= world_facility_hit(i.click, painted->next);
        } else if (v.raw == 42) {
            f.up |= world_facility_hit(i.click, painted->previous);
            f.down |= world_facility_hit(i.click, painted->next);
        }
        for (const auto &row : painted->rows)
            if (row.index >= 0 && row.index < int(v.rows.size()) &&
                row.identity == v.rows[row.index].identity &&
                world_facility_hit(i.click, row.rectangle)) {
                f.selection = row.index;
                // 42 alone registers marker16. No timeout or uncommitted selection shortcut.
                if (v.raw == 42) {
                    f.confirm |= g && g->marked_row == row.index && v.selection == row.index;
                    if (g)
                        g->marked_row = row.index;
                } else
                    f.confirm |= v.rows[row.index].known;
                break;
            }
    }
    if (!f.up && !f.down && !f.left && !f.right && !f.confirm && !f.cancel && !f.selection &&
        !f.scroll)
        return {};
    return f;
}
} // namespace ark::desktop::ui
