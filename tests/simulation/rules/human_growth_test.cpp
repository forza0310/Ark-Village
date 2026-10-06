// Definition growth is separate from actor HP, delayed display and world-page ownership.
#include "ark/simulation/rules/human_growth.hpp"
#include "ark/simulation/rules/human_management.hpp"

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
void profession_management() {
    const std::vector<HumanManagementProfession> directory{{0, 1, 2, -1, 10, 1},
                                                           {1, 2, 2, 0, 20, 2},
                                                           {2, 1, 1, -1, 30, 3},
                                                           {3, 1, 0, 1, 0, 0},
                                                           {4, 0, -1, -1, 0, 0}};
    check(catalogue_human_professions(0, directory) == std::vector<int>{2, 1, 0},
          "job catalogue keeps source swap order, nonzero p and sex restriction");
    check(catalogue_human_professions(1, directory) == std::vector<int>{3, 2, 0},
          "unrestricted professions included for both sexes; locked absent");
    auto target = directory[0];
    check(
        check_human_profession_change(HumanProfessionEntry::catalogue61, 0, 0, 0, target).denial ==
                HumanManagementDenial::same_profession20 &&
            check_human_profession_change(HumanProfessionEntry::confirmation62, 0, 0, 0, target)
                    .denial == HumanManagementDenial::insufficient_medals27,
        "61 and62 have different denial order, not generic all-gates-at-once");
    check(check_human_profession_change(HumanProfessionEntry::confirmation62, 1, 1, 9, target)
                      .denial == HumanManagementDenial::insufficient_points12 &&
              check_human_profession_change(HumanProfessionEntry::confirmation62, 1, 1, 10, target)
                      .denial == HumanManagementDenial::none,
          "point and medal thresholds are inclusive; medals not a payment");
    auto i = fixture();
    i.professions[1].attribute_percent = {200, 300, 400, 500, 600, 700};
    i.definition.equipment[0] = std::array<int, 4>{3, 7, 11, 13};
    const auto preview = preview_human_profession_change(i.definition, i.professions, 1);
    check(preview && preview->before == std::array<int, 6>{22, 19, 6, 5, 7, 11} &&
              preview->after == std::array<int, 6>{44, 57, 24, 25, 42, 77} &&
              preview->difference == std::array<int, 6>{22, 38, 18, 20, 35, 66} &&
              i.definition.current_profession == 0 && i.definition.profession_levels[1] == 1,
          "62 preview is six attributes, not combat equipment bonuses or actual setter");
    HumanProfessionChangeInput change;
    change.definition = i.definition;
    change.professions = i.professions;
    change.target = {1, 1, 2, -1, 20, 2};
    change.village_points = 21;
    change.medals = 2;
    change.satisfaction = 99;
    change.pending_completion = -10;
    constexpr std::array<std::array<int, 2>, 4> requests{{{3, 3}, {0, 1}, {1, 0}, {1, 0}}};
    for (int count = 0; count < 4; ++count) {
        change.prior_changes = count;
        const auto result = prepare_human_profession_change(change);
        check(result.candidate && result.candidate->village_points == 1 &&
                  result.candidate->prior_changes == count + 1 &&
                  result.candidate->satisfaction_request == requests[count][0] &&
                  result.candidate->effort_request == requests[count][1] &&
                  result.candidate->reward.pending_completion == -10 + requests[count][0] &&
                  result.candidate->reward.definition.current_profession == 0 &&
                  result.candidate->reward.celebrations == 2 && change.village_points == 21,
              "R indexed reward plan uses old profession, keeps medals, no partial mutation");
    }
    change.prior_changes = std::numeric_limits<int>::max();
    check(prepare_human_profession_change(change).error == HumanGrowthError::numeric_overflow,
          "R overflow rejected without partially charging points");
    for (const int frame : {0, 54, 55, 56, 196, 197, 198}) {
        const auto waiting = human_profession_change_animation_plan(frame, false);
        const auto confirm = human_profession_change_animation_plan(frame, true);
        check(waiting && confirm && waiting->set_profession == (frame == 55) &&
                  confirm->set_profession == (frame == 55) && !waiting->finish &&
                  confirm->finish == (frame >= 197),
              "raw63 has exact midpoint setter and gated final confirm, no early fast forward");
    }
    const auto bonus = prepare_human_mastery_bonus(i.definition, i.professions, 0, 15);
    check(bonus.candidate && bonus.candidate->definition.extra[0] == 15 &&
              bonus.candidate->stats.attributes[0] == 37 &&
              bonus.candidate->stats.combat[0] == 40 && i.definition.extra[0] == 0,
          "mastery final choice appends extra before existing derive, leaves input untouched");
    const auto no_bonus = prepare_human_mastery_bonus(i.definition, i.professions, 10, 15);
    check(no_bonus.candidate && no_bonus.candidate->definition.extra == i.definition.extra,
          "v10 mastery spell presentation does not invent a six-attribute bonus");
    i.definition.extra[0] = std::numeric_limits<int>::max();
    check(!prepare_human_mastery_bonus(i.definition, i.professions, 0, 1).candidate &&
              !prepare_human_mastery_bonus(i.definition, i.professions, 6, 1).candidate,
          "mastery overflow and source-impossible six-array index reject safely");
}
void equipment_management() {
    HumanManagementEquipment target{7, 0, 1, 2, 3, 51, 0, 0, {10, 20, 30, 40}};
    check(human_equipment_gift_cost(target) == 76, "weapon gift truncates price*3/2");
    target.slot = 1;
    check(human_equipment_gift_cost(target) == 102, "armor gift costs twice price, not points");
    target.stock = 1;
    check(human_equipment_gift_cost(target) == -1, "shared inventory gift quote sentinel");
    const auto directory = std::vector<HumanManagementEquipment>{{0, 0, 1, 2, 0, 0, 0, 0, {}},
                                                                 {1, 0, 2, 2, 0, 0, 0, 0, {}},
                                                                 {2, 0, 1, 1, 0, 0, 0, 0, {}},
                                                                 {3, 1, 1, 0, 0, 0, 0, 0, {}},
                                                                 {4, 0, 0, -1, 0, 0, 0, 0, {}}};
    check(catalogue_human_equipment(0, directory) == std::vector<int>{2, 1, 0},
          "equipment catalogue independent of profession, locks and slots only");
    constexpr int likes[5][6]{{100, 0, 50, 50, 100, 50},
                              {0, 100, 50, 50, 100, 50},
                              {50, 50, 100, 0, 100, 50},
                              {50, 50, 0, 100, 100, 50},
                              {0, 0, 0, 0, 100, 50}};
    constexpr int upgrade_parts[]{0, 0, 26, 52, 80, 80};
    for (int job = 0; job < 5; ++job)
        for (int affinity = 0; affinity < 6; ++affinity)
            for (int delta = -1; delta <= 4; ++delta)
                check(human_equipment_gift_evaluation(4, 4 + delta, job, affinity) ==
                          upgrade_parts[delta + 1] + likes[job][affinity] / 5,
                      "all gift profiles and grade boundaries, integer then float truncation");
    check(human_gift_rewards(0) == std::array<int, 2>{2, 0} &&
              human_gift_rewards(20) == std::array<int, 2>{3, 0} &&
              human_gift_rewards(70) == std::array<int, 2>{6, 3} &&
              human_gift_rewards(100) == std::array<int, 2>{9, 6},
          "gift score independently maps satisfaction and effort, low clamp at20");
    auto f = fixture();
    f.definition.legacy_u = 9;
    f.definition.equipment[0] = std::array<int, 4>{5, 0, 0, 0};
    HumanEquipmentGiftInput gift;
    gift.definition = f.definition;
    gift.professions = f.professions;
    gift.equipment_ids[0] = 2;
    target.slot = 0;
    target.stock = 0;
    gift.target = target;
    gift.money = 200;
    gift.satisfaction = 99;
    gift.medals = 3;
    const auto result = prepare_human_equipment_gift(gift);
    check(result.candidate && result.candidate->money == 124 && result.candidate->stock == 0 &&
              result.candidate->cash_charge == 76 && result.candidate->evaluation == 100 &&
              result.candidate->dialogue_band == 2 &&
              result.candidate->equipment_ids == std::array<int, 4>{7, -1, -1, -1} &&
              result.candidate->equipment_cooldowns == std::array<int, 4>{6, 0, 0, 0},
          "confirm cash, grade reward, setter and acquisition lock6 compose");
    check(result.candidate->reward.satisfaction == 100 &&
              result.candidate->reward.pending_completion == 9 &&
              result.candidate->reward.definition.legacy_u == 15 &&
              result.candidate->reward.derived->combat == std::array<int, 4>{29, 20, 5, 7} &&
              result.candidate->final_stats.combat == std::array<int, 4>{34, 40, 35, 47} &&
              (*result.candidate->equipment_display)[2] == std::array<int, 4>{5, 20, 30, 40},
          "effort comparison retains old equipment, equipment comparison uses new effort");
    check(gift.money == 200 && gift.definition.legacy_u == 9 && gift.equipment_ids[0] == 2,
          "gift result cannot partially mutate owner input");
    gift.target.stock = 2;
    gift.money = -100;
    const auto inventory = prepare_human_equipment_gift(gift);
    check(inventory.candidate && inventory.candidate->stock == 1 &&
              inventory.candidate->money == -100 && inventory.candidate->cash_charge == 0,
          "inventory decrement is global and does not alter signed cash");
    gift.target.stock = 0;
    const auto signed_cash = prepare_human_equipment_gift(gift);
    check(signed_cash.candidate && signed_cash.candidate->money == -176,
          "parent confirmation does not invent a second affordability rejection or zero floor");
    gift.money = std::numeric_limits<std::int64_t>::min();
    check(prepare_human_equipment_gift(gift).error == HumanGrowthError::numeric_overflow,
          "cash numeric underflow fails before a partial reward or item update");
    gift.money = 200;
    gift.target.definition = 2;
    gift.target.grade = 0;
    gift.target.combat = *gift.definition.equipment[0];
    const auto repeated = prepare_human_equipment_gift(gift);
    check(repeated.candidate && !repeated.candidate->equipment_display &&
              repeated.candidate->equipment_cooldowns[0] == 6 &&
              repeated.candidate->reward.pending_completion == 3,
          "same equipment is legal, rewards and resets lock but no fabricated page68");
    gift.equipment_ids[1] = 0;
    check(!prepare_human_equipment_gift(gift).candidate,
          "equipment ID and stat projection mismatch rejects whole candidate");
    auto duplicate = directory;
    duplicate.push_back(directory[0]);
    check(!catalogue_human_equipment(0, duplicate) && !catalogue_human_professions(2, {}) &&
              !human_equipment_gift_evaluation(0, 0, 5, 0) && !human_gift_rewards(101) &&
              !human_profession_change_animation_plan(-1, true),
          "catalogue identity, profiles, scores and animation technical bounds");
}
} // namespace
int main() {
    try {
        stats();
        rewards();
        errors();
        immediate_rewards();
        profession_management();
        equipment_management();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
