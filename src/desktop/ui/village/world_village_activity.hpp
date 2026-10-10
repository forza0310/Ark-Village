#pragma once

// Village activities are management pages, not autonomous actor activities or paid quests.
// The source owns selection, initialization, random portraits and the 52/53 effect boundary.
#include "ark/simulation/village/startup_world_village_activity.hpp"
#include "../common/layout.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldVillageActivityRow {
    int definition{}, points{}, kind{}, before{}, current{};
    std::string name;
    bool supported{}, fresh{};
};
struct WorldVillageActivityView {
    std::uint64_t page{};
    int raw{}, selection{}, first_visible{}, counter{}, points{}, quarter_slots{};
    bool initialized{}, can_confirm{}, can_cancel{};
    std::string title, name, detail, description, result_attribute, status;
    std::vector<WorldVillageActivityRow> rows;
    // The same definition can legitimately occupy both positions. Presence-zero placeholders
    // are hidden just as in the published view, without replacing or redrawing the source pool.
    std::array<std::optional<int>, 2> portrait_images;
};
struct WorldVillageActivityLayout {
    Rectangle panel, heading, rows, description, phase_label, status, feedback;
    Rectangle cancel, confirm;
    std::array<Rectangle, 2> choices, portraits;
    float row_height{};
};
struct WorldVillageActivityInput {
    std::optional<Vector2> click;
    bool up{}, down{}, enter{}, escape{};
    int wheel_rows{};
};
struct WorldVillageActivityIntent {
    simulation::StartupVillageActivityAction action;
    std::uint64_t page{};
    int selection{}; // Source list position, never a definition identity.
};
bool world_village_activity_page(const simulation::rules::WorldScriptPage &page);
WorldVillageActivityView
world_village_activity_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page);
WorldVillageActivityLayout world_village_activity_layout(Extent extent);
std::optional<WorldVillageActivityIntent>
world_village_activity_input(const WorldVillageActivityView &view,
                             const WorldVillageActivityLayout &layout,
                             const WorldVillageActivityInput &input, bool blocked);
void draw_world_village_activity(const WorldVillageActivityView &view,
                                 const WorldVillageActivityLayout &layout, const Skin &skin,
                                 bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
