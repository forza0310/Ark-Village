#pragma once
#include "world_human.hpp"
namespace ark::desktop::ui {
// Steam60 fields and ordered Owner-backed portrait plan with existing PC navigation.
// Drawing consumes source counters; it never advances movement, HP, random or page state.
void draw_world_human_detail(const WorldHumanView &view, const WorldHumanLayout &layout,
                             const Skin &skin, bool enabled, const std::string &feedback);
} // namespace ark::desktop::ui
