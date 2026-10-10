#pragma once

// Product application data joins the worker's world candidate. It owns no second world.
#include "ark/app/save/world_save_files.hpp"
#include "ark/simulation/application/startup_system_records.hpp"
#include "ark/simulation/village/startup_world_clear_score.hpp"

namespace ark::app {
struct WorldClearPage {
    simulation::StartupClearScoreRows rows;
    simulation::StartupClearScorePageState score;
    std::uint64_t page{};
    int animation_counter{}; // Whole presentation updates, separate from stage counter.
};
struct WorldSystemState {
    simulation::StartupSystemRecords records;
    std::optional<WorldClearPage> clear;
};
std::filesystem::path world_system_path(const std::filesystem::path &directory);
simulation::StartupSystemLoadResult read_world_system(const std::filesystem::path &directory);
std::string write_world_system(const std::filesystem::path &directory,
                               const simulation::StartupSystemRecords &records);
bool world_clear_page(const simulation::StartupWorldRuntimeState &state);
// Mutates private candidates only. Rule projection/phase advancement use maintained helpers.
std::string advance_world_clear(simulation::StartupWorldRuntimeState &candidate,
                                WorldSystemState &system, bool confirm);
// File replacement is the last fallible step before the caller installs both candidates.
// Empty directory is the explicit in-memory test/diagnostic mode, never an implicit player path.
std::string commit_world_system(const std::filesystem::path &directory,
                                const simulation::StartupWorldRuntimeState &before,
                                const simulation::StartupWorldRuntimeState &candidate,
                                WorldSystemState &system, bool clear_finished = false);
struct WorldNewGameDraft {
    std::string village{"口袋冒险村"};
    simulation::StartupWorldHumanProfile human{"冒险太郎", 0, false};
    int slot{};
};
void change_world_draft_sex(WorldNewGameDraft &draft, int sex);
// Start from the caller's real cold-start state; save only last-slot/system, never the world slot.
std::string start_world_draft(simulation::StartupWorldRuntimeState &initial,
                              WorldSystemState &system, const WorldNewGameDraft &draft,
                              const std::filesystem::path &directory);
} // namespace ark::app
