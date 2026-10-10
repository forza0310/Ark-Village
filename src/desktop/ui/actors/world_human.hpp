#pragma once

// Human management presents the current Owner. Source-owned selection and page readiness
// travel with snapshots; desktop input emits one action and never initializes a page.
#include "../common/layout.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldHumanRow {
    int identity{}, slot{}, cost{}, stock{}, medals{}, level{};
    std::string name;
    bool fresh{};
    std::array<int, 4> combat{};
    std::vector<simulation::StartupVisualDraw> icon;
};
struct WorldHumanView {
    int raw{}, phase{}, selection{}, counter{}, human{}, portrait_image{};
    std::uint64_t page{};
    bool initialized{}, can_confirm{}, can_cancel{}, can_track{};
    std::string title, name, profession, target_profession, message;
    simulation::StartupHumanDetails details;
    std::optional<simulation::StartupHumanPresentationLive> live;
    std::array<std::string, 4> equipment_names;
    std::array<int, 4> equipment_icons{{-1, -1, -1, -1}};
    std::array<std::array<int, 4>, 4> equipment_values{};
    std::vector<WorldHumanRow> rows;
    std::optional<WorldHumanRow> choice;
    std::array<std::array<int, 6>, 3> attributes{};
    std::array<std::array<int, 4>, 3> combat{};
    int gift_score{}, mastery_attribute{}, mastery_value{};
};
struct WorldHumanLayout {
    Rectangle panel, body, rows, cancel, confirm, previous, next, professions, gifts, inspect,
        track;
    std::array<Rectangle, 4> tabs;
    std::array<Rectangle, 5> gift_tabs;
    float row_height{};
};
struct WorldHumanInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{};
    int wheel_rows{};
    bool professions{}, gifts{}, inspect{};
};
struct WorldHumanIntent {
    simulation::StartupHumanPageAction action;
    std::uint64_t page{};
    int selection{}; // Source list position, not a human/equipment definition identity.
};
bool world_human_page(const simulation::rules::WorldScriptPage &page);
WorldHumanView world_human_view(const simulation::StartupWorldRuntimeState &state,
                                const simulation::rules::WorldScriptPage &page);
WorldHumanLayout world_human_layout(Extent extent);
WorldHumanLayout world_human_detail_layout(const WorldHumanLayout &base);
Rectangle world_tracking_confirm(Extent extent);
int world_human_first_row(const WorldHumanView &view);
std::optional<WorldHumanIntent> world_human_input(const WorldHumanView &view,
                                                  const WorldHumanLayout &layout,
                                                  const WorldHumanInput &input, bool blocked);
void draw_world_human(const WorldHumanView &view, const WorldHumanLayout &layout, const Skin &skin,
                      bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
