#pragma once

// Published artwork composition, not a screenshot-as-texture. No ownership of gameplay state.
#include "../resources.hpp"
namespace ark::desktop::ui {
inline constexpr Color ink{64, 48, 32, 255}, blue{42, 71, 152, 255}, green{91, 174, 53, 255};
class Skin {
  public:
    Skin(Sprites &sprites, const Text &text) : sprites(sprites), text(text) {}
    void tile(const std::string &name, Rectangle source, Rectangle box,
              Sprites::Binding group = Sprites::Binding::common) const;
    void window(Rectangle box, const std::string &title) const;
    void content(Rectangle box, Color fill = {247, 253, 247, 255}) const;
    void button(Rectangle box, const std::string &label, bool enabled = true) const;
    // Orange inline commands seen in S043/S044, distinct from the rounded footer soft keys.
    void choice(Rectangle box, const std::string &label, bool enabled = true) const;
    void centered(const std::string &value, Rectangle box, Color color = ink,
                  float size = 12) const;
    void right(const std::string &value, float x, float y, Color color = ink,
               float size = 12) const;
    void number(std::int64_t value, Vector2 right,
                const std::string &sprite = "number08.seb") const;
    Sprites &sprites;
    const Text &text;
};
} // namespace ark::desktop::ui
