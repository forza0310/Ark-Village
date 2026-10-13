#include "world_village_skin.hpp"
#include "../common/steam_plan_draw.hpp"
#include <type_traits>

namespace ark::desktop::ui {
namespace {
Rectangle translated(std::array<int, 4> r, Vector2 origin) {
    return {origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
}
Rectangle row_box(std::array<int, 4> r, Vector2 origin, Rectangle panel) {
    const auto box = translated(r, origin);
    const float x = std::max(panel.x, box.x),
                right = std::min(panel.x + panel.width, box.x + box.width);
    return {x, box.y, std::max(0.F, right - x), box.height};
}
template <class Plan>
WorldFacilityPaintedControls execute(const Plan &plan, Rectangle panel, const Skin &skin,
                                     WorldFacilityPaintedControls out,
                                     const std::vector<int> &ids) {
    const auto origin = world_list_skin_origin(panel);
    SteamPlanDraw draw(origin, skin, plan.title);
    for (const auto &op : plan.operations)
        std::visit(
            [&](const auto &v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, sim::SteamStartupFrameDraw>)
                    draw.frame(v);
                else if constexpr (std::is_same_v<T, sim::SteamHumanImage>) {
                    // Steam51 publishes NEW as a named whole PNG, unlike64's explicit crop.
                    if (v.asset == sim::SteamHumanAsset::catalogue_new && !v.crop) {
                        const auto r = sim::steam_human_resource(v.asset);
                        if (!r)
                            throw std::invalid_argument("Missing activity NEW resource");
                        draw.whole_image(r->published_image, v.position);
                    } else
                        draw.human_image(v);
                } else if constexpr (std::is_same_v<T, sim::SteamFacilityImage>)
                    draw.facility_image(v);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityNumber>)
                    draw.number(v);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityMapchip2>)
                    draw.mapchip(v);
                else if constexpr (std::is_same_v<T, sim::SteamHumanPortrait>)
                    draw.portrait(v);
                else if constexpr (std::is_same_v<T, sim::SteamHumanOverviewText>)
                    draw.text(v, v.value);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityClip>)
                    draw.clip(v);
                else if constexpr (std::is_same_v<T, sim::SteamTownClip>) {
                    if (v.kind == sim::SteamTownClipKind::replace)
                        draw.replace_clip(v.rectangle);
                    else
                        draw.clip({v.kind == sim::SteamTownClipKind::pop
                                       ? sim::SteamFacilityClipKind::pop
                                       : sim::SteamFacilityClipKind::push_intersect,
                                   v.rectangle});
                } else if constexpr (std::is_same_v<T, sim::SteamTownText> ||
                                     std::is_same_v<T, sim::SteamVillageActivityText>) {
                    const float available = v.rectangle[0] == 26
                                                ? (v.value.rfind("建设", 0) == 0 ? 176.F : 95.F)
                                            : v.rectangle[0] == 50 || v.rectangle[0] == 48 ? 110.F
                                                                                           : 200.F;
                    draw.literal(v.value, v.rectangle, v.anchor, v.font_size, v.layout, v.rgb,
                                 available, v.rich);
                } else if constexpr (std::is_same_v<T, sim::SteamTownImage>) {
                    const auto r = sim::steam_town_resource(v.asset);
                    if (!r)
                        throw std::invalid_argument("Missing town progress resource");
                    if (!r->published_sprite && !v.crop)
                        draw.whole_image(r->published_image, v.position);
                    else
                        draw.published(r->published_image, r->published_sprite, v.position, v.frame,
                                       v.crop);
                } else if constexpr (std::is_same_v<T, sim::SteamVillageActivityImage>) {
                    const auto r = sim::steam_village_activity_resource(v.asset);
                    if (!r)
                        throw std::invalid_argument("Missing village activity resource");
                    if (!r->published_sprite && !v.crop)
                        draw.whole_image(r->published_image, v.position);
                    else
                        draw.published(r->published_image, r->published_sprite, v.position, v.frame,
                                       v.crop);
                } else if constexpr (std::is_same_v<T, sim::SteamHumanCatalogueScroll>) {
                    for (const auto &part : v.rectangles)
                        draw.rectangle(part);
                    const auto &r = *v.touch.rectangle;
                    out.scroll = Rectangle{origin.x + r[0] - 3, origin.y + r[1], float(r[2] + 3),
                                           float(r[3])};
                    out.scroll_visible = v.arguments[1];
                } else if constexpr (std::is_same_v<T, sim::SteamHumanEquipmentRow>) {
                    const int index = v.touch.value & 0xffff;
                    if (v.touch.rectangle)
                        out.rows.push_back(
                            {index, ids.at(index), row_box(*v.touch.rectangle, origin, panel)});
                } else if constexpr (std::is_same_v<T, sim::SteamStartupTouch>) {
                    if (v.component == 4 && v.rectangle)
                        out.confirm = translated(*v.rectangle, origin);
                    if (v.rectangle && (v.component == 11 || v.component == 3)) {
                        const int index = v.value & 0xffff;
                        // 52/48 button rectangles already include their published margins.
                        out.rows.push_back({index, ids.at(index),
                                            v.component == 11 ? row_box(*v.rectangle, origin, panel)
                                                              : translated(*v.rectangle, origin)});
                    }
                } else if constexpr (std::is_same_v<T, sim::SteamFacilityEffectTouch>) {
                    if (v.touch.rectangle && v.touch.component == 3)
                        out.confirm = translated(*v.touch.rectangle, origin);
                }
            },
            op);
    draw.finish();
    return out;
}
} // namespace
WorldFacilityPaintedControls draw_world_village_skin(const sim::SteamVillageActivityPlan &plan,
                                                     Rectangle panel, const Skin &skin,
                                                     WorldFacilityPaintedControls controls,
                                                     const std::vector<int> &ids) {
    return execute(plan, panel, skin, std::move(controls), ids);
}
WorldFacilityPaintedControls draw_world_town_skin(const sim::SteamTownProgressPlan &plan,
                                                  Rectangle panel, const Skin &skin,
                                                  WorldFacilityPaintedControls controls) {
    return execute(plan, panel, skin, std::move(controls), {0, 1, 2, 3, 4});
}
} // namespace ark::desktop::ui
