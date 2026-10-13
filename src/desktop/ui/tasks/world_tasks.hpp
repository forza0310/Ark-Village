#pragma once

// Task pages read Owner cursor/scroll/frozen rows. Device state holds gestures only.
#include "../common/world_list_input.hpp"
#include "ark/simulation/presentation/steam_task_deadline_skin.hpp"
#include "ark/simulation/presentation/steam_task_result_skin.hpp"
#include "ark/simulation/presentation/steam_task_roster_skin.hpp"
#include "ark/simulation/presentation/steam_task_selection_skin.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
using WorldTaskFollowupPlan =
    std::variant<simulation::SteamTaskRosterPlan, simulation::SteamTaskResultPlan,
                 simulation::SteamTaskDeadlinePlan>;
struct WorldTaskRow {
    std::optional<std::uint64_t> task; // Real instance identity, independent from row/definition.
    std::optional<int> human;
    std::string name;
    std::optional<int> fee;
    bool add_member{};
};
struct WorldTaskRecruitmentActor {
    int human{}, profession{}, sex{}, image{};
};
struct WorldTaskView {
    std::uint64_t page{};
    int selected{}, first{}, tab{}, phase{};
    bool interactive{};
    int raw{};
    bool abort_question{};
    std::vector<std::string> details;
    bool initialized{}, animating{};
    std::string title, task_name;
    std::optional<std::uint64_t> task;
    std::optional<int> fee, prediction, deadline_grade;
    int counter{}, extent{}, recruited_count{};
    std::vector<std::string> recruitment_names; // Actual Y identities; no invented portraits.
    // Y.front() is the statistics portrait; the ordered plan separately draws all visible X
    // arrivals.
    std::optional<WorldTaskRecruitmentActor> recruitment_actor;
    std::vector<WorldTaskRow> rows;
    std::vector<std::array<int, 2>> reward_ids;
    std::optional<simulation::SteamTaskSelectionPlan> skin;
    std::optional<WorldTaskFollowupPlan> followup;
};
struct WorldTaskLayout {
    Extent viewport;
    Rectangle panel, body, rows, progress, cancel, confirm, continue_choice, stop_choice;
    Rectangle recruitment_name, recruitment_actor, inspect;
};
struct WorldTaskSelection {
    int prompt{1}; // Source task-abort questions default to No.
    WorldFacilityGesture gesture{};
};
struct WorldTaskInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{};
    int wheel_rows{};
    bool track{};
    std::optional<Vector2> pointer{};
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
    bool depart{}, inspect{}, focus_lost{};
};
struct WorldTaskIntent {
    simulation::StartupWorldTaskAction action{simulation::StartupWorldTaskAction::confirm};
    int selection{}; // Maintained pages use Owner row/choice; raw1 retains its answer.
    std::optional<simulation::StartupTaskPageInput> input{};
};
struct WorldTaskPaintedControls {
    WorldFacilityPaintedControls geometry;
    std::vector<std::uint64_t> tasks; // Preserve real64-bit instance identities and duplicates.
    std::optional<std::uint64_t> task;
    std::vector<int> humans; // Frozen rows, including the -1 add-member row and duplicates.
    std::vector<std::array<int, 2>> reward_ids;
    int tab{};
};
std::optional<WorldTaskIntent>
world_task_plan_input(const WorldTaskView &, const WorldTaskLayout &, WorldTaskSelection &,
                      const WorldTaskInput &, bool blocked,
                      const std::optional<WorldTaskPaintedControls> &);
bool world_task_page(const simulation::rules::WorldScriptPage &page);
// raw1/raw60 belong here only with an explicit runtime binding, never by number alone.
bool world_task_page(const simulation::StartupWorldRuntimeState &state,
                     const simulation::rules::WorldScriptPage &page);
WorldTaskView world_task_view(const simulation::StartupWorldRuntimeState &state,
                              const simulation::rules::WorldScriptPage &page);
WorldTaskLayout world_task_layout(Extent extent, int raw = 0);
void prepare_world_task(WorldTaskView &view, const simulation::StartupWorldRuntimeState &state,
                        const Skin &skin, bool scrolling = false);
void prepare_world_task_followup(WorldTaskView &, const simulation::StartupWorldRuntimeState &,
                                 const Skin &, bool scrolling);
std::optional<WorldTaskPaintedControls> draw_world_task_selection(const WorldTaskView &view,
                                                                  const WorldTaskLayout &layout,
                                                                  const Skin &skin, bool enabled,
                                                                  const std::string &feedback = {});
Rectangle world_task_menu_button(Extent extent);
// Shared ordered GPU execution; returns only the geometry actually painted this frame.
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskSelectionPlan &,
                                                  Vector2 origin, const Skin &);
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskRosterPlan &,
                                                  Vector2 origin, const Skin &);
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskResultPlan &,
                                                  Vector2 origin, const Skin &);
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskDeadlinePlan &,
                                                  Vector2 origin, const Skin &);
std::optional<WorldTaskPaintedControls> draw_world_task_followup(const WorldTaskView &,
                                                                 const WorldTaskLayout &,
                                                                 const Skin &, bool,
                                                                 const std::string &);
int world_task_visible_rows(const WorldTaskLayout &layout);
std::optional<WorldTaskIntent>
world_task_input(const WorldTaskView &view, const WorldTaskLayout &layout,
                 WorldTaskSelection &selection, const WorldTaskInput &input, bool blocked,
                 const std::optional<WorldTaskPaintedControls> &painted = {});
void draw_world_task(const WorldTaskView &view, const WorldTaskLayout &layout, const Skin &skin,
                     const WorldTaskSelection &selection, bool enabled,
                     const std::string &feedback = {});
// Related source consumers use normal page chrome but do not all accept confirmation.
bool world_task_related_confirmation(const simulation::StartupWorldRuntimeState &state,
                                     const simulation::rules::WorldScriptPage &page);
void draw_world_task_monster(const simulation::StartupWorldRuntimeState &state,
                             const simulation::rules::WorldScriptPage &page, Rectangle body,
                             const Skin &skin);
} // namespace ark::desktop::ui
