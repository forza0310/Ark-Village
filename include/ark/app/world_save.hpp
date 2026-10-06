#pragma once

// Ark's versioned manual file format keeps source world state and stable identities.
// Randomness, UI pages and one-shot outputs follow the published non-persistent policy.
#include "ark/simulation/startup_world_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ark::app {
enum class WorldSaveError {
    none,
    ineligible,
    too_large,
    malformed,
    unsupported_version,
    dataset_mismatch,
    invalid_world,
    io_error
};
struct WorldSaveMetadata {
    std::string village;
    int year{};
    int month{};
    int week{};
    int units{};
    std::int64_t funds{};
};
struct WorldSaveImage {
    WorldSaveMetadata metadata;
    std::vector<std::uint8_t> bytes;
};
struct WorldSaveCapture {
    WorldSaveError error{WorldSaveError::none};
    std::string message;
    std::optional<WorldSaveImage> image;
};
struct WorldSaveCandidate {
    WorldSaveError error{WorldSaveError::none};
    std::string message;
    std::optional<simulation::StartupWorldRuntimeState> state;
    std::optional<WorldSaveMetadata> metadata;
};
constexpr std::size_t world_save_max_bytes = 64U * 1024U * 1024U;

// Capture and decode never run a world update, consume input or advance the random stream.
WorldSaveCapture capture_world_save(const simulation::StartupWorldRuntimeState &state);
WorldSaveCandidate decode_world_save(const std::vector<std::uint8_t> &bytes);
bool world_save_eligible(const simulation::StartupWorldRuntimeState &state,
                         std::string *reason = nullptr);
// Validate decoded durable fields without using a live session or changing any state.
WorldSaveError validate_world_save_candidate(const simulation::StartupWorldRuntimeState &candidate,
                                             std::string &reason);

// Restore the Ark stable-main-scene policy and verify all durable cross-domain references.
// The caller supplies the current stream; the file itself has no random state.
WorldSaveError prepare_world_save_candidate(simulation::StartupWorldRuntimeState &candidate,
                                            const simulation::StartupWorldRuntimeState &current,
                                            std::string &reason);
const char *world_save_dataset();
} // namespace ark::app
