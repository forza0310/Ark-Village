#include "world_dungeon_footer.hpp"
#include "../common/steam_plan_draw.hpp"
#include <type_traits>

namespace ark::desktop::ui {
simulation::SteamDungeonFooterPlan world_dungeon_footer(const app::WorldFrame &frame,
                                                        Extent extent) {
    simulation::SteamDungeonFooterPlan out;
    if (!frame.presentation || frame.presentation->ordinal != frame.presentation_epoch ||
        frame.save_menu_open)
        return out;
    for (const auto id : frame.presentation->pages) {
        const auto &pages = frame.state->scripts.pages;
        const auto page =
            std::find_if(pages.begin(), pages.end(), [id](const auto &p) { return p.id == id; });
        if (page == pages.end())
            throw std::invalid_argument("Footer presentation page no longer exists");
        if (page->kind != simulation::rules::WorldScriptPageKind::scene)
            continue;
        auto plan = simulation::steam_dungeon_footer_skin(
            *frame.state, id, *frame.presentation, {0, 0, int(extent.width), int(extent.height)}, 0,
            false);
        if (!plan)
            throw std::invalid_argument("Footer state and frozen presentation disagree");
        out.operations.insert(out.operations.end(), plan->operations.begin(),
                              plan->operations.end());
    }
    return out;
}
void draw_world_dungeon_footer(const simulation::SteamDungeonFooterPlan &plan, const Skin &skin,
                               const simulation::SteamDungeonFooterLanguageProfile &language) {
    SteamPlanDraw draw({0, 0}, skin, {});
    for (const auto &op : plan.operations)
        std::visit(
            [&](const auto &p) {
                using T = std::decay_t<decltype(p)>;
                if constexpr (std::is_same_v<T, simulation::StartupSkinDraw>)
                    skin.sprites.dungeon_footer_image(p, language, draw.clip_bounds());
                else if constexpr (std::is_same_v<T, simulation::SteamHumanClip>)
                    draw.clip({p.kind == simulation::SteamHumanClipKind::pop
                                   ? simulation::SteamFacilityClipKind::pop
                                   : simulation::SteamFacilityClipKind::push_intersect,
                               p.rectangle});
                else if constexpr (std::is_same_v<T, simulation::SteamHumanPortrait>)
                    draw.portrait(p);
                else if constexpr (std::is_same_v<T, simulation::SteamMonsterBody>)
                    draw.monster(p.sprite, p.image, p.frame, p.position);
                else if constexpr (std::is_same_v<T, simulation::SteamFacilityNumber>)
                    draw.number(p);
                else if constexpr (std::is_same_v<T, simulation::StartupSkinRect>) {
                    // Crop individual outline edges; clipping must not invent a new inner border.
                    const auto fill = [&](Rectangle box) {
                        if (const auto clip = draw.clip_bounds()) {
                            const auto b = clip_sprite_blit({{0, 0, 1, 1}, box}, *clip);
                            if (!b)
                                return;
                            box = b->destination;
                        }
                        DrawRectangleRec(box, steam_plan_color(p.rgb));
                    };
                    const auto &r = p.rect;
                    if (!p.outline)
                        fill({float(r[0]), float(r[1]), float(r[2]), float(r[3])});
                    else {
                        fill({float(r[0]), float(r[1]), float(r[2]), 1});
                        fill({float(r[0]), float(r[1] + r[3] - 1), float(r[2]), 1});
                        fill({float(r[0]), float(r[1]), 1, float(r[3])});
                        fill({float(r[0] + r[2] - 1), float(r[1]), 1, float(r[3])});
                    }
                }
            },
            op);
    draw.finish();
}
} // namespace ark::desktop::ui
