#pragma once
#include "layout.hpp"
#include "skin.hpp"
#include "state.hpp"
namespace ark::desktop::ui {
void draw_pages(const app::Game &game, const State &view, const Layout &layout, const Skin &skin);
} // namespace ark::desktop::ui
