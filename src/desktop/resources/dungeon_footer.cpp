// Language resource binding and the approved transparent overflow policy stay at the GPU edge.
#include "ark/simulation/presentation/steam_dungeon_footer_skin.hpp"
#include "resources.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ark::desktop {
std::optional<SpriteBlit> clip_sprite_source(SpriteBlit blit, int width, int height) {
    const auto &s = blit.source;
    const auto &d = blit.destination;
    if (!std::isfinite(s.x) || !std::isfinite(s.y) || !std::isfinite(s.width) ||
        !std::isfinite(s.height) || !std::isfinite(d.x) || !std::isfinite(d.y) ||
        !std::isfinite(d.width) || !std::isfinite(d.height) || s.width == 0 || s.height == 0 ||
        d.width <= 0 || d.height <= 0 || width <= 0 || height <= 0)
        return {};
    const float w = std::abs(s.width), h = std::abs(s.height);
    const float x0 = std::max(0.F, s.x), x1 = std::min(float(width), s.x + w);
    const float y0 = std::max(0.F, s.y), y1 = std::min(float(height), s.y + h);
    if (x1 <= x0 || y1 <= y0)
        return {};
    return clip_sprite_blit(blit,
                            {d.x + (s.width < 0 ? (s.x + w - x1) / w : (x0 - s.x) / w) * d.width,
                             d.y + (s.height < 0 ? (s.y + h - y1) / h : (y0 - s.y) / h) * d.height,
                             (x1 - x0) / w * d.width, (y1 - y0) / h * d.height});
}
namespace {
std::filesystem::path packaged_footer_path(const char *published) {
    if (!published)
        throw std::invalid_argument("Missing footer resource path");
    const std::string value = published;
    const auto prefix = value.rfind("steam-common/", 0) == 0      ? std::string("steam-common/")
                        : value.rfind("original/common/", 0) == 0 ? std::string("original/common/")
                                                                  : std::string{};
    if (prefix.empty())
        throw std::invalid_argument("Unsupported footer resource family");
    const std::filesystem::path tail = value.substr(prefix.size());
    if (tail.is_absolute())
        throw std::invalid_argument("Absolute footer resource path");
    for (const auto &part : tail)
        if (part == ".." || part == ".")
            throw std::invalid_argument("Unsafe footer resource path");
    return std::filesystem::path(prefix == "steam-common/" ? "steam_common" : "common") / tail;
}
} // namespace
void Sprites::dungeon_footer_image(const simulation::StartupSkinDraw &request,
                                   const simulation::SteamDungeonFooterLanguageProfile &language,
                                   std::optional<Rectangle> clip) {
    const auto resource = simulation::steam_dungeon_footer_resource(request.image, language);
    if (!resource)
        throw std::invalid_argument("Unknown footer resource");
    const auto &value = texture(root_ / packaged_footer_path(resource->resource.published_image));
    if (value.width != resource->physical_size[0] || value.height != resource->physical_size[1])
        throw std::runtime_error("Footer language texture dimensions changed");
    const auto blit = [&](std::array<int, 4> logical, Vector2 at, bool flip_x, bool flip_y) {
        const auto source = simulation::steam_dungeon_footer_physical_source(*resource, logical);
        if (!source)
            throw std::invalid_argument("Invalid footer source scaling");
        auto pixel =
            clip_sprite_source({{(*source)[0], (*source)[1], (*source)[2] * (flip_x ? -1 : 1),
                                 (*source)[3] * (flip_y ? -1 : 1)},
                                {at.x, at.y, float(logical[2]), float(logical[3])}},
                               value.width, value.height);
        if (pixel && clip)
            pixel = clip_sprite_blit(*pixel, *clip);
        if (pixel)
            DrawTexturePro(value, pixel->source, pixel->destination, {0, 0}, 0, WHITE);
    };
    const Vector2 anchor{float(request.offset[0]), float(request.offset[1])};
    if (request.sprite < 0) {
        blit(request.crop, anchor, false, false);
        return;
    }
    if (request.sprite != resource->resource.sprite)
        throw std::invalid_argument("Footer sprite identity changed");
    const auto &sprite = definition(packaged_footer_path(resource->resource.published_sprite));
    if (request.frame < 0 || request.frame >= sprite.frame_count || request.layer < 0 ||
        std::size_t(request.layer) >= sprite.layers.size())
        throw std::invalid_argument("Footer sprite frame/layer outside source");
    for (const auto &part : sprite.layers[std::size_t(request.layer)].parts)
        if (part.frame == request.frame && part.image_index >= 0 && part.width > 0 &&
            part.height > 0)
            blit({part.source_x, part.source_y, part.width, part.height},
                 {anchor.x + part.offset_x, anchor.y + part.offset_y}, part.flip_x, part.flip_y);
}
} // namespace ark::desktop
