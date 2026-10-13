// Selection/recruitment consumes ordered research plans. The GPU path owns no task or clock.
#include "../common/steam_plan_draw.hpp"
#include "world_tasks.hpp"
#include <type_traits>

namespace ark::desktop::ui {
void prepare_world_task(WorldTaskView &v, const simulation::StartupWorldRuntimeState &s,
                        const Skin &skin, bool scrolling) {
    v.skin.reset();
    v.followup.reset();
    if ((v.raw >= 25 && v.raw <= 28) || (v.raw >= 31 && v.raw <= 33)) {
        prepare_world_task_followup(v, s, skin, scrolling);
        return;
    }
    if (v.raw < 22 || v.raw > 24 || !v.initialized)
        return;
    simulation::SteamTaskSelectionOptions o;
    o.view_y = 23;
    o.scroll_first_touch = scrolling;
    const std::string title = v.raw == 22 ? "任务选择" : "寻求！冒险者";
    o.title_widths = std::array<int, 2>{int(skin.text.width(title)), int(skin.text.width(title))};
    o.bottom_width = int(skin.text.width(v.raw == 22 ? "请选择任务" : "募集冒险者"));
    constexpr const char *phrases[]{"交给我!",     "多指教",    "来了!",
                                    "我会加油的!", "我要参加!", "好期待"};
    for (int i = 0; i < 6; ++i)
        o.phrase_widths[i] = int(skin.text.width(phrases[i]));
    v.skin = simulation::steam_task_selection_skin(s, v.page, o);
    if (!v.skin)
        throw std::invalid_argument("Invalid bound task selection/recruitment plan");
}
std::optional<WorldTaskPaintedControls> draw_world_task_selection(const WorldTaskView &v,
                                                                  const WorldTaskLayout &l,
                                                                  const Skin &skin, bool enabled,
                                                                  const std::string &feedback) {
    if (!v.initialized) {
        skin.window(l.panel, v.title);
        return {};
    }
    if (!v.skin)
        throw std::invalid_argument("Task rendering requires measured source plan");
    WorldTaskPaintedControls out;
    out.geometry = draw_world_task_plan(*v.skin, world_list_skin_origin(l.panel), skin);
    auto &c = out.geometry;
    c.page = v.page;
    c.raw = v.raw;
    c.selection = v.selected;
    c.phase = v.phase;
    out.task = v.task;
    out.tab = v.tab;
    c.first = v.first;
    c.count = int(v.rows.size());
    c.extent = l.viewport;
    for (const auto &row : v.rows)
        if (row.task)
            out.tasks.push_back(*row.task);
    if (v.raw != 24) {
        c.cancel = l.cancel;
        skin.choice(l.cancel, "返回", enabled);
    }
    if (v.raw == 22) {
        c.confirm = l.confirm;
        c.inspect = l.inspect;
        skin.choice(l.confirm, "选择", enabled);
        skin.choice(l.inspect, "追踪", enabled);
    } else if (v.raw == 24) {
        c.confirm = l.confirm;
        skin.choice(l.confirm, "加速", enabled);
    }
    if (!feedback.empty())
        skin.text.draw(feedback, l.cancel.x - 170, l.cancel.y + 22, MAROON, 9);
    return out;
}
} // namespace ark::desktop::ui
