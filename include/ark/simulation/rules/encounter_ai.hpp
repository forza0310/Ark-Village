#pragma once

#include "ark/simulation/rules/actor_ai.hpp"

#include <functional>

namespace ark::simulation::rules {
enum class EncounterAiError { none, invalid_input, missing_ticket, invalid_ticket, no_candidates };
struct BattleGroupMember {
    CharacterId id;
    std::uint32_t flags{};
};
struct BattleGroupState {
    int tick{};
    int duration{80};
    int cycle{};
    int alternating_side{};
    std::vector<BattleGroupMember> humans; // Duplicates are source append semantics, not deduped.
    std::vector<BattleGroupMember> monsters;
};
struct BattleSlotAssignment {
    CharacterId id;
    int slot{};
    std::optional<int> monster_posture;
};
struct BattleGroupStep {
    BattleGroupState state;
    std::vector<BattleSlotAssignment> assignments;
    std::vector<CharacterId> release_group_flag;
    bool disband{};
    std::size_t consumed_tickets{};
};
struct BattleGroupResult {
    EncounterAiError error{EncounterAiError::none};
    std::optional<BattleGroupStep> candidate;
};
// c.g.a(): increment first; half-cycle human slots, end-cycle pruning/monster slots/postures.
// Only >1 surviving monster draws tickets. Owner commits state and all assignments together.
BattleGroupResult
prepare_battle_group_step(const BattleGroupState &state,
                          const std::vector<int> &posture_tickets = {},
                          const std::function<std::optional<int>(int)> &draw = {});

struct MonsterChoiceDefinition {
    int id{};
    std::uint32_t flags{};
    int required_progress{}; // k.h.
    bool unlocked{};         // k.p==1.
    int growth_counter{};    // k.v, not total spawn count.
    bool introduced{};       // k.y.
};
struct MonsterChoiceCandidate {
    std::optional<int> unlock_definition;
    std::vector<int> eligible; // Reverse original array order, latest five non-bit4 definitions.
    int selected{};
    bool request_introduction{};
};
struct MonsterChoiceResult {
    EncounterAiError error{EncounterAiError::none};
    std::optional<MonsterChoiceCandidate> candidate;
};
// b.k(): first eligible adjacent unlock pair may stop without unlocking when v<8.
// Unlike the original invalid empty-vector draw, maintenance returns explicit no_candidates.
MonsterChoiceResult prepare_monster_choice(const std::vector<MonsterChoiceDefinition> &table,
                                           int progress, bool active_task,
                                           std::optional<int> ticket = std::nullopt,
                                           const std::function<std::optional<int>(int)> &draw = {});
struct MonsterCountInput {
    int year_index{};
    int month_index{};
    int nearby_people{};
    std::optional<int> count_ticket;  // 0..99, always drawn for ordinary type0 creation.
    std::optional<int> nearby_ticket; // 0..nearby_people-1 iff nearby_people>0.
    std::function<std::optional<int>(int)> draw{}; // 仅缺票且真实分支需要时请求动态上限。
};
struct MonsterCountCandidate {
    int count{};
    bool consumes_nearby_ticket{};
};
struct MonsterCountResult {
    EncounterAiError error{EncounterAiError::none};
    std::optional<MonsterCountCandidate> candidate;
};
MonsterCountResult prepare_normal_monster_count(const MonsterCountInput &input);
} // namespace ark::simulation::rules
