// SourceSprites/ChineseFont adapted from research/prototype/src/startup_view.cpp.
// PNG index, SEB index and map display ID are different namespaces; farmer uses job image override.
#include "resources.hpp"
#include "ark/app/startup_data.hpp"
#include "ark/assets/table.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <tuple>

namespace ark::desktop {
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
        if (row.size() != 2)
            throw std::runtime_error("Invalid image index row");
        auto name = std::filesystem::path(row[1]);
        if (name.has_parent_path() || name.is_absolute())
            throw std::runtime_error("Unsafe image path");
        name.replace_extension(".png");
        if (!result.emplace(assets::parse_table_integer(row[0]), name).second)
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
} // namespace
Sprites::Sprites(std::filesystem::path root)
    : root_(std::move(root)), images_(image_index(root_)),
      common_images_(image_index(root_, "common")), common2_images_(image_index(root_, "common2")) {
}
Sprites::~Sprites() {
    for (const auto &entry : textures_)
        UnloadTexture(entry.second);
}
void Sprites::draw(const std::string &sprite, int frame, Vector2 anchor, Color tint,
                   Binding binding, float scale) {
    if (std::filesystem::path(sprite).has_parent_path())
        throw std::runtime_error("Unsafe sprite path");
    const char *group = binding == Binding::farmer                                    ? "human"
                        : binding == Binding::common2                                 ? "common2"
                        : binding == Binding::secretary || binding == Binding::common ? "common"
                                                                                      : "image";
    const auto relative = std::filesystem::path(group) / sprite;
    const auto &sprite_data = definition(relative);
    if (frame < 0 || frame >= sprite_data.frame_count)
        throw std::runtime_error("Source variant outside sprite frames");
    for (const auto &layer : sprite_data.layers)
        for (const auto &p : layer.parts) {
            if (p.frame != frame)
                continue;
            if (binding == Binding::secretary && p.image_index != 126)
                throw std::runtime_error("Secretary image binding changed");
            const auto path =
                binding == Binding::farmer      ? root_ / "human/chara_flower00.png"
                : binding == Binding::secretary ? root_ / "common/chara_hishoko01.png"
                : binding == Binding::common    ? root_ / group / common_images_.at(p.image_index)
                : binding == Binding::common2   ? root_ / group / common2_images_.at(p.image_index)
                                                : root_ / "image" / images_.at(p.image_index);
            const auto &image = texture(path);
            validate(p, image.width, image.height);
            DrawTexturePro(image,
                           {static_cast<float>(p.source_x), static_cast<float>(p.source_y),
                            static_cast<float>(p.flip_x ? -p.width : p.width),
                            static_cast<float>(p.flip_y ? -p.height : p.height)},
                           {anchor.x + p.offset_x * scale, anchor.y + p.offset_y * scale,
                            p.width * scale, p.height * scale},
                           {0, 0}, 0, tint);
        }
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
    if (std::filesystem::path(sprite).has_parent_path())
        throw std::runtime_error("Unsafe sprite path");
    const auto &data = definition(std::filesystem::path("image") / sprite);
    bool found = false;
    int left{}, top{}, right{}, bottom{};
    for (const auto &layer : data.layers)
        for (const auto &part : layer.parts)
            if (part.frame == frame) {
                left = found ? std::min(left, static_cast<int>(part.offset_x)) : part.offset_x;
                top = found ? std::min(top, static_cast<int>(part.offset_y)) : part.offset_y;
                right = found ? std::max(right, part.offset_x + part.width)
                              : part.offset_x + part.width;
                bottom = found ? std::max(bottom, part.offset_y + part.height)
                               : part.offset_y + part.height;
                found = true;
            }
    if (!found || right <= left || bottom <= top)
        throw std::runtime_error("Empty thumbnail frame");
    const float scale = std::min(box.width / (right - left), box.height / (bottom - top));
    draw(sprite, frame,
         {box.x + (box.width - (right - left) * scale) / 2 - left * scale,
          box.y + (box.height - (bottom - top) * scale) / 2 - top * scale},
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
    const char *group = binding == Binding::common2  ? "common2"
                        : binding == Binding::window ? "ui"
                        : binding == Binding::map    ? "image"
                                                     : "common";
    const auto &value = texture(root_ / group / name);
    if (source.x < 0 || source.y < 0 || source.width <= 0 || source.height <= 0 ||
        source.x + source.width > value.width || source.y + source.height > value.height)
        throw std::runtime_error("UI image rectangle outside atlas");
    DrawTexturePro(value, source, destination, {0, 0}, 0, tint);
}
Text::Text(const std::filesystem::path &font_path) : font_path_(font_path) {
    if (!std::filesystem::is_regular_file(font_path))
        throw std::runtime_error("Chinese font not found; use --font TTF");
    std::string glyphs =
        "运行中 · 暂停已暂停 · 继续人物活动待接入：后续活动住所/出口野外人物更新失败"
        "建设返回确定旋转设施冒险者名单点数人气年月份道路植物商店饮食金币暂停继续重新开始研究边界"
        "请选择街道内地域有建筑物金钱不足未知设施不可用状态异常施工剩余招募到访等级农家体力攻击防御"
        "魔法品质魅力尚无本轮结束建造设备一般办公室信息系统保存菜单网站价格使用道具设施信息"
        "距离下个级还有人数周农家暂无到访者冒险者一览要建造在哪里建设完毕设施奖励维护费"
        "没有奖励周围种类正在销售武器防具饰品查看商品入住希望者住宅";
    for (int i = 32; i < 127; ++i)
        glyphs += static_cast<char>(i);
    for (const auto &v : app::startup_data().definitions)
        glyphs += v.name;
    for (const auto &v : app::startup_data().first_talk)
        glyphs += v;
    glyphs += app::startup_data().first_character.name;
    int count{};
    int *raw = LoadCodepoints(glyphs.c_str(), &count);
    if (!raw)
        throw std::runtime_error("Cannot decode font codepoints");
    const std::set<int> unique(raw, raw + count);
    UnloadCodepoints(raw);
    codepoints_.assign(unique.begin(), unique.end());
    prepare(1);
}
void Text::prepare(float pixel_scale) {
    // Current labels are <= 16 logical units. Quantizing growth avoids reloading the glyph
    // atlas on every resize event; map wheel zoom does not change UI density.
    const int pixels = std::max(48, static_cast<int>(std::ceil(16 * pixel_scale / 16)) * 16);
    if (font_.texture.id && font_.baseSize >= pixels)
        return;
    auto next = LoadFontEx(font_path_.string().c_str(), pixels, codepoints_.data(),
                           static_cast<int>(codepoints_.size()));
    if (!next.texture.id || next.texture.id == GetFontDefault().texture.id)
        throw std::runtime_error("Cannot load Chinese font");
    for (auto code : codepoints_)
        if (code > 127 && next.glyphs[GetGlyphIndex(next, code)].value != code) {
            UnloadFont(next);
            throw std::runtime_error("Chinese font missing required glyph");
        }
    SetTextureFilter(next.texture, TEXTURE_FILTER_BILINEAR);
    if (font_.texture.id)
        UnloadFont(font_);
    font_ = next;
}
Text::~Text() { UnloadFont(font_); }
void Text::draw(const std::string &value, float x, float y, Color color, float size) const {
    labels_.push_back({value, {x, y}, color, size});
}
void Text::flush(float scale, Vector2 offset) const {
    for (const auto &label : labels_)
        DrawTextEx(font_, label.value.c_str(),
                   {offset.x + label.point.x * scale, offset.y + label.point.y * scale},
                   label.size * scale, 0, label.color);
    labels_.clear();
}
float Text::width(const std::string &value, float size) const {
    return MeasureTextEx(font_, value.c_str(), size, 0).x;
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
    for (const auto &entry : requested)
        for (int frame : entry.second)
            validate_frame(root / "image" / entry.first, frame, Sprites::Binding::map);
    for (int frame = 0; frame < 4; ++frame)
        validate_frame(root / "human/walk00.seb", frame, Sprites::Binding::farmer);
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
