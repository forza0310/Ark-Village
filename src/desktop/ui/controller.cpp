// Navigation does not mutate domain state except through Game's validated commands.
#include "controller.hpp"
#include <algorithm>
namespace ark::desktop::ui {
std::vector<const facilities::Definition *> catalog_items(int tab) {
    std::vector<const facilities::Definition *> result;
    for (const auto &item : app::startup_data().definitions)
        if (item.tab == tab)
            result.push_back(&item);
    return result;
}
bool blocks_world(const State &view) { return view.page != Page::village; }
void back(app::Game &game, State &view) {
    view.error = app::Error::none;
    if (view.page == Page::definition) {
        view.page = Page::village; // Catalog is still the underlying model mode.
    } else if (view.page == Page::roster) {
        view.page = Page::menu;
    } else if (view.page == Page::facility || view.page == Page::menu) {
        view.page = Page::village;
        view.detail.reset();
    } else if (game.state().mode == app::Mode::placement) {
        game.cancel();
        game.open_catalog();
        view.selection.reset();
    } else if (game.state().mode == app::Mode::catalog) {
        game.cancel();
        view.page = Page::menu;
    }
}
void confirm(app::Game &game, State &view) {
    if (game.state().mode == app::Mode::catalog && view.page == Page::village) {
        const auto items = catalog_items(view.tab);
        if (view.row >= 0 && view.row < static_cast<int>(items.size())) {
            view.error = game.select(items[view.row]->id);
            // Start at the source village's buildable region, not at an arbitrary screen corner.
            if (view.error == app::Error::none) {
                const auto bounds = app::startup_data().build_bounds;
                view.selection = world::Cell{bounds.min_x + 1, bounds.min_y + 1};
                bool found = false;
                for (int y = bounds.min_y + 1; y < bounds.max_y && !found; ++y)
                    for (int x = bounds.min_x + 1; x < bounds.max_x; ++x)
                        if (game.preview({x, y}) == app::Error::none) {
                            view.selection = world::Cell{x, y};
                            found = true;
                            break;
                        }
            }
        }
    } else if (game.state().mode == app::Mode::placement && view.selection) {
        view.error = game.confirm(*view.selection);
        if (view.error == app::Error::none) {
            view.notice_frames = 90;
            view.selection.reset();
        }
    } else if (game.state().mode == app::Mode::tutorial) {
        view.error = game.acknowledge_talk();
    }
}
void scroll(State &view, int delta) {
    const auto count = static_cast<int>(catalog_items(view.tab).size());
    if (!count)
        return;
    view.scroll = std::clamp(view.scroll + delta, 0, std::max(0, count - 4));
    view.row = std::clamp(view.row, view.scroll, std::min(count - 1, view.scroll + 3));
}
void click(app::Game &game, State &view, const Layout &layout, Vector2 point) {
    const auto hit = [&](Rectangle box) { return CheckCollisionPointRec(point, box); };
    if (hit(layout.right_button)) {
        if (game.state().mode == app::Mode::normal && view.page == Page::village)
            view.page = Page::menu;
        else if (game.state().mode != app::Mode::tutorial &&
                 game.state().mode != app::Mode::research_boundary)
            back(game, view);
        return;
    }
    if (view.page == Page::menu) {
        for (int i = 0; i < 5; ++i)
            if (hit(layout.menu_rows[i])) {
                view.menu_row = i;
                if (i == 0) {
                    view.error = game.open_catalog();
                    if (view.error == app::Error::none)
                        view.page = Page::village;
                } else if (i == 3) {
                    view.page = Page::roster;
                }
            }
        return;
    }
    if (view.page == Page::roster || view.page == Page::facility || view.page == Page::definition)
        return;
    if (game.state().mode == app::Mode::catalog) {
        for (int direction : {-1, 1}) {
            const float x =
                direction < 0 ? layout.catalog.x - 8 : layout.catalog.x + layout.catalog.width;
            if (hit({x, layout.catalog.y + 4, 8, 12})) {
                view.tab = (view.tab + direction + 3) % 3;
                view.row = view.scroll = 0;
                view.error = app::Error::none;
                return;
            }
        }
        if (hit(layout.left_button)) {
            view.page = Page::definition;
            return;
        }
        for (int i = 0; i < 3; ++i)
            if (hit(layout.tabs[i])) {
                view.tab = i;
                view.row = view.scroll = 0;
                view.error = app::Error::none;
                return;
            }
        const auto count = static_cast<int>(catalog_items(view.tab).size());
        for (int i = 0; i < 4; ++i)
            if (hit(layout.rows[i]) && view.scroll + i < count) {
                const int row = view.scroll + i;
                if (view.row == row)
                    confirm(game, view);
                else {
                    view.row = row;
                    view.error = app::Error::none;
                }
                return;
            }
    } else if (game.state().mode == app::Mode::placement) {
        if (hit(layout.left_button)) {
            view.error = game.rotate();
            return;
        }
        if (view.selection) {
            const auto p = tile_center(*view.selection, view.camera, layout.extent);
            const auto arrows = layout.arrows(p);
            const world::Cell steps[] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
            for (int i = 0; i < 4; ++i)
                if (hit(arrows[i])) {
                    view.selection->x += steps[i].x;
                    view.selection->y += steps[i].y;
                    view.error = game.preview(*view.selection);
                    return;
                }
        }
        if (hit(layout.scene)) {
            const auto cell = pick(point, view.camera, app::startup_data().map, layout.extent);
            if (cell) {
                if (view.selection == cell)
                    confirm(game, view);
                else {
                    view.selection = cell;
                    view.error = game.preview(*cell);
                }
            }
        }
    } else if (game.state().mode == app::Mode::tutorial) {
        if (hit(layout.dialogue))
            confirm(game, view);
    } else if (game.state().mode == app::Mode::normal && hit(layout.scene)) {
        const auto cell = pick(point, view.camera, app::startup_data().map, layout.extent);
        if (cell && (view.detail = game.facility_at(*cell)))
            view.page = Page::facility;
    }
}
} // namespace ark::desktop::ui
