// Adapted from research 730e7ee example/include/dungeon_village_reference/human_growth.hpp;
// independent product build.
#pragma once

// Definition-shared growth/stats. Actor animation, HP and equipment appearance are separate.
#include "ark/people/actor_effects.hpp"
#include "ark/people/delayed_reward.hpp"

#include <array>
#include <vector>

namespace ark::people {
struct HumanProfessionRule {
    std::array<int, 6> maximum_growth{};    // h.j, contribution from this profession's level1..10.
    std::array<int, 6> attribute_percent{}; // h.k, current profession only.
    int difficulty{1};                      // h.x, threshold scale1..5.
    bool unlocked{};
};
struct HumanDefinitionStatsInput {
    int current_profession{}; // Array index, independent of an actor's cached ad.
    int legacy_u{}; // Quantized down to a multiple of10 BEFORE applying second multiplier.
    std::array<int, 6> base{};
    std::array<int, 6> extra{}; // Definition z, signed attributes from purchases/items.
    std::vector<int> profession_levels;
    std::array<std::optional<std::array<int, 4>>, 4> equipment;
    std::array<bool, 4> learned_spells{};
    std::vector<int> spell_professions; // Original h.D order; at most4, no sorting/deduplication.
};
struct HumanDerivedStats {
    std::array<int, 6> growth{};            // y.
    std::array<int, 6> attributes{};        // x.
    std::array<int, 4> combat{};            // w maps x0/1/3/4, then equipment, upper cap9999 only.
    std::array<bool, 4> available_spells{}; // Q = learned OR current/mastered h.D profession.
};
enum class HumanGrowthError { none, invalid_input, numeric_overflow };
struct HumanStatsResult {
    HumanGrowthError error{HumanGrowthError::none};
    std::optional<HumanDerivedStats> candidate;
};
HumanStatsResult derive_human_stats(const HumanDefinitionStatsInput &input,
                                    const std::vector<HumanProfessionRule> &professions);
std::optional<int> human_growth_threshold(int level, int difficulty);

enum class HumanGrowthRequestKind {
    event109,
    report_growth,
    page70,
    unlock_profession,
    page94,
    event113,
    notice33
};
struct HumanGrowthRequest {
    HumanGrowthRequestKind kind{};
    int profession{};
};
struct HumanGrowthInput {
    HumanDefinitionStatsInput definition;
    std::vector<HumanProfessionRule> professions;
    int experience{};           // L, NOT village currency.
    DelayedRewardState pending; // N/O.
    ActorEffectState effects;   // Only the actor whose d() invokes definition growth gets cd14.
    bool notice_pending{};      // P.
    bool event109_seen{};
    bool event113_seen{};
    std::array<std::array<int, 2>, 4> notice_attributes{{{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}}};
};
struct HumanGrowthCandidate {
    HumanDefinitionStatsInput definition;
    int experience{};
    DelayedRewardState pending;
    ActorEffectState effects;
    bool notice_pending{};
    int levels_gained{};
    std::optional<HumanDerivedStats> stats; // No recalculation when there is no real level gain.
    std::array<std::array<int, 2>, 4> notice_attributes; // ap, {-1,-1} for empty slots.
    std::vector<HumanGrowthRequest> requests;
};
struct HumanGrowthResult {
    HumanGrowthError error{HumanGrowthError::none};
    std::optional<HumanGrowthCandidate> candidate;
};
// One definition a(actor) call, after display/HP and before control. A shared definition can be
// advanced more than once in a world round if more than one actor invokes it; don't
// deduplicate. Retains existing XP after cost subtraction, but any actual upgrade discards
// pending N/O.
HumanGrowthResult prepare_human_growth(const HumanGrowthInput &input);
} // namespace ark::people
