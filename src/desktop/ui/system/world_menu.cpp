// Source pages own hierarchy, selection, NEW markers and modal lifetime.
#include "world_menu.hpp"
#include "../common/skin.hpp"
#include "../information/world_information.hpp"
#include "ark/presentation/script_text.hpp"
#include "ark/simulation/presentation/steam_main_menu_skin.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace ark::desktop::ui {
std::string world_source_menu_label(int tag) {
    switch (tag) {
    case 0:
        return "建造";
    case 1:
        return "冒险";
    case 2:
        return "办公室";
    case 3:
        return "魔法壶";
    case 5:
        return "情报";
    case 6:
        return "系统";
    case 7:
        return "任务";
    case 8:
        return "中止任务";
    case 9:
        return "赠送礼物";
    case 10:
        return "城镇晋级";
    case 11:
        return "南瓜商会";
    case 12:
        return "开展活动";
    case 14:
        return "村情报";
    case 15:
        return "冒险者";
    case 16:
        return "收支情报";
    case 17:
        return "持有物品";
    case 18:
        return "装备一览";
    case 20:
        return "保存游戏";
    case 21:
        return "设置变更";
    case 22:
        return "游戏方法";
    case 23:
        return "高分";
    case 24:
        return "结束游戏";
    default:
        throw std::invalid_argument("Unknown source menu tag");
    }
}
bool world_source_menu_page(const simulation::rules::WorldScriptPage &p) {
    return p.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           (p.legacy_page == 3 || p.legacy_page == 4 || p.legacy_page == 7 || p.legacy_page == 10);
}
bool world_source_menu_enabled(int raw, int tag) { return raw != 10 || tag == 20; }
WorldSourceMenuLayout world_source_menu_layout(const simulation::StartupWorldMenuView &v,
                                               Extent e) {
    const float width = v.raw == 3 ? 91.F : 90.F, height = float(v.tags.size() * 28 + 5);
    const Vector2 origin{float(v.stored_position[0]), float(v.stored_position[1])};
    const float x = origin.x + width > e.width ? e.width - width : origin.x;
    const float y = origin.y + height > e.height ? e.height - height : origin.y;
    WorldSourceMenuLayout result{
        origin, {}, {8, e.height - 25.F, 58, 20}, {e.width - 61.F, e.height - 25.F, 53, 20}};
    for (std::size_t i = 0; i < v.tags.size(); ++i)
        result.rows.push_back({x, y + 28 * float(i), width, 28});
    return result;
}
std::optional<simulation::StartupWorldMenuInput>
world_source_menu_input(const simulation::StartupWorldMenuView &v, const WorldSourceMenuLayout &l,
                        const WorldMenuInput &in, bool left, bool right, bool blocked) {
    if (blocked || v.selection < 0 || static_cast<std::size_t>(v.selection) >= v.tags.size())
        return {};
    simulation::StartupWorldMenuInput out;
    const auto hit = [&](Rectangle r) { return in.click && CheckCollisionPointRec(*in.click, r); };
    if (in.escape || in.toggle || hit(l.back)) {
        out.cancel = true;
        return out;
    }
    if (in.click) {
        for (std::size_t n = 0; n < l.rows.size(); ++n)
            if (hit(l.rows[n])) {
                if (!world_source_menu_enabled(v.raw, v.tags[n]))
                    return {};
                out.select_row = int(n);
                return out;
            }
        if (hit(l.confirm) && world_source_menu_enabled(v.raw, v.tags[v.selection])) {
            out.confirm = true;
            return out;
        }
        return {};
    }
    out.up = in.up;
    out.down = in.down;
    out.left = left;
    out.right = right;
    out.confirm = in.enter;
    if (!out.up && !out.down && (out.right || out.confirm) &&
        !world_source_menu_enabled(v.raw, v.tags[v.selection]))
        return {};
    if (out.up || out.down || out.left || out.right || out.confirm)
        return out;
    return {};
}
void draw_world_source_menus(const simulation::StartupWorldRuntimeState &state, Extent extent,
                             const Skin &skin, bool enabled, const std::string &feedback) {
    namespace sim = simulation;
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() ||
        (!world_source_menu_page(*top) && top->legacy_page != 9))
        return;
    for (const auto &page : state.scripts.pages) {
        if (page.lifecycle == 4 || !world_source_menu_page(page))
            continue;
        const auto view = sim::inspect_startup_world_menu_page(state, page.id);
        if (!view)
            continue;
        const auto layout = world_source_menu_layout(*view, extent);
        const bool on_top = top->id == page.id;
        const Vector2 origin = layout.origin;
        if (view->raw == 3) {
            sim::SteamMainMenuSkinInput input;
            input.frame = view->frame;
            input.selection = view->selection;
            input.magic_unlocked =
                std::find(view->tags.begin(), view->tags.end(), 3) != view->tags.end();
            if (view->frame >= 3) {
                input.notices = sim::steam_main_menu_notices(state);
                if (input.magic_unlocked)
                    input.magic_period = sim::startup_magic_pot_menu_information(state);
            }
            sim::SteamMainMenuSkinOptions options;
            options.canvas = {extent.width, extent.height};
            options.origin = view->stored_position;
            options.on_top = on_top;
            options.top_is_main_menu = on_top;
            const auto plan = sim::steam_main_menu_skin(input, options);
            if (!plan)
                throw std::runtime_error("Invalid source main menu skin");
            for (const auto &draw : plan->draws) {
                if (const auto *p = std::get_if<sim::SteamMainMenuImage>(&draw)) {
                    if (p->sprite >= 0)
                        skin.sprites.indexed_sprite(
                            Sprites::Binding::steam_common, p->sprite, std::max(0, p->frame), 0,
                            p->image, {origin.x + p->position[0], origin.y + p->position[1]});
                    else if (p->crop[2] > 0 && p->crop[3] > 0) {
                        const auto &r = p->crop;
                        const Rectangle from{float(r[0]), float(r[1]), float(r[2]), float(r[3])};
                        const Rectangle to{origin.x + p->position[0], origin.y + p->position[1],
                                           float(r[2]), float(r[3])};
                        if (p->package == sim::SteamMainMenuPackage::common2) {
                            const auto path = sim::steam_main_menu_image(p->package, p->image);
                            if (!path)
                                throw std::runtime_error("Missing menu period resource");
                            skin.sprites.image(std::filesystem::path(*path).filename().string(),
                                               from, to, Sprites::Binding::steam_common);
                        } else
                            skin.sprites.indexed_image(Sprites::Binding::steam_common, p->image,
                                                       from, to);
                    }
                } else if (const auto *p = std::get_if<sim::SteamMainMenuText>(&draw)) {
                    skin.text.draw(world_source_menu_label(p->tag), origin.x + p->position[0],
                                   origin.y + p->position[1],
                                   {static_cast<unsigned char>(p->rgb[0]),
                                    static_cast<unsigned char>(p->rgb[1]),
                                    static_cast<unsigned char>(p->rgb[2]), 255});
                } else if (const auto *p = std::get_if<sim::SteamFacilityNumber>(&draw))
                    draw_information_number(*p, origin, skin);
            }
        } else {
            std::optional<sim::SteamInformationSkinPlan> plan;
            if (view->raw == 10) {
                sim::SteamInformationMenuSkinOptions options;
                options.canvas = {extent.width, extent.height};
                options.origin = view->stored_position;
                options.on_top = on_top;
                std::array<int, 5> widths{};
                for (int n = 0; n < 5; ++n)
                    widths[n] = int(skin.text.width(world_source_menu_label(view->tags[n])));
                options.measured_text_widths = widths;
                plan = sim::steam_system_menu_skin({view->frame, view->selection}, options);
            } else {
                sim::SteamNavigationMenuSkinOptions options;
                options.canvas = {extent.width, extent.height};
                options.origin = view->stored_position;
                options.on_top = on_top;
                std::vector<int> widths;
                for (int tag : view->tags)
                    widths.push_back(int(skin.text.width(world_source_menu_label(tag))));
                options.measured_text_widths = widths;
                plan = sim::steam_navigation_menu_skin(state, page.id, options);
            }
            if (!plan)
                throw std::runtime_error("Invalid source submenu skin");
            for (auto &draw : plan->draws)
                if (auto *label = std::get_if<sim::SteamInformationText>(&draw))
                    if (label->role == sim::SteamInformationTextRole::menu_entry &&
                        !world_source_menu_enabled(view->raw, label->argument))
                        label->rgb = {182, 174, 147};
            sim::StartupInformationPageView labels;
            labels.raw = view->raw;
            draw_information_plan(*plan, labels, origin, skin);
        }
        if (on_top) {
            skin.button(layout.back, "返回", enabled);
            skin.choice(layout.confirm, "选择",
                        enabled &&
                            world_source_menu_enabled(view->raw, view->tags.at(view->selection)));
            if (!feedback.empty())
                skin.text.draw(feedback, 8, extent.height - 42.F, MAROON, 10);
        }
    }
    // The approved PC HUD reads the same pure query; never fake a raw9 page to
    // invoke its menu-specific status skin or fabricate source output requests.
    if (top->legacy_page != 9) {
        const auto data = sim::startup_menu_information(state);
        if (!data)
            throw std::runtime_error("Invalid source menu HUD");
        skin.right("村子点数 " + std::to_string(data->village_points), extent.width - 5.F, 27, ink,
                   10);
        if (data->monster_remaining)
            skin.right("剩余怪物 " + std::to_string(*data->monster_remaining), extent.width - 5.F,
                       42, ink, 10);
        if (data->task) {
            const auto &t = *data->task;
            const std::array<std::string, 4> rows{
                {"冒险者 " + std::to_string(t.humans), "怪物 " + std::to_string(t.monsters),
                 "住宅 " + std::to_string(t.residences),
                 "剩余周数 " + std::to_string(t.remaining_subperiods)}};
            for (int n = 0; n < 4; ++n)
                skin.right(rows[n], extent.width - 5.F, 62 + 15.F * n, ink, 10);
        }
    }
}
Rectangle world_menu_button(Extent extent) {
    // At minimum width popularity begins at x103; pause ends at x60.
    return {62, extent.height - 25.F, 40, 22};
}
} // namespace ark::desktop::ui
