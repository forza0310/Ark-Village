// Font atlas lifetime and deferred labels share one physical-pixel transform.
#include "resources.hpp"
#include "desktop_glyphs.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

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
} // namespace ark::desktop
