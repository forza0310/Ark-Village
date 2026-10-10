// Texture ownership, source-frame binding and clipped sprite drawing. No world mutation.
#include "ark/assets/table.hpp"
#include "resource_metadata.hpp"
#include "resources.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace ark::desktop {
using namespace resource_detail;
namespace {
// Explicit published Steam replacements. A missing replacement is an asset error,
// not permission to silently fall back to the APK image with the same index.
bool steam_common_override(int image) {
    return image == 31 || image == 37 || image == 85 || image == 88 || image == 103 ||
           image == 105 || image == 128;
}
} // namespace
std::optional<SpriteBlit> clip_sprite_blit(SpriteBlit blit, Rectangle clip) {
    const auto destination = blit.destination;
    if (destination.width <= 0 || destination.height <= 0 || clip.width <= 0 || clip.height <= 0)
        return {};
    const float left = std::max(destination.x, clip.x);
    const float top = std::max(destination.y, clip.y);
    const float right = std::min(destination.x + destination.width, clip.x + clip.width);
    const float bottom = std::min(destination.y + destination.height, clip.y + clip.height);
    if (right <= left || bottom <= top)
        return {};
    const float u0 = (left - destination.x) / destination.width;
    const float u1 = (right - destination.x) / destination.width;
    const float v0 = (top - destination.y) / destination.height;
    const float v1 = (bottom - destination.y) / destination.height;
    blit.source.x += std::abs(blit.source.width) * (blit.source.width < 0 ? 1 - u1 : u0);
    blit.source.y += std::abs(blit.source.height) * (blit.source.height < 0 ? 1 - v1 : v0);
    blit.source.width *= u1 - u0;
    blit.source.height *= v1 - v0;
    blit.destination = {left, top, right - left, bottom - top};
    return blit;
}
Sprites::Sprites(std::filesystem::path root)
    : root_(std::move(root)), images_(image_index(root_)),
      common_images_(image_index(root_, "common")), common2_images_(image_index(root_, "common2")) {
}
Sprites::~Sprites() {
    for (const auto &entry : textures_)
        UnloadTexture(entry.second);
}
int Sprites::map_frame(const std::string &sprite, int variant) {
    if (std::filesystem::path(sprite).has_parent_path())
        throw std::runtime_error("Unsafe tenant sprite path");
    const auto &data = definition(std::filesystem::path("image") / sprite);
    return map_frame_index(sprite, data, variant);
}
int Sprites::common_digit_width(const std::string &sprite) {
    if (std::filesystem::path(sprite).has_parent_path())
        throw std::invalid_argument("Unsafe common sprite path");
    const auto &data = definition(std::filesystem::path("common") / sprite);
    if (data.layers.empty())
        throw std::invalid_argument("Number sprite has no source line");
    for (const auto &part : data.layers.front().parts)
        if (part.frame == 0 && part.width > 0)
            return part.width;
    throw std::invalid_argument("Number sprite has no first digit width");
}
int Sprites::map_image_height(const std::string &sprite, int frame) {
    frame = map_frame(sprite, frame);
    const auto &data = definition(std::filesystem::path("image") / sprite);
    if (frame >= data.frame_count)
        return 0;
    int image = -1;
    for (const auto &layer : data.layers)
        for (const auto &part : layer.parts)
            if (part.frame == frame) {
                if (image >= 0 && image != part.image_index)
                    throw std::runtime_error("Tenant frame has ambiguous image height");
                image = part.image_index;
            }
    if (image < 0)
        return 0;
    return texture(root_ / "image" / images_.at(image)).height;
}
void Sprites::draw(const std::string &sprite, int frame, Vector2 anchor, Color tint,
                   Binding binding, float scale, int image_override, std::optional<Rectangle> clip,
                   std::optional<int> selected_layer) {
    if (std::filesystem::path(sprite).has_parent_path())
        throw std::runtime_error("Unsafe sprite path");
    const char *group = binding == Binding::farmer || binding == Binding::human ? "human"
                        : binding == Binding::monster                           ? "monster"
                        : binding == Binding::weapon                            ? "weapon"
                        : binding == Binding::common2                           ? "common2"
                        : binding == Binding::secretary || binding == Binding::common ||
                                binding == Binding::steam_common
                            ? "common"
                            : "image";
    const auto relative = std::filesystem::path(group) / sprite;
    const auto &sprite_data = definition(relative);
    if (binding == Binding::map) {
        frame = map_frame_index(sprite, sprite_data, frame);
        if (frame >= sprite_data.frame_count)
            return;
    }
    if (frame < 0 || frame >= sprite_data.frame_count)
        throw std::runtime_error("Source variant outside sprite frames: " + relative.string() +
                                 " variant=" + std::to_string(frame) +
                                 " frames=" + std::to_string(sprite_data.frame_count));
    if (selected_layer &&
        (*selected_layer < 0 || *selected_layer >= static_cast<int>(sprite_data.layers.size())))
        throw std::runtime_error("Source layer outside sprite layers");
    for (std::size_t layer_index = 0; layer_index < sprite_data.layers.size(); ++layer_index) {
        if (selected_layer && layer_index != static_cast<std::size_t>(*selected_layer))
            continue;
        const auto &layer = sprite_data.layers[layer_index];
        for (const auto &p : layer.parts) {
            if (p.frame != frame)
                continue;
            if (binding == Binding::secretary && p.image_index != 126)
                throw std::runtime_error("Secretary image binding changed");
            const auto path =
                binding == Binding::steam_common &&
                        steam_common_override(image_override >= 0 ? image_override : p.image_index)
                    ? root_ / "steam_common" /
                          common_images_.at(image_override >= 0 ? image_override : p.image_index)
                : image_override >= 0 ? root_ / group / actor_images_.at(group).at(image_override)
                : binding == Binding::farmer    ? root_ / "human/chara_flower00.png"
                : binding == Binding::secretary ? root_ / "common/chara_hishoko01.png"
                : binding == Binding::common || binding == Binding::steam_common
                    ? root_ / group / common_images_.at(p.image_index)
                : binding == Binding::common2 ? root_ / group / common2_images_.at(p.image_index)
                                              : root_ / "image" / images_.at(p.image_index);
            const auto &image = texture(path);
            validate(p, image.width, image.height);
            SpriteBlit blit{{static_cast<float>(p.source_x), static_cast<float>(p.source_y),
                             static_cast<float>(p.flip_x ? -p.width : p.width),
                             static_cast<float>(p.flip_y ? -p.height : p.height)},
                            {anchor.x + p.offset_x * scale, anchor.y + p.offset_y * scale,
                             p.width * scale, p.height * scale}};
            if (clip) {
                const auto cropped = clip_sprite_blit(blit, *clip);
                if (!cropped)
                    continue;
                blit = *cropped;
            }
            record_pick(image, blit, tint);
            DrawTexturePro(image, blit.source, blit.destination, {0, 0}, 0, tint);
        }
    }
}
void Sprites::indexed_sprite(Binding binding, int sprite, int frame, int layer, int image_override,
                             Vector2 anchor, float scale, std::optional<Rectangle> clip) {
    if (binding != Binding::common && binding != Binding::steam_common &&
        binding != Binding::weapon)
        throw std::invalid_argument("Unsupported indexed effect binding");
    const std::string group = binding == Binding::weapon ? "weapon" : "common";
    if (!actor_sprites_.count(group)) {
        std::vector<std::string> names;
        for (const auto &row : assets::parse_tsv(read_bytes(root_ / group / "seb.inf"))) {
            if (row.size() != 1 || std::filesystem::path(row[0]).has_parent_path())
                throw std::runtime_error("Invalid effect sprite index");
            names.push_back(row[0]);
        }
        actor_sprites_.emplace(group, std::move(names));
    }
    if (image_override >= 0 && !actor_images_.count(group))
        actor_images_.emplace(group, image_index(root_, group.c_str()));
    if (sprite < 0)
        throw std::invalid_argument("Invalid effect sprite index");
    draw(actor_sprites_.at(group).at(static_cast<std::size_t>(sprite)), frame, anchor, WHITE,
         binding, scale, image_override, clip, layer);
}
void Sprites::indexed_image(Binding binding, int image_id, Rectangle source, Rectangle destination,
                            std::optional<Rectangle> clip) {
    if (binding != Binding::common && binding != Binding::steam_common &&
        binding != Binding::weapon && binding != Binding::title && binding != Binding::event)
        throw std::invalid_argument("Unsupported indexed image binding");
    const std::string group = binding == Binding::weapon  ? "weapon"
                              : binding == Binding::title ? "title"
                              : binding == Binding::event ? "event"
                                                          : "common";
    if (!actor_images_.count(group))
        actor_images_.emplace(group, image_index(root_, group.c_str()));
    if (clip) {
        const auto blit = clip_sprite_blit({source, destination}, *clip);
        if (!blit)
            return;
        source = blit->source;
        destination = blit->destination;
    }
    image(actor_images_.at(group).at(image_id).string(), source, destination,
          binding == Binding::steam_common && !steam_common_override(image_id) ? Binding::common
                                                                               : binding);
}
void Sprites::human_image(int image_id, Rectangle source, Rectangle destination) {
    if (!actor_images_.count("human"))
        actor_images_.emplace("human", image_index(root_, "human"));
    const auto &image = texture(root_ / "human" / actor_images_.at("human").at(image_id));
    if (source.x < 0 || source.y < 0 || source.width <= 0 || source.height <= 0 ||
        source.x + source.width > image.width || source.y + source.height > image.height)
        throw std::runtime_error("Human portrait outside source image");
    DrawTexturePro(image, source, destination, {0, 0}, 0, WHITE);
}
void Sprites::actor_thumbnail(bool monster, int sprite_index, int image_id, Rectangle box) {
    const std::string group = monster ? "monster" : "human";
    if (!actor_sprites_.count(group)) {
        std::vector<std::string> names;
        for (const auto &row : assets::parse_tsv(read_bytes(root_ / group / "seb.inf"))) {
            if (row.size() != 1 || std::filesystem::path(row[0]).has_parent_path())
                throw std::runtime_error("Invalid actor sprite index");
            names.push_back(row.at(0));
        }
        actor_sprites_.emplace(group, std::move(names));
        actor_images_.emplace(group, image_index(root_, group.c_str()));
    }
    const auto &data =
        definition(std::filesystem::path(group) / actor_sprites_.at(group).at(sprite_index));
    bool found = false;
    int left{}, top{}, right{}, bottom{};
    for (const auto &layer : data.layers)
        for (const auto &p : layer.parts) {
            if (p.frame != 0)
                continue;
            left = found ? std::min(left, static_cast<int>(p.offset_x)) : p.offset_x;
            top = found ? std::min(top, static_cast<int>(p.offset_y)) : p.offset_y;
            right = found ? std::max(right, p.offset_x + p.width) : p.offset_x + p.width;
            bottom = found ? std::max(bottom, p.offset_y + p.height) : p.offset_y + p.height;
            found = true;
        }
    if (!found || right <= left || bottom <= top)
        throw std::runtime_error("Empty actor thumbnail frame");
    const float scale = std::min(box.width / (right - left), box.height / (bottom - top));
    actor(monster, sprite_index, image_id, 0,
          {box.x + (box.width - (right - left) * scale) / 2 - left * scale,
           box.y + (box.height - (bottom - top) * scale) / 2 - top * scale},
          scale);
}
void Sprites::actor(bool monster, int sprite_index, int image_id, int frame, Vector2 anchor,
                    float scale, std::optional<Rectangle> clip) {
    const std::string group = monster ? "monster" : "human";
    if (!actor_sprites_.count(group)) {
        std::vector<std::string> sprites;
        for (const auto &row : assets::parse_tsv(read_bytes(root_ / group / "seb.inf"))) {
            if (row.size() != 1 || std::filesystem::path(row[0]).has_parent_path())
                throw std::runtime_error("Invalid actor sprite index");
            sprites.push_back(row[0]);
        }
        actor_images_.emplace(group, image_index(root_, group.c_str()));
        actor_sprites_.emplace(group, std::move(sprites));
    }
    if (sprite_index < 0 || image_id < 0)
        throw std::runtime_error("Invalid actor sprite/image index");
    draw(actor_sprites_.at(group).at(static_cast<std::size_t>(sprite_index)), frame, anchor, WHITE,
         monster ? Binding::monster : Binding::human, scale, image_id, clip);
}
const assets::SpriteDefinition &Sprites::definition(const std::filesystem::path &relative) {
    auto found = sprites_.find(relative.string());
    if (found == sprites_.end())
        found =
            sprites_
                .emplace(relative.string(), assets::parse_legacy_seb(read_bytes(root_ / relative)))
                .first;
    return found->second;
}
void Sprites::thumbnail(const std::string &sprite, int frame, Rectangle box, Color tint) {
    thumbnail(sprite, std::vector<std::pair<int, Vector2>>{{frame, {0, 0}}}, box, tint);
}
void Sprites::thumbnail(const std::string &sprite,
                        const std::vector<std::pair<int, Vector2>> &frames, Rectangle box,
                        Color tint) {
    if (std::filesystem::path(sprite).has_parent_path())
        throw std::runtime_error("Unsafe sprite path");
    const auto &data = definition(std::filesystem::path("image") / sprite);
    bool found = false;
    float left{}, top{}, right{}, bottom{};
    for (const auto &[variant, offset] : frames) {
        const int frame = map_frame_index(sprite, data, variant);
        if (!std::isfinite(offset.x) || !std::isfinite(offset.y))
            throw std::runtime_error("Invalid thumbnail fragment");
        if (frame >= data.frame_count)
            continue;
        for (const auto &layer : data.layers)
            for (const auto &part : layer.parts)
                if (part.frame == frame) {
                    const float x = offset.x + part.offset_x, y = offset.y + part.offset_y;
                    left = found ? std::min(left, x) : x;
                    top = found ? std::min(top, y) : y;
                    right = found ? std::max(right, x + part.width) : x + part.width;
                    bottom = found ? std::max(bottom, y + part.height) : y + part.height;
                    found = true;
                }
    }
    if (!found)
        return;
    if (right <= left || bottom <= top)
        throw std::runtime_error("Invalid thumbnail bounds");
    const float scale = std::min(box.width / (right - left), box.height / (bottom - top));
    for (const auto &[frame, offset] : frames)
        draw(sprite, frame,
             {box.x + (box.width - (right - left) * scale) / 2 + (offset.x - left) * scale,
              box.y + (box.height - (bottom - top) * scale) / 2 + (offset.y - top) * scale},
             tint, Binding::map, scale);
}
Texture2D &Sprites::texture(const std::filesystem::path &path) {
    auto found = textures_.find(path.string());
    if (found == textures_.end()) {
        auto value = LoadTexture(path.string().c_str());
        if (!value.id)
            throw std::runtime_error("Cannot load texture: " + path.string());
        SetTextureFilter(value, TEXTURE_FILTER_POINT);
        try {
            found = textures_.emplace(path.string(), value).first;
        } catch (...) {
            UnloadTexture(value);
            throw;
        }
    }
    return found->second;
}
void Sprites::record_pick(Texture2D texture, SpriteBlit blit, Color tint) {
    if (pick_map_ && tint.a)
        pick_map_->add(texture, blit.source, blit.destination, pick_target_);
}
void Sprites::image(const std::string &name, Rectangle source, Rectangle destination,
                    Binding binding, Color tint) {
    if (std::filesystem::path(name).has_parent_path())
        throw std::runtime_error("Unsafe image path");
    const char *group = binding == Binding::common2        ? "common2"
                        : binding == Binding::window       ? "ui"
                        : binding == Binding::title        ? "title"
                        : binding == Binding::steam_title  ? "steam_title"
                        : binding == Binding::steam_common ? "steam_common"
                        : binding == Binding::event        ? "event"
                        : binding == Binding::map          ? "image"
                        : binding == Binding::weapon       ? "weapon"
                                                           : "common";
    const auto path = root_ / group / name;
    const auto &value = texture(path);
    if (source.x < 0 || source.y < 0 || source.width <= 0 || source.height <= 0 ||
        source.x + source.width > value.width || source.y + source.height > value.height)
        throw std::runtime_error("UI image rectangle outside atlas");
    record_pick(value, {source, destination}, tint);
    DrawTexturePro(value, source, destination, {0, 0}, 0, tint);
}
} // namespace ark::desktop
