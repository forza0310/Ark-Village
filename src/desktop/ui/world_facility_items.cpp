#include "world_facility_items.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupFacilityItemAction;
bool hit(const WorldFacilityItemsInput &input, Rectangle box) {
    return input.click && CheckCollisionPointRec(*input.click, box);
}
void fitted(const Skin &skin, const std::string &value, Rectangle box, Color color = ink) {
    skin.text.draw(value, box.x, box.y, color,
                   std::min(11.F, 12.F * box.width / std::max(1.F, skin.text.width(value))));
}
WorldFacilityItemRow item(const State &state, int id) {
    const auto source = std::find_if(state.rules->items.begin(), state.rules->items.end(),
                                     [id](const auto &d) { return d.identity == id; });
    if (source == state.rules->items.end())
        throw std::invalid_argument("Facility item view lost its item definition");
    WorldFacilityItemRow row;
    row.identity = id;
    row.name = source->name;
    row.owned = state.items.at(id).inventory;
    for (int n = 0; n < 3; ++n) {
        const int value = source->facility_improvements[n];
        row.hint[n] = value == 0                   ? 0
                      : value < (n == 0 ? 50 : 3)  ? 1
                      : value < (n == 0 ? 100 : 6) ? 2
                                                   : 3;
    }
    return row;
}
} // namespace
bool world_facility_items_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           page.legacy_page >= 75 && page.legacy_page <= 77;
}
WorldFacilityItemsView world_facility_items_view(const State &state, const Page &page) {
    if (!state.rules || !world_facility_items_page(page))
        throw std::invalid_argument("Facility item view requires a supported source page");
    WorldFacilityItemsView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = out.raw == 75 ? "使用道具" : "设施强化";
    if (!state.facility_item_pages_initialized.count(page.id))
        return out;
    if (!simulation::valid_startup_world_facility_item_page(state, page))
        throw std::invalid_argument("Facility item view has invalid initialized source payload");
    out.facility = state.facility_page_bindings.at(page.id);
    const auto &instance = state.scene.world.world.facilities.at(out.facility);
    const auto definition =
        std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                     [&](const auto &d) { return d.id == page.legacy_f; });
    if (definition == state.rules->facilities.end())
        throw std::invalid_argument("Facility item view lost its facility definition");
    out.name = definition->name;
    out.graphic = world_build_graphic(*definition, instance.placement.orientation);
    out.counter = state.page_counters.at(page.id);
    out.response = state.facility_item_response;
    if (out.raw == 75) {
        out.selection = state.facility_item_page_selections.at(page.id);
        for (const int id : state.facility_item_page_lists.at(page.id))
            out.rows.push_back(item(state, id));
        out.first_visible =
            std::clamp(out.selection - 4, 0, std::max(0, static_cast<int>(out.rows.size()) - 5));
        out.choice = out.rows.at(out.selection);
    } else {
        out.choice = item(state, state.facility_item_page_items.at(page.id));
        out.attributes = state.facility_upgrade_display;
    }
    out.can_cancel = out.raw == 75;
    out.can_confirm = out.raw == 75 || (out.raw == 77 && (out.counter < 49 || out.counter >= 55));
    out.initialized = true;
    return out;
}
WorldFacilityItemsLayout world_facility_items_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Facility item page requires supported logical viewport");
    WorldFacilityItemsLayout out;
    const float width = std::min(320.F, extent.width - 16.F);
    const float height = std::min(270.F, extent.height - 58.F);
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.heading = {p.x + 10, p.y + 27, width - 20, 14};
    out.rows = {p.x + 10, p.y + 47, width - 20, height - 118};
    out.row_height = out.rows.height / 5;
    out.hints = {p.x + 10, p.y + height - 65, width - 20, 14};
    out.feedback = {p.x + 10, p.y + height - 43, width - 20, 12};
    out.cancel = {p.x + 10, p.y + height - 26, 48, 20};
    out.confirm = {p.x + width - 58, p.y + height - 26, 48, 20};
    return out;
}
std::optional<WorldFacilityItemsIntent>
world_facility_items_input(const WorldFacilityItemsView &view,
                           const WorldFacilityItemsLayout &layout,
                           const WorldFacilityItemsInput &input, bool blocked) {
    if (blocked || !view.initialized || view.raw == 76)
        return {};
    const auto intent = [&](Action action, int selection = -1) {
        return WorldFacilityItemsIntent{action, view.page, selection};
    };
    if (view.can_cancel && (input.escape || hit(input, layout.cancel)))
        return intent(Action::cancel);
    if (view.raw == 75 && !view.rows.empty()) {
        if (input.up || input.down)
            return intent(input.up ? Action::previous : Action::next);
        if (input.wheel_rows)
            return intent(Action::select, std::clamp(view.selection + input.wheel_rows, 0,
                                                     static_cast<int>(view.rows.size()) - 1));
        for (int n = 0; n < 5 && n + view.first_visible < static_cast<int>(view.rows.size()); ++n)
            if (hit(input, {layout.rows.x, layout.rows.y + n * layout.row_height, layout.rows.width,
                            layout.row_height}))
                return intent(Action::select, n + view.first_visible);
    }
    if (view.can_confirm && (input.enter || hit(input, layout.confirm)))
        return intent(Action::confirm);
    return {};
}
void draw_world_facility_items(const WorldFacilityItemsView &view,
                               const WorldFacilityItemsLayout &layout, const Skin &skin,
                               bool enabled, const std::string &feedback) {
    skin.window(layout.panel, view.title);
    skin.content(
        {layout.rows.x - 2, layout.rows.y - 2, layout.rows.width + 4, layout.rows.height + 4});
    if (view.initialized) {
        fitted(skin, view.name, layout.heading, blue);
        if (view.raw == 75) {
            for (int n = 0; n < 5 && n + view.first_visible < static_cast<int>(view.rows.size());
                 ++n) {
                const int index = n + view.first_visible;
                const auto &row = view.rows[index];
                Rectangle box{layout.rows.x, layout.rows.y + n * layout.row_height,
                              layout.rows.width, layout.row_height};
                if (index == view.selection)
                    DrawRectangleRec(box, {255, 153, 55, 255});
                fitted(skin, row.name, {box.x + 3, box.y + 2, box.width - 40, box.height});
                skin.right(std::to_string(row.owned), box.x + box.width - 3, box.y + 2, blue, 10);
            }
            if (view.choice) {
                constexpr const char *labels[]{"价格", "品质", "魅力"};
                for (int n = 0; n < 3; ++n)
                    fitted(skin, std::string(labels[n]) + std::string(view.choice->hint[n], '+'),
                           {layout.hints.x + n * layout.hints.width / 3, layout.hints.y,
                            layout.hints.width / 3, layout.hints.height},
                           blue);
            }
        } else if (view.raw == 76) {
            skin.sprites.thumbnail(view.graphic.sprite, view.graphic.frames,
                                   {layout.rows.x + 10, layout.rows.y + 5, layout.rows.width - 20,
                                    layout.rows.height - 25});
            if (view.choice)
                skin.centered(view.choice->name,
                              {layout.rows.x, layout.rows.y + layout.rows.height - 20,
                               layout.rows.width, 18});
        } else {
            constexpr const char *labels[]{"价格", "品质", "魅力"};
            for (int n = 0; n < 3; ++n) {
                const float y = layout.rows.y + n * layout.rows.height / 3;
                fitted(skin, labels[n], {layout.rows.x + 3, y, 45, 16});
                skin.right(std::to_string(view.attributes[0][n]) + " > " +
                               std::to_string(view.attributes[1][n]),
                           layout.rows.x + layout.rows.width - 3, y, blue, 11);
                skin.right((view.attributes[2][n] >= 0 ? "+" : "") +
                               std::to_string(view.attributes[2][n]),
                           layout.rows.x + layout.rows.width - 3, y + 15, ink, 10);
            }
        }
    }
    const bool active = enabled && view.initialized;
    if (view.can_cancel)
        skin.button(layout.cancel, "返回", active);
    if (view.raw != 76)
        skin.button(layout.confirm, view.raw == 75 ? "使用" : "确定", active && view.can_confirm);
    if (!feedback.empty())
        fitted(skin, feedback, layout.feedback, MAROON);
}
} // namespace ark::desktop::ui
