#pragma once

// Page31 reads the runtime's initialized participant identities and canonical human statistics.
#include "../common/layout.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
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
WorldCrewSummaryView world_crew_summary_view(const simulation::StartupWorldRuntimeState &state,
                                             std::uint64_t page);
} // namespace ark::desktop::ui
