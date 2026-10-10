#pragma once

#include "ark/app/save/world_save.hpp"
#include <array>
#include <filesystem>

namespace ark::app {
struct WorldSaveSlotInfo {
    bool exists{};
    std::optional<WorldSaveMetadata> metadata;
    WorldSaveError error{WorldSaveError::none};
    std::string message;
};
struct WorldSaveFileResult {
    WorldSaveError error{WorldSaveError::none};
    std::string message;
};
// An explicit directory isolates tests from player files. No executable-relative fallback.
std::filesystem::path default_world_save_directory();
const char *world_save_error_text(WorldSaveError error);
std::filesystem::path world_save_slot_path(const std::filesystem::path &directory, int slot);
std::array<WorldSaveSlotInfo, 2> inspect_world_save_slots(const std::filesystem::path &directory);
WorldSaveCandidate read_world_save_slot(const std::filesystem::path &directory, int slot);
WorldSaveFileResult write_world_save_slot(const std::filesystem::path &directory, int slot,
                                          const WorldSaveImage &image);
} // namespace ark::app
