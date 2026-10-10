#pragma once

// Startup and clear-score artwork consumes immutable application projections only.
#include "ark/app/session/world_system.hpp"
#include "ark/simulation/presentation/startup_title_actor_skin.hpp"
#include "../common/layout.hpp"
#include "../common/skin.hpp"

namespace ark::desktop::ui {
// Static preview of the actual definition-0 new character, not the unbound raw91 a.h.B job.
// The draft only overrides sex; drawing must not create actors or advance title/world random.
std::optional<simulation::StartupTitleActorSkin>
world_configuration_actor(const app::WorldNewGameDraft &draft);
void draw_world_records(const simulation::StartupSystemRecords &records, int page, const Skin &skin,
                        Rectangle panel);
void draw_world_configuration(const app::WorldNewGameDraft &draft, int selected,
                              const std::array<Rectangle, 4> &fields, Rectangle panel,
                              const Skin &skin);
void draw_world_clear(const app::WorldClearPage &clear, Extent extent, const Skin &skin);
} // namespace ark::desktop::ui
