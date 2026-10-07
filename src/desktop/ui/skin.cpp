// Window stripe and title bar follow ui/PAGES: p images28/29; content corners use SEB6.
#include "skin.hpp"
#include <algorithm>
namespace ark::desktop::ui {
void Skin::tile(const std::string &name, Rectangle source, Rectangle box,
                Sprites::Binding group) const {
    for (float y = box.y; y < box.y + box.height; y += source.height)
        for (float x = box.x; x < box.x + box.width; x += source.width) {
            const float w = std::min(source.width, box.x + box.width - x);
            const float h = std::min(source.height, box.y + box.height - y);
            sprites.image(name, {source.x, source.y, w, h}, {x, y, w, h}, group);
        }
}
void Skin::window(Rectangle box, const std::string &title) const {
    tile("wnd_back.png", {0, 0, 4, 240}, box, Sprites::Binding::window);
    DrawRectangleLinesEx(box, 1, Color{83, 69, 33, 255});
    DrawRectangleLinesEx({box.x + 1, box.y + 1, box.width - 2, box.height - 2}, 1,
                         Color{226, 216, 130, 255});
    tile("wnd_bar.png", {0, 0, 240, 17}, {box.x + 2, box.y + 2, box.width - 4, 17},
         Sprites::Binding::window);
    centered(title, {box.x + 2, box.y + 2, box.width - 4, 17}, WHITE);
}
void Skin::content(Rectangle box, Color fill) const {
    DrawRectangleRec(box, fill);
    DrawRectangleLinesEx(box, 1, Color{184, 211, 168, 255});
    const Vector2 corners[] = {{box.x, box.y},
                               {box.x + box.width, box.y},
                               {box.x + box.width, box.y + box.height},
                               {box.x, box.y + box.height}};
    for (int i = 0; i < 4; ++i)
        sprites.draw("wnd_conner.seb", i, corners[i], WHITE, Sprites::Binding::common);
}
void Skin::centered(const std::string &value, Rectangle box, Color color, float size) const {
    const float actual =
        std::min(size, size * (box.width - 4) / std::max(1.0F, text.width(value, size)));
    text.draw(value, box.x + (box.width - text.width(value, actual)) / 2,
              box.y + (box.height - actual) / 2, color, actual);
}
void Skin::right(const std::string &value, float x, float y, Color color, float size) const {
    text.draw(value, x - text.width(value, size), y, color, size);
}
void Skin::button(Rectangle box, const std::string &label, bool enabled) const {
    // S057/S048 soft keys have rounded brown/gold chrome and a light beveled face.
    // Geometry follows the existing hit box; this presentation change adds no action.
    DrawRectangleRounded(box, .22F, 4, {62, 53, 31, 255});
    DrawRectangleRounded({box.x + 1, box.y + 1, box.width - 2, box.height - 2}, .22F, 4,
                         {178, 121, 48, 255});
    DrawRectangleRounded({box.x + 3, box.y + 3, box.width - 6, box.height - 6}, .2F, 4,
                         {243, 239, 212, 255});
    DrawRectangleRounded({box.x + 5, box.y + 5, box.width - 10, box.height - 10}, .18F, 4,
                         {206, 207, 201, 255});
    centered(label, box, enabled ? Color{32, 30, 29, 255} : GRAY);
}
void Skin::choice(Rectangle box, const std::string &label, bool enabled) const {
    const float width = std::min(box.width - 4, text.width(label) + 8);
    const Rectangle highlight{box.x + (box.width - width) / 2, box.y + 2, width, box.height - 4};
    DrawRectangleRec(highlight, enabled ? Color{255, 153, 55, 255} : Color{213, 208, 188, 255});
    centered(label, box, enabled ? ink : GRAY);
    if (enabled)
        sprites.draw("finger_r.seb", 0, {highlight.x - 7, highlight.y + highlight.height / 2},
                     WHITE, Sprites::Binding::common);
}
void Skin::number(std::int64_t value, Vector2 edge, const std::string &sprite) const {
    const auto digits = std::to_string(value);
    float x = edge.x - digits.size() * 8;
    for (char c : digits) {
        if (c >= '0' && c <= '9')
            sprites.draw(sprite, c - '0', {x, edge.y}, WHITE, Sprites::Binding::common);
        else
            text.draw("-", x, edge.y, sprite == "number12.seb" ? MAROON : blue, 10);
        x += 8;
    }
}
} // namespace ark::desktop::ui
