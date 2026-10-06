// Page21/74/80/81 fields and actions follow the frozen 2b479f6 maintained prototype.
// Drawing never initializes a page, charges money or changes a facility's shared level.
#include "world_building.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = WorldBuildingAction;
bool hit(const std::optional<Vector2> &point, Rectangle box) {
    return point && point->x >= box.x && point->y >= box.y && point->x < box.x + box.width &&
           point->y < box.y + box.height;
}
const simulation::StartupDefinition &definition(const State &state, int id) {
    const auto &items = state.rules->facilities;
    const auto found =
        std::find_if(items.begin(), items.end(), [id](const auto &item) { return item.id == id; });
    if (found == items.end())
        throw std::invalid_argument("Building page references an unknown definition");
    return *found;
}
const std::vector<WorldBuildingRow> &rows(const WorldBuildingView &view, int tab) {
    return view.raw == 21 ? view.catalogs.at(std::clamp(tab, 0, 2)) : view.residents;
}
void fitted(const Skin &skin, const std::string &text, Rectangle box, Color color = ink) {
    const float size = std::min(12.F, 12.F * box.width / std::max(1.F, skin.text.width(text)));
    skin.text.draw(text, box.x, box.y, color, size);
}
} // namespace
bool world_building_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 21 || page.legacy_page == 74 || page.legacy_page == 80 ||
            page.legacy_page == 81);
}
WorldBuildingView world_building_view(const State &state, const Page &page) {
    if (!world_building_page(page) || !state.rules)
        throw std::invalid_argument("Building view requires a supported page and catalogue");
    WorldBuildingView view;
    view.raw = page.legacy_page;
    view.page = page.id;
    view.title = view.raw == 21 ? "建设" : view.raw == 80 ? "入住" : "设施情报";
    if (view.raw == 21) {
        const auto catalog = state.build_page_catalogs.find(page.id);
        if (catalog == state.build_page_catalogs.end())
            return view;
        for (std::size_t tab = 0; tab < 3; ++tab)
            for (const int id : catalog->second[tab]) {
                const auto quote = simulation::startup_world_build_quote(state, id);
                if (!quote)
                    throw std::invalid_argument("Building page is missing its current quote");
                view.catalogs[tab].push_back(
                    {id, definition(state, id).name, quote->construction_cost,
                     world_build_graphic(definition(state, id),
                                         simulation::rules::FacilityOrientation::first)});
            }
        view.can_confirm = true;
    } else if (view.raw == 80) {
        const auto list = state.residence_page_candidates.find(page.id);
        if (list == state.residence_page_candidates.end())
            return view;
        for (const int id : list->second) {
            const auto &people = state.rules->humans;
            const auto human = std::find_if(people.begin(), people.end(),
                                            [id](const auto &item) { return item.identity == id; });
            if (human == people.end())
                throw std::invalid_argument("Residence page references an unknown human");
            view.residents.push_back({id, human->name, human->residence_fee});
        }
        view.can_confirm = !view.residents.empty();
    } else if (view.raw == 74 && state.facility_definition_page_bindings.count(page.id)) {
        if (!simulation::valid_startup_world_facility_page(state, page))
            throw std::invalid_argument("Facility definition preview has invalid source payload");
        const int id = state.facility_definition_page_bindings.at(page.id);
        const auto &item = definition(state, id);
        view.definition_preview = true;
        view.title = item.name;
        view.page_count = 1;
        view.phase = state.page_phases.at(page.id);
        const auto &attributes = state.scripts.facilities.at(id).attributes;
        std::copy(attributes.begin(), attributes.end(), view.attributes.begin());
        view.graphic = world_build_graphic(item, simulation::rules::FacilityOrientation::first);
        // The source shop template displays already-open merchandise, not an invented instance.
        const int kind = item.detail == 1 ? 1 : item.detail == 4 ? 2 : item.detail == 5 ? 3 : 0;
        if (kind != 0) {
            view.product_count = 0;
            for (const auto &product : state.rules->equipment)
                if (product.shop.kind == kind &&
                    state.catalog.at({kind, product.shop.id}).status != 0)
                    ++*view.product_count;
        }
        view.can_confirm = true;
    } else {
        const auto binding = state.facility_page_bindings.find(page.id);
        if (binding == state.facility_page_bindings.end())
            return view;
        view.facility = binding->second;
        const auto &facility = state.scene.world.world.facilities.at(*view.facility);
        const auto &item = definition(state, facility.placement.definition_id);
        view.title = item.name;
        const auto phase = state.page_phases.find(page.id);
        if (phase == state.page_phases.end())
            return view;
        view.phase = phase->second;
        if (view.raw == 81) {
            if (!state.facility_upgrade_initialized.count(page.id))
                return view;
            view.title = "设施升级";
            view.upgrade = state.facility_upgrade_display;
            view.can_confirm = true;
        } else {
            const auto values = simulation::startup_world_facility_values(state, *view.facility);
            if (!values)
                throw std::invalid_argument("Facility page is missing its economy projection");
            view.attributes = values->instance_attributes;
            view.page_count = simulation::startup_world_facility_page_count(state, page);
            view.income =
                state.facility_monthly_cash.at(*view.facility).at(state.scene.calendar.month)[0];
            view.neighbours = state.facility_page_neighbours.at(page.id).size();
            view.can_use_items = view.phase == 0 && item.kind != 2 && item.kind != 12 &&
                                 item.detail != 1 && item.detail != 4 && item.detail != 5 &&
                                 item.detail != 6;
            view.can_confirm = (item.detail == 6 && view.phase == 0) || view.can_use_items;
        }
    }
    view.initialized = true;
    return view;
}
WorldBuildingLayout world_building_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Building page requires the supported logical viewport");
    const float width = std::min(310.F, extent.width - 16.F);
    // Keep five source rows at ordinary desktop heights; a shorter window scrolls the
    // remaining rows. Other management pages retain their existing responsive geometry.
    const float height =
        raw == 21 ? std::min(300.F, extent.height - 58.F) : std::min(250.F, extent.height - 68.F);
    WorldBuildingLayout layout;
    layout.row_height = raw == 21 ? 37.F : 38.F;
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &p = layout.panel;
    layout.body = {p.x + 12, p.y + 30, width - 24, height - (raw == 21 ? 61.F : 75.F)};
    layout.rows = {p.x + 12, p.y + (raw == 21 ? 56.F : 59.F), width - 24,
                   height - (raw == 21 ? 87.F : 108.F)};
    for (int tab = 0; tab < 3; ++tab)
        layout.tabs[tab] = {layout.body.x + tab * layout.body.width / 3, p.y + 30,
                            layout.body.width / 3, 22};
    layout.cancel = {p.x + 10, p.y + height - 28, 58, 20};
    layout.confirm = {p.x + width - 68, p.y + height - 28, 58, 20};
    layout.previous = {p.x + width / 2 - 32, p.y + height - 28, 28, 20};
    layout.next = {p.x + width / 2 + 4, p.y + height - 28, 28, 20};
    return layout;
}
int world_building_visible_rows(const WorldBuildingLayout &layout) {
    return std::clamp(static_cast<int>(layout.rows.height / layout.row_height), 1, 5);
}
WorldBuildingIcon world_building_icon(const WorldBuildingLayout &layout, int visible_row) {
    if (visible_row < 0 || visible_row >= world_building_visible_rows(layout))
        throw std::invalid_argument("Building icon row is outside the visible catalogue");
    // 2b479f6 PAGES: raw21 uses a 64x32 crop with a +(2,10) source anchor, never a fitted image.
    const Rectangle clip{layout.rows.x + 2, layout.rows.y + visible_row * layout.row_height + 2, 64,
                         32};
    return {clip, {clip.x + 2, clip.y + 10}};
}
std::optional<WorldBuildingIntent> world_building_input(const WorldBuildingView &view,
                                                        const WorldBuildingLayout &layout,
                                                        WorldBuildingSelection &selection,
                                                        const WorldBuildingInput &input,
                                                        bool blocked) {
    if (blocked || !view.initialized)
        return {};
    const auto intent = [&](Action action, int identity = 0) {
        return WorldBuildingIntent{action, view.page, identity};
    };
    if (view.raw != 81 && (input.escape || hit(input.click, layout.cancel)))
        return intent(view.raw == 21   ? Action::cancel_build
                      : view.raw == 80 ? Action::residence_cancel
                                       : Action::facility_cancel);
    bool confirm = input.enter || hit(input.click, layout.confirm);
    if (view.raw == 74) {
        if (view.page_count > 1 && (input.left || hit(input.click, layout.previous)))
            return intent(Action::facility_previous);
        if (view.page_count > 1 && (input.right || hit(input.click, layout.next)))
            return intent(Action::facility_next);
        if (confirm && view.can_confirm)
            return intent(Action::facility_confirm);
        return {};
    }
    if (view.raw == 81)
        return confirm && view.can_confirm ? std::optional{intent(Action::confirm_upgrade)}
                                           : std::nullopt;
    selection.tab = std::clamp(selection.tab, 0, 2);
    if (view.raw == 21) {
        const int old = selection.tab;
        if (input.left)
            selection.tab = (selection.tab + 2) % 3;
        if (input.right)
            selection.tab = (selection.tab + 1) % 3;
        for (int tab = 0; tab < 3; ++tab)
            if (hit(input.click, layout.tabs[tab]))
                selection.tab = tab;
        if (old != selection.tab)
            selection.selected = selection.first_row = 0;
    }
    const auto &list = rows(view, selection.tab);
    if (list.empty()) {
        selection.selected = selection.first_row = 0;
        return {};
    }
    const int count = static_cast<int>(list.size());
    selection.selected = std::clamp(selection.selected, 0, count - 1);
    if (input.up)
        selection.selected = (selection.selected + count - 1) % count;
    if (input.down)
        selection.selected = (selection.selected + 1) % count;
    const int visible = world_building_visible_rows(layout);
    if (input.wheel_rows)
        selection.selected = std::clamp(selection.selected + input.wheel_rows, 0, count - 1);
    selection.first_row = std::clamp(selection.first_row, 0, std::max(0, count - visible));
    selection.first_row = std::min(selection.first_row, selection.selected);
    selection.first_row = std::max(selection.first_row, selection.selected - visible + 1);
    for (int row = 0; row < visible && row + selection.first_row < count; ++row)
        if (hit(input.click, {layout.rows.x, layout.rows.y + row * layout.row_height,
                              layout.rows.width, layout.row_height})) {
            selection.selected = row + selection.first_row;
            confirm = true;
        }
    if (confirm && view.can_confirm)
        return intent(view.raw == 21 ? Action::select_build : Action::residence_select,
                      list[selection.selected].identity);
    return {};
}
void draw_world_building(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                         const Skin &skin, const WorldBuildingSelection &selection, bool enabled,
                         const std::string &feedback) {
    skin.window(layout.panel, view.title);
    skin.content(layout.body);
    if (view.initialized) {
        if (view.raw == 21 || view.raw == 80) {
            if (view.raw == 21) {
                constexpr const char *titles[]{"道路植物", "商店", "饮食"};
                for (int tab = 0; tab < 3; ++tab) {
                    if (tab == selection.tab)
                        DrawRectangleRec(layout.tabs[tab], {210, 229, 195, 255});
                    skin.centered(titles[tab], layout.tabs[tab], ink, 10);
                }
            }
            const auto &list = rows(view, selection.tab);
            if (list.empty())
                skin.text.draw("暂无候选", layout.rows.x + 4, layout.rows.y + 4, ink);
            const int first = std::clamp(selection.first_row, 0, static_cast<int>(list.size()));
            for (int row = 0; row < world_building_visible_rows(layout) &&
                              row + first < static_cast<int>(list.size());
                 ++row) {
                Rectangle box{layout.rows.x, layout.rows.y + row * layout.row_height,
                              layout.rows.width, layout.row_height};
                if (first + row == selection.selected)
                    DrawRectangleRec(box, {255, 236, 174, 255});
                const auto &item = list[first + row];
                float label_x = box.x + 4;
                if (!item.graphic.frames.empty()) {
                    const auto icon = world_building_icon(layout, row);
                    DrawRectangleRec(icon.clip, icon.background);
                    for (const auto &[frame, offset] : item.graphic.frames)
                        skin.sprites.draw(item.graphic.sprite, frame,
                                          {icon.anchor.x + offset.x, icon.anchor.y + offset.y},
                                          WHITE, Sprites::Binding::map, 1, -1, icon.clip);
                    label_x = box.x + 72;
                }
                fitted(skin, item.name, {label_x, box.y + 3, box.x + box.width - label_x - 4, 14});
                skin.right(std::to_string(item.cost) + "G", box.x + box.width - 4, box.y + 22, ink,
                           10);
            }
        } else if (view.definition_preview) {
            skin.sprites.thumbnail(view.graphic.sprite, view.graphic.frames,
                                   {layout.body.x + 3, layout.body.y + 7, 85, 75});
            if (view.product_count) {
                fitted(skin, "商品种类",
                       {layout.body.x + 94, layout.body.y + 8, layout.body.width - 98, 16});
                skin.right(std::to_string(*view.product_count),
                           layout.body.x + layout.body.width - 4, layout.body.y + 31, blue);
            } else {
                constexpr const char *labels[]{"价格", "品质", "魅力"};
                for (int row = 0; row < 3; ++row) {
                    const float y = layout.body.y + 8 + row * 25;
                    fitted(skin, labels[row], {layout.body.x + 94, y, 40, 16});
                    skin.right(std::to_string(view.attributes[row]),
                               layout.body.x + layout.body.width - 4, y, blue, 11);
                }
            }
        } else {
            constexpr const char *labels[]{"价格", "品质", "魅力", "维护费"};
            for (int row = 0; row < 3; ++row) {
                const float y = layout.body.y + 8 + row * 25;
                std::string label, value;
                if (view.raw == 81) {
                    label = labels[row];
                    value = std::to_string(view.upgrade[0][row]) + " > " +
                            std::to_string(view.upgrade[1][row]);
                } else if (view.phase == 0) {
                    label = labels[row];
                    value = std::to_string(view.attributes[row]);
                } else {
                    label = row == 0 ? "维护费" : row == 1 ? "收入" : "加成";
                    value = row == 0   ? std::to_string(view.attributes[3])
                            : row == 1 ? std::to_string(view.income) + "G"
                                       : std::to_string(view.neighbours);
                }
                skin.text.draw(label, layout.body.x + 4, y);
                skin.right(value, layout.body.x + layout.body.width - 4, y);
            }
        }
    }
    const bool active = enabled && view.initialized;
    if (view.raw != 81)
        skin.button(layout.cancel, "返回", active);
    if (view.raw == 74 && view.page_count > 1) {
        skin.button(layout.previous, "<", active);
        skin.button(layout.next, ">", active);
        skin.right(std::to_string(view.phase + 1) + "/" + std::to_string(view.page_count),
                   layout.panel.x + layout.panel.width - 12, layout.panel.y + 10, WHITE, 10);
    }
    if (view.raw != 74 || view.can_confirm)
        skin.button(layout.confirm,
                    view.raw == 21            ? "建设"
                    : view.raw == 81          ? "确定"
                    : view.definition_preview ? "关闭"
                    : view.can_use_items      ? "使用道具"
                                              : "入住",
                    active && view.can_confirm &&
                        (view.raw != 21 || !rows(view, selection.tab).empty()));
    if (!feedback.empty())
        fitted(skin, feedback,
               {layout.body.x, layout.panel.y + layout.panel.height - 45, layout.body.width, 14},
               MAROON);
}
} // namespace ark::desktop::ui
