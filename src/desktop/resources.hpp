#pragma once

// GPU ownership and text measurements. This module never owns village rules or mutable state.
#include "ark/assets/sprite.hpp"
#include <filesystem>
#include <map>
#include <raylib.h>
#include <string>

namespace ark::desktop {
class Sprites {
  public:
    enum class Binding { map, farmer, secretary, common, common2, window };
    explicit Sprites(std::filesystem::path root);
    ~Sprites();
    Sprites(const Sprites &) = delete;
    Sprites &operator=(const Sprites &) = delete;
    void draw(const std::string &sprite, int frame, Vector2 anchor, Color tint = WHITE,
              Binding binding = Binding::map, float scale = 1);
    // Raw published PNG rectangles for tiled bars and nine-slice window components.
    void image(const std::string &name, Rectangle source, Rectangle destination,
               Binding binding = Binding::common, Color tint = WHITE);
    // Fit the actual SEB frame bounds; the map anchor is not the image's visual center.
    void thumbnail(const std::string &sprite, int frame, Rectangle box, Color tint = WHITE);

  private:
    std::filesystem::path root_;
    std::map<int, std::filesystem::path> images_;
    std::map<int, std::filesystem::path> common_images_, common2_images_;
    std::map<std::string, assets::SpriteDefinition> sprites_;
    std::map<std::string, Texture2D> textures_;
    Texture2D &texture(const std::filesystem::path &path);
    const assets::SpriteDefinition &definition(const std::filesystem::path &relative);
};
class Text {
  public:
    explicit Text(const std::filesystem::path &font_path);
    ~Text();
    Text(const Text &) = delete;
    Text &operator=(const Text &) = delete;
    void draw(const std::string &value, float x, float y, Color color = {48, 44, 46, 255},
              float size = 12) const;
    void paragraph(const std::string &value, float x, float y, float width) const;
    float width(const std::string &value, float size = 12) const;
    // Grow glyph rasterization with physical UI scale (including Retina and window resize).
    // Call before layout measurements; retain the atlas until a larger density is needed.
    void prepare(float pixel_scale);
    // Replay labels directly into the native framebuffer after logical artwork rendering.
    void flush(float scale, Vector2 offset) const;

  private:
    Font font_{};
    std::filesystem::path font_path_;
    std::vector<int> codepoints_;
    struct Label {
        std::string value;
        Vector2 point;
        Color color;
        float size;
    };
    mutable std::vector<Label> labels_;
};
// CPU-only PNG/SEB structural validation, plus bounds/flips for actual requested frames/bindings.
void check_assets(const std::filesystem::path &root);
} // namespace ark::desktop
