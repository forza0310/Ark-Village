// One PC frame becomes one source transaction. The device stores no list cursor or world state.
#include "world_tasks.hpp"
namespace ark::desktop::ui {
namespace sim = simulation;
std::optional<WorldTaskIntent>
world_task_plan_input(const WorldTaskView &v, const WorldTaskLayout &l, WorldTaskSelection &device,
                      const WorldTaskInput &i, bool blocked,
                      const std::optional<WorldTaskPaintedControls> &painted) {
    auto &g = device.gesture;
    if (g.page != v.page) {
        g = {};
        g.page = v.page;
    }
    if (i.focus_lost) {
        g = {};
        return {};
    }
    const auto *p = painted ? &painted->geometry : nullptr;
    std::vector<std::uint64_t> tasks;
    std::vector<int> humans;
    for (const auto &row : v.rows) {
        if (row.task)
            tasks.push_back(*row.task);
        if (row.human || row.add_member)
            humans.push_back(row.human.value_or(-1));
    }
    const bool fresh = p && p->page == v.page && p->raw == v.raw && p->phase == v.phase &&
                       p->selection == v.selected && p->first == v.first &&
                       p->count == int(v.rows.size()) && painted->tab == v.tab &&
                       painted->task == v.task && painted->tasks == tasks &&
                       painted->humans == humans && painted->reward_ids == v.reward_ids &&
                       p->extent.width == l.viewport.width && p->extent.height == l.viewport.height;
    const bool scrollable =
        v.raw == 22 || v.raw == 25 || v.raw == 26 || v.raw == 27 || v.raw == 31 || v.raw == 32;
    const auto drag = world_facility_scroll_input(
        g, fresh ? p : nullptr, fresh && !blocked && v.initialized && v.interactive && scrollable,
        i.pointer, i.pointer_pressed, i.pointer_down, i.pointer_released);
    if (!v.interactive)
        g.marked_row.reset();
    if (blocked || !v.initialized || !v.interactive || v.raw == 24)
        return {};
    sim::StartupTaskPageInput f;
    f.up = scrollable && i.up;
    f.down = scrollable && i.down;
    f.left = (v.raw == 23 || v.raw == 27 || v.raw == 28 || v.raw == 33) && i.left;
    f.right = (v.raw == 23 || v.raw == 27 || v.raw == 28 || v.raw == 33) && i.right;
    f.confirm = i.enter;
    f.cancel = v.raw != 33 && !(v.raw == 28 && v.animating) && i.escape;
    f.track = v.raw == 22 && i.track;
    f.inspect = v.raw >= 25 && v.raw <= 27 && i.inspect;
    f.depart = v.raw == 25 && i.depart;
    if (scrollable && i.wheel_rows)
        f.scroll = sim::StartupTaskPageInput::Scroll{false, i.wheel_rows};
    if (drag.absolute)
        f.scroll = sim::StartupTaskPageInput::Scroll{true, *drag.absolute};
    if (f.up || f.down || f.left || f.right || f.confirm || f.cancel || f.scroll || f.inspect ||
        f.depart)
        g.marked_row.reset();
    if (fresh && !drag.consume_click) {
        const bool primary = world_facility_hit(i.click, p->confirm);
        f.depart |= v.raw == 25 && primary;
        f.confirm |= v.raw != 25 && primary;
        f.cancel |= v.raw != 33 && world_facility_hit(i.click, p->cancel);
        f.track |= v.raw == 22 && world_facility_hit(i.click, p->inspect);
        f.inspect |= v.raw >= 25 && v.raw <= 27 && world_facility_hit(i.click, p->inspect);
        f.left |= world_facility_hit(i.click, p->previous);
        f.right |= world_facility_hit(i.click, p->next);
        if (v.raw != 32 && !(v.raw == 28 && v.animating) && !(v.raw == 33 && v.animating))
            for (const auto &row : p->rows)
                if (row.index >= 0 && world_facility_hit(i.click, row.rectangle)) {
                    f.selection = row.index;
                    f.confirm |=
                        v.raw != 27 || (v.selected == row.index && g.marked_row == row.index);
                    g.marked_row = v.raw == 27 ? std::optional<int>{row.index} : std::nullopt;
                    break;
                }
    }
    if (!f.up && !f.down && !f.left && !f.right && !f.confirm && !f.cancel && !f.track &&
        !f.inspect && !f.depart && !f.selection && !f.scroll)
        return {};
    using Action = sim::StartupWorldTaskAction;
    const int selected = f.selection.value_or(v.selected);
    Action action = f.cancel && !f.confirm     ? Action::cancel
                    : f.inspect                ? Action::inspect
                    : f.depart                 ? Action::depart
                    : v.raw == 27 && f.confirm ? Action::hire
                                               : Action::confirm;
    if ((v.raw == 25 || v.raw == 26) && f.confirm && !f.depart && selected >= 0 &&
        selected < int(v.rows.size()))
        action = v.rows[selected].add_member ? Action::add_member
                 : v.raw == 25               ? Action::depart
                                             : Action::confirm;
    return WorldTaskIntent{action, selected, f};
}
std::optional<WorldTaskIntent>
world_task_input(const WorldTaskView &v, const WorldTaskLayout &l, WorldTaskSelection &s,
                 const WorldTaskInput &i, bool blocked,
                 const std::optional<WorldTaskPaintedControls> &painted) {
    if (v.raw >= 22)
        return world_task_plan_input(v, l, s, i, blocked, painted);
    if (blocked || !v.initialized)
        return {};
    using Action = sim::StartupWorldTaskAction;
    if (v.abort_question) {
        s.prompt = std::clamp(s.prompt, 0, 1);
        if (i.left || i.right)
            s.prompt = 1 - s.prompt;
        if (world_facility_hit(i.click, l.continue_choice))
            s.prompt = 0;
        if (world_facility_hit(i.click, l.stop_choice))
            s.prompt = 1;
        if (i.enter || world_facility_hit(i.click, l.confirm))
            return WorldTaskIntent{Action::confirm, s.prompt};
        return {}; // The source abort question has no Back branch.
    }
    if (i.escape || world_facility_hit(i.click, l.cancel))
        return WorldTaskIntent{Action::cancel, 0};
    if (i.enter || world_facility_hit(i.click, l.confirm))
        return WorldTaskIntent{Action::request_abort, 0};
    return {};
}
} // namespace ark::desktop::ui
