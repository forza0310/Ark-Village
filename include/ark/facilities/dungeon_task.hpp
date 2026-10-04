#pragma once

// Task-success consumer from research/rules/ai/DUNGEONS.md (2026-10-04 maintenance delivery).
// These are current-owner projections, not a second persistent task/monster world or startup data.
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace ark::facilities {
struct DungeonTaskDefinitionProgress {
    int kind{}; // Only researched kinds0/1 are admitted.
    std::uint32_t flags{};
    int completed{}; // Shared task definition t, not per-site/per-actor progress.
    int monster_definition{};
};
struct DungeonMonsterAvailability {
    int status{};          // p: successful unlock writes1 even if the old value was nonzero.
    bool pending_notice{}; // r: only old p0 sets it; other branches preserve it.
};
struct DungeonTaskSuccessState {
    int successes{};             // UserData.v, distinct from pending completion f215e.
    int ordinary_explorations{}; // w: kind0 without flag8 only.
    int exploration_stage{};     // x:0..5, not actor experience or level.
    std::array<std::array<int, 2>, 7> exploration_dates{}; // J[1], original year/month values.
    int task_pool_progress{}; // G, not money/popularity/village points.
    std::map<int, DungeonTaskDefinitionProgress> definitions;
    std::map<int, DungeonMonsterAvailability> monsters;
    std::vector<int> remaining_task_definitions; // Current bq BEFORE clear_active_task.
};
struct DungeonTaskSuccessCandidate {
    DungeonTaskSuccessState state;
    std::vector<int> threshold_notice_ids; // Messages29/30/31; not summary pages30/32.
};

// Consume record_task_success at its original phase2 position, before clearing the active task.
// Caller commits the whole candidate with its wider transaction. No UI, implicit task removal,
// cash/XP/popularity award or deduplication: repeated successful calls really increment again.
// Missing referenced definitions or overflow reject the whole private candidate.
std::optional<DungeonTaskSuccessCandidate>
prepare_dungeon_task_success(const DungeonTaskSuccessState &state, int definition, int raw_year,
                             int raw_month);
} // namespace ark::facilities
