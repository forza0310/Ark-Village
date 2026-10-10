#include "world_human.hpp"
#include "../../scene/world_overlay_render.hpp"
#include "ark/simulation/actors/rules/human_management.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "../common/skin.hpp"
#include "world_human_detail.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupHumanPageAction;
constexpr const char *attributes[]{"体力", "力量", "灵活", "结实", "魔力", "运气"};
constexpr const char *combat[]{"最大HP", "攻击", "防御", "魔法"};
constexpr const char *slots[]{"武器", "衣服", "盾帽", "饰品", "道具"};
bool hit(std::optional<Vector2> p, Rectangle r) {
    return p && p->x >= r.x && p->y >= r.y && p->x < r.x + r.width && p->y < r.y + r.height;
}
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
WorldHumanView world_human_view(const State &state, const Page &page) {
    if (!state.rules || !world_human_page(page))
        throw std::invalid_argument("Human view requires a supported page and catalogue");
    WorldHumanView view;
    view.raw = page.legacy_page;
    view.page = page.id;
    view.title = view.raw == 60   ? "人物情报"
                 : view.raw == 61 ? "转职"
                 : view.raw == 62 ? "转职确认"
                 : view.raw == 63 ? "职业变更"
                 : view.raw == 64 ? "赠送礼物"
                 : view.raw == 65 ? "赠送确认"
                 : view.raw == 66 ? "礼物评价"
                 : view.raw == 68 ? "装备能力"
                 : view.raw == 69 ? "能力提升"
                 : view.raw == 70 ? "职业大师"
                                  : "装备情报";
    if (!simulation::startup_world_human_page_ready(state, page.id))
        return view;
    const auto binding = state.page_human_bindings.find(page.id);
    if (binding == state.page_human_bindings.end())
        return view;
    const auto details = simulation::startup_world_human_details(state, binding->second);
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
        if (details->equipment[slot]) {
            const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
            const int id = *details->equipment[slot];
            const auto found =
                std::find_if(state.rules->equipment.begin(), state.rules->equipment.end(),
                             [&](const auto &e) { return e.shop.kind == kind && e.shop.id == id; });
            if (found == state.rules->equipment.end())
                throw std::invalid_argument("Missing human equipment icon");
            view.equipment_icons[slot] = kind == 1 ? found->shop.type : found->render_image;
            view.equipment_values[slot] = found->shop.combat;
        }
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
    for (int tab = 0; tab < 5; ++tab)
        out.gift_tabs[tab] = {p.x + 10 + tab * (width - 20) / 5, p.y + 49, (width - 20) / 5, 20};
    out.cancel = {p.x + 10, p.y + height - 28, 44, 20};
    out.confirm = {p.x + width - 54, p.y + height - 28, 44, 20};
    out.previous = {p.x + width / 2 - 28, p.y + height - 28, 24, 20};
    out.next = {p.x + width / 2 + 4, p.y + height - 28, 24, 20};
    out.professions = {p.x + width / 2 - 49, p.y + height - 28, 46, 20};
    out.gifts = {p.x + width / 2 + 3, p.y + height - 28, 46, 20};
    out.inspect = {p.x + width / 2 - 24, p.y + height - 28, 48, 20};
    return out;
}
int world_human_first_row(const WorldHumanView &view) { return std::max(0, view.selection - 4); }
std::optional<WorldHumanIntent> world_human_input(const WorldHumanView &view,
                                                  const WorldHumanLayout &base_layout,
                                                  const WorldHumanInput &input, bool blocked) {
    const auto layout = view.raw == 60 ? world_human_detail_layout(base_layout) : base_layout;
    if (blocked || !view.initialized)
        return {};
    const auto intent = [&](Action action, int selection = 0) {
        return WorldHumanIntent{action, view.page, selection};
    };
    if (view.can_cancel && (input.escape || hit(input.click, layout.cancel)))
        return intent(Action::cancel);
    if (view.raw == 60 || view.raw == 64) {
        const int count = view.raw == 60 ? 4 : 5;
        for (int tab = 0; tab < count; ++tab)
            if (hit(input.click, view.raw == 60 ? layout.tabs[tab] : layout.gift_tabs[tab]))
                return intent(view.raw == 60 ? Action::view_tab : Action::equipment_slot, tab);
        if (view.raw == 64 && (input.left || input.right))
            return intent(Action::equipment_slot, (view.phase + (input.left ? 4 : 1)) % 5);
    }
    if (view.raw == 60) {
        if (input.professions || hit(input.click, layout.professions))
            return intent(Action::professions);
        if (input.gifts || hit(input.click, layout.gifts))
            return intent(Action::gifts);
        if (input.inspect && view.phase == 2)
            return intent(Action::inspect_equipment, 0);
        if (input.left || input.right)
            return intent(input.left ? Action::previous : Action::next);
        if (view.phase == 2)
            for (int slot = 0; slot < 4; ++slot)
                if (hit(input.click, {layout.body.x, layout.body.y + slot * layout.body.height / 4,
                                      layout.body.width, layout.body.height / 4}))
                    return intent(Action::inspect_equipment, slot);
    }
    if (list_page(view.raw) || browse_page(view.raw)) {
        if (!view.rows.empty()) {
            if (input.up || input.down || (browse_page(view.raw) && (input.left || input.right)))
                return intent(input.up || input.left ? Action::previous : Action::next);
            if (input.wheel_rows)
                return intent(Action::select, std::clamp(view.selection + input.wheel_rows, 0,
                                                         static_cast<int>(view.rows.size()) - 1));
            if (list_page(view.raw))
                for (int row = 0; row < 5 && world_human_first_row(view) + row <
                                                 static_cast<int>(view.rows.size());
                     ++row)
                    if (hit(input.click, {layout.rows.x, layout.rows.y + row * layout.row_height,
                                          layout.rows.width, layout.row_height}))
                        return intent(Action::select, world_human_first_row(view) + row);
            if (browse_page(view.raw) &&
                (hit(input.click, layout.previous) || hit(input.click, layout.next)))
                return intent(hit(input.click, layout.previous) ? Action::previous : Action::next);
            if (view.raw == 64 && view.phase != 4 &&
                (input.inspect || hit(input.click, layout.inspect)))
                return intent(Action::inspect_equipment);
        }
    }
    if (view.raw == 70 && view.phase == 2) {
        if (input.up || input.down || input.left || input.right)
            return intent(Action::next);
        for (int row = 0; row < 2; ++row)
            if (hit(input.click, {layout.body.x, layout.body.y + (row + 2) * layout.body.height / 4,
                                  layout.body.width, layout.body.height / 4}))
                return intent(Action::select, row);
    }
    if (view.can_confirm && (input.enter || hit(input.click, layout.confirm)))
        return intent(Action::confirm);
    return {};
}

