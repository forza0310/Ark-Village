#pragma once

// Startup and clear-score artwork consumes immutable application projections only.
#include "ark/app/world_system.hpp"
#include "layout.hpp"
#include "skin.hpp"

namespace ark::desktop::ui {
void draw_world_records(const simulation::StartupSystemRecords &records, int page, const Skin &skin,
                        Rectangle panel);
void draw_world_configuration(const app::WorldNewGameDraft &draft, int selected,
                              const std::array<Rectangle, 4> &fields, Rectangle panel,
                              const Skin &skin);
void draw_world_clear(const app::WorldClearPage &clear, Extent extent, const Skin &skin);
} // namespace ark::desktop::ui
