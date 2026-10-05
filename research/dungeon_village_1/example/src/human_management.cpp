#include "dungeon_village_reference/human_management.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>

namespace dungeon_village_reference {
namespace {
bool fits(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
bool valid(const HumanManagementProfession &p) {
    return p.definition >= 0 && p.availability >= 0 && p.sex >= -1 && p.sex <= 1 &&
           p.village_points >= 0 && p.medals >= 0;
}
bool valid(const HumanManagementEquipment &e) {
    return e.definition >= 0 && e.slot >= 0 && e.slot < 4 && e.availability >= 0 && e.grade >= 0 &&
           e.price >= 0 && e.stock >= 0 && e.affinity >= 0 && e.affinity < 6;
}
template <class T> std::vector<int> ordered(std::vector<T> selected) {
    // 原目录由前项和逆序后项交换；等键可因第三项交换重排，不换成stable_sort。
    for (std::size_t a = 0; a + 1 < selected.size(); ++a)
        for (std::size_t b = selected.size(); b-- > a + 1;)
            if (selected[b].sort_order < selected[a].sort_order)
                std::swap(selected[a], selected[b]);
    std::vector<int> ids;
    for (const auto &item : selected)
        ids.push_back(item.definition);
    return ids;
}
int interpolate(int value, int begin, int end, int low, int high) {
    return low + (std::clamp(value, begin, end) - begin) * (high - low) / (end - begin);
}
} // namespace

std::optional<std::vector<int>>
catalogue_human_professions(int sex, const std::vector<HumanManagementProfession> &professions) {
    if (sex < 0 || sex > 1)
        return {};
    std::set<int> seen;
    std::vector<HumanManagementProfession> selected;
    for (const auto &p : professions) {
        if (!valid(p) || !seen.insert(p.definition).second)
            return {};
        if (p.availability != 0 && (p.sex == -1 || p.sex == sex))
            selected.push_back(p);
    }
    return ordered(std::move(selected));
}
HumanProfessionGate check_human_profession_change(HumanProfessionEntry entry,
                                                  int current_profession, int medals,
                                                  int village_points,
                                                  const HumanManagementProfession &target) {
    if (!valid(target) || current_profession < 0 || medals < 0 || village_points < 0 ||
        (entry != HumanProfessionEntry::catalogue61 &&
         entry != HumanProfessionEntry::confirmation62))
        return {HumanGrowthError::invalid_input, {}};
    if (entry == HumanProfessionEntry::confirmation62 && medals < target.medals)
        return {HumanGrowthError::none, HumanManagementDenial::insufficient_medals27};
    if (current_profession == target.definition)
        return {HumanGrowthError::none, HumanManagementDenial::same_profession20};
    if (village_points < target.village_points)
        return {HumanGrowthError::none, HumanManagementDenial::insufficient_points12};
    return {};
}
std::optional<HumanAttributeComparison>
preview_human_profession_change(const HumanDefinitionStatsInput &definition,
                                const std::vector<HumanProfessionRule> &professions,
                                int target_profession) {
    const auto before = derive_human_stats(definition, professions);
    auto preview = definition;
    preview.current_profession = target_profession;
    const auto after = derive_human_stats(preview, professions);
    if (!before.candidate || !after.candidate)
        return {};
    HumanAttributeComparison display{before.candidate->attributes, after.candidate->attributes, {}};
    for (std::size_t n = 0; n < display.difference.size(); ++n) {
        const auto delta = static_cast<std::int64_t>(display.after[n]) - display.before[n];
        if (!fits(delta))
            return {};
        display.difference[n] = static_cast<int>(delta);
    }
    return display;
}
HumanProfessionChangeResult prepare_human_profession_change(const HumanProfessionChangeInput &i) {
    const auto gate = check_human_profession_change(HumanProfessionEntry::confirmation62,
                                                    i.definition.current_profession, i.medals,
                                                    i.village_points, i.target);
    if (gate.error != HumanGrowthError::none || gate.denial != HumanManagementDenial::none)
        return {gate.error, gate.denial, {}};
    if (i.target.availability == 0 || i.prior_changes < 0 ||
        static_cast<std::size_t>(i.target.definition) >= i.professions.size())
        return {HumanGrowthError::invalid_input, {}, {}};
    if (i.prior_changes == std::numeric_limits<int>::max())
        return {HumanGrowthError::numeric_overflow, {}, {}};
    const auto stats = derive_human_stats(i.definition, i.professions);
    if (!stats.candidate)
        return {stats.error, {}, {}};
    const int satisfaction = i.prior_changes == 0 ? 3 : i.prior_changes == 1 ? 0 : 1;
    const int effort = i.prior_changes == 0 ? 3 : i.prior_changes == 1 ? 1 : 0;
    const auto reward = prepare_human_reward(i.definition, i.professions, i.satisfaction, i.medals,
                                             i.pending_completion, satisfaction, effort, false);
    if (!reward.candidate)
        return {reward.error, {}, {}};
    HumanProfessionChangeCandidate c;
    c.village_points = std::clamp(i.village_points - i.target.village_points, 0, 999);
    c.prior_changes = i.prior_changes + 1;
    c.target_profession = i.target.definition;
    c.satisfaction_request = satisfaction;
    c.effort_request = effort;
    c.reward = *reward.candidate;
    return {HumanGrowthError::none, {}, std::move(c)};
}
std::optional<HumanProfessionAnimationPlan> human_profession_change_animation_plan(int frame,
                                                                                   bool confirm) {
    if (frame < 0)
        return {};
    return HumanProfessionAnimationPlan{frame == 55, confirm && frame >= 197};
}
HumanMasteryBonusResult
prepare_human_mastery_bonus(const HumanDefinitionStatsInput &definition,
                            const std::vector<HumanProfessionRule> &professions, int attribute,
                            int amount) {
    auto candidate = definition;
    if (attribute < 0 || (attribute >= 6 && attribute < 10))
        return {HumanGrowthError::invalid_input, {}};
    if (attribute < 6) {
        const auto value = static_cast<std::int64_t>(candidate.extra[attribute]) + amount;
        if (!fits(value))
            return {HumanGrowthError::numeric_overflow, {}};
        candidate.extra[attribute] = static_cast<int>(value);
    }
    const auto result = derive_human_stats(candidate, professions);
    if (!result.candidate)
        return {result.error, {}};
    return {HumanGrowthError::none,
            HumanMasteryBonusCandidate{std::move(candidate), *result.candidate}};
}
std::optional<std::vector<int>>
catalogue_human_equipment(int slot, const std::vector<HumanManagementEquipment> &equipment) {
    if (slot < 0 || slot > 3)
        return {};
    std::set<std::pair<int, int>> seen;
    std::vector<HumanManagementEquipment> selected;
    for (const auto &e : equipment) {
        if (!valid(e) || !seen.insert({e.slot, e.definition}).second)
            return {};
        if (e.slot == slot && e.availability != 0)
            selected.push_back(e);
    }
    return ordered(std::move(selected));
}
std::optional<int> human_equipment_gift_cost(const HumanManagementEquipment &equipment) {
    if (!valid(equipment))
        return {};
    if (equipment.stock > 0)
        return -1;
    const auto scaled = static_cast<std::int64_t>(equipment.price) * (equipment.slot == 0 ? 3 : 2);
    if (!fits(scaled))
        return {};
    return static_cast<int>(equipment.slot == 0 ? scaled / 2 : scaled);
}
std::optional<int> human_equipment_gift_evaluation(int old_grade, int new_grade,
                                                   int profession_affinity,
                                                   int equipment_affinity) {
    if (old_grade < 0 || new_grade < 0 || profession_affinity < 0 || profession_affinity >= 5 ||
        equipment_affinity < 0 || equipment_affinity >= 6)
        return {};
    constexpr int affinity[5][6]{{100, 0, 50, 50, 100, 50},
                                 {0, 100, 50, 50, 100, 50},
                                 {50, 50, 100, 0, 100, 50},
                                 {50, 50, 0, 100, 100, 50},
                                 {0, 0, 0, 0, 100, 50}};
    const int difference = new_grade - old_grade;
    const int upgrade = interpolate(difference, 0, 3, 0, 100);
    return static_cast<int>(static_cast<float>(upgrade) * 0.8f +
                            static_cast<float>(affinity[profession_affinity][equipment_affinity]) *
                                0.2f);
}
std::optional<std::array<int, 2>> human_gift_rewards(int evaluation) {
    if (evaluation < 0 || evaluation > 100)
        return {};
    return std::array<int, 2>{interpolate(evaluation, 0, 100, 2, 9),
                              interpolate(evaluation, 20, 100, 0, 6)};
}
HumanEquipmentGiftResult prepare_human_equipment_gift(const HumanEquipmentGiftInput &i) {
    const auto cost = human_equipment_gift_cost(i.target);
    const auto evaluation = human_equipment_gift_evaluation(
        i.old_grade, i.target.grade, i.profession_affinity, i.target.affinity);
    if (!cost || !evaluation || i.target.availability == 0)
        return {HumanGrowthError::invalid_input, {}, {}};
    for (std::size_t slot = 0; slot < 4; ++slot)
        if (i.equipment_ids[slot] < -1 || i.equipment_cooldowns[slot] < 0 ||
            (i.equipment_ids[slot] == -1) != !i.definition.equipment[slot])
            return {HumanGrowthError::invalid_input, {}, {}};
    const auto amounts = *human_gift_rewards(*evaluation);
    const auto reward = prepare_human_reward(i.definition, i.professions, i.satisfaction, i.medals,
                                             i.pending_completion, amounts[0], amounts[1], false);
    if (!reward.candidate)
        return {reward.error, {}, {}};
    HumanEquipmentGiftCandidate c;
    c.cash_charge = *cost == -1 ? 0 : *cost;
    if (i.money < std::numeric_limits<std::int64_t>::min() + c.cash_charge)
        return {HumanGrowthError::numeric_overflow, {}, {}};
    c.money = i.money - c.cash_charge;
    c.stock = i.target.stock - (*cost == -1 ? 1 : 0);
    c.evaluation = *evaluation;
    c.dialogue_band = *evaluation < 25 ? 0 : *evaluation < 70 ? 1 : 2;
    c.reward = *reward.candidate;
    const auto before = derive_human_stats(c.reward.definition, i.professions);
    c.definition = c.reward.definition;
    c.definition.equipment[i.target.slot] = i.target.combat;
    const auto after = derive_human_stats(c.definition, i.professions);
    if (!before.candidate || !after.candidate)
        return {before.candidate ? after.error : before.error, {}, {}};
    c.final_stats = *after.candidate;
    c.equipment_ids = i.equipment_ids;
    c.equipment_ids[i.target.slot] = i.target.definition;
    c.equipment_cooldowns = i.equipment_cooldowns;
    c.equipment_cooldowns[i.target.slot] = 6;
    if (c.equipment_ids != i.equipment_ids) {
        std::array<std::array<int, 4>, 3> display{
            before.candidate->combat, after.candidate->combat, {}};
        for (std::size_t n = 0; n < 4; ++n) {
            const auto delta = static_cast<std::int64_t>(display[1][n]) - display[0][n];
            if (!fits(delta))
                return {HumanGrowthError::numeric_overflow, {}, {}};
            display[2][n] = static_cast<int>(delta);
        }
        c.equipment_display = display;
    }
    return {HumanGrowthError::none, {}, std::move(c)};
}
} // namespace dungeon_village_reference
