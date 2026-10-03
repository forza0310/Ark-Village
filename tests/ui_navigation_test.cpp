// Coordinate-level input contracts; no OS automation or GPU required. Uses actual Game state.
#include "ui/controller.hpp"
#include <stdexcept>
namespace {
void require(bool ok) {
    if (!ok)
        throw std::runtime_error("UI navigation contract failed");
}
Vector2 center(Rectangle r) { return {r.x + r.width / 2, r.y + r.height / 2}; }
} // namespace
int main() {
    using namespace ark;
    using namespace desktop;
    for (auto size : {Extent{480, 256}, Extent{240, 330}, Extent{640, 256}}) {
        app::Game game;
        ui::State view;
        view.camera = {426, -72};
        const ui::Layout layout(size);
        ui::click(game, view, layout, center(layout.right_button));
        require(view.page == ui::Page::menu && ui::blocks_world(view));
        const auto money = game.state().money;
        ui::click(game, view, layout, center(layout.menu_rows[1]));
        require(view.page == ui::Page::menu && game.state().money == money);
        ui::click(game, view, layout, center(layout.menu_rows[0]));
        require(game.state().mode == app::Mode::catalog && view.page == ui::Page::village);
        ui::click(game, view, layout,
                  {layout.catalog.x + layout.catalog.width + 4, layout.catalog.y + 10});
        require(view.tab == 2 && view.row == 0);
        ui::click(game, view, layout, {layout.catalog.x - 4, layout.catalog.y + 10});
        require(view.tab == 1 && view.row == 0);
        ui::click(game, view, layout, center(layout.left_button));
        require(view.page == ui::Page::definition && game.state().money == money);
        ui::click(game, view, layout, center(layout.right_button));
        require(view.page == ui::Page::village && game.state().mode == app::Mode::catalog);
        ui::click(game, view, layout, center(layout.rows[0]));
        require(game.state().mode == app::Mode::placement && view.selection);
        const auto cell = *view.selection;
        require(game.preview(cell) == app::Error::none);
        auto arrows = layout.arrows(tile_center(cell, view.camera, size));
        ui::click(game, view, layout, center(arrows[0]));
        require(view.selection == world::Cell{cell.x, cell.y + 1} && game.state().money == money);
        arrows = layout.arrows(tile_center(*view.selection, view.camera, size));
        ui::click(game, view, layout, center(arrows[2]));
        require(view.selection == cell && game.state().money == money);
        ui::click(game, view, layout, center(layout.left_button));
        require(game.state().orientation == 1 && game.state().money == money);
        ui::confirm(game, view);
        require(game.state().money == money - 1000 && game.facility_at(cell));
        // Repeated confirmation without a selected cell cannot duplicate the charge.
        ui::confirm(game, view);
        require(game.state().money == money - 1000);
        ui::back(game, view);
        require(game.state().mode == app::Mode::catalog && !view.selection);
        ui::back(game, view);
        require(view.page == ui::Page::menu && game.state().mode == app::Mode::normal);
        ui::click(game, view, layout, center(layout.menu_rows[3]));
        require(view.page == ui::Page::roster && ui::blocks_world(view));
        ui::back(game, view);
        ui::back(game, view);
        require(view.page == ui::Page::village && !ui::blocks_world(view));
        // Empty cells cannot create a fake detail panel; source inn selection binds real identity.
        for (const auto &entry : game.state().facilities)
            if (entry.second.definition_id == 28) {
                view.camera = {30.0F * (entry.second.anchor.x + entry.second.anchor.y),
                               15.0F * (entry.second.anchor.y - entry.second.anchor.x) + 15};
                ui::click(game, view, layout, tile_center(entry.second.anchor, view.camera, size));
                require(view.page == ui::Page::facility && view.detail == entry.first);
                break;
            }
    }
}
