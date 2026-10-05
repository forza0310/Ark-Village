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
// Portraits use e8f66d9 current-job/sex walk01 crop; HP capacity comes from shared growth.
// Building anchor and internal colors remain desktop adaptations. Retained occupancy follows
// the existing source retirement contract, even though the new source row helper is live-only.
std::vector<WorldRestRow> world_rest_rows(const simulation::StartupWorldRuntimeState &state,
                                          std::uint64_t facility);
} // namespace ark::desktop
