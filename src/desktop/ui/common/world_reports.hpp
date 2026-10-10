#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"
#include "world_panels.hpp"

namespace ark::desktop::ui {
// Read-only page/overlay projections. No reward calculation, clock, or second task owner.
struct ReportPortrait {
    int definition{};
    int sprite{};
    int image{};
};
struct WorldVictoryView {
    bool initialized{};
    std::uint64_t task{};
    int definition{};
    std::string name;
    int experience{}, popularity{}, phase{}, counter{};
    std::vector<ReportPortrait> members;
};
WorldVictoryView world_victory_view(const simulation::StartupWorldRuntimeState &state,
                                    const simulation::rules::WorldScriptPage &page);
WorldPageLayout world_victory_layout(Extent extent);
void draw_world_victory(const WorldVictoryView &view, const WorldPageLayout &layout,
                        const Skin &skin, bool interactive);
struct WorldMonthView {
    int phase{}, defeats{}, points{}, income{}, expenses{}, balance{};
    float offset_x{};
    bool ellipsis{}, record{};
    std::vector<ReportPortrait> monsters;
    std::optional<int> human_image;
};
WorldMonthView world_month_view(const simulation::StartupWorldRuntimeState &state);
void draw_world_month(const WorldMonthView &view, const Skin &skin);
} // namespace ark::desktop::ui
