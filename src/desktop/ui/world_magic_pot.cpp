#include "world_magic_pot.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using Action = simulation::StartupMagicPotAction;
void fitted(const Skin &skin, const std::string &text, Rectangle box, Color color = ink) {
    skin.text.draw(text, box.x, box.y, color,
                   std::min(11.F, 12.F * box.width / std::max(1.F, skin.text.width(text))));
}
const simulation::rules::WorldMagicPotRecipeDefinition &
recipe(const simulation::StartupWorldRuntimeState &state, int id) {
    const auto found =
        std::find_if(state.rules->magic_pot_recipes.begin(), state.rules->magic_pot_recipes.end(),
                     [id](const auto &r) { return r.identity == id; });
    if (found == state.rules->magic_pot_recipes.end())
        throw std::invalid_argument("Magic pot view lost recipe definition");
    return *found;
}
std::string elements(const std::array<std::int32_t, 4> &values) {
    return "火 " + std::to_string(values[0]) + " 冰 " + std::to_string(values[1]) + " 雷 " +
           std::to_string(values[2]) + " 暗 " + std::to_string(values[3]);
}
} // namespace
bool world_magic_pot_page(const simulation::rules::WorldScriptPage &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           page.legacy_page >= 41 && page.legacy_page <= 47;
}
WorldMagicPotView world_magic_pot_view(const simulation::StartupWorldRuntimeState &state,
                                       const simulation::rules::WorldScriptPage &page) {
    if (!state.rules || !world_magic_pot_page(page))
        throw std::invalid_argument("Unsupported magic pot page");
    WorldMagicPotView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = "魔法壶";
    if (!state.magic_pot_pages_initialized.count(page.id))
        return out;
    const auto source = simulation::inspect_startup_world_magic_pot_page(state, page.id);
    if (!source || source->raw != out.raw)
        throw std::invalid_argument("Invalid initialized magic pot payload");
    out.phase = source->phase;
    out.selection = source->selection;
    out.first_visible = source->first_visible;
    out.counter = source->counter;
    out.heading =
        elements({state.legacy_n[3], state.legacy_n[4], state.legacy_n[5], state.legacy_n[6]});
    const auto capacity = simulation::rules::world_magic_pot_capacity(state.legacy_n[11]);
    if (!capacity)
        throw std::invalid_argument("Invalid magic pot level");
    out.status = "等级 " + std::to_string(state.legacy_n[11]) + "  投入 " +
                 std::to_string(state.legacy_n[1]) + "/" + std::to_string(*capacity) + "  经验 " +
                 std::to_string(state.legacy_n[0]);
    out.can_cancel = out.raw != 45;
    out.can_confirm = out.raw <= 43 || (out.raw == 44 && out.counter >= 36) ||
                      (out.raw == 45 && (out.counter < 77 || out.counter >= 83)) ||
                      (out.raw >= 46 && out.counter > 6);
    if (out.raw == 41) {
        out.rows = {{0, "投入道具", ""}, {1, "开发", ""}};
    } else if (out.raw == 42) {
        out.title = "投入道具";
        for (int id : source->entries) {
            const auto item = std::find_if(state.rules->items.begin(), state.rules->items.end(),
                                           [id](const auto &r) { return r.identity == id; });
            if (item == state.rules->items.end())
                throw std::invalid_argument("Magic pot view lost deposit item");
            out.rows.push_back(
                {id, item->name, "持有 " + std::to_string(state.items.at(id).inventory)});
        }
        out.can_confirm = !out.rows.empty();
    } else if (out.raw == 43) {
        out.title = "开发 " + std::to_string(out.phase + 1) + "/2";
        for (int id : source->entries) {
            const auto &r = recipe(state, id);
            const auto &progress = state.magic_pot_recipes.at(id);
            out.rows.push_back({id, progress.status == 0 ? "????" : r.name,
                                out.phase == 0 ? std::to_string(r.experience_required) + "经验"
                                               : elements(r.costs),
                                progress.status != 0, progress.pending_notice});
        }
        out.can_confirm = !out.rows.empty() && out.rows.at(out.selection).known;
    } else if (out.raw == 44 || out.raw == 45) {
        out.title = out.raw == 44 ? "投入结果" : "魔法壶处理";
        out.display = state.magic_pot_display;
        if (out.raw == 44)
            out.comment = state.magic_pot_comment;
    } else {
        const auto &r = recipe(state, source->binding);
        out.title = out.raw == 46 ? "发现配方" : "开发确认";
        out.comment = r.name;
        out.costs = r.costs;
    }
    out.initialized = true;
    return out;
}
std::optional<WorldMagicPotIntent> world_magic_pot_input(const WorldMagicPotView &view,
                                                         const WorldCommerceLayout &layout,
                                                         const WorldCommerceInput &input,
                                                         bool blocked) {
    if (blocked || !view.initialized)
        return {};
    const auto hit = [&](Rectangle box) {
        return input.click && CheckCollisionPointRec(*input.click, box);
    };
    const auto intent = [](Action action, int selection = 0) {
        return WorldMagicPotIntent{action, selection};
    };
    if (view.can_cancel && (input.escape || hit(layout.cancel)))
        return intent(Action::cancel);
    if (view.raw == 43) {
        if (input.left || (view.phase != 0 && hit(layout.tabs[0])))
            return intent(Action::previous_tab);
        if (input.right || (view.phase != 1 && hit(layout.tabs[1])))
            return intent(Action::next_tab);
    }
    if (!view.rows.empty()) {
        if (input.up || input.down)
            return intent(input.up ? Action::previous : Action::next);
        if (input.wheel_rows)
            return intent(Action::select, std::clamp(view.selection + input.wheel_rows, 0,
                                                     static_cast<int>(view.rows.size()) - 1));
        for (int n = 0; n < 5 && n + view.first_visible < static_cast<int>(view.rows.size()); ++n)
            if (hit({layout.rows.x, layout.rows.y + n * layout.row_height, layout.rows.width,
                     layout.row_height}))
                return intent(Action::select, n + view.first_visible);
    }
    if (view.can_confirm && (input.enter || hit(layout.confirm)))
        return intent(Action::confirm);
    return {};
}
void draw_world_magic_pot(const WorldMagicPotView &view, const WorldCommerceLayout &layout,
                          const Skin &skin, bool enabled, const std::string &feedback) {
    skin.window(layout.panel, view.title);
    skin.content(layout.rows);
    const bool active = enabled && view.initialized;
    if (view.initialized) {
        fitted(skin, view.heading, layout.heading, blue);
        if (view.raw == 43) {
            skin.centered("经验", layout.tabs[0], view.phase == 0 ? blue : ink, 10);
            skin.centered("元素", layout.tabs[1], view.phase == 1 ? blue : ink, 10);
        }
        if (!view.rows.empty()) {
            for (int n = 0; n < 5 && n + view.first_visible < static_cast<int>(view.rows.size());
                 ++n) {
                const int index = n + view.first_visible;
                const auto &row = view.rows[index];
                const Rectangle box{layout.rows.x, layout.rows.y + n * layout.row_height,
                                    layout.rows.width, layout.row_height};
                if (index == view.selection)
                    DrawRectangleRec(box, {255, 153, 55, 255});
                fitted(skin, row.name,
                       {box.x + 4, box.y + 1, box.width * .50F - (row.fresh ? 23 : 0), 12},
                       row.known ? ink : GRAY);
                if (row.fresh)
                    skin.sprites.image("wnd_new.png", {0, 0, 20, 9},
                                       {box.x + box.width * .50F - 20, box.y + 2, 20, 9});
                fitted(skin, row.value, {box.x + box.width * .53F, box.y + 1, box.width * .45F, 12},
                       blue);
            }
        } else if (view.raw == 44 || view.raw == 45) {
            constexpr const char *labels[]{"火", "冰", "雷", "暗", "经验"};
            for (int n = 0; n < (view.raw == 44 ? 4 : 5); ++n) {
                const float y = layout.rows.y + n * layout.row_height;
                skin.text.draw(labels[n], layout.rows.x + 4, y, ink, 10);
                skin.right(std::to_string(view.display[0][n]) + " > " +
                               std::to_string(view.display[1][n]),
                           layout.rows.x + layout.rows.width - 4, y, blue, 10);
            }
        } else {
            fitted(skin, view.comment,
                   {layout.rows.x + 4, layout.rows.y + 4, layout.rows.width - 8, 16});
            fitted(skin, elements(view.costs),
                   {layout.rows.x + 4, layout.rows.y + 26, layout.rows.width - 8, 16}, blue);
        }
        fitted(skin, view.status, layout.status, blue);
        if (view.raw == 44)
            fitted(skin, view.comment, layout.feedback);
    }
    if (view.can_cancel)
        skin.button(layout.cancel, "返回", active);
    skin.button(layout.confirm,
                view.raw == 42   ? "投入"
                : view.raw == 47 ? "开发"
                                 : "确定",
                active && view.can_confirm);
    if (!feedback.empty())
        fitted(skin, feedback, layout.feedback, MAROON);
}
} // namespace ark::desktop::ui
