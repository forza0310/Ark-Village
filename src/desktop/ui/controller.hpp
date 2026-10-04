#pragma once
#include "layout.hpp"
#include "state.hpp"
namespace ark::desktop::ui {
// Both native mouse input and coordinate-level regression tests use this controller.
void back(app::Game &game, State &view);
void click(app::Game &game, State &view, const Layout &layout, Vector2 point);
void confirm(app::Game &game, State &view);
// Mouse and Space share this guard; modal pages must not change explicit pause state.
void toggle_pause(app::Game &game, const State &view);
void scroll(State &view, int delta);
void turn_facility_page(const app::Game &game, State &view);
void scroll_sources(const app::Game &game, State &view, int delta);
} // namespace ark::desktop::ui
