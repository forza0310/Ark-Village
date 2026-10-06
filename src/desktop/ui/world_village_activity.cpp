#include "world_village_activity.hpp"
#include "ark/simulation/startup_world_human.hpp"
#include "ark/simulation/startup_world_visuals.hpp"
#include "script_text.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupVillageActivityAction;
constexpr const char *attributes[]{"体力", "力量", "灵活", "结实", "魔力", "运气"};
const simulation::StartupWorldActivity &definition(const State &state, int id) {
    const auto found = std::find_if(state.rules->activities.begin(), state.rules->activities.end(),
                                    [&](const auto &a) { return a.identity == id; });
    if (found == state.rules->activities.end())
        throw std::invalid_argument("Village activity view lost its source definition");
    return *found;
}
bool hit(const WorldVillageActivityInput &input, Rectangle box) {
    return input.click && CheckCollisionPointRec(*input.click, box);
}
void fitted(const Skin &skin, const std::string &value, Rectangle box, Color color = ink,
            float size = 11) {
    skin.text.draw(value, box.x, box.y, color,
                   std::min(size, 12.F * box.width / std::max(1.F, skin.text.width(value))));
}
void paragraph(const Skin &skin, const std::string &value, Rectangle box) {
    const auto decoded = decode_script_text(value);
    const auto lines = wrap_plain_text(decoded.text, box.width,
                                       [&](const auto &line) { return skin.text.width(line, 10); });
    const int visible =
        std::min(static_cast<int>(lines.size()), std::max(0, static_cast<int>(box.height / 13)));
    for (int line = 0; line < visible; ++line)
        skin.text.draw(lines[line], box.x, box.y + line * 13.F, ink, 10);
}
} // namespace
bool world_village_activity_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           page.legacy_page >= 51 && page.legacy_page <= 54;
}
WorldVillageActivityView world_village_activity_view(const State &state, const Page &page) {
    if (!state.rules || !world_village_activity_page(page))
        throw std::invalid_argument("Village activity view requires a source management page");
    WorldVillageActivityView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = out.raw == 51   ? "村办活动"
                : out.raw == 52 ? "开展活动"
                : out.raw == 53 ? "活动进行中"
                                : "活动结果";
    const auto source = simulation::inspect_startup_world_village_activity_page(state, page.id);
    if (!source)
        return out;
    // A returned child answer must be consumed by the next real parent update. Reading its
    // old list remains valid, but another click must not bypass that source resume boundary.
    if (out.raw == 51 && state.activity_page_answers.count(page.id))
        return out;
    out.selection = source->selection;
    out.first_visible = source->first_visible;
    out.counter = source->counter;
    out.points = state.village_points;
    out.quarter_slots = state.quarter_counter;
    if (source->activity) {
        const auto &activity = definition(state, *source->activity);
        out.name = activity.name;
        out.detail = activity.detail;
        out.description = activity.description;
        if (out.raw == 54) {
            const int attribute = activity.parameters[3];
            if (activity.parameters[2] == 0)
                out.result_attribute = "满足";
            else if (activity.parameters[2] == 1 && attribute >= 0 && attribute < 6)
                out.result_attribute = attributes[attribute];
            else
                throw std::invalid_argument("Village result references an unsupported effect");
        }
    }
    for (const int entry : source->entries) {
        WorldVillageActivityRow row;
        row.definition = entry;
        if (out.raw == 51) {
            const auto &activity = definition(state, entry);
            row.name = activity.name;
            row.points = activity.parameters[4];
            row.kind = activity.parameters[2];
            row.supported = row.kind >= 0 && row.kind <= 2;
            row.fresh = state.scripts.activities.at(entry).pending_notice;
        } else {
            const auto human = simulation::startup_world_human_details(state, entry);
            const auto name = std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                                           [&](const auto &h) { return h.identity == entry; });
            if (!human || name == state.rules->humans.end() || !source->activity)
                throw std::invalid_argument("Village result lost its frozen human reference");
            const auto &activity = definition(state, *source->activity);
            row.name = name->name;
            row.before = state.human_activity_previous.at(entry);
            row.current = activity.parameters[2] == 0
                              ? human->satisfaction
                              : human->attributes.at(activity.parameters[3]);
            row.supported = true;
        }
        out.rows.push_back(std::move(row));
    }
    if (out.raw == 51 || out.raw == 54)
        for (std::size_t n = 0; n < source->display_humans.size(); ++n) {
            const int human = source->display_humans[n];
            if (state.human_presence.at(human) == 0)
                continue;
            const auto portrait = simulation::startup_world_portrait(state, human);
            if (!portrait)
                throw std::invalid_argument("Village display lost its current profession portrait");
            out.portrait_images[n] = portrait->image;
        }
    out.can_cancel = out.raw != 53;
    out.can_confirm = out.raw == 53   ? out.counter >= 120
                      : out.raw == 51 ? !out.rows.empty() && out.rows.at(out.selection).supported
                                      : true;
    if (out.raw == 51 && !out.rows.empty() && !out.rows.at(out.selection).supported)
        out.status = "此活动尚未接入";
    out.initialized = true;
    return out;
}
WorldVillageActivityLayout world_village_activity_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Village activity requires supported logical viewport");
    const float width = std::min(340.F, extent.width - 16.F);
    const float height = std::min(296.F, extent.height - 60.F);
    WorldVillageActivityLayout out;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.heading = {p.x + 10, p.y + 28, width - 20, 14};
    out.rows = {p.x + 10, p.y + 44, width - 20, height - 111};
    out.row_height = out.rows.height / 5;
    for (int n = 0; n < 2; ++n) {
        out.choices[n] = {out.rows.x, out.rows.y + n * 24, out.rows.width, 22};
        out.portraits[n] = {p.x + width / 2 - 49 + n * 66, p.y + height - 54, 32, 18};
    }
    out.description = {out.rows.x + 3, out.rows.y + 52, out.rows.width - 6, out.rows.height - 52};
    out.phase_label = {p.x + 10, p.y + height - 91, width - 20, 12};
    out.status = {p.x + 10, p.y + height - 67, width - 20, 12};
    out.feedback = {p.x + 10, p.y + height - 35, width - 20, 10};
    out.cancel = {p.x + 10, p.y + height - 24, 48, 20};
    out.confirm = {p.x + width - 58, p.y + height - 24, 48, 20};
    return out;
}
std::optional<WorldVillageActivityIntent>
world_village_activity_input(const WorldVillageActivityView &view,
                             const WorldVillageActivityLayout &layout,
                             const WorldVillageActivityInput &input, bool blocked) {
    if (blocked || !view.initialized)
        return {};
    const auto intent = [&](Action action, int selection = 0) {
        return WorldVillageActivityIntent{action, view.page, selection};
    };
    if (view.can_cancel && (input.escape || hit(input, layout.cancel)))
        return intent(Action::cancel);
    const int count = view.raw == 52 ? 2 : static_cast<int>(view.rows.size());
    if (view.raw != 53 && count > 0) {
        if (input.up || input.down)
            return intent(input.up ? Action::previous : Action::next);
        if (input.wheel_rows)
            return intent(Action::select,
                          std::clamp(view.selection + input.wheel_rows, 0, count - 1));
        if (view.raw == 52) {
            for (int n = 0; n < 2; ++n)
                if (hit(input, layout.choices[n]))
                    return intent(Action::select, n);
        } else
            for (int row = 0; row < 5 && view.first_visible + row < count; ++row)
                if (hit(input, {layout.rows.x, layout.rows.y + row * layout.row_height,
                                layout.rows.width, layout.row_height}))
                    return intent(Action::select, view.first_visible + row);
    }
    if (view.can_confirm && (input.enter || hit(input, layout.confirm)))
        return intent(Action::confirm);
    return {};
}
void draw_world_village_activity(const WorldVillageActivityView &view,
                                 const WorldVillageActivityLayout &layout, const Skin &skin,
                                 bool enabled, const std::string &feedback) {
    skin.window(layout.panel, view.title);
    skin.content(layout.rows);
    if (view.initialized) {
        fitted(skin,
               view.raw == 51 ? "村子点 " + std::to_string(view.points) + "  季度剩余 " +
                                    std::to_string(view.quarter_slots)
                              : view.name,
               layout.heading, blue, 10);
        if (view.raw == 51 || view.raw == 54) {
            for (int n = view.first_visible;
                 n < static_cast<int>(view.rows.size()) && n < view.first_visible + 5; ++n) {
                const auto &row = view.rows[n];
                Rectangle box{layout.rows.x,
                              layout.rows.y + (n - view.first_visible) * layout.row_height,
                              layout.rows.width, layout.row_height};
                if (n == view.selection)
                    DrawRectangleRec(box, {255, 236, 174, 255});
                const float name_width = box.width * .62F;
                fitted(skin, (row.fresh ? "* " : "") + row.name,
                       {box.x + 3, box.y + 2, name_width - 6, box.height},
                       row.supported ? ink : GRAY, 10);
                const auto value = view.raw == 51 ? std::to_string(row.points) + "点"
                                                  : std::to_string(row.before) + " > " +
                                                        std::to_string(row.current);
                fitted(skin, value,
                       {box.x + name_width, box.y + 2, box.width - name_width - 3, box.height},
                       blue, 10);
            }
            for (std::size_t n = 0; n < view.portrait_images.size(); ++n)
                if (view.portrait_images[n])
                    skin.sprites.actor_thumbnail(false, 0, *view.portrait_images[n],
                                                 layout.portraits[n]);
            fitted(skin, view.raw == 54 ? view.result_attribute : view.status, layout.status,
                   view.raw == 54 ? ink : MAROON, 10);
        } else if (view.raw == 52) {
            for (int n = 0; n < 2; ++n) {
                if (n == view.selection)
                    DrawRectangleRec(layout.choices[n], {255, 236, 174, 255});
                fitted(skin, n == 0 ? "开展活动" : "返回",
                       {layout.choices[n].x + 3, layout.choices[n].y + 4,
                        layout.choices[n].width - 6, 14});
            }
            paragraph(skin, view.detail, layout.description);
        } else {
            paragraph(skin, view.description,
                      {layout.rows.x + 3, layout.rows.y + 3, layout.rows.width - 6,
                       layout.phase_label.y - layout.rows.y - 8});
            skin.centered(view.counter < 120 ? "活动进行中" : "活动完成", layout.phase_label, blue,
                          10);
        }
    }
    if (view.can_cancel)
        skin.button(layout.cancel, "返回", enabled && view.initialized);
    skin.button(layout.confirm, view.raw == 54 ? "关闭" : "确定",
                enabled && view.initialized && view.can_confirm);
    if (!feedback.empty())
        fitted(skin, feedback, layout.feedback, MAROON, 10);
}
} // namespace ark::desktop::ui
