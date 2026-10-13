// Interpret maintained order, clipping and artwork without publishing business effects.
#include "world_facility_skin.hpp"
#include "../common/steam_plan_draw.hpp"
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
Rectangle row_box(std::array<int, 4> r, Vector2 origin, Rectangle panel) {
    const float x = std::max(panel.x, origin.x + r[0]);
    const float right = std::min(panel.x + panel.width, origin.x + r[0] + r[2]);
    return {x, origin.y + r[1], std::max(0.F, right - x), float(r[3])};
}
} // namespace
Vector2 world_facility_skin_origin(Rectangle p) {
    return world_list_skin_origin(p);
}
void draw_world_facility_effect(const sim::SteamFacilityEffectPlan &p, Vector2 origin,
                                const Skin &skin) {
    SteamPlanDraw draw(origin, skin, p.title);
    for (const auto &op : p.operations) {
        if (const auto *v = std::get_if<sim::SteamStartupFrameDraw>(&op))
            draw.frame(*v);
        else if (const auto *v = std::get_if<sim::SteamHumanImage>(&op))
            draw.human_image(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityImage>(&op))
            draw.facility_image(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityNumber>(&op))
            draw.number(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityMapchip2>(&op))
            draw.mapchip(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityClip>(&op))
            draw.clip(*v);
        else if (const auto *v = std::get_if<sim::SteamHumanPortrait>(&op))
            draw.portrait(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityEffectText>(&op))
            draw.literal(v->value, v->rectangle, v->anchor, v->font_size, v->layout, v->rgb,
                         v->rectangle[0] == 50    ? 132.F
                         : v->rectangle[0] == 120 ? 200.F
                                                  : 60.F);
        else if (const auto *v = std::get_if<sim::SteamFacilityEffectImage>(&op)) {
            const auto r = sim::steam_facility_effect_resource(v->asset);
            if (!r)
                throw std::invalid_argument("Missing facility effect resource");
            draw.published(r->published_image, r->published_sprite, v->position, v->frame);
        } else if (const auto *v = std::get_if<sim::SteamFacilityEffectScroll>(&op))
            for (const auto &part : v->skin.rectangles)
                draw.rectangle(part);
        // Touch requests only describe the separate PC input conversion below.
    }
    draw.finish();
}
void draw_world_facility_reputation(const sim::SteamFacilityReputationPlan &p, Vector2 origin,
                                    const Skin &skin) {
    SteamPlanDraw draw(origin, skin, p.title);
    for (const auto &op : p.operations) {
        if (const auto *v = std::get_if<sim::SteamStartupFrameDraw>(&op))
            draw.frame(*v);
        else if (const auto *v = std::get_if<sim::SteamHumanImage>(&op))
            draw.human_image(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityImage>(&op))
            draw.facility_image(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityNumber>(&op))
            draw.number(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityMapchip2>(&op))
            draw.mapchip(*v);
        else if (const auto *v = std::get_if<sim::SteamFacilityClip>(&op))
            draw.clip(*v);
        else if (const auto *v = std::get_if<sim::SteamHumanPortrait>(&op))
            draw.portrait(*v);
        else if (const auto *v = std::get_if<sim::SteamHumanOverviewText>(&op))
            draw.literal(v->value, v->rectangle, v->anchor, v->font_size, v->layout, v->rgb, 202.F);
        else if (const auto *v = std::get_if<sim::SteamFacilityReputationImage>(&op)) {
            const auto r = sim::steam_facility_reputation_resource(v->asset);
            if (!r)
                throw std::invalid_argument("Missing facility reputation resource");
            draw.published(r->published_image, r->published_sprite, v->position, v->frame);
        }
    }
    draw.finish();
}
WorldFacilityPaintedControls world_facility_controls(const sim::SteamHumanOverviewPlan &p,
                                                     Vector2 origin, Rectangle panel,
                                                     const Skin &skin,
                                                     WorldFacilityPaintedControls out,
                                                     const std::vector<int> &ids) {
    for (const auto &op : p.operations) {
        if (const auto *v = std::get_if<sim::SteamHumanEquipmentRow>(&op);
            v && v->touch.rectangle) {
            const int i = v->touch.value & 0xffff;
            out.rows.push_back({i, ids.at(i), row_box(*v->touch.rectangle, origin, panel)});
        } else if (const auto *v = std::get_if<sim::SteamFacilityArrowTouch>(&op)) {
            const auto expanded = steam_plan_arrow(*v, skin);
            const auto &r = expanded.touch_rectangle;
            const Rectangle hit{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            if (v->value == 16)
                out.previous = hit;
            else if (v->value == 18)
                out.next = hit;
        } else if (const auto *v = std::get_if<sim::SteamHumanCatalogueScroll>(&op)) {
            const auto &r = *v->touch.rectangle;
            out.scroll =
                Rectangle{origin.x + r[0] - 3, origin.y + r[1], float(r[2] + 3), float(r[3])};
            out.scroll_visible = v->arguments[1];
        }
    }
    return out;
}
WorldFacilityPaintedControls world_facility_controls(const sim::SteamFacilityEffectPlan &p,
                                                     Vector2 origin, Rectangle panel,
                                                     WorldFacilityPaintedControls out,
                                                     const std::vector<int> &ids) {
    for (const auto &op : p.operations) {
        if (const auto *v = std::get_if<sim::SteamFacilityEffectTouch>(&op);
            v && v->touch.component == 11 && v->touch.rectangle) {
            const int i = v->touch.value & 0xffff;
            out.rows.push_back({i, ids.at(i), row_box(*v->touch.rectangle, origin, panel)});
        } else if (const auto *v = std::get_if<sim::SteamFacilityEffectScroll>(&op)) {
            const auto &r = *v->touch.rectangle;
            out.scroll =
                Rectangle{origin.x + r[0] - 3, origin.y + r[1], float(r[2] + 3), float(r[3])};
            out.scroll_visible = v->arguments[1];
        }
    }
    return out;
}
} // namespace ark::desktop::ui
