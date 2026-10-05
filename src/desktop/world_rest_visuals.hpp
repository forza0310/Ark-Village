#pragma once

// Facility-owned rest presentation; hidden actor bodies do not suppress their inn status.
#include "ark/simulation/startup_world_runtime.hpp"
#include "world_overlay.hpp"

namespace ark::desktop {
struct WorldRestRow {
    simulation::rules::CharacterId actor;
    int counter{};
    OverlayPlan plan;
};
// PAGES.md: scan the first four occupants, then select bit32 without sorting/deduplicating.
// Plans use the first row's top-left anchor; subsequent visible rows rise by20 source pixels.
// Building anchor, exact internal placement/colors and omitted portraits remain desktop
// adaptations until the maintained source publishes their complete drawing bindings.
std::vector<WorldRestRow> world_rest_rows(const simulation::StartupWorldRuntimeState &state,
                                          std::uint64_t facility);
} // namespace ark::desktop
