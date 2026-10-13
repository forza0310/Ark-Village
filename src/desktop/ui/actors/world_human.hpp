#pragma once

// Human management presents the current Owner. Source-owned selection and page readiness
// travel with snapshots; desktop input emits one source frame and never initializes a page.
#include "../common/layout.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "ark/simulation/presentation/steam_human_skin.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
inline constexpr const char *world_human_down_text = "倒下中";
struct WorldHumanRow {
    int identity{}, slot{}, cost{}, stock{}, medals{}, level{};
    std::string name;
    bool fresh{};
    std::array<int, 4> combat{};
    std::vector<simulation::StartupVisualDraw> icon;
};
struct WorldHumanView {
    int raw{}, phase{}, selection{}, counter{}, human{}, portrait_image{}, first_visible{},
        visible_rows{5};
    std::uint64_t page{};
    bool initialized{}, can_confirm{}, can_cancel{}, can_track{};
    std::string title, name, profession, target_profession, message;
    simulation::StartupHumanDetails details;
    std::optional<simulation::StartupHumanPresentationLive> live;
    // Input-only projections may omit the measured down text width; rendering must supply it.
    std::optional<simulation::SteamHumanDetailSkin> detail_skin;
    // Prepared from the same snapshot after actual font widths have been supplied.
    std::optional<simulation::SteamHumanOverviewPlan> detail_page_skin;
    std::array<std::string, 4> equipment_names;
    std::vector<WorldHumanRow> rows;
    std::optional<WorldHumanRow> choice;
    std::array<std::array<int, 6>, 3> attributes{};
    std::array<std::array<int, 4>, 3> combat{};
    int gift_score{}, mastery_attribute{}, mastery_value{};
};
struct WorldHumanLayout {
    Rectangle panel, body, rows, cancel, confirm, previous, next, professions, gifts, inspect,
        track, primary;
    std::array<Rectangle, 4> tabs;
    float row_height{};
};
struct WorldHumanInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{};
    int wheel_rows{};
    bool professions{}, gifts{}, inspect{};
    std::optional<Vector2> pointer;
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
};
// Last drawn child controls only; semantic tags reject stale mouse areas without owning a world.
struct WorldHumanPaintedControls {
    std::uint64_t page{};
    int raw{}, phase{}, selection{}, human{}, choice{-1}, first{}, count{};
    Extent extent;
    Rectangle back{}, other_back{};
    std::optional<Rectangle> primary, previous, next, inspect, scroll;
    std::array<std::optional<Rectangle>, 2> mastery_buttons; // Source Touch3, raw70 phase2 only.
    struct Row {
        int index{}, identity{};
        Rectangle rectangle{};
    };
    std::vector<Row> rows;
    int scroll_count{}, scroll_visible{};
};
// Short-lived pointer capture/marker only. Cursor and catalogue remain in the Owner.
struct WorldHumanGesture {
    std::uint64_t page{};
    int raw{}, phase{}, human{}, marked_identity{-1};
    bool scrolling{};
};
struct WorldHumanIntent {
    simulation::StartupHumanPageAction action;
    std::uint64_t page{};
    int selection{}; // Source list position, not a human/equipment definition identity.
    std::optional<simulation::StartupHumanCatalogueInput>
        input; // Composite source frame when present.
};
bool world_human_page(const simulation::rules::WorldScriptPage &page);
WorldHumanView world_human_view(const simulation::StartupWorldRuntimeState &state,
                                const simulation::rules::WorldScriptPage &page,
                                std::optional<int> down_text_width = {});
void prepare_world_human_detail(WorldHumanView &view,
                                const simulation::StartupWorldRuntimeState &state, const Skin &skin,
                                bool scrollbar_active = false);
WorldHumanLayout world_human_layout(Extent extent);
WorldHumanLayout world_human_detail_layout(const WorldHumanLayout &base);
bool world_human_source_child(const WorldHumanView &view);
WorldHumanLayout world_human_subwindow_layout(const WorldHumanLayout &base, int raw);
Vector2 world_human_skin_origin(const WorldHumanLayout &layout);
std::optional<WorldHumanPaintedControls> world_human_painted_controls(const WorldHumanView &view,
                                                                      const WorldHumanLayout &base,
                                                                      Extent extent,
                                                                      const Skin &skin);
Rectangle world_tracking_confirm(Extent extent);
std::optional<WorldHumanIntent>
world_human_input(const WorldHumanView &view, const WorldHumanLayout &layout,
                  const WorldHumanInput &input, bool blocked,
                  const std::optional<WorldHumanPaintedControls> &painted = {}, Extent extent = {},
                  WorldHumanGesture *gesture = nullptr);
void draw_world_human(const WorldHumanView &view, const WorldHumanLayout &layout, const Skin &skin,
                      bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
