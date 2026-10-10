#pragma once

// Read-only visibility and diagnostic skip around the maintained automatic report consumer.
#include "ark/simulation/world/startup_world_runtime.hpp"

namespace ark::app {
// Visibility is not a modal/update gate. Source-admitted world updates advance the report
// automatically; an unrelated script page keeps its original update/input precedence.
bool world_report_visible(const simulation::StartupWorldRuntimeState &state);
// Compatibility name for existing inspections. Never use it to suspend the world or input.
bool world_report_waiting(const simulation::StartupWorldRuntimeState &state);
// Diagnostic-only source skip for a visible phase1/2/3. Normal UI needs no acknowledgement.
// Stale phase, explicit pause, missing report or non-world scene mode rejects without mutation.
bool acknowledge_world_report(simulation::StartupWorldRuntimeState &state, int expected_phase);
} // namespace ark::app
