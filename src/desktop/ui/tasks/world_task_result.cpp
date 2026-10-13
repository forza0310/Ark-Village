// Victory animation consumes the Owner's independent clocks and ordered source plan.
#include "../common/steam_plan_draw.hpp"
#include "../common/world_list_controls.hpp"
#include "../common/world_reports.hpp"
#include <type_traits>

namespace ark::desktop::ui {
void prepare_world_victory(WorldVictoryView &v, const simulation::StartupWorldRuntimeState &s,
                           const Skin &skin) {
    v.skin.reset();
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [&](const auto &p) { return p.id == v.page; });
    if (!v.initialized || page == s.scripts.pages.end() || page->lifecycle != 2)
        return;
    simulation::SteamTaskVictoryOptions o;
    o.view_y = 23;
    o.title_widths =
        std::array<int, 2>{int(skin.text.width("胜利！")), int(skin.text.width("胜利！"))};
    o.reputation_suffix_width = int(skin.text.width("上升了!"));
    v.skin = simulation::steam_task_victory_skin(s, v.page, o);
    if (!v.skin)
        throw std::invalid_argument("Invalid bound victory plan");
}
void draw_world_victory(const WorldVictoryView &v, const WorldPageLayout &l, const Skin &skin,
                        bool interactive) {
    if (!v.skin) {
        skin.window(l.panel, "胜利！");
        return;
    }
    const auto origin = world_list_skin_origin(l.panel);
    SteamPlanDraw draw(origin, skin, v.skin->title);
    for (const auto &op : v.skin->operations)
        std::visit(
            [&](const auto &p) {
                using T = std::decay_t<decltype(p)>;
                if constexpr (std::is_same_v<T, simulation::SteamStartupFrameDraw>)
                    draw.frame(p);
                else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionText>)
                    draw.literal(p.value, p.rectangle, p.anchor, p.font_size, p.layout, p.rgb, 200);
                else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionClip>)
                    draw.clip({p.push ? simulation::SteamFacilityClipKind::push_intersect
                                      : simulation::SteamFacilityClipKind::pop,
                               p.rectangle});
                else if constexpr (std::is_same_v<T, simulation::SteamHumanImage>)
                    draw.human_image(p);
                else if constexpr (std::is_same_v<T, simulation::SteamFacilityNumber>)
                    draw.number(p);
                else if constexpr (std::is_same_v<T, simulation::SteamTaskRosterImage>) {
                    const auto resource = simulation::steam_task_roster_resource(p.asset);
                    if (!resource)
                        throw std::invalid_argument("Missing victory background or shadow");
                    if (!resource->published_sprite && !p.crop)
                        draw.whole_image(resource->published_image, p.position);
                    else
                        draw.published(resource->published_image, resource->published_sprite,
                                       p.position, p.frame, p.crop);
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskVictoryActor>) {
                    const auto actor = simulation::steam_task_victory_actor_skin(p);
                    if (!actor)
                        throw std::invalid_argument("Invalid victory actor pose");
                    draw.portrait(simulation::SteamHumanPortrait{*actor, p.position, {}, false});
                }
            },
            op);
    draw.finish();
    // PC confirmation forwards one source command; drawing cannot award or fast-forward.
    skin.button(l.confirm, v.counter < 40 ? "显示全文" : "确定", interactive);
}
} // namespace ark::desktop::ui
