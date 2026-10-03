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
    // Gray inset command style observed in S001/S003; metrics are desktop adaptation.
    DrawRectangleRec(box, Color{107, 75, 28, 255});
    DrawRectangleLinesEx(box, 1, Color{52, 55, 41, 255});
    DrawRectangleLinesEx({box.x + 1, box.y + 1, box.width - 2, box.height - 2}, 1,
                         Color{206, 161, 68, 255});
    DrawRectangleRec({box.x + 4, box.y + 4, box.width - 8, box.height - 8},
                     Color{205, 206, 199, 255});
    DrawRectangleLinesEx({box.x + 5, box.y + 5, box.width - 10, box.height - 10}, 1, WHITE);
    DrawLine(static_cast<int>(box.x + 6), static_cast<int>(box.y + box.height - 6),
             static_cast<int>(box.x + box.width - 6), static_cast<int>(box.y + box.height - 6),
             GRAY);
    centered(label, box, enabled ? Color{32, 30, 29, 255} : GRAY);
}
void Skin::number(std::int64_t value, Vector2 edge, const std::string &sprite) const {
    const auto digits = std::to_string(value);
    float x = edge.x - digits.size() * 8;
    for (char c : digits) {
        if (c >= '0' && c <= '9')
            sprites.draw(sprite, c - '0', {x, edge.y}, WHITE, Sprites::Binding::common);
        else
            text.draw("-", x, edge.y, blue, 10);
        x += 8;
    }
}
} // namespace ark::desktop::ui
