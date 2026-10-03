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
std::map<int, std::filesystem::path> image_index(const std::filesystem::path &root) {
    std::map<int, std::filesystem::path> result;
    for (const auto &row : assets::parse_tsv(read_bytes(root / "image/img.inf"))) {
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
    : root_(std::move(root)), images_(image_index(root_)) {}
Sprites::~Sprites() {
    for (const auto &entry : textures_)
        UnloadTexture(entry.second);
}
void Sprites::draw(const std::string &sprite, int frame, Vector2 anchor, Color tint,
                   Binding binding, float scale) {
    if (std::filesystem::path(sprite).has_parent_path())
        throw std::runtime_error("Unsafe sprite path");
    const char *group = binding == Binding::farmer      ? "human"
                        : binding == Binding::secretary ? "common"
                                                        : "image";
    const auto relative = std::filesystem::path(group) / sprite;
    auto found = sprites_.find(relative.string());
    if (found == sprites_.end())
        found =
            sprites_
                .emplace(relative.string(), assets::parse_legacy_seb(read_bytes(root_ / relative)))
                .first;
    if (frame < 0 || frame >= found->second.frame_count)
        throw std::runtime_error("Source variant outside sprite frames");
    for (const auto &layer : found->second.layers)
        for (const auto &p : layer.parts) {
            if (p.frame != frame)
                continue;
            if (binding == Binding::secretary && p.image_index != 126)
                throw std::runtime_error("Secretary image binding changed");
            const auto path = binding == Binding::farmer ? root_ / "human/chara_flower00.png"
                              : binding == Binding::secretary
                                  ? root_ / "common/chara_hishoko01.png"
                                  : root_ / "image" / images_.at(p.image_index);
            auto image = textures_.find(path.string());
            if (image == textures_.end()) {
                auto texture = LoadTexture(path.string().c_str());
                if (!texture.id)
                    throw std::runtime_error("Cannot load texture: " + path.string());
                SetTextureFilter(texture, TEXTURE_FILTER_POINT);
                try {
                    image = textures_.emplace(path.string(), texture).first;
                } catch (...) {
                    UnloadTexture(texture);
                    throw;
                }
            }
            validate(p, image->second.width, image->second.height);
            DrawTexturePro(image->second,
                           {static_cast<float>(p.source_x), static_cast<float>(p.source_y),
                            static_cast<float>(p.flip_x ? -p.width : p.width),
                            static_cast<float>(p.flip_y ? -p.height : p.height)},
                           {anchor.x + p.offset_x * scale, anchor.y + p.offset_y * scale,
                            p.width * scale, p.height * scale},
                           {0, 0}, 0, tint);
        }
}
Text::Text(const std::filesystem::path &font_path) {
    if (!std::filesystem::is_regular_file(font_path))
        throw std::runtime_error("Chinese font not found; use --font TTF");
    std::string glyphs =
        "建设返回确定旋转设施冒险者名单点数人气年月份道路植物商店饮食金币暂停继续重新开始研究边界"
        "请选择街道内地域有建筑物金钱不足未知设施不可用状态异常施工剩余招募到访等级农家体力攻击防御"
        "魔法品质魅力尚无本轮结束";
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
    std::vector<int> codes(unique.begin(), unique.end());
    font_ =
        LoadFontEx(font_path.string().c_str(), 24, codes.data(), static_cast<int>(codes.size()));
    if (!font_.texture.id || font_.texture.id == GetFontDefault().texture.id)
        throw std::runtime_error("Cannot load Chinese font");
    for (auto code : codes)
        if (code > 127 && font_.glyphs[GetGlyphIndex(font_, code)].value != code) {
            UnloadFont(font_);
            throw std::runtime_error("Chinese font missing required glyph");
        }
    SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
}
Text::~Text() { UnloadFont(font_); }
void Text::draw(const std::string &value, float x, float y, Color color, float size) const {
    DrawTextEx(font_, value.c_str(), {x, y}, size, 0, color);
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
                const auto path = binding == Sprites::Binding::farmer
                                      ? root / "human/chara_flower00.png"
                                  : binding == Sprites::Binding::secretary
                                      ? root / "common/chara_hishoko01.png"
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
    validate_frame(root / "human/walk00.seb", 0, Sprites::Binding::farmer);
    validate_frame(root / "common/chara_hishoko01.seb", 0, Sprites::Binding::secretary);
}
} // namespace ark::desktop
