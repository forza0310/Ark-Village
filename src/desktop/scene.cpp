// Source-seed projection from research/prototype. Ground refresh after initialization is pending.
#include "scene.hpp"
#include <algorithm>
namespace ark::desktop {
namespace {
void outline(world::Cell cell, Vector2 camera, Extent extent, Color color) {
    const auto p = tile_center(cell, camera, extent);
    DrawLineEx({p.x - 30, p.y}, {p.x, p.y - 15}, 1, color);
    DrawLineEx({p.x, p.y - 15}, {p.x + 30, p.y}, 1, color);
    DrawLineEx({p.x + 30, p.y}, {p.x, p.y + 15}, 1, color);
    DrawLineEx({p.x, p.y + 15}, {p.x - 30, p.y}, 1, color);
}
} // namespace
void draw_scene(const app::Game &game, Sprites &sprites, Vector2 camera, Extent extent) {
    const auto &data = app::startup_data();
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
            const auto source = data.map.cells[data.map.index({x, y})];
            const bool occupied = game.facility_at({x, y}).has_value();
            ground.push_back({game.display(occupied ? 27 : source.display_id).sprite,
                              occupied ? 0 : source.variant, project({x, y}, camera, extent), WHITE,
                              false});
        }
    for (const auto &entry : game.state().facilities) {
        const auto &v = entry.second;
        const auto &item = game.definition(v.definition_id);
        for (const auto &part : facilities::footprint(item.shape, v.orientation, v.anchor))
            objects.push_back({game.display(item.display_id).sprite, part.fragment,
                               project(part.cell, camera, extent),
                               v.remaining_ticks ? Color{180, 180, 180, 255} : WHITE, false});
    }
    if (game.state().adventurer)
        objects.push_back(
            {"walk00.seb", 0, project(game.state().adventurer->cell, camera, extent), WHITE, true});
    const auto draw = [&](std::vector<Tile> &tiles) {
        std::stable_sort(tiles.begin(), tiles.end(),
                         [](const auto &a, const auto &b) { return a.point.y < b.point.y; });
        for (const auto &tile : tiles)
            sprites.draw(tile.sprite, tile.frame, tile.point, tile.tint,
                         tile.farmer ? Sprites::Binding::farmer : Sprites::Binding::map);
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
        const auto p = project(part.cell, view.camera, layout.extent);
        outline(part.cell, view.camera, layout.extent, valid ? YELLOW : RED);
        sprites.draw(game.display(item.display_id).sprite, part.fragment, p,
                     valid ? Color{255, 255, 255, 215} : Color{255, 135, 135, 190});
    }
    const auto arrows = layout.arrows(tile_center(*cell, view.camera, layout.extent));
    for (int i = 0; i < 4; ++i)
        sprites.draw("touch_arrow.seb", i, {arrows[i].x + 14, arrows[i].y + 8}, WHITE,
                     Sprites::Binding::common2);
}
} // namespace ark::desktop
