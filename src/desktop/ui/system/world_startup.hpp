#pragma once

// Read-only title records presentation; configuration and scoring have separate modules.
#include "../common/skin.hpp"
#include "ark/simulation/application/startup_system_records.hpp"

namespace ark::desktop::ui {
void draw_world_records(const simulation::StartupSystemRecords &records, int page, const Skin &skin,
                        Rectangle panel);
} // namespace ark::desktop::ui