void draw_world_human(const WorldHumanView &view, const WorldHumanLayout &layout, const Skin &skin,
                      bool enabled, const std::string &feedback) {
    if (view.raw == 60) {
        draw_world_human_detail(view, world_human_detail_layout(layout), skin, enabled, feedback);
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
        if (view.raw == 60 || view.raw == 64) {
            constexpr const char *tabs[]{"概况", "属性", "装备", "魔法"};
            const int count = view.raw == 60 ? 4 : 5;
            for (int n = 0; n < count; ++n) {
                const auto box = view.raw == 60 ? layout.tabs[n] : layout.gift_tabs[n];
                if (view.phase == n)
                    DrawRectangleRec(box, {210, 229, 195, 255});
                skin.centered(view.raw == 60 ? tabs[n] : slots[n], box, ink, 10);
            }
        }
        const auto line = [&](const std::string &value, int row, int count, Color color = ink) {
            fitted(skin, value,
                   {layout.body.x + 3, layout.body.y + row * layout.body.height / count + 2,
                    layout.body.width - 6, layout.body.height / count},
                   color);
        };
        const auto grid = [&](const std::string &value, int index, int row_count) {
            fitted(skin, value,
                   {layout.body.x + 3 + (index % 2) * layout.body.width / 2,
                    layout.body.y + (index / 2) * layout.body.height / row_count + 2,
                    layout.body.width / 2 - 6, layout.body.height / row_count});
        };
        if (list_page(view.raw)) {
            if (view.rows.empty())
                line("暂无候选", 0, 5);
            const int first = world_human_first_row(view);
            for (int n = first; n < static_cast<int>(view.rows.size()) && n < first + 5; ++n) {
                const auto &row = view.rows[n];
                Rectangle box{layout.rows.x, layout.rows.y + (n - first) * layout.row_height,
                              layout.rows.width, layout.row_height};
                if (n == view.selection)
                    DrawRectangleRec(box, {255, 236, 174, 255});
                const auto value = view.raw == 61 ? std::to_string(row.cost) + "点" : quote(row);
                const float icon_width = row.icon.empty() ? 0 : 20;
                draw_world_visuals(row.icon, skin.sprites, {box.x + 4, box.y + 1}, 1);
                fitted(
                    skin, (row.fresh ? "* " : "") + row.name,
                    {box.x + 3 + icon_width, box.y + 2, box.width * .60F - icon_width, box.height});
                skin.right(value, box.x + box.width - 3, box.y + 2, blue, 10);
            }
        } else if (view.raw == 60) {
            if (view.phase == 0) {
                grid("经验 " + std::to_string(view.details.experience) + "/" +
                         std::to_string(view.details.threshold),
                     0, 4);
                grid(view.details.resident ? "住宅 已入住" : "住宅 未入住", 1, 4);
                grid("满足 " + std::to_string(view.details.satisfaction), 2, 4);
                grid("努力 " + std::to_string(view.details.effort), 3, 4);
                grid("勋章 " + std::to_string(view.details.medals), 4, 4);
                grid(std::string(combat[0]) + " " + std::to_string(view.details.combat[0]), 5, 4);
                grid("攻击 " + std::to_string(view.details.combat[1]), 6, 4);
                grid("防御 " + std::to_string(view.details.combat[2]) + " 魔法 " +
                         std::to_string(view.details.combat[3]),
                     7, 4);
            } else if (view.phase == 1) {
                for (int n = 0; n < 6; ++n)
                    grid(std::string(attributes[n]) + " " +
                             std::to_string(view.details.attributes[n]),
                         n, 3);
            } else if (view.phase == 2) {
                for (int n = 0; n < 4; ++n)
                    line(std::string(slots[n]) + "  " + view.equipment_names[n], n, 4);
            } else {
                for (int n = 0; n < 4; ++n)
                    line("魔法 " + std::to_string(n + 1) +
                             (view.details.spells[n] ? "  可使用" : "  不可使用"),
                         n, 4);
            }
        } else if (view.raw == 62 && view.choice) {
            fitted(skin,
                   view.choice->name + "  " + std::to_string(view.choice->cost) + "点  勋章" +
                       std::to_string(view.choice->medals),
                   {layout.tabs[0].x, layout.tabs[0].y + 3, layout.body.width, 16}, blue);
            for (int n = 0; n < 6; ++n)
                grid(std::string(attributes[n]) + " " + std::to_string(view.attributes[0][n]) +
                         ">" + std::to_string(view.attributes[1][n]),
                     n, 3);
        } else if (view.raw == 63) {
            line(view.target_profession, 0, 4, blue);
            line(view.counter < 55 ? "转职准备中" : "职业已变更", 1, 4);
            line(view.message, 2, 4);
        } else if (view.raw == 65 && view.choice) {
            line(view.choice->name, 0, 4, blue);
            line(quote(*view.choice), 1, 4);
            line("赠送给 " + view.name + "？", 2, 4);
        } else if (view.raw == 66) {
            if (view.choice)
                line(view.choice->name, 0, 4);
            line(view.message, 1, 4);
            line("评价 " + std::to_string(view.gift_score), 2, 4, blue);
        } else if (view.raw == 68) {
            for (int n = 0; n < 4; ++n)
                line(std::string(combat[n]) + "  " + std::to_string(view.combat[0][n]) + " > " +
                         std::to_string(view.combat[1][n]),
                     n, 4);
        } else if (view.raw == 69) {
            for (int n = 0; n < 6; ++n)
                grid(std::string(attributes[n]) + " " + std::to_string(view.attributes[0][n]) +
                         " > " + std::to_string(view.attributes[1][n]),
                     n, 3);
        } else if (view.raw == 70) {
            line(view.profession + " 大师", 0, 4, blue);
            if (view.mastery_attribute >= 0 && view.mastery_attribute < 6)
                line(std::string(attributes[view.mastery_attribute]) + " +" +
                         std::to_string(view.mastery_value),
                     1, 4);
            else if (view.mastery_attribute >= 10)
                line("魔法 " + std::to_string(view.mastery_attribute - 9), 1, 4);
            if (view.phase == 2)
                for (int n = 0; n < 2; ++n) {
                    Rectangle box{layout.body.x, layout.body.y + (n + 2) * layout.body.height / 4,
                                  layout.body.width, layout.body.height / 4};
                    if (n == view.selection)
                        DrawRectangleRec(box, {255, 236, 174, 255});
                    line(n == 0 ? "转职" : "保持现状", n + 2, 4);
                }
        } else if (view.raw == 73 && view.choice) {
            fitted(skin, view.choice->name,
                   {layout.tabs[0].x, layout.tabs[0].y + 3, layout.body.width, 16}, blue);
            for (int n = 0; n < 4; ++n)
                line(std::string(combat[n]) + "  " + std::to_string(view.choice->combat[n]), n, 4);
        }
    }
    if (view.can_cancel)
        skin.button(layout.cancel, "返回", active);
    skin.button(layout.confirm, view.raw == 60 || view.raw == 73 ? "关闭" : "确定",
                active && view.can_confirm);
    if (view.raw == 60) {
        skin.button(layout.professions, "转职", active);
        skin.button(layout.gifts, "赠送", active);
    } else if (browse_page(view.raw)) {
        skin.button(layout.previous, "<", active && !view.rows.empty());
        skin.button(layout.next, ">", active && !view.rows.empty());
    } else if (view.raw == 64 && view.phase != 4)
        skin.button(layout.inspect, "情报", active && !view.rows.empty());
    if (!feedback.empty())
        fitted(skin, feedback,
               {layout.body.x, layout.panel.y + layout.panel.height - 49, layout.body.width, 14},
               MAROON);
}
} // namespace ark::desktop::ui
