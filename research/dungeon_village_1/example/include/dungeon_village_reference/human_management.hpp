#pragma once

#include "dungeon_village_reference/human_growth.hpp"

#include <cstdint>

namespace dungeon_village_reference {
// 目录和玩家管理字段独立于成长系数；definition 是原数组下标。
struct HumanManagementProfession {
    int definition{};
    int availability{}; // h.p；非零都进入目录。
    int sort_order{};   // h.d，不按名称或难度排序。
    int sex{-1};        // h.f；-1 不限。
    int village_points{};
    int medals{};
};
std::optional<std::vector<int>>
catalogue_human_professions(int sex, const std::vector<HumanManagementProfession> &professions);

enum class HumanProfessionEntry { catalogue61, confirmation62 };
enum class HumanManagementDenial {
    none,
    same_profession20,
    insufficient_points12,
    insufficient_medals27
};
struct HumanProfessionGate {
    HumanGrowthError error{HumanGrowthError::none};
    HumanManagementDenial denial{HumanManagementDenial::none};
};
HumanProfessionGate check_human_profession_change(HumanProfessionEntry entry,
                                                  int current_profession, int medals,
                                                  int village_points,
                                                  const HumanManagementProfession &target);

struct HumanAttributeComparison {
    std::array<int, 6> before{};
    std::array<int, 6> after{};
    std::array<int, 6> difference{};
};
// 只预览目标职业系数，不改变 t、人物 ad、经验或装备。
std::optional<HumanAttributeComparison>
preview_human_profession_change(const HumanDefinitionStatsInput &definition,
                                const std::vector<HumanProfessionRule> &professions,
                                int target_profession);

struct HumanProfessionChangeInput {
    HumanDefinitionStatsInput definition;
    std::vector<HumanProfessionRule> professions;
    HumanManagementProfession target;
    int village_points{};
    int medals{};        // E；只检查，不扣勋章。
    int prior_changes{}; // R[target]，不是总转职次数。
    int satisfaction{};
    int pending_completion{};
};
struct HumanProfessionChangeCandidate {
    int village_points{};
    int prior_changes{};
    int target_profession{};
    int satisfaction_request{};
    int effort_request{};
    HumanRewardCandidate reward; // 此刻仍使用旧职业系数。
};
struct HumanProfessionChangeResult {
    HumanGrowthError error{HumanGrowthError::none};
    HumanManagementDenial denial{HumanManagementDenial::none};
    std::optional<HumanProfessionChangeCandidate> candidate;
};
// raw62：扣点 -> 插63 -> 共用奖励/event58/可选67 -> R++ -> 关闭62。
// 返回候选不创建页面；Owner 必须将上述请求与此候选整事务提交。
HumanProfessionChangeResult
prepare_human_profession_change(const HumanProfessionChangeInput &input);

struct HumanProfessionAnimationPlan {
    bool set_profession{}; // 恰55：t 与所有 type0 同定义人物 ad；此刻不重算。
    bool finish{};         // 确认且>=197：重算/邻接刷新/关闭，随后清L、N；保留O。
};
std::optional<HumanProfessionAnimationPlan> human_profession_change_animation_plan(int frame,
                                                                                   bool confirm);

struct HumanMasteryBonusCandidate {
    HumanDefinitionStatsInput definition;
    HumanDerivedStats stats;
};
struct HumanMasteryBonusResult {
    HumanGrowthError error{HumanGrowthError::none};
    std::optional<HumanMasteryBonusCandidate> candidate;
};
// raw70 最终选择时才应用 h.v/h.w，不在达到满级或开页时应用；v>=10 无额外属性。
HumanMasteryBonusResult
prepare_human_mastery_bonus(const HumanDefinitionStatsInput &definition,
                            const std::vector<HumanProfessionRule> &professions, int attribute,
                            int amount);

struct HumanManagementEquipment {
    int definition{};
    int slot{}; // 0武器/1防具类型2/2其余防具/3饰品。
    int availability{};
    int sort_order{};
    int grade{};    // weapon.g / armor.f / accessory.f。
    int price{};    // weapon.m / armor.g / accessory.g。
    int affinity{}; // weapon.t / armor.i / accessory.i，索引0..5。
    int stock{};    // weapon.v / armor.k / accessory.k；共享库存，不是人物拥有量。
    std::array<int, 4> combat{};
};
std::optional<std::vector<int>>
catalogue_human_equipment(int slot, const std::vector<HumanManagementEquipment> &equipment);
std::optional<int> human_equipment_gift_cost(const HumanManagementEquipment &equipment);
std::optional<int> human_equipment_gift_evaluation(int old_grade, int new_grade,
                                                   int profession_affinity, int equipment_affinity);
std::optional<std::array<int, 2>> human_gift_rewards(int evaluation);

struct HumanEquipmentGiftInput {
    HumanDefinitionStatsInput definition;
    std::vector<HumanProfessionRule> professions;
    std::array<int, 4> equipment_ids{{-1, -1, -1, -1}};
    std::array<int, 4> equipment_cooldowns{};
    HumanManagementEquipment target;
    int old_grade{};
    int profession_affinity{}; // h.h 是礼物评价喜好，不是装备准入限制。
    std::int64_t money{};
    int satisfaction{};
    int medals{};
    int pending_completion{};
};
struct HumanEquipmentGiftCandidate {
    std::int64_t money{};
    int stock{};
    int evaluation{};
    int dialogue_band{}; // <25 / <70 / >=70；Owner 从该档两条文本抽一次。
    int cash_charge{};   // 库存礼物是0；不要把quote=-1当作负收费。
    std::array<int, 4> equipment_ids{};
    std::array<int, 4> equipment_cooldowns{};
    HumanRewardCandidate reward;          // 共用奖励在装备setter之前执行。
    HumanDefinitionStatsInput definition; // setter后的定义；保留reward的中间快照。
    HumanDerivedStats final_stats;
    std::optional<std::array<std::array<int, 4>, 3>> equipment_display;
};
struct HumanEquipmentGiftResult {
    HumanGrowthError error{HumanGrowthError::none};
    HumanManagementDenial denial{HumanManagementDenial::none};
    std::optional<HumanEquipmentGiftCandidate> candidate;
};
// raw64 接到子65确认才调用：现金/库存 -> 共用奖励 -> setter及A[slot]=6 ->
// 66/可选68/可选67 -> 首个同定义actor.ae同步。取消不调用；同装备仍奖励、锁6。
// 预算检查只发生于打开65前，确认消费者不再添加二次现金拒绝，原余额可为负。
HumanEquipmentGiftResult prepare_human_equipment_gift(const HumanEquipmentGiftInput &input);
} // namespace dungeon_village_reference
