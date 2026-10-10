#include "ark/simulation/startup_world_profile.hpp"
// Page21/74/80/81 fields and actions follow the frozen 2b479f6 maintained prototype.
// Drawing never initializes a page, charges money or changes a facility's shared level.
#include "../world_overlay_render.hpp"
#include "skin.hpp"
#include "world_building.hpp"
#include "world_facility_upgrade.hpp"
#include <algorithm>
#include <limits>
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
app::WorldFacilityTemplate detail_type(const simulation::StartupDefinition &d) {
    using Type = app::WorldFacilityTemplate;
    // The same published priority applies to definition previews without an instance query.
    return d.kind == 12                                      ? Type::home
           : d.detail == 1 || d.detail == 4 || d.detail == 5 ? Type::equipment
           : d.detail == 6                                   ? Type::recruitment
           : d.kind == 2                                     ? Type::booster
                                                             : Type::ordinary;
}
std::optional<std::size_t> open_products(const State &state,
                                         const simulation::StartupDefinition &d) {
    const int kind = d.detail == 1 ? 1 : d.detail == 4 ? 2 : d.detail == 5 ? 3 : 0;
    if (kind == 0)
        return {};
    std::size_t count{};
    for (const auto &product : state.rules->equipment)
        if (product.shop.kind == kind && state.catalog.at({kind, product.shop.id}).status != 0)
            ++count;
    return count;
}
const std::vector<WorldBuildingRow> &rows(const WorldBuildingView &view, int tab) {
    return view.raw == 21 ? view.catalogs.at(std::clamp(tab, 0, 2)) : view.residents;
}
void fitted(const Skin &skin, const std::string &text, Rectangle box, Color color = ink) {
    const float size = std::min(12.F, 12.F * box.width / std::max(1.F, skin.text.width(text)));
    skin.text.draw(text, box.x, box.y, color, size);
}
void detail_money(const Skin &skin, std::int64_t value, Rectangle box) {
    skin.number(value, {box.x + box.width - 10, box.y + 2}, "number08.seb",
                Sprites::Binding::steam_common);
    skin.sprites.draw("number08.seb", 20, {box.x + box.width - 9, box.y + 2}, WHITE,
                      Sprites::Binding::steam_common);
}
void detail_field(Rectangle box, Color fill, Color border) {
    DrawRectangleRec(box, fill);
    DrawRectangleLinesEx(box, 1, border);
}
void draw_facility_footer(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                          const Skin &skin) {
    if (!view.initialized || !view.facility || view.definition_preview)
        return;
    const auto name = layout.footer_name, profit = layout.footer_profit;
    // Cover only the footer strip, leaving the return key and the world above it intact.
    skin.tile("btmbar.png", {116, 1, 4, 20},
              {name.x, name.y, profit.x + profit.width - name.x, name.height});
    skin.content(name, {255, 255, 222, 255});
    fitted(skin, view.title, {name.x + 3, name.y + 2, name.width - 6, 15});
    if (!view.cumulative_profit)
        return; // A non-business facility has no invented zero-profit field.
    fitted(skin, "收益", {profit.x + 2, profit.y + 2, 26, 15});
    const auto value = *view.cumulative_profit;
    auto digits = std::to_string(value);
    if (value < 0)
        digits.erase(0, 1); // Magnitude without signed negation, including INT64_MIN.
    const char *sprite = value < 0 ? "number12.seb" : "number05.seb";
    const float width = 8.F * (digits.size() + 1);
    const float scale = std::min(1.F, (profit.width - 31) / width);
    float x = profit.x + profit.width - 2 - width * scale;
    const float y = profit.y + (profit.height - 10 * scale) / 2;
    for (const char digit : digits) {
        skin.sprites.draw(sprite, digit - '0', {x, y}, WHITE, Sprites::Binding::common, scale);
        x += 8 * scale;
    }
    skin.sprites.draw(sprite, 20, {x, y}, WHITE, Sprites::Binding::common, scale);
}
void draw_detail(const WorldBuildingView &view, const WorldBuildingLayout &layout, const Skin &skin,
                 const WorldBuildingSelection &selection) {
    using Type = app::WorldFacilityTemplate;
    const auto boxes = world_building_detail_layout(layout, view.detail_type);
    if (view.phase == 1 && !view.definition_preview) {
        fitted(skin, "设施加成", boxes.source_heading);
        fitted(skin, "维护费", {boxes.maintenance.x, boxes.maintenance.y, 42, 14}, blue);
        detail_money(skin, view.attributes[3], boxes.maintenance);
        if (view.bonus_rows.empty()) {
            skin.centered("没有奖励", boxes.sources, ink, 11);
        } else {
            // Published source positions, translated to the existing centered panel.
            const Vector2 origin{layout.panel.x - 8, layout.panel.y - 44};
            const int visible =
                std::min(5, static_cast<int>(boxes.sources.height / boxes.source_row_height));
            const int first =
                std::clamp(selection.first_row, 0,
                           std::max(0, static_cast<int>(view.bonus_rows.size()) - visible));
            for (int visible_row = 0; visible_row < visible; ++visible_row) {
                const int source_index = first + visible_row;
                if (source_index >= static_cast<int>(view.bonus_rows.size()))
                    break;
                const auto &bonus = view.bonus_rows[source_index];
                // Scrolling changes source_index only; vertical position uses visible_row.
                const float y = origin.y + 97 + visible_row * 19;
                draw_world_visuals({bonus.icon}, skin.sprites, {origin.x + 26, y - 2}, 1);
                const float name_x = origin.x + 44;
                const float first_value_x =
                    origin.x + (bonus.source.values.size() == 2 ? 121 : 147);
                const float name_width = first_value_x - name_x - 2;
                const float name_size =
                    12 * std::min(1.F, name_width /
                                           std::max(1.F, skin.text.width(bonus.source.name, 12)));
                skin.text.draw(bonus.source.name, name_x, y, ink, name_size);
                for (const auto &value : bonus.source.values) {
                    const float x = origin.x + (value.attribute == 0   ? 121
                                                : value.attribute == 1 ? 171
                                                                       : 147);
                    const float right = origin.x + (value.attribute == 0 ? 169 : 219);
                    const float measured =
                        skin.text.width(value.label, 12) + skin.text.width(value.text, 12);
                    const float size = 12 * std::min(1.F, (right - x) / std::max(1.F, measured));
                    skin.text.draw(value.label, x, y, {0, 101, 255, 255}, size);
                    skin.text.draw(value.text, x + skin.text.width(value.label, size), y, ink,
                                   size);
                }
            }
        }
        const int count = std::max(1, static_cast<int>(view.bonus_rows.size()));
        const int visible =
            std::min(5, static_cast<int>(boxes.sources.height / boxes.source_row_height));
        const int first = std::clamp(selection.first_row, 0, std::max(0, count - visible));
        const auto track = boxes.source_scroll;
        DrawRectangleRec(track, {223, 234, 215, 255});
        const float height = track.height * std::min(count, visible) / count;
        const float y = track.y + (track.height - height) * first / std::max(1, count - visible);
        DrawRectangleRec({track.x, y, track.width, height}, {50, 164, 234, 255});
        skin.centered("周围设施的加成", boxes.source_footer, ink, 12);
        return;
    }
    if (view.category_icon)
        draw_world_visuals({*view.category_icon}, skin.sprites,
                           {layout.panel.x + 13, layout.panel.y + 22}, 1);
    fitted(skin, view.title, boxes.name, ink);
    if (view.detail_type == Type::ordinary) {
        fitted(skin, "价格", {boxes.price.x, boxes.price.y, 32, 14}, blue);
        detail_money(skin, view.attributes[0], boxes.price);
        if (!view.definition_preview && view.attributes[0] >= view.attribute_limits[0])
            skin.sprites.image("wnd_max.png", {0, 0, 20, 6},
                               {boxes.price.x + 27, boxes.price.y + 3, 20, 6});
    }
    detail_field(boxes.picture, {226, 247, 212, 255}, {184, 211, 168, 255});
    // Steam details always use orientation0 and Mapchip2's source center, with clipping;
    // fitting the image to its frame changed building scale and concealed tall buildings.
    const auto pieces = simulation::steam_facility_mapchip2_draws({view.mapchip, {49, 37}, 0});
    if (!pieces)
        throw std::invalid_argument("Facility detail has invalid mapchip");
    for (const auto &piece : *pieces)
        skin.sprites.draw(
            piece.sprite, piece.frame,
            {boxes.picture.x + piece.position[0], boxes.picture.y + piece.position[1]}, WHITE,
            Sprites::Binding::map, 1, -1, boxes.picture);
    if (view.detail_type == Type::ordinary) {
        detail_field(boxes.values, {255, 248, 214, 255}, {239, 208, 119, 255});
        // Effects carry absolute source coordinates; translate once from panel (8,44).
        detail_field(boxes.effects, {255, 248, 214, 255}, {239, 208, 119, 255});
        for (const auto &effect : view.exit_effects) {
            const Vector2 origin{layout.panel.x - 8, layout.panel.y - 44};
            const auto &icon = effect.icon;
            const auto &r = icon.crop;
            skin.sprites.image(
                "icon_param00.png", {float(r[0]), float(r[1]), float(r[2]), float(r[3])},
                {origin.x + icon.offset[0], origin.y + icon.offset[1], float(r[2]), float(r[3])},
                Sprites::Binding::steam_common);
            for (const auto &plus : effect.pluses)
                skin.sprites.draw("number08.seb", plus.frame,
                                  {origin.x + plus.offset[0], origin.y + plus.offset[1]}, WHITE,
                                  Sprites::Binding::steam_common);
        }
        constexpr const char *labels[]{"品质", "魅力"};
        for (int row = 0; row < 2; ++row) {
            const float y = boxes.values.y + 4 + row * 15;
            fitted(skin, labels[row], {boxes.values.x + 6, y, 40, 14}, blue);
            skin.number(view.attributes[row + 1], {boxes.values.x + boxes.values.width - 6, y + 1},
                        "number08.seb", Sprites::Binding::steam_common);
            if (!view.definition_preview &&
                view.attributes[row + 1] >= view.attribute_limits[row + 1])
                skin.sprites.image("wnd_max.png", {0, 0, 20, 6},
                                   {boxes.values.x + 39, y + 3, 20, 6});
        }
        skin.sprites.image("wnd_lv.png", {0, 0, 17, 10}, {boxes.level.x, boxes.level.y, 17, 10});
        if (view.level == 5)
            skin.sprites.image("wnd_max.png", {0, 0, 20, 6},
                               {boxes.level.x + 22, boxes.level.y + 2, 20, 6});
        else {
            skin.number(view.level, {boxes.level.x + 31, boxes.level.y}, "number05.seb",
                        Sprites::Binding::steam_common);
            if (view.remaining_uses) {
                fitted(skin, "距离下个等级还有", {boxes.remaining.x, boxes.remaining.y, 126, 14});
                skin.number(*view.remaining_uses,
                            {boxes.remaining.x + boxes.remaining.width - 16, boxes.remaining.y + 2},
                            "number09.seb");
                skin.text.draw("人", boxes.remaining.x + boxes.remaining.width - 13,
                               boxes.remaining.y, ink);
            }
        }
    } else {
        const std::string label = view.detail_type == Type::equipment
                                      ? "商品种类 " + std::to_string(view.product_count.value_or(0))
                                  : view.detail_type == Type::recruitment ? "入住希望者"
                                  : view.detail_type == Type::home        ? "住宅"
                                                                          : "周围设施";
        skin.centered(label, boxes.remaining, ink, 11);
    }
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
                    {id,
                     definition(state, id).detail == 6 ? "募集入住" : definition(state, id).name,
                     quote->construction_cost,
                     world_build_graphic(state, id,
                                         simulation::rules::FacilityOrientation::first)});
            }
        // The maintained base catalogue contains buildings only. The desktop raw21
        // projection adds the published road/edit identities without changing that cache.
        std::vector<WorldBuildingRow> roads;
        for (const auto &item : state.rules->facilities) {
            const auto presence = state.facility_presence.find(item.id);
            if (item.kind != 6 || !(item.flags & 4) || presence == state.facility_presence.end() ||
                presence->second == 0)
                continue;
            const auto quote = simulation::startup_world_build_quote(state, item.id);
            if (!quote)
                throw std::invalid_argument("Road catalogue is missing its current quote");
            roads.push_back({item.id, "道路", quote->construction_cost,
                             world_build_graphic(state, item.id,
                                                 simulation::rules::FacilityOrientation::first)});
        }
        view.catalogs[0].insert(view.catalogs[0].begin(), roads.begin(), roads.end());
        std::vector<WorldBuildingRow> tools{
            {-1, "撤除", 0, {}, {}, "destruct00.png", {0, 0, 60, 29}, {4, 3}}};
        if (state.scripts.user_flags & 32U)
            tools.push_back(
                {-2, "交换位置", 300, {}, {}, "moveTenant.png", {0, 0, 63, 32}, {1, 0}});
        // S057 places editing entries directly after roads. Preserve the base building
        // order/duplicates and all command identities; road-absent behavior stays unchanged.
        const auto tools_position =
            roads.empty() ? view.catalogs[0].end() : view.catalogs[0].begin() + roads.size();
        view.catalogs[0].insert(tools_position, tools.begin(), tools.end());
        for (auto &tab : view.catalogs)
            for (auto &row : tab)
                if (row.identity >= 0 && definition(state, row.identity).kind == 12)
                    row.residence_qualifications = state.facility_free_builds.at(row.identity);
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
            view.residents.push_back(
                {id, simulation::startup_world_human_profile(state, human->identity).value().name,
                 human->residence_fee});
        }
        view.can_confirm = !view.residents.empty();
    } else if (view.raw == 74 && state.facility_definition_page_bindings.count(page.id)) {
        if (!simulation::valid_startup_world_facility_page(state, page))
            throw std::invalid_argument("Facility definition preview has invalid source payload");
        const int id = state.facility_definition_page_bindings.at(page.id);
        const auto &item = definition(state, id);
        view.mapchip = item.display_id;
        view.definition_preview = true;
        view.title = item.name;
        view.page_count = 1;
        view.phase = state.page_phases.at(page.id);
        const auto &attributes = state.scripts.facilities.at(id).attributes;
        std::copy(attributes.begin(), attributes.end(), view.attributes.begin());
        view.graphic =
            world_build_graphic(state, item.id, simulation::rules::FacilityOrientation::first);
        view.detail_type = detail_type(item);
        const auto &progress = state.scene.world.world.facility_uses.at(id);
        view.level = progress.level;
        if (view.level != 5) {
            const auto quote = simulation::startup_world_build_quote(state, id);
            if (!quote)
                throw std::invalid_argument("Definition preview is missing its shared level quote");
            view.remaining_uses = quote->upgrade_uses - progress.completed_uses;
        }
        // The source shop template displays already-open merchandise, not an invented instance.
        view.product_count = open_products(state, item);
        view.can_confirm = true;
    } else {
        const auto binding = state.facility_page_bindings.find(page.id);
        if (binding == state.facility_page_bindings.end())
            return view;
        view.facility = binding->second;
        const auto &facility = state.scene.world.world.facilities.at(*view.facility);
        const auto &item = definition(state, facility.placement.definition_id);
        view.mapchip = item.display_id;
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
            view.facility_name = item.name;
            simulation::SteamFacilityUpgradeSkinInput input;
            input.definition = item.id;
            input.mapchip = item.display_id;
            input.level = state.scene.world.world.facility_uses.at(item.id).level;
            input.phase = view.phase;
            input.frame = state.page_counters.at(page.id);
            const auto secondary = state.page_secondary_counters.find(page.id);
            view.upgrade_secondary_available = secondary != state.page_secondary_counters.end();
            if (view.upgrade_secondary_available)
                input.frame2 = secondary->second;
            const auto checked = [](std::int64_t n) {
                if (n < std::numeric_limits<int>::min() || n > std::numeric_limits<int>::max())
                    throw std::invalid_argument("Upgrade display outside source integer range");
                return static_cast<int>(n);
            };
            for (int slot = 0; slot < 3; ++slot) {
                for (int phase = 0; phase < 3; ++phase)
                    input.attributes[slot][phase] = checked(view.upgrade[phase][slot]);
                input.limits[slot] = checked(std::int64_t(item.economy.attributes[slot].fifth) * 2);
            }
            view.upgrade_skin = input;
            view.can_confirm = true;
        } else {
            const auto values = simulation::startup_world_facility_values(state, *view.facility);
            if (!values)
                throw std::invalid_argument("Facility page is missing its economy projection");
            view.attributes = values->instance_attributes;
            const auto detail = app::query_world_facility_detail(state, *view.facility);
            if (!detail.detail)
                throw std::invalid_argument(
                    "Facility page is missing its validated read-only detail");
            view.detail_type = detail.detail->type;
            view.level = detail.detail->level;
            view.remaining_uses = detail.detail->remaining_uses;
            view.cumulative_profit = detail.detail->cumulative_profit;
            view.graphic = world_build_graphic(state, item.id, facility.placement.orientation);
            view.product_count = open_products(state, item);
            const auto bonuses = simulation::startup_world_facility_bonus_rows(state, page.id);
            if (!bonuses)
                throw std::invalid_argument("Facility page has invalid bonus rows");
            for (const auto &source : *bonuses) {
                const auto icon =
                    simulation::startup_world_facility_icon_draw(state, source.definition);
                if (!icon)
                    throw std::invalid_argument("Facility bonus row has invalid category icon");
                view.bonus_rows.push_back({source, *icon});
            }
            view.page_count = simulation::startup_world_facility_page_count(state, page);
            view.income =
                state.facility_monthly_cash.at(*view.facility).at(state.scene.calendar.month)[0];
            view.neighbours = state.facility_page_neighbours.at(page.id).size();
            view.can_use_items = view.phase == 0 && item.kind != 2 && item.kind != 12 &&
                                 item.detail != 1 && item.detail != 4 && item.detail != 5 &&
                                 item.detail != 6;
            view.can_view_products =
                view.phase == 0 && (item.detail == 1 || item.detail == 4 || item.detail == 5);
            view.can_confirm = (item.detail == 6 && view.phase == 0) || view.can_use_items ||
                               view.can_view_products;
        }
    }
    if (view.raw == 74) {
        const int id =
            view.definition_preview
                ? state.facility_definition_page_bindings.at(page.id)
                : state.scene.world.world.facilities.at(*view.facility).placement.definition_id;
        for (int n = 0; n < 3; ++n)
            view.attribute_limits[n] =
                std::int64_t(definition(state, id).economy.attributes[n].fifth) * 2;
        view.category_icon = simulation::startup_world_facility_icon_draw(state, id);
        if (!view.category_icon)
            throw std::invalid_argument("Facility detail has invalid category icon");
        if (view.detail_type == app::WorldFacilityTemplate::ordinary) {
            const auto effects = simulation::startup_world_facility_exit_effect_draws(state, id);
            if (!effects)
                throw std::invalid_argument("Facility detail has invalid exit effects");
            view.exit_effects = *effects;
        }
    }
    view.initialized = true;
    return view;
}
WorldBuildingLayout world_building_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Building page requires the supported logical viewport");
    const float width = std::min(raw == 74 ? 224.F : 310.F, extent.width - 16.F);
    // Keep five source rows at ordinary desktop heights; a shorter window scrolls the
    // remaining rows. Other management pages retain their existing responsive geometry.
    const float height =
        raw == 21 ? std::min(300.F, extent.height - 58.F) : std::min(250.F, extent.height - 68.F);
    WorldBuildingLayout layout;
    if (raw == 81) {
        layout.panel = {(extent.width - 222.F) / 2, (extent.height - 170.F) / 2, 222, 170};
        layout.body = {layout.panel.x + 6, layout.panel.y + 104, 209, 63};
        // Existing PC confirmation area is an explicit adaptation; no invented source rectangle.
        layout.confirm = {layout.panel.x + 174, layout.panel.y + 151, 42, 18};
        return layout;
    }
    if (raw == 74) {
        // S019 and the supplied page2 share a compact ~224x172 window. PAGES gives
        // the 97x74 picture and two right-hand fields; desktop centering is an adaptation.
        layout.panel = {(extent.width - 224.F) / 2, (extent.height - 172.F) / 2, 224, 172};
        const auto p = layout.panel;
        layout.body = {p.x + 8, p.y + 40, 208, 110};
        layout.rows = {p.x + 12, p.y + 44, 196, 102};
        layout.cancel = Layout(extent).right_button;
        const float available = layout.cancel.x - 64;
        layout.footer_name = {62, extent.height - 20.F, available * .48F, 19};
        layout.footer_profit = {layout.footer_name.x + layout.footer_name.width + 2,
                                layout.footer_name.y, available * .52F - 2, 19};
        layout.confirm = {p.x + 62, p.y + 152, 100, 18};
        layout.previous = {p.x + 8, p.y + 2, 14, 17};
        layout.next = {p.x + 202, p.y + 2, 14, 17};
        return layout;
    }
    if (raw == 21) {
        // S057: a narrow catalogue begins with the tabs, then five37-pitch picture rows.
        // It has no wood title or buy button. Return is the screen's lower-right soft key.
        constexpr float catalogue_width = 176, catalogue_height = 210;
        layout.panel = {(extent.width - catalogue_width) / 2,
                        (extent.height - catalogue_height) / 2, catalogue_width, catalogue_height};
        if (extent.width < 300)
            layout.panel.y = std::min(layout.panel.y, extent.height - 29.F - 206);
        const auto p = layout.panel;
        layout.row_height = 37;
        layout.body = {p.x + 3, p.y + 21, 162, 185};
        layout.rows = layout.body;
        for (int tab = 0; tab < 3; ++tab)
            layout.tabs[tab] = {p.x + 3 + tab * 57.F, p.y + 3, 57, 16};
        layout.previous = {p.x - 9, p.y + 3, 9, 16};
        layout.next = {p.x + p.width, p.y + 3, 9, 16};
        layout.cancel = Layout(extent).right_button;
        layout.confirm = {}; // Row/Enter activates its bound definition; no hidden command box.
        return layout;
    }
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
WorldBuildingDetailLayout world_building_detail_layout(const WorldBuildingLayout &layout,
                                                       app::WorldFacilityTemplate type) {
    const auto b = layout.body;
    const auto p = layout.panel;
    WorldBuildingDetailLayout out;
    out.name = {p.x + 30, p.y + 25, 90, 14};
    out.price = {p.x + 124, p.y + 25, 87, 14};
    out.picture = {type == app::WorldFacilityTemplate::ordinary ? p.x + 13
                                                                : b.x + (b.width - 97) / 2,
                   p.y + 44, 97, 74};
    out.level = {out.picture.x + 3, out.picture.y + 61, 44, 10};
    out.values = {p.x + 113, p.y + 44, 90, 34};
    out.effects = {p.x + 113, p.y + 80, 90, 38};
    out.remaining = {p.x + 30, p.y + 128, 173, 14};
    out.source_heading = {p.x + 17, p.y + 25, 88, 14};
    out.maintenance = {p.x + 124, p.y + 25, 87, 14};
    out.sources = {b.x + 6, b.y + 6, b.width - 18, b.height - 12};
    out.source_scroll = {b.x + b.width + 1, b.y, 5, b.height};
    out.source_footer = {p.x + 15, p.y + 153, p.width - 30, 17};
    return out;
}
std::optional<WorldBuildingIntent> world_building_input(const WorldBuildingView &view,
                                                        const WorldBuildingLayout &layout,
                                                        WorldBuildingSelection &selection,
                                                        const WorldBuildingInput &input,
                                                        bool blocked) {
    if (blocked || !view.initialized)
        return {};
    if (view.raw == 21 &&
        (selection.marker_page != view.page || input.enter || input.escape || input.up ||
         input.down || input.left || input.right || input.wheel_rows)) {
        selection.marked_definition.reset();
        selection.marker_page = view.page;
    }
    const auto intent = [&](Action action, int identity = 0) {
        return WorldBuildingIntent{action, view.page, identity};
    };
    if (view.raw != 81 && (input.escape || hit(input.click, layout.cancel)))
        return intent(view.raw == 21   ? Action::cancel_build
                      : view.raw == 80 ? Action::residence_cancel
                                       : Action::facility_cancel);
    bool confirm = input.enter || hit(input.click, layout.confirm);
    if (view.raw == 74) {
        if (view.phase == 1 && !view.definition_preview) {
            const auto details = world_building_detail_layout(layout, view.detail_type);
            const int visible =
                std::min(5, static_cast<int>(details.sources.height / details.source_row_height));
            const int scroll = input.wheel_rows + (input.down ? 1 : 0) - (input.up ? 1 : 0);
            selection.first_row =
                std::clamp(selection.first_row + scroll, 0,
                           std::max(0, static_cast<int>(view.bonus_rows.size()) - visible));
        } else
            selection.first_row = 0;
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
        if (input.left || hit(input.click, layout.previous))
            selection.tab = (selection.tab + 2) % 3;
        if (input.right || hit(input.click, layout.next))
            selection.tab = (selection.tab + 1) % 3;
        for (int tab = 0; tab < 3; ++tab)
            if (hit(input.click, layout.tabs[tab]))
                selection.tab = tab;
        if (old != selection.tab) {
            selection.selected = selection.first_row = 0;
            selection.marked_definition.reset();
        }
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
            const int index = row + selection.first_row;
            const bool marked =
                selection.selected == index && selection.marked_definition == list[index].identity;
            selection.selected = row + selection.first_row;
            confirm = view.raw != 21 || marked;
            if (view.raw == 21)
                selection.marked_definition = list[index].identity;
        }
    if (confirm && view.can_confirm)
        return intent(view.raw == 21 ? Action::select_build : Action::residence_select,
                      list[selection.selected].identity);
    return {};
}
void draw_world_building(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                         const Skin &skin, const WorldBuildingSelection &selection, bool enabled,
                         const std::string &feedback) {
    if (view.raw == 81 && view.initialized) {
        draw_world_facility_upgrade(view, layout, skin);
        return;
    }
    // S043/S047 label the page, while the real facility name remains in its body.
    const auto title =
        view.raw == 74 && !view.definition_preview
            ? "设施信息" + (view.page_count > 1 ? " " + std::to_string(view.phase + 1) + "/" +
                                                      std::to_string(view.page_count)
                                                : "")
            : view.title;
    if (view.raw == 21) {
        DrawRectangleRec(layout.panel, {222, 230, 144, 255});
        DrawRectangleLinesEx(layout.panel, 1, {59, 66, 18, 255});
        DrawRectangleLinesEx({layout.panel.x + 1, layout.panel.y + 1, layout.panel.width - 2,
                              layout.panel.height - 2},
                             1, {139, 142, 53, 255});
        DrawRectangleRec(layout.body, {250, 254, 248, 255});
    } else {
        skin.window(layout.panel, title);
        skin.content(layout.body);
    }
    if (view.initialized) {
        if (view.raw == 21 || view.raw == 80) {
            if (view.raw == 21) {
                constexpr const char *titles[]{"设备", "一般", "饮食"}; // S057 display wording.
                for (int tab = 0; tab < 3; ++tab) {
                    DrawRectangleRec(layout.tabs[tab], tab == selection.tab
                                                           ? Color{0, 241, 115, 255}
                                                           : Color{225, 255, 69, 255});
                    DrawRectangleLinesEx(layout.tabs[tab], 1, {60, 96, 6, 255});
                    skin.centered(titles[tab], layout.tabs[tab], ink, 10);
                }
                const auto p = layout.panel;
                DrawTriangle({p.x - 8, p.y + 11}, {p.x, p.y + 18}, {p.x, p.y + 4}, GOLD);
                DrawTriangle({p.x + p.width + 8, p.y + 11}, {p.x + p.width, p.y + 4},
                             {p.x + p.width, p.y + 18}, GOLD);
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
                if (view.raw != 21 && first + row == selection.selected)
                    DrawRectangleRec(box, {255, 236, 174, 255});
                const auto &item = list[first + row];
                float label_x = box.x + 4;
                if (!item.graphic.frames.empty() || !item.common_image.empty()) {
                    const auto icon = world_building_icon(layout, row);
                    DrawRectangleRec(icon.clip, icon.background);
                    for (const auto &[frame, offset] : item.graphic.frames)
                        skin.sprites.draw(item.graphic.sprite, frame,
                                          {icon.anchor.x + offset.x, icon.anchor.y + offset.y},
                                          WHITE, Sprites::Binding::map, 1, -1, icon.clip);
                    if (!item.common_image.empty()) {
                        const Rectangle target{icon.clip.x + item.image_offset.x,
                                               icon.clip.y + item.image_offset.y,
                                               item.image_source.width, item.image_source.height};
                        const auto clipped =
                            clip_sprite_blit({item.image_source, target}, icon.clip);
                        if (clipped)
                            skin.sprites.image(item.common_image, clipped->source,
                                               clipped->destination, Sprites::Binding::common);
                    }
                    label_x = box.x + 72;
                }
                const auto &name = item.name;
                if (view.raw == 21) {
                    DrawRectangleLinesEx(box, 1, {11, 181, 0, 255});
                    if (first + row == selection.selected) {
                        const float label_width =
                            std::min(box.x + box.width - label_x - 3, skin.text.width(name) + 4);
                        DrawRectangleRec({label_x - 1, box.y + 2, label_width, 15},
                                         {255, 185, 87, 255});
                        skin.sprites.draw("finger_r.seb", 0, {label_x - 8, box.y + 9}, WHITE,
                                          Sprites::Binding::common);
                    }
                }
                fitted(skin, name, {label_x, box.y + 3, box.x + box.width - label_x - 4, 14});
                if (item.residence_qualifications)
                    skin.text.draw("H " + std::to_string(*item.residence_qualifications), label_x,
                                   box.y + 22, blue, 10);
                if (view.raw == 21) {
                    skin.number(item.cost, {box.x + box.width - 13, box.y + 23}, "number05.seb",
                                Sprites::Binding::steam_common);
                    skin.sprites.draw("number05.seb", 20, {box.x + box.width - 12, box.y + 23},
                                      WHITE, Sprites::Binding::steam_common);
                } else
                    skin.right(std::to_string(item.cost) + "G", box.x + box.width - 4, box.y + 22,
                               ink, 10);
            }
            if (view.raw == 21) {
                const Rectangle track{layout.panel.x + layout.panel.width - 7, layout.rows.y, 5,
                                      layout.rows.height};
                DrawRectangleRec(track, {33, 20, 91, 255});
                const int count = std::max(1, static_cast<int>(list.size()));
                const float visible = std::min(count, world_building_visible_rows(layout));
                const float thumb_height = track.height * visible / count;
                const float thumb_y = track.y + (track.height - thumb_height) * first /
                                                    std::max(1, count - static_cast<int>(visible));
                DrawRectangleRec({track.x, thumb_y, track.width, thumb_height},
                                 {50, 164, 234, 255});
            }
        } else if (view.raw == 74) {
            draw_detail(view, layout, skin, selection);
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
    if (view.raw == 74)
        draw_facility_footer(view, layout, skin);
    if (view.raw != 81)
        skin.button(layout.cancel, "返回", active);
    if (view.raw == 74 && view.page_count > 1) {
        // arrow02 frames2/3 crop the gold left/right pair; 0/1 are the grey pair.
        skin.sprites.draw("arrow02.seb", active ? 2 : 0,
                          {layout.previous.x + 4, layout.previous.y + 9}, WHITE,
                          Sprites::Binding::common);
        skin.sprites.draw("arrow02.seb", active ? 3 : 1, {layout.next.x + 4, layout.next.y + 9},
                          WHITE, Sprites::Binding::common);
    }
    if (view.raw != 21 && (view.raw != 74 || view.can_confirm))
        skin.choice(layout.confirm,
                    view.raw == 21            ? "建设"
                    : view.raw == 81          ? "确定"
                    : view.definition_preview ? "关闭"
                    : view.can_use_items      ? "使用道具"
                    : view.can_view_products  ? "商品"
                                              : "入住希望者",
                    active && view.can_confirm &&
                        (view.raw != 21 || !rows(view, selection.tab).empty()));
    if (!feedback.empty())
        fitted(skin, feedback,
               {layout.body.x, layout.panel.y + layout.panel.height - 45, layout.body.width, 14},
               MAROON);
}
} // namespace ark::desktop::ui
