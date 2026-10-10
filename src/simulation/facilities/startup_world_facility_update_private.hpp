#pragma once

#include "ark/simulation/facilities/rules/world_facility_update.hpp"

namespace ark::simulation {
struct StartupWorldRuntimeState;

struct StartupFacilityUpdateStep {
    rules::WorldFacilityUpdateError error{rules::WorldFacilityUpdateError::none};
    bool written{};
};

// Internal only: owner must be the frame's disposable, independent candidate.
// nullopt means no mutation and requests the unchanged full projection consumer.
// A failed value may have modified the disposable owner: discard the whole frame.
// error==none && !written preserves the legacy facility.write failure diagnostic.
std::optional<StartupFacilityUpdateStep>
try_consume_startup_world_facility_update(StartupWorldRuntimeState &owner, std::uint64_t identity,
                                          const rules::WorldScriptCatalog &catalog);

// Equivalent to the unchanged finish projection's round-trip normalizations when
// its only domain changes are the selected facility/progress/details. This helper
// lives beside the full route/finish writers and reuses their consistency checks.
bool normalize_startup_world_facility_writeback(StartupWorldRuntimeState &owner);
bool valid_startup_world_facility_projection(const StartupWorldRuntimeState &owner);
} // namespace ark::simulation
