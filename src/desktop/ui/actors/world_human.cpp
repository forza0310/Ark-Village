#include "world_human.hpp"
#include "../../scene/world_overlay_render.hpp"
#include "../common/skin.hpp"
#include "ark/simulation/actors/rules/human_management.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "world_human_detail.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
void fitted(const Skin &skin, const std::string &text, Rectangle box, Color color = ink) {
    const float size = std::min(11.F, 12.F * box.width / std::max(1.F, skin.text.width(text)));
    skin.text.draw(text, box.x, box.y, color, size);
}
WorldHumanRow equipment(const State &state, int slot, int id) {
    if (slot < 0 || slot > 3)
        throw std::invalid_argument("Equipment view requires one of the four equipment slots");
    const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
    const auto found =
        std::find_if(state.rules->equipment.begin(), state.rules->equipment.end(),
                     [&](const auto &e) { return e.shop.kind == kind && e.shop.id == id; });
    if (found == state.rules->equipment.end())
        throw std::invalid_argument("Human page references missing equipment");
    const auto &e = *found;
    const auto &current = state.catalog.at({kind, id});
    const auto cost = simulation::rules::human_equipment_gift_cost(
        {id, slot, current.status, e.gift_order, e.reward_difficulty, e.shop.price, e.gift_rating,
         current.free_purchases, e.shop.combat});
    if (!cost)
        throw std::invalid_argument("Human page has invalid equipment quote");
    WorldHumanRow row;
    row.identity = id;
    row.slot = slot;
    row.name = e.name;
    row.cost = *cost;
    row.stock = current.free_purchases;
    row.fresh = current.newly_unlocked;
    row.combat = e.shop.combat;
    return row;
}
WorldHumanRow item(const State &state, int id) {
    const auto found = std::find_if(state.rules->items.begin(), state.rules->items.end(),
                                    [id](const auto &d) { return d.identity == id; });
    if (found == state.rules->items.end())
        throw std::invalid_argument("Human page references missing ordinary item");
    const auto &owned = state.items.at(id);
    if (owned.inventory < 0 || owned.inventory > 999 ||
        state.catalog.at({0, id}).inventory != owned.inventory)
        throw std::invalid_argument("Human item view has inconsistent owned inventory");
    WorldHumanRow row;
    row.identity = id;
    row.slot = 4;
    row.name = found->name;
    row.cost = -1;
    row.stock = owned.inventory;
    row.fresh = owned.newly_unlocked;
    const auto icon = simulation::startup_world_item_icon_draws(state, id);
    if (!icon)
        throw std::invalid_argument("Human gift item has invalid icon data");
    row.icon = *icon;
    return row;
}
std::string quote(const WorldHumanRow &row) {
    return row.cost == -1 ? "库存 " + std::to_string(row.stock) : std::to_string(row.cost) + "G";
}
bool list_page(int raw) { return raw == 61 || raw == 64; }
bool browse_page(int raw) { return raw == 62 || raw == 73; }
} // namespace

