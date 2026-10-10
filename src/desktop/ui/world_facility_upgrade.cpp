#include "world_facility_upgrade.hpp"
#include "skin.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
using Asset = sim::SteamFacilityAsset;
std::string label(const sim::SteamFacilityText &request, const WorldBuildingView &view) {
    using Role = sim::SteamFacilityTextRole;
    switch (request.role) {
    case Role::upgrade_title:
        return "设施升级";
    case Role::facility_notice:
        return view.facility_name;
    case Role::level_prefix:
        return "等级";
    case Role::level_suffix:
        return "！";
    case Role::level_completed:
        return "等级提升了！";
    case Role::parameter_name:
        return std::array<const char *, 3>{"价格", "品质", "魅力"}.at(request.argument);
    }
    throw std::invalid_argument("Unknown facility upgrade label");
}
} // namespace
void draw_world_facility_upgrade(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                                 const Skin &skin) {
    if (!view.initialized || !view.upgrade_skin)
        return;
    auto input = *view.upgrade_skin;
    input.title_widths = std::array<int, 2>{static_cast<int>(skin.text.width("设施升级")),
                                            static_cast<int>(skin.text.width("设施升级"))};
    input.notice_widths = std::array<int, 4>{
        static_cast<int>(skin.text.width(std::to_string(input.level))),
        static_cast<int>(skin.text.width("等级")), static_cast<int>(skin.text.width("！")),
        static_cast<int>(skin.text.width("等级"))};
    const auto plan = sim::steam_facility_upgrade_skin(input);
    if (!plan)
        throw std::invalid_argument("Invalid bound Steam facility upgrade plan");
    const Vector2 origin{layout.panel.x - 9, layout.panel.y - 35};
    // Desktop font/line-spacing adaptation: the source rows are 18 pixels apart, but
    // the bundled Chinese glyphs and ten-pixel SEB digits touch the source box bottom.
    // Keep the first row, window and artwork fixed; compress only the three complete
    // attribute rows to a 15-pixel pitch. Source counters/parabolas remain unchanged.
    const auto row_shift = [&](int parameter) {
        return input.phase == 1 && parameter >= 0 && parameter < 3 ? parameter * 3 : 0;
    };
    std::optional<Rectangle> clip;
    const auto image = [&](const sim::SteamFacilityImage &part) {
        // Independent frame2 is absent from older maintained Owners. Until supplied,
        // leave only that unbound animation out rather than resetting it on phase changes.
        if (part.asset == Asset::mini && !view.upgrade_secondary_available)
            return;
        const auto resource = sim::steam_facility_resource(part.asset);
        if (!resource)
            throw std::invalid_argument("Unknown Steam facility resource");
        const auto binding = part.asset == Asset::number05 || part.asset == Asset::number08
                                 ? Sprites::Binding::steam_common
                             : std::string(resource->group) == "event" ? Sprites::Binding::event
                                                                       : Sprites::Binding::common;
        const Vector2 anchor{origin.x + part.position[0], origin.y + part.position[1]};
        if (part.crop) {
            const auto &r = *part.crop;
            if (r[2] <= 0 || r[3] <= 0)
                return;
            SpriteBlit blit{{float(r[0]), float(r[1]), float(r[2]), float(r[3])},
                            {anchor.x, anchor.y, float(r[2]), float(r[3])}};
            if (clip) {
                const auto cropped = clip_sprite_blit(blit, *clip);
                if (!cropped)
                    return;
                blit = *cropped;
            }
            skin.sprites.image(std::filesystem::path(resource->published_image).filename().string(),
                               blit.source, blit.destination, binding);
        } else {
            // A source-negative numeric frame has no published pixel mapping. Do not map
            // it to an invented minus glyph; valid nonnegative growth follows original SEB.
            if (part.frame < 0)
                return;
            skin.sprites.draw(std::filesystem::path(resource->published_sprite).filename().string(),
                              part.frame, anchor, WHITE, binding, 1, -1, clip);
        }
    };
    for (const auto &request : plan->draws) {
        if (const auto *part = std::get_if<sim::StartupSkinRect>(&request)) {
            const auto &r = part->rect;
            const auto &c = part->rgb;
            const Rectangle box{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            const Color color{static_cast<unsigned char>(c[0]), static_cast<unsigned char>(c[1]),
                              static_cast<unsigned char>(c[2]), 255};
            if (part->outline)
                DrawRectangleLinesEx(box, 1, color);
            else
                DrawRectangleRec(box, color);
        } else if (const auto *part = std::get_if<sim::SteamFacilityImage>(&request)) {
            auto adapted = *part;
            // MAX is the sole per-row direct image. Currency is emitted only for row0,
            // which does not move; numeric images are adapted with their parent below.
            if (part->asset == Asset::maximum)
                adapted.position[1] -= row_shift((part->position[1] - 161) / 18);
            image(adapted);
        } else if (const auto *part = std::get_if<sim::SteamFacilityClip>(&request)) {
            if (part->kind == sim::SteamFacilityClipKind::pop)
                clip.reset();
            else {
                const auto &r = part->rectangle;
                clip = Rectangle{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            }
        } else if (const auto *part = std::get_if<sim::SteamFacilityText>(&request)) {
            const auto value = label(*part, view);
            const float base = part->font_size ? float(part->font_size) : 12.F;
            const float size =
                part->role == sim::SteamFacilityTextRole::facility_notice
                    ? base * std::min(1.F, 200.F / std::max(1.F, skin.text.width(value, base)))
                    : base;
            const float width = skin.text.width(value, size);
            const int anchor = part->anchor.value_or(1);
            const float x = origin.x + part->rectangle[0] -
                            (anchor & 2   ? width / 2
                             : anchor & 4 ? width
                                          : 0);
            const float y = origin.y + part->rectangle[1] -
                            (part->role == sim::SteamFacilityTextRole::parameter_name
                                 ? row_shift(part->argument)
                                 : 0);
            const auto &c = part->rgb;
            const Color color{static_cast<unsigned char>(c[0]), static_cast<unsigned char>(c[1]),
                              static_cast<unsigned char>(c[2]), 255};
            if (clip)
                skin.text.clipped(value, x, y, *clip, color, size);
            else
                skin.text.draw(value, x, y, color, size);
        } else if (const auto *part = std::get_if<sim::SteamFacilityMapchip2>(&request)) {
            const auto pieces = sim::steam_facility_mapchip2_draws(*part);
            if (!pieces)
                throw std::invalid_argument("Invalid upgrade facility mapchip");
            for (const auto &piece : *pieces)
                skin.sprites.draw(piece.sprite, piece.frame,
                                  {origin.x + piece.position[0], origin.y + piece.position[1]},
                                  WHITE, Sprites::Binding::map, 1, -1, clip);
        } else if (const auto *part = std::get_if<sim::SteamFacilityNumber>(&request)) {
            const auto resource = sim::steam_facility_resource(part->asset);
            const auto sprite =
                std::filesystem::path(resource->published_sprite).filename().string();
            auto adapted = *part;
            adapted.position[1] -= row_shift(part->parameter);
            const auto digits =
                sim::steam_facility_number_draws(adapted, skin.sprites.common_digit_width(sprite));
            if (!digits)
                throw std::invalid_argument("Invalid upgrade number request");
            for (const auto &digit : *digits)
                image(digit);
        }
    }
}
} // namespace ark::desktop::ui
