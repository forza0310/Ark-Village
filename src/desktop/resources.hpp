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
    enum class Binding { map, farmer, secretary };
    explicit Sprites(std::filesystem::path root);
    ~Sprites();
    Sprites(const Sprites &) = delete;
    Sprites &operator=(const Sprites &) = delete;
    void draw(const std::string &sprite, int frame, Vector2 anchor, Color tint = WHITE,
              Binding binding = Binding::map, float scale = 1);

  private:
    std::filesystem::path root_;
    std::map<int, std::filesystem::path> images_;
    std::map<std::string, assets::SpriteDefinition> sprites_;
    std::map<std::string, Texture2D> textures_;
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

  private:
    Font font_{};
};
// CPU-only PNG/SEB structural validation, plus bounds/flips for actual requested frames/bindings.
void check_assets(const std::filesystem::path &root);
} // namespace ark::desktop
