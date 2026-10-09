// SourceSprites/ChineseFont adapted from research/prototype/src/startup_view.cpp.
// PNG index, SEB index and map display ID are different namespaces; farmer uses job image override.
#include "resources.hpp"
#include "ark/app/startup_data.hpp"
#include "ark/assets/table.hpp"
#include "ark/simulation/startup.hpp"
#include "desktop_glyphs.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <tuple>

namespace ark::desktop {
std::filesystem::path desktop_font_path(const std::filesystem::path &assets,
                                        const std::string &override_path) {
    if (!override_path.empty())
        return override_path;
    const auto fonts = assets.parent_path() / "fonts";
    for (const auto &path :
         {fonts / "default.otf", fonts / "default.ttf",
          std::filesystem::path("/System/Library/Fonts/Supplemental/Arial Unicode.ttf")})
        if (std::filesystem::is_regular_file(path))
            return path;
    throw std::runtime_error("Chinese font not found; place default.otf or default.ttf in fonts "
                             "beside the executable, or use --font TTF/OTF");
}
namespace {
std::vector<std::uint8_t> read_bytes(const std::filesystem::path &path) {
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("Asset metadata too large");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot read asset metadata: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::map<int, std::filesystem::path> image_index(const std::filesystem::path &root,
                                                 const char *group = "image") {
    std::map<int, std::filesystem::path> result;
    for (const auto &row : assets::parse_tsv(read_bytes(root / group / "img.inf"))) {
        const bool ordinal = std::string(group) == "title";
        if (row.size() != (ordinal ? 1U : 2U))
            throw std::runtime_error("Invalid image index row");
        auto name = std::filesystem::path(row.back());
        if (name.has_parent_path() || name.is_absolute())
            throw std::runtime_error("Unsafe image path");
        name.replace_extension(".png");
        const int id =
            ordinal ? static_cast<int>(result.size()) : assets::parse_table_integer(row[0]);
        if (!result.emplace(id, name).second)
            throw std::runtime_error("Duplicate image index");
    }
    return result;
}
void validate(const assets::SpritePart &p, int width, int height) {
    if (p.image_index < 0 || p.source_x < 0 || p.source_y < 0 || p.width <= 0 || p.height <= 0 ||
        p.source_x + p.width > width || p.source_y + p.height > height || p.flip_x < 0 ||
        p.flip_x > 1 || p.flip_y < 0 || p.flip_y > 1)
        throw std::runtime_error("Unsupported sprite command or invalid image rectangle/flip");
}
int map_frame_index(const std::string &sprite, const assets::SpriteDefinition &data, int variant) {
    // 2b479f6 PAGES: map layers outside their source frame range return null and do not draw.
    // A logical second orientation must not become the single resource's frame0.
    if (variant < 0)
        throw std::runtime_error("Map sprite frame outside source: " + sprite +
                                 " variant=" + std::to_string(variant) +
                                 " frames=" + std::to_string(data.frame_count));
    return variant;
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
    const char *group = binding == Binding::farmer || binding == Binding::human       ? "human"
                        : binding == Binding::monster                                 ? "monster"
                        : binding == Binding::weapon                                  ? "weapon"
                        : binding == Binding::common2                                 ? "common2"
                        : binding == Binding::secretary || binding == Binding::common ? "common"
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
                image_override >= 0 ? root_ / group / actor_images_.at(group).at(image_override)
                : binding == Binding::farmer    ? root_ / "human/chara_flower00.png"
                : binding == Binding::secretary ? root_ / "common/chara_hishoko01.png"
                : binding == Binding::common    ? root_ / group / common_images_.at(p.image_index)
                : binding == Binding::common2   ? root_ / group / common2_images_.at(p.image_index)
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
            DrawTexturePro(image, blit.source, blit.destination, {0, 0}, 0, tint);
        }
    }
}
void Sprites::indexed_sprite(Binding binding, int sprite, int frame, int layer, int image_override,
                             Vector2 anchor, float scale) {
    if (binding != Binding::common && binding != Binding::weapon)
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
         binding, scale, image_override, {}, layer);
}
void Sprites::indexed_image(Binding binding, int image_id, Rectangle source,
                            Rectangle destination) {
    if (binding != Binding::common && binding != Binding::weapon && binding != Binding::title &&
        binding != Binding::event)
        throw std::invalid_argument("Unsupported indexed image binding");
    const std::string group = binding == Binding::weapon  ? "weapon"
                              : binding == Binding::title ? "title"
                              : binding == Binding::event ? "event"
                                                          : "common";
    if (!actor_images_.count(group))
        actor_images_.emplace(group, image_index(root_, group.c_str()));
    image(actor_images_.at(group).at(image_id).string(), source, destination, binding);
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
                    float scale) {
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
         monster ? Binding::monster : Binding::human, scale, image_id);
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
void Sprites::image(const std::string &name, Rectangle source, Rectangle destination,
                    Binding binding, Color tint) {
    if (std::filesystem::path(name).has_parent_path())
        throw std::runtime_error("Unsafe image path");
    const char *group = binding == Binding::common2       ? "common2"
                        : binding == Binding::window      ? "ui"
                        : binding == Binding::title       ? "title"
                        : binding == Binding::steam_title ? "steam_title"
                        : binding == Binding::event       ? "event"
                        : binding == Binding::map         ? "image"
                        : binding == Binding::weapon      ? "weapon"
                                                          : "common";
    const auto &value = texture(root_ / group / name);
    if (source.x < 0 || source.y < 0 || source.width <= 0 || source.height <= 0 ||
        source.x + source.width > value.width || source.y + source.height > value.height)
        throw std::runtime_error("UI image rectangle outside atlas");
    DrawTexturePro(value, source, destination, {0, 0}, 0, tint);
}
Text::Text(const std::filesystem::path &font_path, const std::string &extra_glyphs)
    : font_path_(font_path) {
    if (!std::filesystem::is_regular_file(font_path))
        throw std::runtime_error("Chinese font not found; use --font TTF");
    // The same traced Unicode demand produces the packaged font and this runtime atlas.
    // Explicit dynamic/diagnostic text remains additive; no full-font atlas is loaded.
    std::string glyphs = generated::desktop_glyphs;
    for (int i = 32; i < 127; ++i)
        glyphs += static_cast<char>(i);
    glyphs += extra_glyphs;
    int count{};
    int *raw = LoadCodepoints(glyphs.c_str(), &count);
    if (!raw)
        throw std::runtime_error("Cannot decode font codepoints");
    std::set<int> unique(raw, raw + count);
    UnloadCodepoints(raw);
    // Script tables include line/tab separators; layout consumes them without raster glyphs.
    for (auto it = unique.begin(); it != unique.end();)
        if (*it < 32 || *it == 127)
            it = unique.erase(it);
        else
            ++it;
    codepoints_.assign(unique.begin(), unique.end());
    prepare(1);
}
void Text::prepare(float pixel_scale) {
    // Current labels are <= 16 logical units. Quantizing growth avoids reloading the glyph
    // atlas on every resize event; map wheel zoom does not change UI density.
    const int pixels = std::max(48, static_cast<int>(std::ceil(16 * pixel_scale / 16)) * 16);
    if (!glyphs_dirty_ && font_.texture.id && font_.baseSize >= pixels)
        return;
    auto next = LoadFontEx(font_path_.string().c_str(), pixels, codepoints_.data(),
                           static_cast<int>(codepoints_.size()));
    if (!next.texture.id || next.texture.id == GetFontDefault().texture.id)
        throw std::runtime_error("Cannot load Chinese font");
    for (auto code : codepoints_)
        if (code > 127 && next.glyphs[GetGlyphIndex(next, code)].value != code) {
            UnloadFont(next);
            throw std::runtime_error("Chinese font missing required glyph (Unicode decimal " +
                                     std::to_string(code) + ")");
        }
    SetTextureFilter(next.texture, TEXTURE_FILTER_BILINEAR);
    if (font_.texture.id)
        UnloadFont(font_);
    font_ = next;
    glyphs_dirty_ = false;
}
bool Text::include_text(const std::string &value) {
    int count{};
    int *raw = LoadCodepoints(value.c_str(), &count);
    if (!raw)
        return false;
    std::set<int> points(codepoints_.begin(), codepoints_.end());
    for (int i = 0; i < count; ++i)
        if (raw[i] >= 32 && raw[i] != 127)
            points.insert(raw[i]);
    UnloadCodepoints(raw);
    if (points.size() == codepoints_.size())
        return true;
    auto old = codepoints_;
    codepoints_.assign(points.begin(), points.end());
    glyphs_dirty_ = true;
    try {
        prepare(std::max(1.F, font_.baseSize / 16.F));
        return true;
    } catch (const std::exception &) {
        codepoints_ = std::move(old);
        glyphs_dirty_ = false;
        return false;
    }
}
Text::~Text() { UnloadFont(font_); }
void Text::draw(const std::string &value, float x, float y, Color color, float size) const {
    labels_.push_back({value, {x, y}, color, size, {}});
}
void Text::clipped(const std::string &value, float x, float y, Rectangle clip, Color color,
                   float size) const {
    labels_.push_back({value, {x, y}, color, size, clip});
}
void Text::flush(float scale, Vector2 offset) const {
    for (const auto &label : labels_) {
        if (label.clip)
            BeginScissorMode(static_cast<int>(offset.x + label.clip->x * scale),
                             static_cast<int>(offset.y + label.clip->y * scale),
                             static_cast<int>(label.clip->width * scale),
                             static_cast<int>(label.clip->height * scale));
        DrawTextEx(font_, label.value.c_str(),
                   {offset.x + label.point.x * scale, offset.y + label.point.y * scale},
                   label.size * scale, 0, label.color);
        if (label.clip)
            EndScissorMode();
    }
    labels_.clear();
}
float Text::width(const std::string &value, float size) const {
    return MeasureTextEx(font_, value.c_str(), size, 0).x;
}
void Text::scene_text(const std::string &value, Vector2 point, Color color, float size) const {
    DrawTextEx(font_, value.c_str(), point, size, 0, color);
}
void Text::paragraph(const std::string &value, float x, float y, float width) const {
    // Raylib decodes codepoints; wrap only at UTF-8 boundaries using actual font measurements.
    std::string line;
    for (std::size_t i = 0; i < value.size();) {
        int bytes{};
        GetCodepointNext(value.c_str() + i, &bytes);
        const auto next = value.substr(i, static_cast<std::size_t>(bytes));
        if (!line.empty() && MeasureTextEx(font_, (line + next).c_str(), 12, 0).x > width) {
            draw(line, x, y);
            line.clear();
            y += 17;
        }
        line += next;
        i += static_cast<std::size_t>(bytes);
    }
    draw(line, x, y);
}
void check_assets(const std::filesystem::path &root) {
    for (const auto &[name, width, height] :
         {std::tuple{"title00.png", 600, 380}, std::tuple{"upper.png", 240, 9},
          std::tuple{"title_grass.png", 240, 18}}) {
        auto image = LoadImage((root / "steam_title" / name).string().c_str());
        const bool valid = image.data && image.width == width && image.height == height;
        if (image.data)
            UnloadImage(image);
        if (!valid)
            throw std::runtime_error("Missing or invalid Steam title image: " + std::string(name));
    }
    for (const auto &[name, width, height] :
         {std::tuple{"title00.png", 240, 330}, std::tuple{"title_logo.png", 236, 115},
          std::tuple{"title_window.png", 98, 68}}) {
        auto image = LoadImage((root / "title" / name).string().c_str());
        const bool valid = image.data && image.width == width && image.height == height;
        if (image.data)
            UnloadImage(image);
        if (!valid)
            throw std::runtime_error("Missing or invalid title image: " + std::string(name));
    }
    const auto images = image_index(root);
    const auto common_images = image_index(root, "common");
    const auto common2_images = image_index(root, "common2");
    for (const auto &[name, width, height] :
         {std::tuple{"road4block00.png", 30, 20}, std::tuple{"road4block01.png", 27, 15}}) {
        auto image = LoadImage((root / "common" / name).string().c_str());
        const bool valid_size = image.data && image.width == width && image.height == height;
        if (image.data)
            UnloadImage(image);
        if (!valid_size)
            throw std::runtime_error("Road patch missing or wrong dimensions");
    }
    // Structural validation retains unused source records, including out-of-atlas legacy records.
    // Pixel bounds apply to the frames this finite adapter actually requests (research/assets).
    for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.path().extension() == ".png") {
            const auto image = LoadImage(entry.path().string().c_str());
            if (!image.data || image.width <= 0 || image.height <= 0)
                throw std::runtime_error("Packaged PNG failed decode");
            UnloadImage(image);
        } else if (entry.path().extension() == ".seb") {
            assets::parse_legacy_seb(read_bytes(entry.path()));
        }
    }
    const auto validate_frame = [&](const std::filesystem::path &sprite_path, int frame,
                                    Sprites::Binding binding) {
        const auto sprite = assets::parse_legacy_seb(read_bytes(sprite_path));
        if (binding == Sprites::Binding::map) {
            frame = map_frame_index(sprite_path.filename().string(), sprite, frame);
            if (frame >= sprite.frame_count)
                return;
        }
        if (frame < 0 || frame >= sprite.frame_count)
            throw std::runtime_error("Requested sprite frame invalid: " + sprite_path.string());
        for (const auto &layer : sprite.layers)
            for (const auto &part : layer.parts) {
                if (part.frame != frame)
                    continue;
                if (binding == Sprites::Binding::secretary && part.image_index != 126)
                    throw std::runtime_error("Secretary image binding changed");
                const auto path =
                    binding == Sprites::Binding::farmer      ? root / "human/chara_flower00.png"
                    : binding == Sprites::Binding::secretary ? root / "common/chara_hishoko01.png"
                    : binding == Sprites::Binding::common
                        ? root / "common" / common_images.at(part.image_index)
                    : binding == Sprites::Binding::common2
                        ? root / "common2" / common2_images.at(part.image_index)
                        : root / "image" / images.at(part.image_index);
                auto image = LoadImage(path.string().c_str());
                if (!image.data)
                    throw std::runtime_error("Requested sprite image missing: " + path.string());
                try {
                    validate(part, image.width, image.height);
                } catch (...) {
                    UnloadImage(image);
                    throw;
                }
                UnloadImage(image);
            }
    };
    std::map<std::string, std::set<int>> requested;
    // Construction can remove an existing road: every adjacency frame can become reachable.
    for (int frame = 0; frame < 16; ++frame)
        requested["road00.seb"].insert(frame);
    for (const auto &cell : app::startup_data().map.cells) {
        const auto &displays = app::startup_data().displays;
        const auto it = std::find_if(displays.begin(), displays.end(),
                                     [&](const auto &v) { return v.id == cell.display_id; });
        requested[it->sprite].insert(cell.variant);
    }
    for (const auto &item : app::startup_data().definitions) {
        if (item.tab < 0)
            continue;
        const auto &displays = app::startup_data().displays;
        const auto it = std::find_if(displays.begin(), displays.end(),
                                     [&](const auto &v) { return v.id == item.display_id; });
        // Disabled catalog entries need only their thumbnail, not unsupported rotations.
        const int orientations = (item.kind == 6 || item.kind == 13) ? 1 : 2;
        for (int orientation = 0; orientation < orientations; ++orientation)
            for (const auto &part : facilities::footprint(item.shape, orientation, {0, 0}))
                requested[it->sprite].insert(part.fragment);
    }
    // The complete world's catalogue includes recruitment and later private housing that
    // the legacy slice disabled. Validate every authored fragment in both orientations.
    const auto &evidence = simulation::startup_evidence();
    for (const auto &item : evidence.definitions) {
        if ((!(item.flags & 4) && item.kind != 12) || item.kind == 6)
            continue;
        const auto display = std::find_if(evidence.displays.begin(), evidence.displays.end(),
                                          [&](const auto &d) { return d.id == item.display_id; });
        if (display == evidence.displays.end())
            throw std::runtime_error("Building references an unknown map display");
        for (const auto orientation : {simulation::rules::FacilityOrientation::first,
                                       simulation::rules::FacilityOrientation::second}) {
            const auto footprint = simulation::rules::facility_footprint(
                static_cast<simulation::rules::FacilityShape>(item.shape), orientation, {1, 0}, 3,
                3);
            if (footprint.error != simulation::rules::GeometryError::none)
                throw std::runtime_error("Building has an unsupported footprint");
            for (const auto &part : footprint.cells)
                requested[display->sprite].insert(part.fragment_index);
        }
    }
    for (const auto &entry : requested)
        for (int frame : entry.second)
            validate_frame(root / "image" / entry.first, frame, Sprites::Binding::map);
    for (const auto *sprite : {"walk00.seb", "walk01.seb", "walk02.seb", "walk03.seb"})
        for (int frame = 0; frame < 4; ++frame)
            validate_frame(root / "human" / sprite, frame, Sprites::Binding::farmer);
    validate_frame(root / "common/chara_hishoko01.seb", 0, Sprites::Binding::secretary);
    // Every expansion skin and corner can be consumed; retain the published common IDs.
    for (const auto &[name, frames, image_id] :
         {std::tuple{"fence010.seb", 6, 64}, std::tuple{"fence011.seb", 6, 64},
          std::tuple{"fence012.seb", 6, 64}, std::tuple{"door00.seb", 2, 5}}) {
        const auto path = root / "common" / name;
        const auto definition = assets::parse_legacy_seb(read_bytes(path));
        if (definition.frame_count != frames)
            throw std::runtime_error("Boundary sprite frame count changed");
        for (const auto &layer : definition.layers)
            for (const auto &part : layer.parts)
                if (part.image_index != image_id)
                    throw std::runtime_error("Boundary common image binding changed");
        for (int frame = 0; frame < frames; ++frame)
            validate_frame(path, frame, Sprites::Binding::common);
    }
    for (const auto &name :
         {"menu.seb", "wnd_menuIcon.seb", "finger_r.seb", "number01.seb", "number05.seb",
          "number08.seb", "number12.seb", "icon_season.seb", "wnd_conner.seb", "arrow02.seb"}) {
        const auto path = root / "common" / name;
        const auto definition = assets::parse_legacy_seb(read_bytes(path));
        for (int frame = 0; frame < definition.frame_count; ++frame)
            validate_frame(path, frame, Sprites::Binding::common);
    }
    for (const auto &name : {"touch_arrow.seb", "buildCategoryBack.seb"}) {
        const auto path = root / "common2" / name;
        const auto definition = assets::parse_legacy_seb(read_bytes(path));
        for (int frame = 0; frame < definition.frame_count; ++frame)
            validate_frame(path, frame, Sprites::Binding::common2);
    }
}
} // namespace ark::desktop
