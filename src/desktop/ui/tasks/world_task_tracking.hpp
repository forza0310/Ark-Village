#pragma once
// PC controls read frozen scene4/5 payloads; the Owner moves the camera and creates offers.
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include "world_tasks.hpp"
namespace ark::desktop::ui {
struct WorldTaskTrackingView {
    std::uint64_t scene{};
    int selected{}, count{}, fee{};
    bool ready{}, affordable{};
    std::string name;
};
struct WorldTaskTrackingLayout {
    Rectangle strip, previous, next, confirm, cancel;
};
std::optional<WorldTaskTrackingView>
world_task_tracking_view(const simulation::StartupWorldRuntimeState &);
WorldTaskTrackingLayout world_task_tracking_layout(Extent);
std::optional<simulation::StartupTaskPageInput>
world_task_tracking_input(const WorldTaskTrackingView &, const WorldTaskTrackingLayout &,
                          const WorldTaskInput &, bool);
void draw_world_task_tracking(const WorldTaskTrackingView &, const WorldTaskTrackingLayout &,
                              const Skin &, bool);
} // namespace ark::desktop::ui
