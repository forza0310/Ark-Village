#pragma once

// Page31 reads the runtime's initialized participant identities and canonical human statistics.
#include "ark/simulation/startup_world_runtime.hpp"
#include "layout.hpp"
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldCrewSummaryRow {
    int definition{};
    std::string name;
    int kills{}, downs{};
};
struct WorldCrewSummaryView {
    bool initialized{};
    std::vector<WorldCrewSummaryRow> rows;
};
struct WorldCrewSummaryLayout {
    Rectangle panel, rows, confirm;
};
WorldCrewSummaryView world_crew_summary_view(const simulation::StartupWorldRuntimeState &state,
                                             std::uint64_t page);
WorldCrewSummaryLayout world_crew_summary_layout(Extent extent);
int world_crew_summary_visible_rows(const WorldCrewSummaryLayout &layout);
void draw_world_crew_summary(const WorldCrewSummaryView &view, const WorldCrewSummaryLayout &layout,
                             const Skin &skin, int first_row, bool enabled);
} // namespace ark::desktop::ui
