#include "world_overlay_render.hpp"

#include <type_traits>

namespace ark::desktop {
void draw_world_overlay(const OverlayPlan &plan, Sprites &sprites, Vector2 anchor, float zoom) {
    for (const auto &command : plan)
        std::visit(
            [&](const auto &part) {
                using Part = std::decay_t<decltype(part)>;
                if constexpr (std::is_same_v<Part, OverlaySprite>)
                    sprites.draw(part.name, part.frame,
                                 {anchor.x + part.x * zoom, anchor.y + part.y * zoom}, WHITE,
                                 Sprites::Binding::common, zoom);
                else if constexpr (std::is_same_v<Part, OverlayRectangle>)
                    DrawRectangleRec({anchor.x + part.x * zoom, anchor.y + part.y * zoom,
                                      part.width * zoom, part.height * zoom},
                                     {part.rgb[0], part.rgb[1], part.rgb[2], 255});
                else
                    sprites.image(part.name,
                                  {part.source[0], part.source[1], part.source[2], part.source[3]},
                                  {anchor.x + part.destination[0] * zoom,
                                   anchor.y + part.destination[1] * zoom,
                                   part.destination[2] * zoom, part.destination[3] * zoom});
            },
            command);
}
} // namespace ark::desktop
