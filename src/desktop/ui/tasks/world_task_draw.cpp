// Execute ordered task operations through the shared GPU/font backend; no Owner or clock.
#include "../common/steam_plan_draw.hpp"
#include "world_tasks.hpp"
#include <type_traits>
namespace ark::desktop::ui {
namespace {
template <class Plan>
WorldFacilityPaintedControls execute(const Plan &plan, Vector2 origin, const Skin &skin) {
    SteamPlanDraw draw(origin, skin, plan.title);
    WorldFacilityPaintedControls c;
    for (const auto &op : plan.operations)
        std::visit(
            [&](const auto &p) {
                using T = std::decay_t<decltype(p)>;
                if constexpr (std::is_same_v<T, simulation::SteamStartupFrameDraw>)
                    draw.frame(p);
                else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionImage>) {
                    const auto r = simulation::steam_task_selection_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing task selection resource");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.published(r->published_image, r->published_sprite, p.position, p.frame,
                                       p.crop);
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionText>)
                    draw.literal(p.value, p.rectangle, p.anchor, p.font_size, p.layout, p.rgb,
                                 p.rectangle[0] == 29   ? 102.F
                                 : p.rectangle[0] == 69 ? 150.F
                                                        : 180.F);
                else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionClip>)
                    draw.clip({p.push ? simulation::SteamFacilityClipKind::push_intersect
                                      : simulation::SteamFacilityClipKind::pop,
                               p.rectangle});
                else if constexpr (std::is_same_v<T, simulation::SteamHumanImage>) {
                    const auto r = simulation::steam_human_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing task decoration");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.human_image(p);
                } else if constexpr (std::is_same_v<T, simulation::SteamHumanPortrait>)
                    draw.portrait(p);
                else if constexpr (std::is_same_v<T, simulation::SteamFacilityImage>) {
                    const auto r = simulation::steam_facility_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing recruitment label");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.facility_image(p);
                } else if constexpr (std::is_same_v<T, simulation::SteamFacilityNumber>)
                    draw.number(p);
                else if constexpr (std::is_same_v<T, simulation::SteamHumanCatalogueScroll>) {
                    for (const auto &rect : p.rectangles)
                        draw.rectangle(rect);
                    const auto &r = *p.touch.rectangle;
                    c.scroll = Rectangle{origin.x + r[0] - 3, origin.y + r[1], float(r[2] + 3),
                                         float(r[3])};
                    c.scroll_visible = 5;
                } else if constexpr (std::is_same_v<T, simulation::SteamFacilityArrowTouch>) {
                    const auto arrow = steam_plan_arrow(p, skin);
                    const auto &crop = arrow.crop;
                    const auto &destination = arrow.destination;
                    skin.sprites.image(
                        "buttoneffect.png",
                        {float(crop[0]), float(crop[1]), float(crop[2]), float(crop[3])},
                        {origin.x + destination[0], origin.y + destination[1], destination[2],
                         destination[3]},
                        Sprites::Binding::steam_common);
                    const auto &r = arrow.touch_rectangle;
                    const Rectangle hit{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
                    if (p.value == 16)
                        c.previous = hit;
                    else if (p.value == 18)
                        c.next = hit;
                    else
                        throw std::invalid_argument("Unsupported task arrow intent");
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskRosterImage>) {
                    const auto r = simulation::steam_task_roster_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing task roster resource");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.published(r->published_image, r->published_sprite, p.position, p.frame,
                                       p.crop);
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskRosterTouch>) {
                    if (p.touch.rectangle) {
                        const auto &r = *p.touch.rectangle;
                        c.rows.push_back(
                            {p.touch.value & 0xffff,
                             p.touch.option,
                             {origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])}});
                    }
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskDepartureActor>) {
                    const auto actor = simulation::steam_task_departure_actor_skin(p);
                    if (!actor)
                        throw std::invalid_argument("Invalid task departure actor");
                    draw.portrait(simulation::SteamHumanPortrait{*actor, p.position, {}, false});
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskDeadlineImage>) {
                    const auto r = simulation::steam_task_deadline_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing task deadline resource");
                    draw.published(r->published_image, r->published_sprite, p.position, p.frame);
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskDeadlineMonster>) {
                    draw.monster(p.sprite, p.image, p.frame, p.position);
                } else if constexpr (std::is_same_v<T, simulation::SteamFacilityMapchipDraw>) {
                    draw.mapchip(p);
                } else if constexpr (std::is_same_v<T, simulation::SteamStartupTouch>) {
                    if (p.rectangle) {
                        const auto &r = *p.rectangle;
                        const Rectangle b{origin.x + r[0], origin.y + r[1], float(r[2]),
                                          float(r[3])};
                        if (p.component == 11) {
                            const int index = p.value & 0xffff;
                            c.rows.push_back({index, 0, b});
                        } else if (p.component == 3) {
                            c.rows.push_back({p.value & 0xffff, 0, b});
                        } else if (p.component == 4)
                            c.confirm = b;
                    }
                }
            },
            op);
    draw.finish();
    return c;
}
} // namespace
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskSelectionPlan &p,
                                                  Vector2 o, const Skin &s) {
    return execute(p, o, s);
}
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskRosterPlan &p,
                                                  Vector2 o, const Skin &s) {
    return execute(p, o, s);
}
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskResultPlan &p,
                                                  Vector2 o, const Skin &s) {
    return execute(p, o, s);
}
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskDeadlinePlan &p,
                                                  Vector2 o, const Skin &s) {
    return execute(p, o, s);
}
} // namespace ark::desktop::ui