bool world_human_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           ((page.legacy_page >= 60 && page.legacy_page <= 66) || page.legacy_page == 68 ||
            page.legacy_page == 69 || page.legacy_page == 70 || page.legacy_page == 73);
}
WorldHumanView world_human_view(const State &state, const Page &page,
                                std::optional<int> down_text_width) {
    if (!state.rules || !world_human_page(page))
        throw std::invalid_argument("Human view requires a supported page and catalogue");
    WorldHumanView view;
    view.raw = page.legacy_page;
    view.page = page.id;
    view.title = view.raw == 60   ? "冒险者信息"
                 : view.raw == 61 ? "职业一览"
                 : view.raw == 62 ? "职业一览"
                 : view.raw == 63 ? "转职"
                 : view.raw == 64 ? "赠送礼物"
                 : view.raw == 65 ? "赠送礼物"
                 : view.raw == 66 ? "赠送礼物"
                 : view.raw == 68 ? "装备变更"
                 : view.raw == 69 ? "能力上升"
                 : view.raw == 70 ? "职业大师"
                                  : "装备信息";
    // Init payload readiness can precede the framework's first page lifecycle update.
    // Maintained skin plans require a drawable page; neither drawing nor input advances it.
    if (page.lifecycle < 1 || !simulation::startup_world_human_page_ready(state, page.id))
        return view;
    const auto binding = state.page_human_bindings.find(page.id);
    if (binding == state.page_human_bindings.end())
        return view;
    auto details = simulation::startup_world_human_details(state, binding->second);
    if (view.raw == 60) {
        const auto presentation =
            simulation::inspect_startup_world_human_presentation(state, page.id);
        if (!presentation)
            return view;
        details = presentation->human;
        view.live = presentation->live;
        view.can_track = presentation->tracking_available;
        // Source plan preserves medal/weapon/body/HP order and reads only the Owner page tick.
        // Keyboard/mouse routing needs no font; only a down portrait waits for real text width.
        if (!view.live || view.live->state != 7 || down_text_width) {
            view.detail_skin = simulation::steam_human_detail_skin(state, page.id, down_text_width);
            if (!view.detail_skin)
                throw std::runtime_error("Invalid human detail presentation plan");
        }
    }
    const auto portrait = simulation::startup_world_portrait(state, binding->second);
    if (!details || !portrait)
        return view;
    view.human = binding->second;
    view.details = *details;
    view.name = simulation::startup_world_human_profile(state, view.human).value().name;
    view.profession = state.rules->jobs.at(details->profession).name;
    view.portrait_image = portrait->image;
    view.phase = state.page_phases.at(page.id);
    view.counter = state.page_counters.at(page.id);
    view.selection = state.human_page_selections.at(page.id);
    if (view.raw == 61 || view.raw == 64) {
        const auto cursor =
            simulation::inspect_startup_world_human_catalogue_cursor(state, page.id);
        if (!cursor)
            return view;
        view.first_visible = cursor->first;
        view.visible_rows = cursor->visible;
    }
    view.can_cancel = view.raw == 60 || view.raw == 61 || view.raw == 62 || view.raw == 64 ||
                      view.raw == 65 || view.raw == 73;
    view.can_confirm = view.raw != 63 || view.counter >= 197;
    if (view.raw == 69)
        view.can_confirm = view.counter < 39 || view.counter >= 45;
    if ((view.raw == 61 || view.raw == 64) && state.human_page_answers.count(page.id))
        return view; // The resumed parent must consume its answer on the simulation thread first.
    for (int slot = 0; slot < 4; ++slot) {
        view.equipment_names[slot] = details->equipment[slot]
                                         ? equipment(state, slot, *details->equipment[slot]).name
                                         : "无";
    }
    if (view.raw == 61 || view.raw == 62) {
        for (const int id : state.human_page_catalogs.at(page.id)) {
            const auto &job = state.rules->jobs.at(id);
            WorldHumanRow row;
            row.identity = id;
            row.name = job.name;
            row.cost = job.change_points;
            row.medals = job.required_medals;
            row.level = state.scene.world.world.ai.growth.at(view.human)
                            .definition.profession_levels.at(id);
            row.fresh = state.scripts.professions.at(id).pending_notice;
            view.rows.push_back(row);
        }
        view.attributes = state.human_attribute_display;
    } else if (view.raw == 64 || view.raw == 73) {
        for (const int id : state.equipment_page_catalogs.at(page.id).at(view.phase))
            view.rows.push_back(view.raw == 64 && view.phase == 4
                                    ? item(state, id)
                                    : equipment(state, view.phase, id));
    } else if (view.raw == 65 || view.raw == 66) {
        const auto &choice = state.human_equipment_choices.at(page.id);
        view.choice =
            choice[0] == 4 ? item(state, choice[1]) : equipment(state, choice[0], choice[1]);
        if (view.raw == 66) {
            view.gift_score = state.human_gift_scores.at(page.id);
            view.message = state.human_gift_messages.at(page.id);
        }
    } else if (view.raw == 63) {
        view.message = state.human_gift_messages.at(page.id);
        // The target is announced before counter 55 changes the shared current profession.
        // Keep the header's profession and level paired from the same current-owner details.
        view.target_profession = state.rules->jobs.at(state.page_job_bindings.at(page.id)).name;
    } else if (view.raw == 68) {
        const auto &choice = state.human_equipment_choices.at(page.id);
        view.choice = equipment(state, choice[0], choice[1]);
        view.combat = state.equipment_attribute_display;
    } else if (view.raw == 69) {
        const auto &choice = state.human_equipment_choices.at(page.id);
        if (choice[0] != 4)
            throw std::invalid_argument("Ordinary item result requires its item binding");
        view.choice = item(state, choice[1]);
        view.attributes = state.human_attribute_display;
    } else if (view.raw == 70) {
        const auto &job = state.rules->jobs.at(details->profession);
        view.mastery_attribute = job.mastery_attribute;
        view.mastery_value = job.mastery_value;
    }
    if (list_page(view.raw) || browse_page(view.raw)) {
        if (!view.rows.empty()) {
            if (view.selection < 0 || view.selection >= static_cast<int>(view.rows.size()))
                throw std::invalid_argument("Human page selection is outside its source catalogue");
            view.choice = view.rows[view.selection];
        }
        view.can_confirm = !view.rows.empty() || view.raw == 73;
    }
    view.initialized = true;
    return view;
}
WorldHumanLayout world_human_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Human page requires the supported logical viewport");
    const float width = std::min(340.F, extent.width - 16.F);
    const float height = std::min(300.F, extent.height - 58.F);
    WorldHumanLayout out;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.body = {p.x + 10, p.y + 71, width - 20, height - 122};
    out.rows = out.body;
    out.row_height = out.rows.height / 5;
    for (int tab = 0; tab < 4; ++tab)
        out.tabs[tab] = {p.x + 10 + tab * (width - 20) / 4, p.y + 49, (width - 20) / 4, 20};
    out.cancel = {p.x + 10, p.y + height - 28, 44, 20};
    out.confirm = {p.x + width - 54, p.y + height - 28, 44, 20};
    out.previous = {p.x + width / 2 - 28, p.y + height - 28, 24, 20};
    out.next = {p.x + width / 2 + 4, p.y + height - 28, 24, 20};
    out.professions = {p.x + width / 2 - 49, p.y + height - 28, 46, 20};
    out.gifts = {p.x + width / 2 + 3, p.y + height - 28, 46, 20};
    out.inspect = {p.x + width / 2 - 24, p.y + height - 28, 48, 20};
    return out;
}
Rectangle world_tracking_confirm(Extent extent) {
    return {extent.width / 2.F - 37, extent.height - 51.F, 74, 20};
}
void draw_world_human(const WorldHumanView &view, const WorldHumanLayout &layout, const Skin &skin,
                      bool enabled, const std::string &feedback) {
    if (view.raw == 60) {
        draw_world_human_detail(view, world_human_detail_layout(layout), skin, enabled, feedback);
        return;
    }
    if (world_human_source_child(view)) {
        draw_world_human_subwindow(view, world_human_subwindow_layout(layout, view.raw), skin,
                                   enabled, feedback);
        return;
    }
    skin.window(layout.panel, view.title);
    skin.content(
        {layout.body.x - 2, layout.body.y - 2, layout.body.width + 4, layout.body.height + 4});
    const bool active = enabled && view.initialized;
    if (view.initialized) {
        skin.sprites.human_image(view.portrait_image, {1, 27, 15, 14},
                                 {layout.panel.x + 10, layout.panel.y + 29, 15, 14});
        fitted(skin,
               view.name + "  " + view.profession + " Lv" + std::to_string(view.details.level),
               {layout.panel.x + 29, layout.panel.y + 30, layout.panel.width - 39, 14});
        const auto line = [&](const std::string &value, int row, int count, Color color = ink) {
            fitted(skin, value,
                   {layout.body.x + 3, layout.body.y + row * layout.body.height / count + 2,
                    layout.body.width - 6, layout.body.height / count},
                   color);
        };
        if (view.raw == 65 && view.choice) {
            line(view.choice->name, 0, 4, blue);
            line(quote(*view.choice), 1, 4);
            line("赠送给 " + view.name + "？", 2, 4);
        }
    }
    if (view.can_cancel)
        skin.button(layout.cancel, "返回", active);
    skin.button(layout.confirm, "确定", active && view.can_confirm);
    if (!feedback.empty())
        fitted(skin, feedback,
               {layout.body.x, layout.panel.y + layout.panel.height - 49, layout.body.width, 14},
               MAROON);
}
} // namespace ark::desktop::ui
