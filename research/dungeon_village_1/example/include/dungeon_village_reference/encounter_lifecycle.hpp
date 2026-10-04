#pragma once

// Event state advancement/reward plans. The owner runs group requests and allocates spawned IDs.
#include "dungeon_village_reference/encounter_ai.hpp"

namespace dungeon_village_reference {
struct EncounterRuntimeState {
    std::uint64_t id{};
    Position center;
    int state{};   // k:0 normal,1 retiring,2 retained,3 quest.
    int counter{}; // l, incremented before branch.
    int idle{};    // m, not reset by the event itself while monsters exist.
    int spawned{};
    int quota{};
    int reward{}; // f168f, not cash balance.
};
struct VictoryHuman {
    CharacterId id;
    int definition{};
    Position cell;
    bool inside_town{}; // ax cached before current round's d movement.
    int state{};
    int attack_count{}; // as, state10 only if>=2 for normal victory.
    std::uint32_t flags{};
    int recent_reward{}; // e.J.
    int recent_kills{};  // e.K, NOT a relationship level.
};
struct EncounterMonster {
    CharacterId id;
    std::optional<std::uint64_t> encounter;
};
struct QuestSpawnCell {
    Position cell;
    int state{};
    bool inside_town{};
};
struct EncounterQuest {
    std::uint32_t flags{}; // a.m.o, boss special flag2.
    int boss_definition{};
    int boss_reward{};                   // Already resolved k.d().
    std::vector<int> normal_definitions; // a.m.j source order.
    std::vector<int> participants;       // UserData.m definition IDs, duplicates kept.
    int completion_delta{};              // a.m.m, passed to UserData.e, NOT automatic money.
    int event_offset{};                  // a.m.f71f+145 only special flag2.
};
struct EncounterRandomTicket {
    int bound{};
    int value{};
};
enum class EncounterRequestKind {
    update_group,
    snapshot_influence,
    cancel_monster,
    refresh_map,
    clear_task,
    state10,
    reset_attack_count,
    reset_hp,
    expression5,
    task_victory_expression,
    reset_recent_reward_and_kills,
    reward_display,
    reward_accumulation,
    clear_boost2048,
    event,
    page30,
    page31,
    completion_delta,
    mark_task_complete,
    refresh_global,
    refresh_task_catalog,
    set_feature16
};
struct EncounterRequest {
    EncounterRequestKind kind{};
    std::optional<CharacterId> actor;
    int parameter{};
    int value{};
    int delay{};
};
struct EncounterSpawnCandidate {
    Position cell;
    int definition{};
    bool boss_instance{};
};
struct EncounterStepInput {
    EncounterRuntimeState state;
    bool town_overlap{}; // h.c(center.x-1..+1,center.y-1), only normal branch.
    bool group_exists{true};
    bool task_exists{true};
    std::vector<VictoryHuman> humans; // Original bl; stable IDs must uniquely resolve instances.
    std::vector<EncounterMonster> monsters; // Count every matching db, including death state3.
    EncounterQuest quest;
    std::vector<QuestSpawnCell> quest_cells; // Original eight cells, filtered state4/map boundary.
    std::vector<EncounterRandomTicket>
        tickets; // Semantic event draws; sub-consumer draws separate.
    bool event91_present{};
    bool event128_present{};
    bool event205_present{};
    bool feature16{};
    std::function<std::optional<int>(int)> draw{};
    // 随机子消费者在原req点同步执行，防止先规划所有语义票再执行姿态/表情导致反序。
    // 所有req均可影响下一抽（重复参与者的显示抑制等）；仅作用于外层私有候选。
    std::function<bool(const EncounterRequest &)> random_request{};
    // 外部脚本/任务消费者必须看到req时点已更新的k/l/f；旧req回调仅为兼容接口。
    std::function<bool(const EncounterRuntimeState &, const EncounterRequest &)>
        synchronous_request{};
};
struct EncounterStepCandidate {
    EncounterRuntimeState state;
    std::vector<VictoryHuman> humans;
    int nearby_humans{};
    int linked_monsters{};
    std::optional<EncounterSpawnCandidate> spawn;
    bool remove{};
    bool victory{};
    bool cancelled{};
    std::size_t consumed_tickets{};
    std::vector<EncounterRequest> requests; // In source order, after owner revalidation.
};
struct EncounterStepResult {
    EncounterAiError error{EncounterAiError::none};
    std::optional<EncounterStepCandidate> candidate;
};
// Prepare full k0/1/2/3 branch plan, including suppressed spawn draws and task completion order.
// Does not copy global fields into a new owner or execute UI/scripts/experience/cash effects.
EncounterStepResult prepare_encounter_step(const EncounterStepInput &input);

struct DelayedRewardState {
    int amount{};
    int counter{};
}; // e.N/O.
// Merge only the unconsumed pending portion of the earlier reward; replace delay with -newDelay.
std::optional<DelayedRewardState> prepare_delayed_reward(const DelayedRewardState &state,
                                                         int amount, int delay);
struct DelayedRewardStep {
    DelayedRewardState state;
    int increment{};
};
// Monetary/XP consumer is not assumed: original transfers these increments into definition L.
std::optional<DelayedRewardStep> advance_delayed_reward(const DelayedRewardState &state);
// Call AFTER consuming increment into definition L and evaluating profession growth. Any actual
// level-up discards the remaining pending N; otherwise counter9 ends the accumulation.
std::optional<DelayedRewardState> complete_delayed_reward_step(const DelayedRewardStep &step,
                                                               bool level_increased);
} // namespace dungeon_village_reference
