// Pointer geometry is derived from the last painted plan. The Owner applies one source
// input frame, including simultaneous directions/selection/confirmation, atomically.
#include "world_human.hpp"
#include <algorithm>
#include <cmath>

namespace ark::desktop::ui {
namespace {
using Action = simulation::StartupHumanPageAction;
bool hit(std::optional<Vector2> p, Rectangle r) {
    return p && r.width > 0 && r.height > 0 && p->x >= r.x && p->y >= r.y &&
           p->x < r.x + r.width && p->y < r.y + r.height;
}
bool current(const WorldHumanView &v, const WorldHumanPaintedControls &p, Extent e) {
    if (p.page != v.page || p.raw != v.raw || p.phase != v.phase ||
        p.selection != v.selection || p.human != v.human ||
        p.choice != (v.choice ? v.choice->identity : -1) ||
        p.extent.width != e.width || p.extent.height != e.height ||
        p.first != v.first_visible || p.count != int(v.rows.size()))
        return false;
    for (const auto &row : p.rows)
        if (v.raw != 60 && (row.index < 0 || row.index >= int(v.rows.size()) ||
                           v.rows[row.index].identity != row.identity))
            return false;
    return true;
}
} // namespace

std::optional<WorldHumanIntent>
world_human_input(const WorldHumanView &v, const WorldHumanLayout &base,
                  const WorldHumanInput &i, bool blocked,
                  const std::optional<WorldHumanPaintedControls> &painted, Extent extent,
                  WorldHumanGesture *gesture) {
    const auto layout = v.raw == 60 ? world_human_detail_layout(base)
                        : world_human_source_child(v) ? world_human_subwindow_layout(base, v.raw)
                                                     : base;
    const bool keys = i.enter || i.escape || i.up || i.down || i.left || i.right ||
                      i.wheel_rows || i.professions || i.gifts || i.inspect;
    if (gesture && (keys || gesture->page != v.page || gesture->raw != v.raw ||
                    (gesture->phase != v.phase && !gesture->scrolling) || gesture->human != v.human)) {
        *gesture = {v.page, v.raw, v.phase, v.human};
    }
    if (gesture && blocked && (i.pointer_released || !i.pointer))
        gesture->scrolling = false; // A released button cannot remain captured behind a FIFO gate.
    if (blocked || !v.initialized)
        return {};
    const bool fresh = painted && current(v, *painted, extent);
    if (!fresh && gesture)
        gesture->scrolling = false;
    const auto intent = [&](Action action, int selection = 0) {
        return WorldHumanIntent{action, v.page, selection, {}};
    };
    if (v.raw == 60 || v.raw == 61 || v.raw == 64) {
        simulation::StartupHumanCatalogueInput frame;
        frame.up = i.up || (v.raw == 60 && i.left);
        frame.down = i.down || (v.raw == 60 && i.right);
        frame.left = v.raw != 60 && i.left;
        frame.right = v.raw != 60 && i.right;
        // Confirmation follows navigation in the source consumer, so an empty old tab
        // must not suppress a confirm that targets the new tab in this same frame.
        frame.confirm = i.enter;
        frame.cancel = i.escape && v.can_cancel;
        frame.inspect_equipment = v.raw == 64 && v.phase != 4 && i.inspect;
        if (i.wheel_rows)
            frame.scroll = simulation::StartupHumanCatalogueInput::Scroll{false, i.wheel_rows};
        if (v.raw == 60) {
            if (v.can_track && hit(i.click, layout.track))
                return intent(Action::track);
            if (i.professions || hit(i.click, layout.professions))
                return intent(Action::professions);
            if (i.gifts || hit(i.click, layout.gifts))
                return intent(Action::gifts);
            if (i.inspect && v.phase == 2)
                return intent(Action::inspect_equipment, v.selection);
            for (int tab = 0; tab < 4; ++tab)
                if (hit(i.click, layout.tabs[tab]))
                    frame.scroll = simulation::StartupHumanCatalogueInput::Scroll{true, tab};
            frame.cancel = frame.cancel || hit(i.click, layout.cancel);
            if (v.phase <= 1 && hit(i.click, layout.primary))
                frame.confirm = true;
            // Existing PC equipment-row selection remains separate from Enter/gift opening.
            if (v.phase == 2 && fresh)
                for (const auto &row : painted->rows)
                    if (hit(i.click, row.rectangle))
                        frame.selection = row.index;
        } else if (fresh) {
            frame.cancel = frame.cancel || hit(i.click, painted->back);
            frame.confirm = frame.confirm || (v.can_confirm && hit(i.click, painted->primary.value_or(Rectangle{})));
            frame.inspect_equipment = frame.inspect_equipment || hit(i.click, painted->inspect.value_or(Rectangle{}));
            frame.left = frame.left || hit(i.click, painted->previous.value_or(Rectangle{}));
            frame.right = frame.right || hit(i.click, painted->next.value_or(Rectangle{}));
            for (const auto &row : painted->rows)
                if (hit(i.click, row.rectangle)) {
                    frame.selection = row.index;
                    const bool ordinary = v.raw == 64 && v.phase == 4;
                    frame.confirm = !ordinary || (gesture && v.selection == row.index &&
                                                  gesture->marked_identity == row.identity);
                    if (gesture)
                        gesture->marked_identity = ordinary ? row.identity : -1;
                }
        }
        if (fresh && painted->scroll && gesture) {
            const auto track = *painted->scroll;
            if (i.pointer_pressed && hit(i.pointer, track)) {
                gesture->scrolling = true;
                gesture->marked_identity = -1;
            }
            if (gesture->scrolling && i.pointer && (i.pointer_down || i.pointer_released)) {
                const int count = painted->scroll_count, visible = painted->scroll_visible;
                if (count > visible && visible > 0 && track.height > 0) {
                    const float thumb = track.height * visible / count;
                    const float fraction = std::clamp((i.pointer->y - track.y - thumb / 2) /
                                                      (track.height - thumb), 0.F, 1.F);
                    frame.scroll = simulation::StartupHumanCatalogueInput::Scroll{
                        true, int(std::lround(fraction * (count - visible)))};
                }
            }
            if (i.pointer_released || !i.pointer)
                gesture->scrolling = false;
        }
        if (v.raw == 60 && v.phase == 3 && !frame.up && !frame.down && !frame.scroll)
            frame.confirm = false; // The unchanged magic page has no confirm action.
        if (!frame.up && !frame.down && !frame.left && !frame.right && !frame.confirm &&
            !frame.cancel && !frame.selection && !frame.scroll && !frame.inspect_equipment)
            return {};
        if (v.raw == 64 && (frame.left || frame.right) && gesture)
            gesture->marked_identity = -1;
        auto out = intent(frame.confirm ? (v.raw == 60 ? (v.phase == 1 ? Action::professions : Action::gifts) : Action::confirm)
                          : frame.cancel ? Action::cancel : frame.selection ? Action::select
                          : frame.up || frame.left ? Action::previous : Action::next,
                          frame.selection.value_or(0));
        out.input = frame;
        return out;
    }
    if (world_human_source_child(v)) {
        if (v.can_cancel && (i.escape || (fresh && (hit(i.click, painted->back) || hit(i.click, painted->other_back)))))
            return intent(Action::cancel);
        if ((v.raw == 73 || v.raw == 62) && !v.rows.empty()) {
            if (i.left || (fresh && hit(i.click, painted->previous.value_or(Rectangle{}))))
                return intent(Action::previous);
            if (i.right || (fresh && hit(i.click, painted->next.value_or(Rectangle{}))))
                return intent(Action::next);
        }
        if (v.can_confirm && (i.enter || (fresh && hit(i.click, painted->primary.value_or(Rectangle{})))))
            return intent(Action::confirm);
        return {};
    }
    if (v.can_cancel && (i.escape || hit(i.click, layout.cancel)))
        return intent(Action::cancel);
    if (v.raw == 70 && v.phase == 2) {
        if (i.up || i.down || i.left || i.right)
            return intent(Action::next);
        for (int row = 0; row < 2; ++row)
            if (hit(i.click, {layout.body.x, layout.body.y + (row + 2) * layout.body.height / 4,
                              layout.body.width, layout.body.height / 4}))
                return intent(Action::select, row);
    }
    if (v.can_confirm && (i.enter || hit(i.click, layout.confirm)))
        return intent(Action::confirm);
    return {};
}
} // namespace ark::desktop::ui
