#pragma once

// Product interaction policy around the unchanged maintained monthly report consumer.
#include "ark/simulation/startup_world_runtime.hpp"

namespace ark::app {
// The report overlay waits only while it is actually exposed on the main scene. A script
// modal remains independently eligible for its own update/input until it uncovers the report.
bool world_report_waiting(const simulation::StartupWorldRuntimeState &state);
// Advance exactly the current visible phase1/2/3 using the source skip-display input. An old
// phase or missing report is rejected without mutation; closing consumes village points once.
bool acknowledge_world_report(simulation::StartupWorldRuntimeState &state, int expected_phase);
} // namespace ark::app
