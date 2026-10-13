// Steam91 is one ordered source plan; PC text editing and starting remain in the title controller.
#include "world_new_game.hpp"
#include "../common/steam_plan_draw.hpp"

#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
Rectangle translate(const std::array<int, 4> &r, Vector2 origin) {
    return {origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
}
Rectangle intersect(Rectangle r, Extent extent) {
    const float left = std::max(0.F, r.x), top = std::max(0.F, r.y);
    return {left, top, std::max(0.F, std::min(float(extent.width), r.x + r.width) - left),
            std::max(0.F, std::min(float(extent.height), r.y + r.height) - top)};
}
void background(const sim::StartupSkinDraw &part, const WorldNewGameLayout &layout,
                const Skin &skin) {
    if (part.package != sim::StartupSkinPackage::event || part.sprite != -1)
        throw std::invalid_argument("Unsupported new-game background resource");
    const auto &r = part.crop;
    skin.sprites.indexed_image(Sprites::Binding::event, part.image,
                               {float(r[0]), float(r[1]), float(r[2]), float(r[3])},
                               {layout.origin.x + part.offset[0], layout.origin.y + part.offset[1],
                                float(r[2]), float(r[3])});
}
} // namespace
std::optional<sim::SteamNewGamePlan> world_new_game_plan(const app::WorldNewGameDraft &draft,
                                                         int selected, int main, int finger_frame,
                                                         std::array<int, 2> title_widths,
                                                         int start_width) {
    const auto &jobs = sim::startup_world_rules().jobs;
    const auto adventure = std::find_if(jobs.begin(), jobs.end(),
                                        [](const auto &job) { return (job.flags & 4U) != 0; });
    if (adventure == jobs.end())
        return {};
    sim::SteamNewGameInput input{draft.village,
                                 draft.human.name,
                                 draft.human.sex,
                                 selected,
                                 main,
                                 int(adventure - jobs.begin())};
    sim::SteamNewGameOptions options;
    options.view_y = 23;
    options.finger_frame = finger_frame;
    options.title_widths = title_widths;
    options.start_width = start_width;
    return sim::steam_new_game_skin(input, options);
}
WorldNewGameLayout world_new_game_layout(const sim::SteamNewGamePlan &source, Extent extent,
                                         const std::array<sim::SteamManualArrowPlan, 2> &arrows) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("New-game skin requires supported viewport");
    WorldNewGameLayout out;
    out.extent = extent;
    out.origin = {float((extent.width - 240) / 2), float((extent.height - 256) / 2)};
    std::array<bool, 4> fields{};
    std::array<bool, 2> arrow_fields{};
    bool frame{};
    for (const auto &operation : source.operations) {
        if (const auto *draw = std::get_if<sim::SteamStartupFrameDraw>(&operation)) {
            if (!frame)
                if (const auto *rectangle = std::get_if<sim::StartupSkinRect>(draw)) {
                    out.panel = translate(rectangle->rect, out.origin);
                    frame = true;
                }
        } else if (const auto *touch = std::get_if<sim::SteamStartupTouch>(&operation)) {
            const int field = touch->value & 0xffff;
            if (!touch->rectangle || field < 0 || field > 3 ||
                touch->component != (field == 3 ? 3 : 11) || fields[field])
                throw std::invalid_argument("New-game field Touch identity is invalid");
            out.fields[field] = intersect(translate(*touch->rectangle, out.origin), extent);
            fields[field] = true;
        } else if (const auto *arrow = std::get_if<sim::SteamNewGameArrow>(&operation)) {
            const int index = arrow->value == 16 ? 0 : arrow->value == 18 ? 1 : -1;
            if (index < 0 || arrow->component != 18 || arrow_fields[index])
                throw std::invalid_argument("New-game arrow Touch identity is invalid");
            const auto &bounds = arrows[index].touch_rectangle;
            const auto &margin = arrow->margin;
            const std::array<int, 4> expanded{bounds[0] - margin[0], bounds[1] - margin[2],
                                              bounds[2] + margin[0] + margin[1],
                                              bounds[3] + margin[2] + margin[3]};
            out.arrows[index] = intersect(translate(expanded, out.origin), extent);
            arrow_fields[index] = true;
        }
    }
    if (!frame || !std::all_of(fields.begin(), fields.end(), [](bool found) { return found; }) ||
        !std::all_of(arrow_fields.begin(), arrow_fields.end(), [](bool found) { return found; }))
        throw std::invalid_argument("New-game skin is missing painted input geometry");
    return out;
}
WorldNewGamePaint prepare_world_new_game(const app::WorldNewGameDraft &draft, int selected,
                                         Extent extent, const Skin &skin, int main,
                                         int finger_frame) {
    const std::array<int, 2> titles{int(skin.text.width("重新开始")),
                                    int(skin.text.width("重新开始"))};
    auto plan = world_new_game_plan(draft, selected, main, finger_frame, titles,
                                    int(skin.text.width("开始游戏")));
    if (!plan)
        throw std::invalid_argument("New-game draft cannot supply the source skin");
    WorldNewGamePaint out;
    out.source = std::move(*plan);
    for (const auto &operation : out.source.operations)
        if (const auto *arrow = std::get_if<sim::SteamNewGameArrow>(&operation)) {
            const auto bounds = skin.sprites.frame_bounds("arrow02.seb", arrow->source.bounds_frame,
                                                          Sprites::Binding::common);
            const auto result = sim::steam_manual_arrow_skin(
                arrow->source,
                {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
            if (!result)
                throw std::invalid_argument("New-game AddTouch arrow cannot resolve its bounds");
            out.arrows[arrow->value == 16 ? 0 : 1] = *result;
        }
    out.layout = world_new_game_layout(out.source, extent, out.arrows);
    return out;
}
void draw_world_new_game(const WorldNewGamePaint &paint, const Skin &skin) {
    SteamPlanDraw draw(paint.layout.origin, skin, paint.source.title);
    for (const auto &operation : paint.source.operations) {
        if (const auto *part = std::get_if<sim::SteamStartupFrameDraw>(&operation))
            draw.frame(*part);
        else if (const auto *part = std::get_if<sim::StartupSkinDraw>(&operation))
            background(*part, paint.layout, skin);
        else if (const auto *part = std::get_if<sim::SteamHumanOverviewText>(&operation)) {
            // Preserve source anchors and order; long PC names fit the original value columns.
            const float available = part->rectangle[0] == 163 && part->rectangle[1] == 135 ? 94.F
                                    : part->rectangle[0] == 163 && part->rectangle[1] == 157
                                        ? 82.F
                                        : 200.F;
            draw.literal(part->value, part->rectangle, part->anchor, part->font_size, part->layout,
                         part->rgb, available);
        } else if (const auto *part = std::get_if<sim::SteamHumanPortrait>(&operation))
            draw.portrait(*part);
        else if (const auto *part = std::get_if<sim::SteamFacilityImage>(&operation))
            draw.facility_image(*part);
        else if (const auto *part = std::get_if<sim::SteamNewGameArrow>(&operation)) {
            const auto &arrow = paint.arrows[part->value == 16 ? 0 : 1];
            const auto &crop = arrow.crop;
            const auto &destination = arrow.destination;
            skin.sprites.image("buttoneffect.png",
                               {float(crop[0]), float(crop[1]), float(crop[2]), float(crop[3])},
                               {paint.layout.origin.x + destination[0],
                                paint.layout.origin.y + destination[1], destination[2],
                                destination[3]},
                               Sprites::Binding::steam_common);
        }
        // Source Touch is metadata only. The title controller dispatches one PC action.
    }
    draw.finish();
}
} // namespace ark::desktop::ui
