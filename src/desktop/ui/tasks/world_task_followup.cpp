// Measure and execute source-bound rosters/results/deadline plans; never initialize a page.
#include "../common/skin.hpp"
#include "world_tasks.hpp"
#include <algorithm>
#include <stdexcept>
namespace ark::desktop::ui {
namespace sim = simulation;
void prepare_world_task_followup(WorldTaskView &v, const sim::StartupWorldRuntimeState &s,
                                 const Skin &skin, bool scrolling) {
    if (!v.initialized)
        return;
    const auto width = [&](const std::string &text) { return int(skin.text.width(text)); };
    if (v.raw >= 25 && v.raw <= 28) {
        sim::SteamTaskRosterOptions o;
        o.view_y = 23;
        o.scroll_first_touch = scrolling;
        const std::string title = v.raw <= 26   ? "参加者名单"
                                  : v.raw == 27 ? "参加委托 " + std::to_string(v.tab + 1) + "/2"
                                                : "任务开始";
        o.title_widths = std::array<int, 2>{width(title), width(title)};
        if (v.raw == 28) {
            constexpr const char *labels[]{"有些困难吧",     "有点担心",   "这些人真是了不起",
                                           "我觉得会好的。", "值得期待！", "能轻松通关！",
                                           "他们能做到!"};
            if (!v.prediction || *v.prediction < 0 || *v.prediction > 6)
                throw std::invalid_argument("Invalid departure prediction");
            o.prediction_width = width(labels[*v.prediction]);
            o.button_widths = {width("立即开始"), width("重新考虑")};
        }
        const auto plan = sim::steam_task_roster_skin(s, v.page, o);
        if (!plan)
            throw std::invalid_argument("Invalid bound task roster plan");
        v.followup = *plan;
    } else if (v.raw == 31 || v.raw == 32) {
        auto in = sim::steam_task_result_input(s, v.page);
        if (!in)
            throw std::invalid_argument("Invalid bound task result payload");
        auto &o = in->options;
        o.view_y = 23;
        o.scroll_first_touch = scrolling;
        o.title_widths = std::array<int, 2>{width("成果"), width("成果")};
        o.bottom_width = width(v.raw == 31 ? in->task_name + "完成!" : "获得了宝物!");
        const auto plan = sim::steam_task_result_skin(*in);
        if (!plan)
            throw std::invalid_argument("Invalid measured task result plan");
        v.followup = *plan;
    } else if (v.raw == 33) {
        auto in = sim::steam_task_deadline_input(s, v.page);
        if (!in)
            throw std::invalid_argument("Invalid bound task deadline payload");
        auto &o = in->options;
        o.view_y = 23;
        o.title_widths = std::array<int, 2>{width("任务继续确认"), width("任务继续确认")};
        std::int64_t downs = 0;
        for (int id : in->participants) {
            const auto h = std::find_if(in->humans.begin(), in->humans.end(),
                                        [=](const auto &value) { return value.identity == id; });
            if (h == in->humans.end() || h->downs < 0)
                throw std::invalid_argument("Invalid deadline participant down count");
            downs += h->downs;
        }
        constexpr const char *labels[]{"再加把劲!!", "再加加油!!", "需要补充战力!",
                                       "重新来过比较好", "应该撤退…"};
        if (in->grade < 0 || in->grade > 4)
            throw std::invalid_argument("Invalid deadline grade");
        o.down_number_width = width(std::to_string(downs));
        o.prediction_width = width(labels[in->grade]);
        o.bubble_width = width(in->answer.value_or(0) == 0 ? "加油哦!" : "遗憾…");
        o.button_widths = {width("继续"), width("中止")};
        const auto plan = sim::steam_task_deadline_skin(*in);
        if (!plan)
            throw std::invalid_argument("Invalid measured task deadline plan");
        v.followup = *plan;
    }
}
std::optional<WorldTaskPaintedControls> draw_world_task_followup(const WorldTaskView &v,
                                                                 const WorldTaskLayout &l,
                                                                 const Skin &skin, bool enabled,
                                                                 const std::string &feedback) {
    if (!v.initialized) {
        skin.window(l.panel, v.title);
        return {};
    }
    if (!v.followup)
        throw std::invalid_argument("Task followup rendering requires measured plan");
    WorldTaskPaintedControls out;
    out.geometry = std::visit(
        [&](const auto &plan) {
            return draw_world_task_plan(plan, world_list_skin_origin(l.panel), skin);
        },
        *v.followup);
    auto &c = out.geometry;
    c.page = v.page;
    c.raw = v.raw;
    c.phase = v.phase;
    c.selection = v.selected;
    c.first = v.first;
    c.count = int(v.rows.size());
    c.extent = l.viewport;
    out.task = v.task;
    out.tab = v.tab;
    out.reward_ids = v.reward_ids;
    for (const auto &row : v.rows)
        if (row.human || row.add_member)
            out.humans.push_back(row.human.value_or(-1));
    // Original margin/focus processing is platform-specific; PC hits are clipped to the frame.
    for (auto &row : c.rows) {
        const float left = std::max(l.panel.x, row.rectangle.x);
        const float right =
            std::min(l.panel.x + l.panel.width, row.rectangle.x + row.rectangle.width);
        const float top = std::max(l.panel.y, row.rectangle.y);
        const float bottom =
            std::min(l.panel.y + l.panel.height, row.rectangle.y + row.rectangle.height);
        row.rectangle.x = left;
        row.rectangle.width = std::max(0.F, right - left);
        row.rectangle.y = top;
        row.rectangle.height = std::max(0.F, bottom - top);
    }
    if (v.raw <= 27) {
        c.cancel = l.cancel;
        c.confirm = l.confirm;
        c.inspect = l.inspect;
        skin.choice(l.cancel, "返回", enabled);
        skin.choice(l.confirm, v.raw == 25 ? "出发" : v.raw == 27 ? "参加" : "确定", enabled);
        skin.choice(l.inspect, "详情", enabled);
    } else if (v.raw == 28) {
        if (!v.animating) {
            c.cancel = l.cancel;
            skin.choice(l.cancel, "返回", enabled);
        } else {
            c.confirm = l.confirm;
            skin.choice(l.confirm, "加速", enabled);
        }
    } else if (v.raw == 31 || v.raw == 32) {
        c.confirm = l.confirm;
        c.cancel = l.cancel;
        skin.choice(l.confirm, "确定", enabled);
        skin.choice(l.cancel, "返回", enabled);
    } else if (v.raw == 33 && v.animating) {
        c.confirm = l.confirm;
        skin.choice(l.confirm, "确定", enabled);
    }
    if (!feedback.empty())
        skin.text.draw(feedback, l.panel.x, l.panel.y + l.panel.height + 24, MAROON, 9);
    return out;
}
} // namespace ark::desktop::ui
