#pragma once

// Read-only annual-page projection and hit testing. Award rules remain in the sole world owner.
#include "../common/layout.hpp"
#include "ark/simulation/presentation/steam_human_skin.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
bool world_page_automatic(const simulation::rules::WorldScriptPage &page);
bool world_page_regular_confirmation(const simulation::rules::WorldScriptPage &page);
struct WorldAwardRow {
    int definition{};
    std::string name;
    int contribution{};
    int effort{}, portrait_image{};
    int profession{}, sex{}, year_acquired{}, year_spent{}, residence{}, medals{};
};
struct WorldAwardView {
    bool initialized{};
    bool termination_pending{};
    int medals{};
    int frame{};
    std::optional<int> pending_human;
    std::string pending_name;
    std::vector<WorldAwardRow> rows; // Preserve the source exchange-sort order, including ties.
};
struct WorldAwardLayout {
    Rectangle panel, rows, terminate, grant, prompt, yes, no, previous, next;
    float row_height{};
};
WorldAwardView world_award_view(const simulation::StartupWorldRuntimeState &state,
                                std::uint64_t page);
WorldAwardLayout world_award_layout(Extent extent, bool termination_pending);
struct WorldAwardSelection {
    int selected{}, first_row{}, prompt{1}, tab{};
};
struct WorldAwardInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{};
    int wheel_rows{};
};
struct WorldAwardIntent {
    simulation::rules::WorldAwardAction action;
    int selection{}; // Source ranking index, never a definition identity.
};
// Requests only bind a source question; awards are committed by its separate confirmation.
std::optional<WorldAwardIntent> world_award_input(const WorldAwardView &view,
                                                  const WorldAwardLayout &layout,
                                                  WorldAwardSelection &selection,
                                                  const WorldAwardInput &input, bool blocked);
void draw_world_award(const WorldAwardView &view, const WorldAwardLayout &layout, const Skin &skin,
                      const WorldAwardSelection &selection, bool enabled);
// PC cursor feeds the frozen source DTO; this projection never sorts or awards the roster.
std::optional<simulation::SteamHumanOverviewPlan>
prepare_world_award(const WorldAwardView &, const WorldAwardSelection &, const Skin &);
} // namespace ark::desktop::ui
