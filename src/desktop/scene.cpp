// Source-seed scene and evidenced connected road frames. Full loaded-map overlays remain pending.
#include "scene.hpp"
#include "ark/world/terrain.hpp"
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
void draw_scene(const app::Game &game, Sprites &sprites, Vector2 camera, Extent extent,
                float zoom) {
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
    };
    std::vector<Tile> ground, objects;
    for (int y = 0; y < data.map.height; ++y)
        for (int x = 0; x < data.map.width; ++x) {
            const auto source = terrain.cells[terrain.index({x, y})];
            ground.push_back({game.display(source.display_id).sprite, source.variant,
                              project({x, y}, camera, extent, zoom), WHITE, false});
        }
    for (const auto &entry : game.state().facilities) {
        const auto &v = entry.second;
        const auto &item = game.definition(v.definition_id);
        for (const auto &part : facilities::footprint(item.shape, v.orientation, v.anchor))
            objects.push_back({game.display(item.display_id).sprite, part.fragment,
                               project(part.cell, camera, extent, zoom),
                               v.remaining_ticks ? Color{180, 180, 180, 255} : WHITE, false});
    }
    if (game.state().adventurer)
        objects.push_back({"walk00.seb", 0,
                           project(game.state().adventurer->cell, camera, extent, zoom), WHITE,
                           true});
    const auto draw = [&](std::vector<Tile> &tiles) {
        std::stable_sort(tiles.begin(), tiles.end(),
                         [](const auto &a, const auto &b) { return a.point.y < b.point.y; });
        for (const auto &tile : tiles)
            sprites.draw(tile.sprite, tile.frame, tile.point, tile.tint,
                         tile.farmer ? Sprites::Binding::farmer : Sprites::Binding::map, zoom);
    };
    draw(ground);
    draw(objects);
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
