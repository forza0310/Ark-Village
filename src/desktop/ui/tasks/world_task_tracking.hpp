#pragma once
// Information plans read frozen task bindings. Only the Owner moves the camera or pays.
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include "world_tasks.hpp"
namespace ark::desktop::ui {
struct WorldTaskTrackingView {
    std::uint64_t scene{};
    int selected{}, count{}, fee{};
    bool ready{}, affordable{}, initialized{};
    std::string name;
    int raw{}, available_count{};
    std::vector<std::uint64_t> tasks;
    std::optional<simulation::SteamTaskSelectionPlan> skin;
};
struct WorldTaskTrackingLayout {
    Extent viewport;
    Vector2 origin;
    Rectangle cancel;
};
struct WorldTaskTrackingPaintedControls {
    WorldFacilityPaintedControls geometry;
    std::vector<std::uint64_t> tasks;
    int available_count{};
};
std::optional<WorldTaskTrackingView>
world_task_tracking_view(const simulation::StartupWorldRuntimeState &);
WorldTaskTrackingView world_task_information_view(const simulation::StartupWorldRuntimeState &,
                                                  const simulation::rules::WorldScriptPage &);
void prepare_world_task_tracking(WorldTaskTrackingView &,
                                 const simulation::StartupWorldRuntimeState &, const Skin &);
WorldTaskTrackingLayout world_task_tracking_layout(Extent);
std::optional<simulation::StartupTaskPageInput>
world_task_tracking_input(const WorldTaskTrackingView &, const WorldTaskTrackingLayout &,
                          const WorldTaskInput &, bool,
                          const std::optional<WorldTaskTrackingPaintedControls> & = {});
std::optional<WorldTaskTrackingPaintedControls>
draw_world_task_tracking(const WorldTaskTrackingView &, const WorldTaskTrackingLayout &,
                         const Skin &, bool);
} // namespace ark::desktop::ui
