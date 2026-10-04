// Loaded ground and current instances. Boundary/exterior extra overlays still await bindings.
#include "scene.hpp"
#include "ark/world/terrain.hpp"
#include "road_render.hpp"
#include <algorithm>
namespace ark::desktop {
namespace {
void outline(world::Cell cell, Vector2 camera, Extent extent, float zoom, Color color) {
    const auto p = tile_center(cell, camera, extent, zoom);
    DrawLineEx({p.x - 30 * zoom, p.y}, {p.x, p.y - 15 * zoom}, 1, color);
    DrawLineEx({p.x, p.y - 15 * zoom}, {p.x + 30 * zoom, p.y}, 1, color);
    DrawLineEx({p.x + 30 * zoom, p.y}, {p.x, p.y + 15 * zoom}, 1, color);
    DrawLineEx({p.x, p.y + 15 * zoom}, {p.x - 30 * zoom, p.y}, 1, color);
}
} // namespace
void draw_scene(const app::Game &game, Sprites &sprites, Vector2 camera, Extent extent, float zoom,
                std::optional<world::WorldPosition> inspection_actor) {
    const auto &data = app::startup_data();
    auto terrain = data.map;
    // Build the visual input from current occupancy, rather than immutable startup instance IDs.
    for (int y = 0; y < terrain.height; ++y)
        for (int x = 0; x < terrain.width; ++x)
            if (game.facility_at({x, y}))
                terrain.cells[terrain.index({x, y})] = {27, 0};
    terrain = world::connect_roads(std::move(terrain), game.definition(18).display_id);
    struct Tile {
        std::string sprite;
        int frame{};
        Vector2 point{};
        Color tint{};
        bool farmer{};
        float depth{};
        int image_width{}, image_height{}; // Nonzero only for whole-PNG road patches.
    };
    std::vector<Tile> tiles;
    const auto base_depth = [&](const app::Display &display, Vector2 p) {
        return p.y + zoom * (((display.flags & 1) ? -50 : 15) + display.depth_offset);
    };
    for (int y = data.map.height - 1; y >= 0; --y)
        for (int x = 0; x < data.map.width; ++x) {
            const auto source = terrain.cells[terrain.index({x, y})];
            const auto &display = game.display(source.display_id);
            const auto p = project({x, y}, camera, extent, zoom);
            tiles.push_back(
                {display.sprite, source.variant, p, WHITE, false, base_depth(display, p)});
        }
    // Second pass submits to the same depth queue. Never overlay patches over all foreground.
    // This finite renderer draws all base tiles (no patch-specific viewport culling).
    for (const auto &patch : road_patches(terrain, game.definition(18).display_id)) {
        const auto p = project(patch.cell, camera, extent, zoom);
        tiles.push_back({patch.image,
                         0,
                         {p.x + zoom * patch.offset_x, p.y + zoom * patch.offset_y},
                         WHITE,
                         false,
                         p.y + zoom * patch.depth_offset,
                         patch.width,
                         patch.height});
    }
    for (const auto id : game.state().instance_order) {
        const auto &v = game.state().facilities.at(id);
        const auto &item = game.definition(v.definition_id);
        for (const auto &part : facilities::footprint(item.shape, v.orientation, v.anchor)) {
            const auto &display = game.display(item.display_id);
            const auto p = project(part.cell, camera, extent, zoom);
            tiles.push_back({display.sprite, part.fragment, p,
                             v.remaining_ticks ? Color{180, 180, 180, 255} : WHITE, false,
                             base_depth(display, p)});
        }
    }
    if (game.state().adventurer) {
        const auto p = project_position(
            inspection_actor.value_or(game.state().adventurer->position), camera, extent, zoom);
        tiles.push_back({"walk00.seb", 0, p, WHITE, true, p.y});
    }
    std::stable_sort(tiles.begin(), tiles.end(),
                     [](const auto &a, const auto &b) { return a.depth < b.depth; });
    for (const auto &tile : tiles) {
        if (tile.image_width) {
            sprites.image(
                tile.sprite, {0, 0, float(tile.image_width), float(tile.image_height)},
                {tile.point.x, tile.point.y, zoom * tile.image_width, zoom * tile.image_height});
            continue;
        }
        sprites.draw(tile.sprite, tile.frame, tile.point, tile.tint,
                     tile.farmer ? Sprites::Binding::farmer : Sprites::Binding::map, zoom);
    }
}
void draw_preview(const app::Game &game, const ui::State &view, const ui::Layout &layout,
                  Sprites &sprites, std::optional<world::Cell> hovered) {
    if (game.state().mode != app::Mode::placement)
        return;
    const auto cell = view.selection ? view.selection : hovered;
    if (!cell)
        return;
    const auto &item = game.definition(*game.state().selection);
    const bool valid = game.preview(*cell) == app::Error::none;
    for (const auto &part : facilities::footprint(item.shape, game.state().orientation, *cell)) {
        const auto p = project(part.cell, view.camera, layout.extent, view.zoom);
        outline(part.cell, view.camera, layout.extent, view.zoom, valid ? YELLOW : RED);
        sprites.draw(game.display(item.display_id).sprite, part.fragment, p,
                     valid ? Color{255, 255, 255, 215} : Color{255, 135, 135, 190},
                     Sprites::Binding::map, view.zoom);
    }
    const auto arrows = layout.arrows(tile_center(*cell, view.camera, layout.extent, view.zoom));
    for (int i = 0; i < 4; ++i)
        sprites.draw("touch_arrow.seb", i, {arrows[i].x + 14, arrows[i].y + 8}, WHITE,
                     Sprites::Binding::common2);
}
} // namespace ark::desktop
