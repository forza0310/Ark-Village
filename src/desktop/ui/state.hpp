#pragma once

// Transient UI ownership only. Model state, unlocks, charges and event timing remain in Game.
#include "ark/app/game.hpp"
#include <raylib.h>
namespace ark::desktop::ui {
enum class Page { village, menu, roster, facility, definition };
struct State {
    Vector2 camera{};
    float zoom{1};
    Page page{Page::village};
    int tab{1}, row{}, scroll{}, menu_row{}, speed{1};
    int facility_page{}, source_scroll{};
    std::optional<world::Cell> selection;
    std::optional<facilities::InstanceId> detail;
    app::Error error{app::Error::none};
    int notice_frames{};
};
std::vector<const facilities::Definition *> catalog_items(int tab);
bool blocks_world(const State &view);
} // namespace ark::desktop::ui
