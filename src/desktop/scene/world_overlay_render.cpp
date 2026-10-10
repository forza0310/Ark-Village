#include "world_overlay_render.hpp"

#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace ark::desktop {
void draw_world_actor_effects(const simulation::StartupWorldRuntimeState &state,
                              simulation::rules::CharacterId actor, Sprites &sprites,
                              const Text &text, Vector2 anchor, float zoom) {
    const auto lifts = simulation::startup_world_equipment_lift_draws(state, actor);
    // Bundled Noto at size16 measures decimal digits at the source helper's 6px pitch.
    // Use the same real font size for measurement and paint; never clamp the measured width.
    constexpr float font_size = 16;
    const auto gains =
        simulation::startup_world_attribute_gain_draws(state, actor, [&](const auto &value) {
            return static_cast<int>(std::ceil(text.width(value, font_size)));
        });
    if (!lifts || !gains)
        throw std::runtime_error("Invalid actor effect display state");
    for (const auto &lift : *lifts)
        if (!lift.record_index)
            throw std::runtime_error("Equipment lift lost its source record index");
    auto lift = lifts->begin();
    auto gain = gains->begin();
    while (lift != lifts->end() || gain != gains->end()) {
        if (lift != lifts->end() &&
            (gain == gains->end() || *lift->record_index < gain->record_index)) {
            draw_world_visuals({*lift++}, sprites, anchor, zoom);
        } else {
            draw_world_visuals(gain->before_text, sprites, anchor, zoom);
            const auto &rgb = gain->text_rgb;
            text.scene_text(
                gain->text,
                {anchor.x + gain->text_offset[0] * zoom, anchor.y + gain->text_offset[1] * zoom},
                {static_cast<unsigned char>(rgb[0]), static_cast<unsigned char>(rgb[1]),
                 static_cast<unsigned char>(rgb[2]), 255},
                font_size * zoom);
            draw_world_visuals(gain->after_text, sprites, anchor, zoom);
            ++gain;
        }
    }
}
void draw_world_visuals(const std::vector<simulation::StartupVisualDraw> &plan, Sprites &sprites,
                        Vector2 anchor, float zoom) {
    for (const auto &part : plan) {
        const auto binding = part.resource == simulation::StartupVisualResource::weapon
                                 ? Sprites::Binding::weapon
                                 : Sprites::Binding::common;
        const Vector2 point{anchor.x + part.offset[0] * zoom, anchor.y + part.offset[1] * zoom};
        if (part.sprite >= 0)
            sprites.indexed_sprite(binding, part.sprite, part.frame, part.layer, part.image, point,
                                   zoom);
        else
            sprites.indexed_image(
                binding, part.image,
                {static_cast<float>(part.crop[0]), static_cast<float>(part.crop[1]),
                 static_cast<float>(part.crop[2]), static_cast<float>(part.crop[3])},
                {point.x, point.y, part.crop[2] * zoom, part.crop[3] * zoom});
    }
}
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
                else if constexpr (std::is_same_v<Part, OverlayImage>)
                    sprites.image(part.name,
                                  {part.source[0], part.source[1], part.source[2], part.source[3]},
                                  {anchor.x + part.destination[0] * zoom,
                                   anchor.y + part.destination[1] * zoom,
                                   part.destination[2] * zoom, part.destination[3] * zoom});
                else
                    sprites.human_image(
                        part.image,
                        {part.source[0], part.source[1], part.source[2], part.source[3]},
                        {anchor.x + part.destination[0] * zoom,
                         anchor.y + part.destination[1] * zoom, part.destination[2] * zoom,
                         part.destination[3] * zoom});
            },
            command);
}
} // namespace ark::desktop
