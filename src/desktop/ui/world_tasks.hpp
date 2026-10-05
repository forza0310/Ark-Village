#pragma once

// Task pages read the sole world owner. Selection/scroll are desktop state; actions are FIFO
// intents, never direct writes to participants, money, page counters or task progress.
#include "ark/simulation/startup_world_runtime.hpp"
#include "layout.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
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
    int raw{};
    bool initialized{}, animating{};
    std::string title, task_name;
    std::optional<std::uint64_t> task;
    std::optional<int> fee, prediction, deadline_grade;
    int counter{}, extent{}, recruited_count{};
    std::vector<std::string> recruitment_names; // Actual Y identities; no invented portraits.
    // Only Y.front() is visible in the published prototype; this is not an actor instance.
    std::optional<WorldTaskRecruitmentActor> recruitment_actor;
    std::vector<WorldTaskRow> rows;
};
struct WorldTaskLayout {
    Rectangle panel, body, rows, progress, cancel, confirm, continue_choice, stop_choice;
    Rectangle recruitment_name, recruitment_actor;
};
struct WorldTaskSelection {
    int selected{}, first_row{};
};
struct WorldTaskInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{};
    int wheel_rows{};
};
struct WorldTaskIntent {
    simulation::StartupWorldTaskAction action{simulation::StartupWorldTaskAction::confirm};
    int selection{}; // Row index for22, actual human definition for27, choice for33.
};
bool world_task_page(const simulation::rules::WorldScriptPage &page);
WorldTaskView world_task_view(const simulation::StartupWorldRuntimeState &state,
                              const simulation::rules::WorldScriptPage &page);
WorldTaskLayout world_task_layout(Extent extent);
Rectangle world_task_menu_button(Extent extent);
int world_task_visible_rows(const WorldTaskLayout &layout);
std::optional<WorldTaskIntent> world_task_input(const WorldTaskView &view,
                                                const WorldTaskLayout &layout,
                                                WorldTaskSelection &selection,
                                                const WorldTaskInput &input, bool blocked);
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
