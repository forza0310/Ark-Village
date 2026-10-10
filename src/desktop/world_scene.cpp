// Adapted from the frozen maintained prototype --world renderer, using native product resources.
#include "world_scene.hpp"
#include "character_status.hpp"
#include "ui/layout.hpp"
#include "world_combat_visuals.hpp"
#include "world_dungeon_visuals.hpp"
#include "world_overlay_render.hpp"
#include "world_rest_visuals.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace ark::desktop {
namespace {
namespace rules = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
Vector2 raw_anchor(const WorldCameraView &view, float x, float y, float zoom) {
    const auto &v = view.viewport;
    return {zoom * ((v[0] + v[2]) / 2 + x - view.camera[0]),
            zoom * (v[1] + v[3] - (v[1] + v[3]) / 2 - y + view.camera[1])};
}
Vector2 raw_anchor(const State &s, float x, float y, float zoom) {
    return raw_anchor(WorldCameraView{s.camera, s.reference_viewport}, x, y, zoom);
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
    if (!std::isfinite(zoom) || zoom < .25F || zoom > 2.F || extent.width < 240 ||
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
std::optional<rules::CharacterId> world_pick_human(const State &s, const WorldCameraView &view,
                                                   Vector2 pointer, float zoom) {
    if (!std::isfinite(zoom) || zoom <= 0 || !std::isfinite(pointer.x) || !std::isfinite(pointer.y))
        return {};
    const auto &ai = s.scene.world.world.ai;
    for (const auto id : ai.human_order) {
        const auto found = ai.battle.actors.find(id);
        if (found == ai.battle.actors.end() || found->second.control.state == 4)
            continue;
        const auto p = simulation::startup_world_raw_projection(found->second.position);
        const auto point = raw_anchor(view, static_cast<float>(p.x), static_cast<float>(p.y), zoom);
        if (CheckCollisionPointRec(
                pointer, {point.x - 10 * zoom, point.y - 24 * zoom, 20 * zoom, 26 * zoom}))
            return id;
    }
    return {};
}
void world_zoom_camera(WorldCameraView &view, Extent extent, Vector2 pointer, float wheel,
                       float &zoom) {
    const auto next = std::clamp(zoom * std::pow(1.05F, wheel), .25F, 2.F);
    const auto origin = raw_anchor(view, 0, 0, zoom);
    const float x = (pointer.x - origin.x) / zoom, y = (origin.y - pointer.y) / zoom;
    view.viewport = world_viewport(extent, next);
    const auto shifted = raw_anchor(view, x, y, next);
    view.camera[0] += (shifted.x - pointer.x) / next;
    view.camera[1] += (pointer.y - shifted.y) / next;
    zoom = next;
}
void world_zoom_at(State &s, Extent extent, Vector2 pointer, float wheel, float &zoom) {
    WorldCameraView view{s.camera, s.reference_viewport};
    world_zoom_camera(view, extent, pointer, wheel, zoom);
    s.camera = view.camera;
    s.reference_viewport = view.viewport;
}
simulation::rules::CombatPoint world_actor_render_position(const State &s, rules::CharacterId id,
                                                           const State *previous, float alpha) {
    const auto current = s.actor_metadata.at(id).render_position;
    if (!previous || s.scene.framework_paused || !previous->actor_metadata.count(id))
        return current;
    const auto &actor = s.scene.world.world.ai.battle.actors.at(id);
    const auto old_actor = previous->scene.world.world.ai.battle.actors.find(id);
    if (old_actor == previous->scene.world.world.ai.battle.actors.end() ||
        actor.control.action != 0 || old_actor->second.control.action != 0 ||
        actor.control.state != old_actor->second.control.state || (actor.control.flags & 1U) ||
        (old_actor->second.control.flags & 1U) || actor.kind != old_actor->second.kind)
        return current;
    const auto old = previous->actor_metadata.at(id).render_position;
    // A cell is 100 units: larger discontinuities are map/focus transitions, never walking.
    if (std::abs(current.x - old.x) > 100 || std::abs(current.z - old.z) > 100 ||
        std::abs(current.height - old.height) > 100 || !std::isfinite(alpha))
        return current;
    const auto t = std::clamp(alpha, 0.F, 1.F);
    return {old.x + (current.x - old.x) * t, old.height + (current.height - old.height) * t,
            old.z + (current.z - old.z) * t};
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
void draw_world_scene(const State &s, Sprites &sprites, const Text &text, float zoom,
                      const State *previous, float alpha, const WorldCameraView *override_view,
                      SpritePickMap *picks) {
    const auto &world = s.scene.world.world;
    const auto view =
        override_view ? *override_view : WorldCameraView{s.camera, s.reference_viewport};
    const auto project = [&](rules::CombatPoint p) {
        return raw_anchor(view, (p.x + p.z) * .3F, (p.z - p.x) * .15F + p.height, zoom);
    };
    struct Draw {
        float depth{};
        std::function<void()> paint;
        std::optional<SpritePickTarget> pick{};
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
            const auto p = raw_anchor(view, 30.F * (x + y), 15.F * (y - x) + 15, zoom);
            const float depth = p.y + zoom * (((record.flags & 1U) ? -50 : 15) + record.offset_y);
            SpritePickTarget target;
            const auto &binding = world.map.cells.at(index).facility;
            if (binding && world.facilities.count(binding->instance_id.value) &&
                world.facilities.at(binding->instance_id.value).status != 0)
                target = {SpritePickTarget::Kind::facility, binding->instance_id.value};
            queue.push_back({depth,
                             [&, p, sprite = record.sprite, frame = cell.variant] {
                                 sprites.draw(sprite, frame, p, WHITE, Sprites::Binding::map, zoom);
                             },
                             target});
            if (cell.fragment >= 0 && cell.fragment < 6 &&
                world.map.cells.at(index).category == rules::RouteCategory::blocked) {
                const auto offset = fence.at(cell.fragment);
                queue.push_back({depth + 60 * zoom,
                                 [&, p, offset, frame = cell.fragment] {
                                     sprites.draw(
                                         "fence01" + std::to_string(s.fence_level) + ".seb", frame,
                                         {p.x + offset.x * zoom, p.y + offset.y * zoom}, WHITE,
                                         Sprites::Binding::common, zoom);
                                 },
                                 SpritePickTarget{}});
            }
            if (cell.instance >= 0) {
                const auto offset = doors.at(static_cast<std::size_t>(cell.instance));
                queue.push_back({p.y + offset.y * zoom,
                                 [&, p, offset, frame = cell.instance / 2] {
                                     sprites.draw("door00.seb", frame,
                                                  {p.x + offset.x * zoom, p.y + offset.y * zoom},
                                                  WHITE, Sprites::Binding::common, zoom);
                                 },
                                 SpritePickTarget{}});
            }
            if (s.road_patches.at(index)[0] || s.road_patches.at(index)[1]) {
                const bool quad = s.road_patches.at(index)[0];
                const float w = quad ? 30 : 27, h = quad ? 20 : 15;
                const float dx = quad ? 14 : 20, dy = quad ? 21 : 19;
                patches.push_back(
                    {p.y - 10 * zoom,
                     [&, p, quad, w, h, dx, dy] {
                         sprites.image(quad ? "road4block00.png" : "road4block01.png", {0, 0, w, h},
                                       {p.x + dx * zoom, p.y + dy * zoom, w * zoom, h * zoom},
                                       Sprites::Binding::common);
                     },
                     SpritePickTarget{}});
            }
        }
    queue.insert(queue.end(), patches.begin(), patches.end());
    for (const auto *roster : {&world.ai.human_order, &world.ai.monster_order})
        for (const auto id : *roster) {
            const auto &actor = world.ai.battle.actors.at(id);
            if (actor.control.flags & 1U)
                continue;
            auto ground_position = world_actor_render_position(s, id, previous, alpha);
            ground_position.height = 0;
            const auto depth = project(ground_position).y;
            queue.push_back(
                {depth,
                 [&, id] {
                     const auto &a = world.ai.battle.actors.at(id);
                     const auto point =
                         project(world_actor_render_position(s, id, previous, alpha));
                     const auto pose = world_actor_pose(s, id);
                     sprites.actor(pose.monster, pose.sprite, pose.image, pose.frame, point, zoom);
                     // Floating status/artwork does not grow the physical actor's hit surface.
                     Sprites::PickScope no_labels(sprites, nullptr, {});
                     if (!pose.monster)
                         draw_world_actor_effects(s, id, sprites, text, point, zoom);
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
                     draw_world_overlay(world_actor_combat_visuals(s, id), sprites, point, zoom);
                 },
                 SpritePickTarget{actor.kind == rules::ActorKind::human && actor.control.state != 4
                                      ? SpritePickTarget::Kind::human
                                      : SpritePickTarget::Kind::blocker,
                                  id.value}});
        }
    // Neighbourhood notices already belong to the instance. Its f() projection is the
    // source anchor; neither tile centre nor texture height determines this position.
    for (const auto id : s.scene.world.facility_order) {
        const auto plan = simulation::startup_world_facility_growth_draws(s, id);
        if (!plan)
            throw std::runtime_error("Invalid facility growth display state");
        if (plan->empty())
            continue;
        const auto target = simulation::startup_world_runtime_facility_target(s, id);
        if (!target)
            throw std::runtime_error("Facility growth display lost its source anchor");
        const auto point = raw_anchor(view, (*target)[0], (*target)[1], zoom);
        queue.push_back({point.y, [&, point, plan = *plan] {
                             draw_world_visuals(plan, sprites, point, zoom);
                         }});
    }
    // The task's bar and ground marker have different source depths. Keep both in the
    // shared stable queue so actors can occlude them according to the same world ordering.
    const auto dungeon = world_dungeon_view(s);
    if (dungeon.active) {
        const auto point = raw_anchor(view, 30.F * (dungeon.site.x + dungeon.site.y),
                                      15.F * (dungeon.site.y - dungeon.site.x) + 15, zoom);
        const int height =
            dungeon.has_facility && dungeon.phase == 1 && dungeon.progress >= 0
                ? sprites.map_image_height(dungeon.tenant_sprite, dungeon.tenant_frame)
                : 0;
        // Use the published wider text offsets for the bundled Chinese label so 100% does
        // not overlap it. This desktop artwork choice does not certify the APK language flag.
        for (auto layer : world_dungeon_visuals(dungeon, height, true))
            queue.push_back(
                {point.y + layer.depth_offset * zoom, [&, point, plan = std::move(layer.plan)] {
                     draw_world_overlay(plan, sprites, point, zoom);
                 }});
    }
    std::stable_sort(queue.begin(), queue.end(),
                     [](const auto &a, const auto &b) { return a.depth < b.depth; });
    for (const auto &draw : queue) {
        Sprites::PickScope capture(sprites, draw.pick ? picks : nullptr,
                                   draw.pick.value_or(SpritePickTarget{}));
        draw.paint();
    }
    // Inn occupants can hide their body (bit1); their facility-owned rest rows remain visible.
    // This screen position is a desktop adaptation until the original L/portrait mapping is
    // published.
    for (const auto id : s.scene.world.facility_order) {
        const auto rows = world_rest_rows(s, id);
        if (rows.empty())
            continue;
        const auto &placement = world.facilities.at(id).placement;
        const auto cells =
            rules::facility_footprint(placement.shape, placement.orientation, placement.anchor,
                                      world.map.width, world.map.height);
        if (cells.cells.empty())
            throw std::runtime_error("Inn display has no valid footprint");
        float x{}, y{};
        for (const auto &cell : cells.cells) {
            x += cell.position.x + .5F;
            y += cell.position.y + .5F;
        }
        x /= cells.cells.size();
        y /= cells.cells.size();
        auto point = raw_anchor(view, 30.F * (x + y), 15.F * (y - x) + 15, zoom);
        point.x -= 24 * zoom;
        point.y -= 48 * zoom;
        for (const auto &row : rows)
            draw_world_overlay(row.plan, sprites, point, zoom);
    }
    const auto coins = simulation::startup_world_coin_effect_draws(s);
    if (!coins)
        throw std::invalid_argument("World coin effects have invalid source payload");
    auto coin = coins->begin();
    for (std::size_t index = 0; index < s.visual_effects.size(); ++index) {
        const auto &effect = s.visual_effects[index];
        // X2/X3 and X4 retain the Owner's order. The death anchor is already projected;
        // apply only the camera transform, then the source dy and SEB offset once.
        while (coin != coins->end() && coin->image.record_index == index) {
            const auto point = raw_anchor(view, static_cast<float>(coin->raw_anchor[0]),
                                          static_cast<float>(coin->raw_anchor[1]), zoom);
            draw_world_visuals({coin->image}, sprites, point, zoom);
            ++coin;
        }
        const auto plan = world_cash_visuals(effect);
        if (plan.empty())
            continue;
        const auto point =
            raw_anchor(view, static_cast<float>(effect[2]), static_cast<float>(effect[3]), zoom);
        draw_world_overlay(plan, sprites, point, zoom);
    }
}
} // namespace ark::desktop
