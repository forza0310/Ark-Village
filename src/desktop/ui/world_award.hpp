#pragma once

// Read-only annual-page projection and hit testing. Award rules remain in the sole world owner.
#include "ark/simulation/startup_world_runtime.hpp"
#include "layout.hpp"
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
};
struct WorldAwardView {
    bool initialized{};
    bool termination_pending{};
    int medals{};
    std::optional<int> pending_human;
    std::string pending_name;
    std::vector<WorldAwardRow> rows; // Preserve the source exchange-sort order, including ties.
};
struct WorldAwardLayout {
    Rectangle panel, rows, terminate, grant, prompt, yes, no;
};
WorldAwardView world_award_view(const simulation::StartupWorldRuntimeState &state,
                                std::uint64_t page);
WorldAwardLayout world_award_layout(Extent extent, bool termination_pending);
struct WorldAwardSelection {
    int selected{}, first_row{}, prompt{1};
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
} // namespace ark::desktop::ui
