#pragma once

// GPU ownership and text measurements. This module never owns village rules or mutable state.
#include "ark/assets/sprite.hpp"
#include <filesystem>
#include <map>
#include <optional>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>

namespace ark::desktop {
// Explicit --font wins; packaged fonts resolve beside the executable's assets, not the cwd.
std::filesystem::path desktop_font_path(const std::filesystem::path &assets,
                                        const std::string &override_path);
class Sprites {
  public:
    enum class Binding { map, farmer, secretary, common, common2, window, human, monster };
    explicit Sprites(std::filesystem::path root);
    ~Sprites();
    Sprites(const Sprites &) = delete;
    Sprites &operator=(const Sprites &) = delete;
    void draw(const std::string &sprite, int frame, Vector2 anchor, Color tint = WHITE,
              Binding binding = Binding::map, float scale = 1, int image_override = -1);
    // Source actor SEB indices and profession/body image indices are independent namespaces.
    void actor(bool monster, int sprite_index, int image_index, int frame, Vector2 anchor,
               float scale = 1);
    // CPU-verified portrait crop from the current human image; no nested screen scissor or
    // render-resolution assumptions, so world zoom and Retina use the same source pixels.
    void human_image(int image_index, Rectangle source, Rectangle destination);
    // Fit a source frame into a desktop icon box; not the unshipped exact monster head crop.
    void actor_thumbnail(bool monster, int sprite_index, int image_index, Rectangle destination);
    // Raw published PNG rectangles for tiled bars and nine-slice window components.
    void image(const std::string &name, Rectangle source, Rectangle destination,
               Binding binding = Binding::common, Color tint = WHITE);
    // Fit the actual SEB frame bounds; the map anchor is not the image's visual center.
    void thumbnail(const std::string &sprite, int frame, Rectangle box, Color tint = WHITE);
    // Multi-cell facilities fit all source fragments together at their relative map anchors.
    void thumbnail(const std::string &sprite, const std::vector<std::pair<int, Vector2>> &frames,
                   Rectangle box, Color tint = WHITE);
    // Dungeon labels use the bound tenant PNG height, not a SEB frame bounding box.
    // Reject ambiguous multi-image frames instead of guessing an art height.
    int map_image_height(const std::string &sprite, int frame);
    // Resolve a logical map fragment without loading textures or changing world orientation.
    int map_frame(const std::string &sprite, int variant);

  private:
    std::filesystem::path root_;
    std::map<int, std::filesystem::path> images_;
    std::map<int, std::filesystem::path> common_images_, common2_images_;
    std::map<std::string, std::map<int, std::filesystem::path>> actor_images_;
    std::map<std::string, std::vector<std::string>> actor_sprites_;
    std::map<std::string, assets::SpriteDefinition> sprites_;
    std::map<std::string, Texture2D> textures_;
    Texture2D &texture(const std::filesystem::path &path);
    const assets::SpriteDefinition &definition(const std::filesystem::path &relative);
};
class Text {
  public:
    explicit Text(const std::filesystem::path &font_path, const std::string &extra_glyphs = {});
    ~Text();
    Text(const Text &) = delete;
    Text &operator=(const Text &) = delete;
    void draw(const std::string &value, float x, float y, Color color = {48, 44, 46, 255},
              float size = 12) const;
    void clipped(const std::string &value, float x, float y, Rectangle clip,
                 Color color = {48, 44, 46, 255}, float size = 12) const;
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
        std::optional<Rectangle> clip;
    };
    mutable std::vector<Label> labels_;
};
// CPU-only PNG/SEB structural validation, plus bounds/flips for actual requested frames/bindings.
void check_assets(const std::filesystem::path &root);
} // namespace ark::desktop
