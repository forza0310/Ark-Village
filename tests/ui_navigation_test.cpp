// Coordinate-level input contracts; no OS automation or GPU required. Uses actual Game state.
#include "ui/controller.hpp"
#include "ui/facility_page.hpp"
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
    // Real controller clicks must pause the arrival clock, consume the scene hit, and resume
    // the original first-visitor event exactly once; the modal dialogue cannot be unpaused.
    for (auto size : {Extent{384, 256}, Extent{240, 330}}) {
        app::Game arrival;
        ui::State view;
        const ui::Layout layout(size);
        ui::click(arrival, view, layout, center(layout.pause_button));
        require(arrival.state().paused && view.page == ui::Page::village && !view.detail);
        for (int i = 0; i < 600; ++i)
            arrival.update();
        require(arrival.state().simulation_steps == 0 && !arrival.state().adventurer);
        ui::click(arrival, view, layout, center(layout.pause_button));
        require(!arrival.state().paused);
        for (int i = 0; i < 419; ++i)
            arrival.update();
        require(!arrival.state().adventurer);
        arrival.update();
        require(arrival.state().adventurer && arrival.state().event89_count == 1 &&
                arrival.state().money == 5000 && arrival.state().mode == app::Mode::tutorial);
        ui::toggle_pause(arrival, view);
        require(!arrival.state().paused);
    }
    for (auto size : {Extent{480, 256}, Extent{240, 330}, Extent{640, 256}}) {
        app::Game game;
        ui::State view;
        view.camera = {426, -72};
        const ui::Layout layout(size);
        ui::click(game, view, layout, center(layout.right_button));
        require(view.page == ui::Page::menu && ui::blocks_world(view));
        ui::toggle_pause(game, view);
        require(!game.state().paused); // Hidden pause control/Space cannot change modal state.
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
        require(ui::facility_page_count(game, view) == 1);
        ui::click(game, view, layout, center(layout.detail_next));
        require(view.facility_page == 0);
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
                view.zoom = 1.5F;
                ui::click(game, view, layout,
                          tile_center(entry.second.anchor, view.camera, size, view.zoom));
                require(view.page == ui::Page::facility && view.detail == entry.first);
                require(ui::facility_page_count(game, view) == 2);
                const auto cash = game.state().money;
                ui::click(game, view, layout, center(layout.detail_next));
                require(view.facility_page == 1 && game.state().money == cash);
                ui::scroll_sources(game, view, 999);
                require(view.source_scroll == 0);
                ui::click(game, view, layout, center(layout.detail_previous));
                require(view.facility_page == 0 && game.state().money == cash);
                break;
            }
    }
    app::Game game;
    require(ui::facility_template(game.definition(28)) == ui::FacilityTemplate::ordinary &&
            ui::facility_template(game.definition(30)) == ui::FacilityTemplate::equipment &&
            ui::facility_template(game.definition(31)) == ui::FacilityTemplate::equipment &&
            ui::facility_template(game.definition(66)) == ui::FacilityTemplate::booster &&
            ui::facility_template(game.definition(24)) == ui::FacilityTemplate::recruitment &&
            ui::facility_template(game.definition(25)) == ui::FacilityTemplate::home);
    // Window routing uses the same pause eligibility in the live AI preview.
    app::Game ai(20261004, app::PlayMode::ai_preview);
    for (int n = 0; n < 420; ++n)
        ai.update();
    ai.acknowledge_talk();
    ai.acknowledge_talk();
    ai.finish_camera();
    ui::State view;
    const ui::Layout layout({384, 256});
    ui::click(ai, view, layout, center(layout.pause_button));
    const auto rounds = ai.ai_state()->rounds;
    ai.update();
    require(ai.state().paused && ai.ai_state()->rounds == rounds);
    ui::click(ai, view, layout, center(layout.pause_button));
    ai.update();
    require(!ai.state().paused && ai.ai_state()->rounds == rounds + 1);
    ui::click(ai, view, layout, center(layout.right_button));
    require(view.page == ui::Page::menu && ui::blocks_world(view));
    ui::click(ai, view, layout, center(layout.menu_rows[0]));
    require(view.page == ui::Page::menu && ai.state().mode == app::Mode::normal &&
            view.error == app::Error::unavailable);
    ui::back(ai, view);
    require(view.page == ui::Page::village && !ui::blocks_world(view));
}
