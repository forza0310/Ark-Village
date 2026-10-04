// Adapted from the frozen maintained prototype --world renderer, using native product resources.
#include "world_scene.hpp"
#include "character_status.hpp"
#include "ui/layout.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace ark::desktop {
namespace {
namespace rules = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
Vector2 raw_anchor(const State &s, float x, float y, float zoom) {
    const auto &v = s.reference_viewport;
    return {zoom * ((v[0] + v[2]) / 2 + x - s.camera[0]),
            zoom * (v[1] + v[3] - (v[1] + v[3]) / 2 - y + s.camera[1])};
}
const simulation::StartupDisplay &display(int id) {
    const auto &list = simulation::startup_evidence().displays;
    const auto found =
        std::find_if(list.begin(), list.end(), [id](const auto &entry) { return entry.id == id; });
    if (found == list.end())
        throw std::runtime_error("World surface references an unknown display");
    return *found;
}
} // namespace

std::array<int, 4> world_viewport(Extent extent, float zoom) {
    if (!std::isfinite(zoom) || zoom < .5F || zoom > 2.F || extent.width < 240 ||
        extent.height < 256)
        throw std::invalid_argument("Invalid desktop world viewport");
    const auto clip = ui::Layout(extent).scene_clip;
    // Viewport fields are source left/top/width/height. Inverse zoom puts rule-side visibility
    // in the same coordinate space as the sprite drawing; scene clipping still uses native pixels.
    return {static_cast<int>(std::floor(clip.x / zoom)),
            static_cast<int>(std::floor(clip.y / zoom)),
            static_cast<int>(std::ceil(clip.width / zoom)),
            static_cast<int>(std::ceil(clip.height / zoom))};
}
Vector2 world_anchor(const State &s, rules::CombatPoint p, float zoom) {
    return raw_anchor(s, (p.x + p.z) * .3F, (p.z - p.x) * .15F + p.height, zoom);
}
void world_zoom_at(State &s, Extent extent, Vector2 pointer, float wheel, float &zoom) {
    const auto next = std::clamp(zoom * std::pow(1.05F, wheel), .5F, 2.F);
    const auto origin = raw_anchor(s, 0, 0, zoom);
    const float x = (pointer.x - origin.x) / zoom, y = (origin.y - pointer.y) / zoom;
    s.reference_viewport = world_viewport(extent, next);
    const auto shifted = raw_anchor(s, x, y, next);
    s.camera[0] += (shifted.x - pointer.x) / next;
    s.camera[1] += (pointer.y - shifted.y) / next;
    zoom = next;
}
WorldActorPose world_actor_pose(const State &s, rules::CharacterId id) {
    const auto &a = s.scene.world.world.ai.battle.actors.at(id);
    const int action = a.control.action, direction = a.control.facing;
    const int tick = (a.control.flags & 2U) ? a.control.action_counter : 0;
    WorldActorPose pose;
    pose.monster = a.kind == rules::ActorKind::monster;
    if (!pose.monster) {
        const auto &meta = s.actor_metadata.at(id);
        pose.image = s.rules->jobs.at(meta.profession).sprites.at(meta.sex);
        constexpr std::array<int, 12> offsets{{0, 4, 4, 4, 20, 4, 8, 12, 16, 20, 24, 20}};
        if (action == 0 || action == 8)
            pose.frame = (tick % 16) / 4;
        else if (action == 1)
            pose.frame = tick % 12 < 4 ? 0 : 1;
        else if (action == 2 || action == 5)
            pose.frame = 1;
        else if (action == 3)
            pose.frame = tick % 12 < 6 ? 0 : 1;
        else if (action == 4)
            pose.frame = tick % 42 < 26 ? 0 : 1;
        else if (action == 10)
            pose.frame = tick % 18 < 12 ? 0 : 1;
        pose.sprite = offsets.at(action) + (action == 5 ? tick % 16 % 4 : direction);
    } else {
        const auto &definition = s.scene.world.world.ai.monster_growth.at(a.definition);
        constexpr std::array<int, 4> lengths{{6, 6, 8, 10}}, attacks{{2, 4, 6, 8}};
        const int body = definition.body;
        pose.frame = (tick % (lengths.at(body) * 4)) / lengths.at(body);
        if (action == 3) {
            if (tick < 10 || (tick >= 28 && tick < 34))
                pose.frame = 0;
            else if (tick < 28)
                pose.frame = (tick % (attacks.at(body) * 4)) / attacks.at(body);
        } else if (action == 6) {
            if (tick < 6)
                pose.frame = (tick % (attacks.at(body) * 4)) / attacks.at(body);
            else if (tick < 12)
                pose.frame = 0;
        }
        if (action == 9)
            pose.frame = 0;
        pose.sprite = body * 4 + direction;
        pose.image = body * 30 + definition.sprite_variant;
    }
    return pose;
}
void draw_world_scene(const State &s, Sprites &sprites, float zoom) {
    const auto &world = s.scene.world.world;
    struct Draw {
        float depth{};
        std::function<void()> paint;
    };
    std::vector<Draw> queue, patches;
    constexpr std::array<rules::Position, 6> fence{
        {{13, 22}, {16, 22}, {13, 21}, {42, 22}, {15, 23}, {16, 9}}};
    constexpr std::array<rules::Position, 4> doors{{{15, 24}, {26, 19}, {15, 18}, {28, 24}}};
    for (int y = world.map.height - 1; y >= 0; --y)
        for (int x = 0; x < world.map.width; ++x) {
            const auto index = static_cast<std::size_t>(y * world.map.width + x);
            const auto &cell = s.surface.at(index);
            if (cell.display_definition < 0)
                continue;
            const auto &record = display(cell.display_definition);
            const auto p = raw_anchor(s, 30.F * (x + y), 15.F * (y - x) + 15, zoom);
            const float depth = p.y + zoom * (((record.flags & 1U) ? -50 : 15) + record.offset_y);
            queue.push_back({depth, [&, p, sprite = record.sprite, frame = cell.variant] {
                                 sprites.draw(sprite, frame, p, WHITE, Sprites::Binding::map, zoom);
                             }});
            if (cell.fragment >= 0 && cell.fragment < 6 &&
                world.map.cells.at(index).category == rules::RouteCategory::blocked) {
                const auto offset = fence.at(cell.fragment);
                queue.push_back({depth + 60 * zoom, [&, p, offset, frame = cell.fragment] {
                                     sprites.draw(
                                         "fence01" + std::to_string(s.fence_level) + ".seb", frame,
                                         {p.x + offset.x * zoom, p.y + offset.y * zoom}, WHITE,
                                         Sprites::Binding::common, zoom);
                                 }});
            }
            if (cell.instance >= 0) {
                const auto offset = doors.at(static_cast<std::size_t>(cell.instance));
                queue.push_back({p.y + offset.y * zoom, [&, p, offset, frame = cell.instance / 2] {
                                     sprites.draw("door00.seb", frame,
                                                  {p.x + offset.x * zoom, p.y + offset.y * zoom},
                                                  WHITE, Sprites::Binding::common, zoom);
                                 }});
            }
            if (s.road_patches.at(index)[0] || s.road_patches.at(index)[1]) {
                const bool quad = s.road_patches.at(index)[0];
                const float w = quad ? 30 : 27, h = quad ? 20 : 15;
                const float dx = quad ? 14 : 20, dy = quad ? 21 : 19;
                patches.push_back(
                    {p.y - 10 * zoom, [&, p, quad, w, h, dx, dy] {
                         sprites.image(quad ? "road4block00.png" : "road4block01.png", {0, 0, w, h},
                                       {p.x + dx * zoom, p.y + dy * zoom, w * zoom, h * zoom},
                                       Sprites::Binding::common);
                     }});
            }
        }
    queue.insert(queue.end(), patches.begin(), patches.end());
    for (const auto *roster : {&world.ai.human_order, &world.ai.monster_order})
        for (const auto id : *roster) {
            const auto &actor = world.ai.battle.actors.at(id);
            if (actor.control.flags & 1U)
                continue;
            auto ground_position = actor.position;
            ground_position.height = 0;
            const auto depth = world_anchor(s, ground_position, zoom).y;
            queue.push_back(
                {depth, [&, id] {
                     const auto &a = world.ai.battle.actors.at(id);
                     const auto point =
                         world_anchor(s, s.actor_metadata.at(id).render_position, zoom);
                     const auto pose = world_actor_pose(s, id);
                     sprites.actor(pose.monster, pose.sprite, pose.image, pose.frame, point, zoom);
                     const auto &hp = a.hp;
                     const CharacterStatusInput status{{hp.requested_delta, hp.displayed, hp.origin,
                                                        hp.target, hp.animating, hp.legacy_tick},
                                                       a.capacity,
                                                       a.control.action,
                                                       !pose.monster,
                                                       false,
                                                       true};
                     for (const auto &bar : character_hp_bar(status))
                         DrawRectangleRec({point.x + bar.x * zoom, point.y + bar.y * zoom,
                                           bar.width * zoom, bar.height * zoom},
                                          {bar.rgb[0], bar.rgb[1], bar.rgb[2], 255});
                 }});
        }
    std::stable_sort(queue.begin(), queue.end(),
                     [](const auto &a, const auto &b) { return a.depth < b.depth; });
    for (const auto &draw : queue)
        draw.paint();
    for (const auto &effect : s.visual_effects) {
        if (effect.size() < 2 || effect[0] != 2 || effect[1] < 0)
            continue;
        if (effect.size() != 7 || effect[4] < 0)
            throw std::runtime_error("Invalid world cash-display payload");
        auto p = raw_anchor(s, static_cast<float>(effect[2]), static_cast<float>(effect[3]), zoom);
        const int n = effect[1];
        p.y += zoom * (n < 6 ? -16 + (effect[5] * n + effect[6] * n * (n + 1) / 2) / 1000 : -26);
        const auto digits = std::to_string(effect[4]);
        p.x += zoom * (28 - (digits.size() * 8 + 9) / 2.F);
        for (const auto digit : digits) {
            sprites.draw("number05.seb", digit - '0', {p.x, p.y - 10 * zoom}, WHITE,
                         Sprites::Binding::common, zoom);
            p.x += 8 * zoom;
        }
        sprites.draw("number05.seb", 20, {p.x, p.y - 10 * zoom}, WHITE, Sprites::Binding::common,
                     zoom);
    }
}
} // namespace ark::desktop
