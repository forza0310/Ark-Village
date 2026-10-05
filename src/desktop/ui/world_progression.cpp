#include "world_progression.hpp"
#include "ark/simulation/rules/world_calendar_tasks.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
bool hit(std::optional<Vector2> p, Rectangle r) {
    return p && p->x >= r.x && p->y >= r.y && p->x < r.x + r.width && p->y < r.y + r.height;
}
std::string human_name(const State &state, int id) {
    const auto found = std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                                    [id](const auto &h) { return h.identity == id; });
    if (found == state.rules->humans.end())
        throw std::invalid_argument("Progression page references an unknown human definition");
    return found->name;
}
} // namespace
bool world_progression_page(const Page &page) {
    if (page.kind != simulation::rules::WorldScriptPageKind::raw_page)
        return false;
    const int raw = page.legacy_page;
    return raw == 48 || raw == 49 || raw == 50 || raw == 67 || raw == 88 || raw == 96;
}
WorldProgressionView world_progression_view(const State &state, const Page &page) {
    if (!state.rules || !world_progression_page(page))
        throw std::invalid_argument("Progression view requires source data and supported page");
    WorldProgressionView view;
    view.raw = page.legacy_page;
    view.rank = state.rank;
    view.title = view.raw == 48   ? "晋级申请"
                 : view.raw == 49 ? "城镇等级"
                 : view.raw == 50 ? "晋级庆典"
                 : view.raw == 67 ? "能力上升"
                 : view.raw == 88 ? "勋章授予"
                                  : "自宅完成";
    const auto counter = state.page_counters.find(page.id);
    if (counter == state.page_counters.end())
        return view;
    view.counter = counter->second;
    const auto phase = state.page_phases.find(page.id);
    if (phase != state.page_phases.end())
        view.phase = phase->second;
    if (view.raw == 48 || view.raw == 49) {
        if (state.rank >= 5)
            return view; // Owner replaces the page with the source maximum-rank message.
        if (view.raw == 48)
            view.rows.push_back("晋级申请");
        static constexpr const char *labels[]{"月收入",     "设施数",   "居住数",    "指定建设",
                                              "任务完成数", "街道人气", "举办活动数"};
        const auto terms = simulation::rules::fixed_calendar_task_rank_terms().at(state.rank);
        for (std::size_t n = 0; n < terms.size(); ++n)
            view.rows.push_back(std::string(labels[terms[n].type]) + " " +
                                std::to_string(state.rank_values[n]) +
                                (state.rank_met[n] ? " 达成" : " 未达成"));
        view.confirm_enabled = true;
    } else if (view.raw == 50) {
        const auto cast = state.rank_celebration_participants.find(page.id);
        if (cast == state.rank_celebration_participants.end() || phase == state.page_phases.end())
            return view;
        for (const auto &actor : cast->second)
            view.rows.push_back(human_name(state, actor[0]));
        view.confirm_enabled = view.phase == 2 && view.counter >= 140;
    } else {
        const auto binding = state.page_human_bindings.find(page.id);
        if (binding == state.page_human_bindings.end())
            return view;
        view.human = human_name(state, binding->second);
        if (view.raw == 67) {
            static constexpr const char *stats[]{"HP", "攻击", "防御", "魔法"};
            for (std::size_t n = 0; n < 4; ++n)
                view.rows.push_back(std::string(stats[n]) + " " +
                                    std::to_string(state.effort_display[0][n]) + " > " +
                                    std::to_string(state.effort_display[1][n]));
            view.confirm_enabled = view.counter < 67 || view.counter >= 73;
        } else {
            if (phase == state.page_phases.end())
                return view;
            for (std::size_t n = 0; n < 2; ++n)
                view.rows.push_back(std::string(n == 0 ? "满足 " : "努力 ") +
                                    std::to_string(state.reward_display[0][n]) + " > " +
                                    std::to_string(state.reward_display[1][n]));
            view.confirm_enabled = view.raw == 96 || view.phase == 1;
        }
    }
    view.initialized = true;
    return view;
}
WorldProgressionLayout world_progression_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Progression page requires the supported logical viewport");
    const float width = std::min(310.F, extent.width - 16.F);
    const float height = std::min(250.F, extent.height - 68.F);
    WorldProgressionLayout out;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.body = {p.x + 12, p.y + 52, width - 24, height - 87};
    out.cancel = {p.x + 12, p.y + height - 28, 58, 20};
    out.confirm = {p.x + width - 70, p.y + height - 28, 58, 20};
    return out;
}
std::optional<WorldProgressionIntent>
world_progression_input(const WorldProgressionView &view, const WorldProgressionLayout &layout,
                        int &selection, const WorldProgressionInput &input, bool blocked) {
    if (blocked || !view.initialized)
        return {};
    if (view.raw == 48) {
        const int count = static_cast<int>(view.rows.size());
        const float pitch = std::min(22.F, layout.body.height / 5);
        if (!count)
            return {};
        selection = std::clamp(selection, 0, count - 1);
        if (input.up)
            selection = (selection + count - 1) % count;
        if (input.down)
            selection = (selection + 1) % count;
        for (int n = 0; n < count; ++n)
            if (hit(input.click,
                    {layout.body.x, layout.body.y + n * pitch, layout.body.width, pitch}))
                selection = n;
        if (input.escape || hit(input.click, layout.cancel))
            return WorldProgressionIntent{true, true, selection};
    }
    if (view.confirm_enabled && (input.enter || hit(input.click, layout.confirm)))
        return WorldProgressionIntent{view.raw == 48, false, view.raw == 48 ? selection : 0};
    return {};
}
void draw_world_progression(const WorldProgressionView &view, const WorldProgressionLayout &layout,
                            const Skin &skin, int selection, bool enabled) {
    skin.window(layout.panel, view.title);
    skin.content(
        {layout.body.x - 5, layout.body.y - 5, layout.body.width + 10, layout.body.height + 10});
    const auto headline = view.raw == 48 || view.raw == 49 || view.raw == 50
                              ? "城镇等级 " + std::to_string(view.rank)
                              : view.human;
    skin.text.draw(headline, layout.panel.x + 12, layout.panel.y + 28);
    const float pitch = std::min(22.F, layout.body.height / 5);
    for (std::size_t n = 0; n < view.rows.size(); ++n) {
        const bool cast = view.raw == 50;
        Rectangle row{layout.body.x + (cast ? n % 2 * layout.body.width / 2 : 0),
                      layout.body.y + (cast ? n / 2 : n) * pitch,
                      cast ? layout.body.width / 2 : layout.body.width, pitch - 1};
        if (view.raw == 48 && static_cast<int>(n) == selection)
            DrawRectangleRec(row, Color{219, 232, 204, 255});
        const float size =
            std::min(12.F, 12.F * (row.width - 6) / std::max(1.F, skin.text.width(view.rows[n])));
        skin.text.draw(view.rows[n], row.x + 3, row.y + 3, ink, size);
    }
    if (view.raw == 48)
        skin.button(layout.cancel, "返回", enabled && view.initialized);
    skin.button(layout.confirm, "确定", enabled && view.initialized && view.confirm_enabled);
}
} // namespace ark::desktop::ui
