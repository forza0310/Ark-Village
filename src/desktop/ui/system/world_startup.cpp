// Static RankForm composition uses system2 records, never the current world's wallet or actors.
#include "world_startup.hpp"
#include "../common/skin.hpp"
#include "ark/simulation/map/startup_world_projection.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
Color color(const std::array<int, 3> &rgb) {
    return {static_cast<unsigned char>(rgb[0]), static_cast<unsigned char>(rgb[1]),
            static_cast<unsigned char>(rgb[2]), 255};
}
void image(const sim::StartupSkinDraw &part, Vector2 origin, const Skin &skin) {
    const auto binding = part.package == sim::StartupSkinPackage::event ? Sprites::Binding::event
                                                                        : Sprites::Binding::common;
    const Vector2 anchor{origin.x + part.offset[0], origin.y + part.offset[1]};
    if (part.sprite >= 0)
        skin.sprites.indexed_sprite(binding, part.sprite, part.frame, part.layer, part.image,
                                    anchor);
    else if (part.crop[2] > 0 && part.crop[3] > 0) {
        const auto &c = part.crop;
        skin.sprites.indexed_image(binding, part.image,
                                   {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                                   {anchor.x, anchor.y, float(c[2]), float(c[3])});
    }
}
float fitted_size(const Skin &skin, const std::string &value, float available) {
    return 12.F * std::min(1.F, available / std::max(1.F, skin.text.width(value)));
}
} // namespace
std::optional<sim::SteamRecordSkinPlan>
world_records_plan(const sim::StartupSystemRecords &records, int page, Extent extent,
                   const WorldRecordsMeasurements &measured) {
    sim::SteamRecordSkinInput input;
    input.view_width = extent.width;
    input.view_height = extent.height;
    input.page = page;
    input.value = page == 0 ? records.high_score : records.cash_peak;
    input.village = page == 0 ? records.score_village : records.cash_village;
    input.labels = {"", "排行榜", page == 0 ? "最高通关点数" : "16年间的最高资金", "排行榜"};
    input.topic_width = measured.topic_width;
    input.value_width = measured.value_width;
    input.village_width = measured.village_width;
    input.definition_count = int(sim::startup_world_rules().humans.size());
    // No initialized record list/clock exists in the product. Drawing never synthesizes either.
    input.frame = 0;
    input.frozen_definitions = {};
    return sim::steam_record_skin(input);
}
WorldRecordsLayout world_records_layout(const sim::SteamRecordSkinPlan &plan, int page,
                                        Extent extent,
                                        const std::array<sim::SteamManualArrowPlan, 2> &arrows) {
    if (extent.width < 240 || extent.height < 256 || page < 0 || page > 1)
        throw std::invalid_argument("Records require supported viewport/page");
    WorldRecordsLayout out;
    out.extent = extent;
    out.page = page;
    out.origin = {float(plan.origin[0]), float(plan.origin[1])};
    for (int side = 0; side < 2; ++side) {
        const auto &r = arrows[side].touch_rectangle;
        out.arrows[side] = {out.origin.x + r[0], out.origin.y + r[1], float(r[2]), float(r[3])};
    }
    // Preserve a PC return button outside the source record text, including the minimum window.
    out.back = {extent.width - 58.F, extent.height - 25.F, 54, 21};
    return out;
}
WorldRecordsAction world_records_input(const WorldRecordsLayout *painted,
                                       std::optional<Vector2> click, bool left, bool right,
                                       bool back) {
    const auto hit = [&](Rectangle box) {
        return painted && click && CheckCollisionPointRec(*click, box);
    };
    if (left || (painted && hit(painted->arrows[0])))
        return WorldRecordsAction::turn;
    if (right || (painted && hit(painted->arrows[1])))
        return WorldRecordsAction::turn;
    if (back || (painted && hit(painted->back)))
        return WorldRecordsAction::back;
    return WorldRecordsAction::none;
}
WorldRecordsPaint prepare_world_records(const sim::StartupSystemRecords &records, int page,
                                        Extent extent, const Skin &skin) {
    const auto value = sim::steam_record_value_text(
        page, page == 0 ? records.high_score : records.cash_peak, false);
    if (!value)
        throw std::invalid_argument("Invalid system record value/page");
    const std::string topic = page == 0 ? "最高通关点数" : "16年间的最高资金";
    const auto &village = page == 0 ? records.score_village : records.cash_village;
    WorldRecordsPaint out;
    // Non-Japanese source anchors are label x46 and value right198, with a four-unit PC gap.
    out.text_sizes = {fitted_size(skin, topic, 196), fitted_size(skin, village, 196), 12,
                      fitted_size(skin, *value, 148 - skin.text.width("排行榜"))};
    auto source = world_records_plan(records, page, extent,
                                     {int(skin.text.width(topic, out.text_sizes[0])),
                                      int(skin.text.width(*value, out.text_sizes[3])),
                                      skin.text.width(village, out.text_sizes[1])});
    if (!source)
        throw std::invalid_argument("Invalid static record skin");
    out.source = std::move(*source);
    int windows{}, arrows{};
    for (const auto &operation : out.source.draws) {
        if (const auto *window = std::get_if<sim::SteamRecordWindow>(&operation)) {
            if (windows == int(out.windows.size()))
                throw std::invalid_argument("Unexpected record window count");
            const auto plan = sim::steam_startup_window_skin(
                window->source, 23,
                std::array<int, 2>{int(skin.text.width(window->title)),
                                   int(skin.text.width(window->title))});
            if (!plan)
                throw std::invalid_argument("Invalid record window frame");
            out.windows[windows++] = *plan;
        } else if (const auto *arrow = std::get_if<sim::SteamManualArrowRequest>(&operation)) {
            if (arrows == int(out.arrows.size()))
                throw std::invalid_argument("Unexpected record arrow count");
            const auto resource = sim::steam_startup_resource(sim::SteamStartupAsset::arrows);
            if (!resource || !resource->sprite_name)
                throw std::invalid_argument("Record arrow has no published bounds resource");
            const auto bounds = skin.sprites.frame_bounds(
                resource->sprite_name, arrow->bounds_frame, Sprites::Binding::common);
            const auto plan = sim::steam_manual_arrow_skin(
                *arrow, {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
            if (!plan)
                throw std::invalid_argument("Invalid record arrow helper");
            out.arrows[arrows++] = *plan;
        } else if (std::holds_alternative<sim::SteamManualActorRequest>(operation))
            throw std::invalid_argument("Static records cannot invent an Init decoration list");
    }
    if (windows != int(out.windows.size()) || arrows != int(out.arrows.size()))
        throw std::invalid_argument("Incomplete static record skin");
    out.layout = world_records_layout(out.source, page, extent, out.arrows);
    return out;
}
void draw_world_records(const WorldRecordsPaint &paint, const Skin &skin) {
    const auto origin = paint.layout.origin;
    int window_index{}, arrow_index{}, text_index{};
    for (const auto &operation : paint.source.draws) {
        if (const auto *window = std::get_if<sim::SteamRecordWindow>(&operation)) {
            for (const auto &part : paint.windows.at(window_index++).draws) {
                if (const auto *r = std::get_if<sim::StartupSkinRect>(&part)) {
                    const auto &b = r->rect;
                    const Rectangle box{origin.x + b[0], origin.y + b[1], float(b[2]), float(b[3])};
                    if (r->outline)
                        DrawRectangleLinesEx(box, 1, color(r->rgb));
                    else
                        DrawRectangleRec(box, color(r->rgb));
                } else if (const auto *i = std::get_if<sim::StartupSkinDraw>(&part))
                    image(*i, origin, skin);
                else if (const auto *t = std::get_if<sim::SteamStartupText>(&part)) {
                    if (!t->rgb)
                        throw std::invalid_argument("Record frame text has no source color");
                    // Immediate text keeps the first title beneath the second source window.
                    skin.text.scene_text(
                        window->title,
                        {origin.x + float(t->rectangle[0]), origin.y + float(t->rectangle[1])},
                        color(*t->rgb), t->font_size ? float(t->font_size) : 12.F);
                }
            }
        } else if (const auto *t = std::get_if<sim::SteamRecordText>(&operation))
            skin.text.scene_text(t->value, {origin.x + t->position[0], origin.y + t->position[1]},
                                 color(t->rgb), paint.text_sizes.at(text_index++));
        else if (const auto *i = std::get_if<sim::StartupSkinDraw>(&operation))
            image(*i, origin, skin);
        else if (std::holds_alternative<sim::SteamManualArrowRequest>(operation)) {
            const auto &arrow = paint.arrows.at(arrow_index++);
            const auto &crop = arrow.crop;
            const auto &d = arrow.destination;
            skin.sprites.image("buttoneffect.png",
                               {float(crop[0]), float(crop[1]), float(crop[2]), float(crop[3])},
                               {origin.x + d[0], origin.y + d[1], d[2], d[3]},
                               Sprites::Binding::steam_common);
        } else if (!std::holds_alternative<sim::SteamStartupTouch>(operation))
            throw std::invalid_argument("Unsupported static record draw");
    }
}
} // namespace ark::desktop::ui
