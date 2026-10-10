// Consume the formal raw13 draw stream; never initialize, shuffle or advance its Owner here.
#include "world_manual.hpp"
#include "../common/skin.hpp"
#include "ark/presentation/script_text.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/presentation/startup_title_actor_skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
Color color(const std::array<int, 3> &c) {
    return {static_cast<unsigned char>(c[0]), static_cast<unsigned char>(c[1]),
            static_cast<unsigned char>(c[2]), 255};
}
Rectangle translated(std::array<int, 4> r, Vector2 o) {
    return {o.x + r[0], o.y + r[1], float(r[2]), float(r[3])};
}
Rectangle intersect(Rectangle a, Rectangle b) {
    const float x = std::max(a.x, b.x), y = std::max(a.y, b.y);
    return {x, y, std::max(0.F, std::min(a.x + a.width, b.x + b.width) - x),
            std::max(0.F, std::min(a.y + a.height, b.y + b.height) - y)};
}
void image(const sim::StartupSkinDraw &p, Vector2 origin, const Skin &skin,
           std::optional<Rectangle> clip) {
    const Vector2 at{origin.x + p.offset[0], origin.y + p.offset[1]};
    if (p.sprite >= 0)
        skin.sprites.indexed_sprite(Sprites::Binding::steam_common, p.sprite, std::max(0, p.frame),
                                    p.layer, p.image, at, 1, clip);
    else if (p.crop[2] > 0 && p.crop[3] > 0)
        skin.sprites.indexed_image(
            Sprites::Binding::steam_common, p.image,
            {float(p.crop[0]), float(p.crop[1]), float(p.crop[2]), float(p.crop[3])},
            {at.x, at.y, float(p.crop[2]), float(p.crop[3])}, clip);
}
// Shared decoder handles br/co. Wrapping uses actual glyph widths; color runs retain byte offsets.
// This is the bounded product TextLayout adapter, not the original platform font engine or LT.
void body_text(const sim::SteamManualText &p, Vector2 origin, const Skin &skin,
               std::optional<Rectangle> clip) {
    const auto decoded = decode_script_text(p.body);
    const float size = p.font_size ? float(p.font_size) : 12.F;
    const auto box = translated(p.rectangle, origin);
    const auto bounds = clip ? intersect(*clip, box) : box;
    const auto lines = wrap_plain_text(decoded.text, box.width,
                                       [&](const auto &s) { return skin.text.width(s, size); });
    std::size_t offset{}, run_start{}, run_index{};
    float y = box.y;
    for (const auto &line : lines) {
        float x = box.x;
        const auto end = offset + line.size();
        while (offset < end) {
            while (run_index < decoded.runs.size() &&
                   run_start + decoded.runs[run_index].text.size() <= offset) {
                run_start += decoded.runs[run_index].text.size();
                ++run_index;
            }
            if (run_index == decoded.runs.size())
                throw std::runtime_error("Manual text color offsets diverged");
            const auto &run = decoded.runs[run_index];
            const auto count = std::min(end - offset, run_start + run.text.size() - offset);
            const auto fragment = decoded.text.substr(offset, count);
            Color tint = color(p.rgb);
            if (run.rgb)
                tint = {static_cast<unsigned char>(*run.rgb >> 16),
                        static_cast<unsigned char>(*run.rgb >> 8),
                        static_cast<unsigned char>(*run.rgb), 255};
            skin.text.clipped(fragment, x, y, bounds, tint, size);
            x += skin.text.width(fragment, size);
            offset += count;
        }
        if (offset < decoded.text.size() && decoded.text[offset] == '\n')
            ++offset;
        y += size + p.line_space;
    }
}
} // namespace
bool world_manual_page(const sim::rules::WorldScriptPage &page) {
    return page.kind == sim::rules::WorldScriptPageKind::raw_page && page.legacy_page == 13;
}
WorldManualView world_manual_view(const sim::StartupWorldRuntimeState &state,
                                  const sim::rules::WorldScriptPage &page) {
    WorldManualView view;
    if (!world_manual_page(page))
        return view;
    view.source = sim::inspect_startup_world_manual_page(state, page.id);
    view.interactive =
        view.source.has_value() && page.lifecycle == 2 && !state.scene.framework_paused;
    return view;
}
std::optional<sim::StartupManualInput>
world_manual_input(const WorldManualView &view, Extent extent,
                   const std::optional<WorldManualLayout> &painted, const WorldManualInput &input,
                   bool blocked) {
    if (blocked || !view.interactive || !view.source)
        return {};
    sim::StartupManualInput out{input.confirm, input.right, input.left, input.cancel};
    if (input.click) {
        // One pointer event is one action; stale rectangles after resize/page changes cannot click
        // through.
        out = {};
        if (!painted || painted->page != view.source->page ||
            painted->index != view.source->index || painted->extent.width != extent.width ||
            painted->extent.height != extent.height)
            return {};
        if (CheckCollisionPointRec(*input.click, painted->back))
            out.cancel = true;
        else if (CheckCollisionPointRec(*input.click, painted->arrows[0]))
            out.left = true;
        else if (CheckCollisionPointRec(*input.click, painted->arrows[1]))
            out.right = true;
    }
    if (out.confirm || out.right || out.left || out.cancel)
        return out;
    return {};
}
WorldManualLayout draw_world_manual(const sim::StartupWorldRuntimeState &state,
                                    const WorldManualView &view, Extent extent, const Skin &skin,
                                    bool enabled) {
    WorldManualLayout layout{};
    layout.extent = extent;
    if (!view.source)
        return layout;
    const auto &source = *view.source;
    layout.page = source.page;
    layout.index = source.index;
    layout.counter = source.counter;
    sim::SteamManualSkinInput input;
    input.width = extent.width;
    input.height = extent.height;
    input.body_pages = int(state.rules->manual_pages.size());
    input.page = source.index;
    input.frame = source.counter;
    input.body_text = source.text;
    input.localize_text = source.localize_text;
    input.frozen_definitions = source.decorations;
    input.definition_count = int(state.rules->humans.size());
    const auto plan = sim::steam_manual_skin(input);
    if (!plan)
        throw std::runtime_error("Invalid initialized manual skin");
    const Vector2 origin{float(plan->origin[0]), float(plan->origin[1])};
    layout.back = {origin.x + 179, origin.y + 218, 53, 20};
    std::optional<Rectangle> clip;
    std::vector<std::optional<Rectangle>> clips;
    const auto frame = [&](const sim::SteamStartupFramePlan &parts, const std::string &title) {
        for (const auto &draw : parts.draws) {
            if (const auto *p = std::get_if<sim::StartupSkinRect>(&draw)) {
                const auto r = translated(p->rect, origin);
                if (p->outline)
                    DrawRectangleLinesEx(r, 1, color(p->rgb));
                else
                    DrawRectangleRec(r, color(p->rgb));
            } else if (const auto *p = std::get_if<sim::StartupSkinDraw>(&draw))
                image(*p, origin, skin, clip);
            else if (const auto *p = std::get_if<sim::SteamStartupText>(&draw))
                skin.text.draw(title, origin.x + float(p->rectangle[0]),
                               origin.y + float(p->rectangle[1]),
                               color(p->rgb.value_or(std::array<int, 3>{30, 30, 30})),
                               p->font_size ? float(p->font_size) : 12.F);
        }
    };
    for (const auto &draw : plan->draws) {
        if (const auto *p = std::get_if<sim::SteamManualWindow>(&draw)) {
            const auto title =
                "游戏方法 " + std::to_string(p->page_number) + "/" + std::to_string(p->total_pages);
            const auto parts = sim::steam_startup_window_skin(
                p->source, 23,
                std::array<int, 2>{int(skin.text.width(title)), int(skin.text.width(title))});
            if (!parts)
                throw std::runtime_error("Invalid manual window");
            frame(*parts, title);
        } else if (const auto *p = std::get_if<sim::SteamStartupBox>(&draw)) {
            const auto parts = sim::steam_startup_box_skin(*p, 23);
            if (!parts)
                throw std::runtime_error("Invalid manual content frame");
            frame(*parts, {});
        } else if (const auto *p = std::get_if<sim::SteamManualArrowRequest>(&draw)) {
            const auto resource = sim::steam_startup_resource(sim::SteamStartupAsset::arrows);
            if (!resource || !resource->sprite_name)
                throw std::runtime_error("Missing manual arrow bounds resource");
            const auto bounds = skin.sprites.frame_bounds(resource->sprite_name, p->bounds_frame,
                                                          Sprites::Binding::common);
            const auto arrow = sim::steam_manual_arrow_skin(
                *p, {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
            if (!arrow)
                throw std::runtime_error("Invalid manual arrow");
            const auto &c = arrow->crop;
            const auto &d = arrow->destination;
            const Rectangle box{origin.x + d[0], origin.y + d[1], d[2], d[3]};
            skin.sprites.image("buttoneffect.png",
                               {float(c[0]), float(c[1]), float(c[2]), float(c[3])}, box,
                               Sprites::Binding::steam_common);
            layout.arrows[p->bounds_frame == 3 ? 0 : 1] = box;
        } else if (const auto *p = std::get_if<sim::SteamManualText>(&draw)) {
            if (p->role == sim::SteamManualTextRole::body)
                body_text(*p, origin, skin, clip);
            else {
                const std::string value = p->role == sim::SteamManualTextRole::about ? "关于本游戏"
                                          : p->role == sim::SteamManualTextRole::game_name
                                              ? "Ark-Village"
                                              : "原作：开罗软件";
                const float size = p->font_size ? float(p->font_size) : 12.F;
                float x = origin.x + p->rectangle[0];
                const int anchor = p->anchor.value_or(0);
                if (anchor & 2)
                    x -= skin.text.width(value, size) / 2;
                else if (anchor & 4)
                    x -= skin.text.width(value, size);
                if (clip)
                    skin.text.clipped(value, x, origin.y + p->rectangle[1], *clip, color(p->rgb),
                                      size);
                else
                    skin.text.draw(value, x, origin.y + p->rectangle[1], color(p->rgb), size);
            }
        } else if (const auto *p = std::get_if<sim::SteamManualClip>(&draw)) {
            if (p->push) {
                clips.push_back(clip);
                const auto next = translated(p->rectangle, origin);
                clip = clip ? intersect(*clip, next) : next;
            } else {
                if (clips.empty())
                    throw std::runtime_error("Unbalanced manual clip");
                clip = clips.back();
                clips.pop_back();
            }
        } else if (const auto *p = std::get_if<sim::StartupSkinDraw>(&draw))
            image(*p, origin, skin, clip);
        else if (const auto *p = std::get_if<sim::SteamManualActorRequest>(&draw)) {
            const auto human = sim::startup_world_human_details(state, p->definition);
            if (!human)
                throw std::runtime_error("Missing current manual decoration definition");
            const auto actor = sim::startup_title_actor_skin(human->profession, human->sex,
                                                             human->equipment[0].value_or(-1),
                                                             p->anime_index, p->direction, true);
            if (!actor)
                throw std::runtime_error("Invalid manual basic character plan");
            const Vector2 at{origin.x + p->position[0], origin.y + p->position[1]};
            for (const auto *layer : {&actor->shadow, &actor->weapon})
                if (*layer) {
                    const auto &part = **layer;
                    skin.sprites.indexed_sprite(part.resource == sim::StartupVisualResource::weapon
                                                    ? Sprites::Binding::weapon
                                                    : Sprites::Binding::common,
                                                part.sprite, part.frame, part.layer, part.image,
                                                {at.x + part.offset[0], at.y + part.offset[1]}, 1,
                                                clip);
                }
            skin.sprites.actor(false, actor->body.sprite, actor->body.image, actor->body.frame, at,
                               1, clip);
        } else if (const auto *p = std::get_if<sim::StartupSkinRect>(&draw)) {
            const auto r = translated(p->rect, origin);
            if (p->outline)
                DrawRectangleLinesEx(r, 1, color(p->rgb));
            else
                DrawRectangleRec(r, color(p->rgb));
        }
    }
    if (!clips.empty())
        throw std::runtime_error("Unclosed manual clip");
    skin.button(layout.back, "返回", enabled && view.interactive);
    return layout;
}
} // namespace ark::desktop::ui
