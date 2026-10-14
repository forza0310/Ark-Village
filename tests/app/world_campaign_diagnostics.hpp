#pragma once

#include "ark/app/session/world_session.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace ark::test {
// Diagnostic files use the maintained full-state codec with explicit schema/data identities.
// They are isolated test artifacts, never player slots or partially committed candidates.
std::string describe_campaign_failure(const simulation::StartupWorldRuntimeResult &result);
void capture_campaign_failure(const std::filesystem::path &directory, const app::WorldState &before,
                              const simulation::StartupWorldRuntimeResult &result,
                              std::uint64_t successful_rounds);
void replay_campaign_failure(const std::filesystem::path &directory, bool expect_fixed = false);
void capture_campaign_observation(const std::filesystem::path &directory,
                                  const app::WorldState &state);
app::WorldState read_campaign_observation(const std::filesystem::path &directory);
void campaign_diagnostic_contract(const std::filesystem::path &build);
// A cold prefix can reveal missing progress without advancing actors, pages or random input.
void inspect_campaign_progress(const app::WorldState &state, std::ostream &out);
// Compare the one real successful update candidate with its input, without replaying decisions.
// Output records only boundary facts; it cannot reveal intervening random draws or priorities.
void inspect_campaign_activity(const app::WorldState &before, const app::WorldState &after,
                               std::uint64_t successful_rounds, std::ostream &out);
} // namespace ark::test
