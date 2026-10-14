#include "world_build_menu.hpp"
#include "../common/steam_plan_draw.hpp"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
void line(const sim::SteamBuildMenuLine &part, Vector2 origin) {
    const auto &r = part.endpoints;
    if (part.width != 1 || (r[0] != r[2] && r[1] != r[3]))
        throw std::invalid_argument("Build menu line is outside its published axis-aligned subset");
    // Source DrawLine includes the last pixel; use an exact half-open one-pixel rectangle.
    DrawRectangleRec({origin.x + std::min(r[0], r[2]), origin.y + std::min(r[1], r[3]),
                       float(std::abs(r[2] - r[0]) + 1), float(std::abs(r[3] - r[1]) + 1)},
                      steam_plan_color(part.rgb));
}
void arrow(const sim::SteamBuildMenuArrowTouch &part, Vector2 origin, const Skin &skin) {
    const auto resource = sim::steam_facility_resource(sim::SteamFacilityAsset::title_arrows);
    if (!resource || !resource->published_sprite)
        throw std::invalid_argument("Build menu arrow has no published bounds resource");
    const auto bounds = skin.sprites.frame_bounds(
        std::filesystem::path(resource->published_sprite).filename().string(), part.frame);
    const auto plan = sim::steam_manual_arrow_skin(
        {part.position, part.frame, part.selected},
        {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
    if (!plan)
        throw std::invalid_argument("Build menu arrow rejected its real source bounds");
    const auto &c = plan->crop;
    const auto &d = plan->destination;
    skin.sprites.image("buttoneffect.png", {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                        {origin.x + d[0], origin.y + d[1], d[2], d[3]},
                        Sprites::Binding::steam_common);
}
} // namespace
void draw_world_build_menu(const WorldBuildMenuView &view, Extent extent, const Skin &skin,
                           bool controls, bool enabled, const std::string &feedback) {
    if (!view.initialized)
        return;
    if (!view.skin)
        throw std::invalid_argument("Build menu drawing requires the measured source plan");
    const auto origin = world_build_menu_origin(extent);
    SteamPlanDraw draw(origin, skin, {});
    for (const auto &operation : view.skin->operations)
        std::visit([&](const auto &part) {
            using T = std::decay_t<decltype(part)>;
            if constexpr (std::is_same_v<T, sim::SteamStartupFrameDraw>)
                draw.frame(part);
            else if constexpr (std::is_same_v<T, sim::SteamBuildMenuLine>)
                line(part, origin);
            else if constexpr (std::is_same_v<T, sim::SteamBuildMenuClip>) {
                if (part.kind == sim::SteamBuildMenuClipKind::set)
                    draw.set_clip(part.rectangle);
                else
                    draw.clear_clip();
            } else if constexpr (std::is_same_v<T, sim::SteamBuildMenuImage>) {
                const auto resource = sim::steam_build_menu_resource(part.asset);
                if (!resource)
                    throw std::invalid_argument("Build menu image has no published resource");
                if (resource->published_sprite || part.crop)
                    draw.published(resource->published_image, resource->published_sprite,
                                    part.position, part.frame, part.crop);
                else
                    draw.whole_image(resource->published_image, part.position);
            } else if constexpr (std::is_same_v<T, sim::SteamFacilityImage>)
                draw.facility_image(part);
            else if constexpr (std::is_same_v<T, sim::SteamFacilityNumber>)
                draw.number(part);
            else if constexpr (std::is_same_v<T, sim::SteamFacilityMapchipDraw>)
                draw.mapchip(part);
            else if constexpr (std::is_same_v<T, sim::SteamHumanOverviewText>)
                draw.text(part, part.value);
            else if constexpr (std::is_same_v<T, sim::SteamBuildMenuArrowTouch>)
                arrow(part, origin, skin);
            else
                static_assert(std::is_same_v<T, sim::SteamBuildMenuTouch>, "Unhandled build menu operation");
        }, operation);
    draw.finish();
    if (controls) {
        skin.choice({float(extent.width - 58), float(extent.height - 25), 54, 21}, "返回", enabled);
        if (view.skin->soft_labels[0] == 7)
            skin.choice({4, float(extent.height - 25), 54, 21}, "情报", enabled);
        if (!feedback.empty())
            skin.text.draw(feedback, origin.x + 70, extent.height - 21.F, MAROON, 9);
    }
}
} // namespace ark::desktop::ui
