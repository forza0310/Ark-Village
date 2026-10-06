#include "ark/app/world_report.hpp"

#include <algorithm>

namespace ark::app {
bool world_report_visible(const simulation::StartupWorldRuntimeState &state) {
    if (state.report_state <= 0)
        return false;
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &page) { return page.lifecycle != 4; });
    return top != state.scripts.pages.rend() &&
           top->kind == simulation::rules::WorldScriptPageKind::scene;
}
bool world_report_waiting(const simulation::StartupWorldRuntimeState &state) {
    return world_report_visible(state);
}
bool acknowledge_world_report(simulation::StartupWorldRuntimeState &state, int expected_phase) {
    if (expected_phase < 1 || expected_phase > 3 || state.report_state != expected_phase ||
        state.scene.framework_paused || !world_report_visible(state) ||
        (state.scene.scene_state != 0 && state.scene.scene_state != 2))
        return false;
    const auto adapter = simulation::startup_world_runtime_adapter();
    if (!adapter.report || !adapter.report_input)
        return false;
    auto input = adapter.report_input(state);
    if (!input || !input->admitted)
        return false;
    input->skip_display = true;
    const auto report =
        simulation::rules::prepare_world_month_report(adapter.report.read(state), *input);
    if (!report.candidate)
        return false;
    // The source owns transitions, display-counter reset and close-time points. Never drive
    // fake world rounds or re-run phase0 fee preparation to satisfy a diagnostic skip.
    auto candidate = state;
    if (!adapter.report.write(candidate, report.candidate->state))
        return false;
    state = std::move(candidate);
    return true;
}
} // namespace ark::app
