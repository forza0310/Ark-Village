#pragma once

#include "ark/app/world_session.hpp"

namespace ark::app::detail {
// Explicit decisions share recoverable source rejection and bounded FIFO acknowledgements.
bool is_world_decision(WorldCommandKind kind);
bool is_decision_page(const simulation::rules::WorldScriptPage *page);
// Mutates only a private candidate. The session owns menu/report gates and commit/failure.
void apply_world_decision(WorldState &candidate, const WorldCommand &command,
                          WorldCommandResult &result);
} // namespace ark::app::detail
