// Definition growth is separate from actor HP, delayed display and world-page ownership.
#include "ark/simulation/rules/human_growth.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
HumanGrowthInput fixture() {
    HumanGrowthInput i;
    i.definition.base = {22, 19, 6, 5, 7, 11};
    i.definition.profession_levels = {1, 1, 1};
    i.professions.resize(3);
    for (auto &p : i.professions) {
        p.attribute_percent.fill(100);
        p.maximum_growth = {27, 18, 9, 36, 45, 54};
    }
    return i;
}
void stats() {
    auto i = fixture();
    auto s = derive_human_stats(i.definition, i.professions);
    check(s.candidate && s.candidate->attributes == i.definition.base &&
              s.candidate->combat == std::array<int, 4>{22, 19, 5, 7},
          "base and combat mapping");
    for (int level = 1; level <= 10; ++level)
        for (int other = 1; other <= 10; ++other)
            for (int u = 0; u <= 119; ++u) {
                i.definition.profession_levels = {level, other, 1};
                i.definition.legacy_u = u;
                i.professions[0].maximum_growth.fill(7);
                i.professions[1].maximum_growth.fill(8);
                i.professions[0].attribute_percent.fill(123);
                s = derive_human_stats(i.definition, i.professions);
                const int growth = (level - 1) * 7 / 9 + (other - 1) * 8 / 9;
                const int expected = ((22 + growth) * 123 / 100) * (100 + u / 10 * 10) / 100;
                check(s.candidate && s.candidate->growth[0] == growth &&
                          s.candidate->attributes[0] == expected,
                      "each profession truncates before summing; u truncates before scaling");
            }
    i = fixture();
    i.definition.base.fill(15000);
    i.definition.legacy_u = 99;
    i.definition.equipment[0] = std::array<int, 4>{-10000, 5, -30, 10000};
    s = derive_human_stats(i.definition, i.professions);
    check(s.candidate && s.candidate->attributes[0] == 9999 &&
              s.candidate->combat == std::array<int, 4>{-1, 9999, 9969, 9999},
          "two caps precede equipment; final cap has no lower clamp");
    i = fixture();
    i.definition.current_profession = 1;
    i.definition.profession_levels = {10, 1, 9};
    i.definition.learned_spells = {false, false, true, false};
    i.definition.spell_professions = {2, 0, 1, 1};
    s = derive_human_stats(i.definition, i.professions);
    check(s.candidate &&
              s.candidate->available_spells == std::array<bool, 4>{false, true, true, true},
          "Q keeps original spell-profession order and duplicates, learned/current/mastered union");
}
void rewards() {
    for (int level = 1; level <= 10; ++level)
        for (int difficulty = 1; difficulty <= 5; ++difficulty) {
            const int base = 3 * level * level + 4 * level + 5;
            check(human_growth_threshold(level, difficulty) ==
                      base + (difficulty - 1) * base * 5 / 4,
                  "threshold integer interpolation");
        }
    auto i = fixture();
    i.experience = 1000;
    i.pending = {0, -2};
    i.notice_attributes[0] = {3, 9};
    auto c = prepare_human_growth(i);
    check(c.candidate && c.candidate->experience == 1000 && c.candidate->pending.counter == -2 &&
              c.candidate->levels_gained == 0 &&
              c.candidate->notice_attributes == i.notice_attributes,
          "N0 returns before counter and pre-existing XP, preserves ap");
    i.experience = 0;
    i.pending = {9, -2};
    c = prepare_human_growth(i);
    check(c.candidate && c.candidate->pending.counter == -1 && c.candidate->experience == 0,
          "negative delay waits after increment");
    i.pending = c.candidate->pending;
    c = prepare_human_growth(i);
    check(c.candidate && c.candidate->pending.counter == 0 && c.candidate->experience == 0,
          "O0 enters growth check but grants zero");
    for (int step = 1; step <= 9; ++step) {
        i.pending = c.candidate->pending;
        i.experience = c.candidate->experience;
        c = prepare_human_growth(i);
        check(c.candidate && c.candidate->experience == step && c.candidate->levels_gained == 0,
              "nine increment steps compose");
    }
    check(c.candidate->pending.amount == 0 && c.candidate->pending.counter == 0,
          "after ninth step clears reward");
    i = fixture();
    i.pending = {1, -1};
    i.experience = 12 + 25 + 44;
    i.effects.display = {{24, 10, 0, 5}, {13, 3, 0, 4}, {24, 0}};
    c = prepare_human_growth(i);
    check(c.candidate && c.candidate->levels_gained == 3 &&
              c.candidate->definition.profession_levels[0] == 4 && c.candidate->experience == 0 &&
              c.candidate->pending.amount == 0 && c.candidate->notice_pending,
          "one call can gain three levels and discards pending reward");
    check(c.candidate->notice_attributes ==
              std::array<std::array<int, 2>, 4>{{{3, 5}, {2, 4}, {0, 3}, {1, 2}}},
          "ap compares final level4 to3 rather than initial level1");
    check(c.candidate->effects.display == std::vector<ActorEffectRecord>{{13, 3, 0, 4}, {14, 0}} &&
              c.candidate->requests.size() == 2 &&
              c.candidate->requests[0].kind == HumanGrowthRequestKind::event109 &&
              c.candidate->requests[1].kind == HumanGrowthRequestKind::report_growth,
          "remove all24 then append14; event109 once before growth report, no HP request");
    i = fixture();
    i.definition.profession_levels[0] = 9;
    i.experience = *human_growth_threshold(9, 1);
    i.pending = {1, -1};
    i.effects.display = {{14, 12}, {24, 0}};
    c = prepare_human_growth(i);
    const std::vector<HumanGrowthRequestKind> expected{
        HumanGrowthRequestKind::event109, HumanGrowthRequestKind::report_growth,
        HumanGrowthRequestKind::page70,   HumanGrowthRequestKind::unlock_profession,
        HumanGrowthRequestKind::page94,   HumanGrowthRequestKind::event113,
        HumanGrowthRequestKind::notice33};
    check(c.candidate && c.candidate->requests.size() == expected.size(),
          "mastery requests present");
    for (std::size_t n = 0; n < expected.size(); ++n)
        check(c.candidate->requests[n].kind == expected[n], "mastery side effects ordered");
    check(c.candidate->effects.display == std::vector<ActorEffectRecord>{{14, 12}},
          "existing14 retained without duplicate or counter reset");
    i.definition.profession_levels[0] = 10;
    i.experience = 1000;
    c = prepare_human_growth(i);
    check(c.candidate && c.candidate->experience == 655 && c.candidate->levels_gained == 0 &&
              c.candidate->requests.empty() && c.candidate->pending.amount == 1,
          "already mastered deducts level10 threshold once, no level-up actions");
    i = fixture();
    i.definition.profession_levels[0] = 9;
    i.experience = *human_growth_threshold(9, 1);
    i.pending = {1, -1};
    i.event109_seen = i.event113_seen = i.professions[0].unlocked = true;
    c = prepare_human_growth(i);
    check(c.candidate && c.candidate->requests.size() == 2 &&
              c.candidate->requests[0].kind == HumanGrowthRequestKind::report_growth &&
              c.candidate->requests[1].kind == HumanGrowthRequestKind::page70,
          "existing events and job unlock guarded independently");
    i = fixture();
    i.pending = {9, 0};
    c = prepare_human_growth(i);
    i.pending = c.candidate->pending;
    i.experience = c.candidate->experience;
    c = prepare_human_growth(i);
    check(c.candidate && c.candidate->experience == 2 && c.candidate->pending.counter == 2,
          "two actors of one definition each advance reward, no per-world deduplication");
}
void errors() {
    const int max = std::numeric_limits<int>::max();
    auto i = fixture();
    i.definition.base.fill(max);
    i.definition.extra.fill(max);
    i.professions[0].attribute_percent.fill(max);
    check(derive_human_stats(i.definition, i.professions).error ==
              HumanGrowthError::numeric_overflow,
          "oversize sum rejected before multiplication, no int64 UB");
    i = fixture();
    i.definition.legacy_u = max;
    check(derive_human_stats(i.definition, i.professions).error ==
              HumanGrowthError::numeric_overflow,
          "quantized u multiplier overflow rejected");
    i = fixture();
    i.experience = max;
    i.pending = {9, 0};
    check(!prepare_human_growth(i).candidate && i.pending.counter == 0,
          "XP overflow has no partial update");
    i = fixture();
    i.definition.profession_levels[0] = 0;
    check(!prepare_human_growth(i).candidate, "malformed profession level rejected");
    i = fixture();
    i.definition.spell_professions = {3};
    check(!prepare_human_growth(i).candidate, "spell profession out of range rejected");
    i = fixture();
    i.pending = {1, -1};
    i.experience = 12;
    i.effects.display = {{27, 0}};
    check(!prepare_human_growth(i).candidate && i.definition.profession_levels[0] == 1,
          "bad effect rejects upgrade candidate without changing definition");
    check(!human_growth_threshold(0, 1) && !human_growth_threshold(1, 6), "threshold bounds");
}
void immediate_rewards() {
    auto i = fixture();
    for (const int u : {0, 9, 90, 95, 100}) {
        i.definition.legacy_u = u;
        const auto r = prepare_human_reward(i.definition, i.professions, 100, 2, 7, 10, 10, true);
        check(r.candidate && r.candidate->satisfaction == 100 &&
                  r.candidate->definition.legacy_u == std::min(u + 10, 100) &&
                  r.candidate->celebrations == 3 && r.candidate->pending_completion == 17 &&
                  r.candidate->reward_display[2] == std::array<int, 2>{10, 10} &&
                  r.candidate->effort_display.has_value() == (u < 100),
              "award capped value does not replace requested completion; effort threshold only");
    }
    i.definition.legacy_u = 0;
    const auto residence =
        prepare_human_reward(i.definition, i.professions, 99, 2, 0, 5, 15, false);
    check(residence.candidate && residence.candidate->celebrations == 2 &&
              residence.candidate->pending_completion == 5 &&
              residence.candidate->reward_display[1] == std::array<int, 2>{100, 15},
          "residence uses same calculation but does not increment E");
    for (const auto &input : {std::array<int, 3>{2, std::numeric_limits<int>::max(), 10},
                              std::array<int, 3>{std::numeric_limits<int>::max(), 0, 10}})
        check(!prepare_human_reward(i.definition, i.professions, 1, input[0], input[1], input[2],
                                    10, true)
                   .candidate,
              "celebration and completion overflow return no partial reward");
    const auto signed_pending =
        prepare_human_reward(i.definition, i.professions, 1, 0, -10, 5, 9, false);
    check(signed_pending.candidate && signed_pending.candidate->pending_completion == -5,
          "abort may leave signed pending popularity; reward adds request without invented zero "
          "floor");
    const auto subthreshold =
        prepare_human_reward(i.definition, i.professions, 1, 0, 0, 5, 9, false);
    check(subthreshold.candidate && !subthreshold.candidate->derived &&
              !subthreshold.candidate->effort_display,
          "below decade does not fabricate new stat cache or effort display");
}
} // namespace
int main() {
    try {
        stats();
        rewards();
        errors();
        immediate_rewards();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
