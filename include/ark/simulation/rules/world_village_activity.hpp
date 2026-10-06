// 村办规则不持有页面、人物实例或随机流；所有候选由唯一世界 Owner 提交。
#pragma once

#include "ark/simulation/rules/human_growth.hpp"

namespace ark::simulation::rules {
// 村办a.c，不是人物自主活动类别，也不是冒险任务；原定义序由调用方保留。
struct WorldVillageActivityDefinition {
    int identity{};
    int status{};
    std::uint32_t flags{};
    int held{}; // 原m；季度重置不清此累计次数。
    int kind{};
    int attribute{};
    int points{};
    int magnitude{};
};
enum class WorldVillageActivityError { none, invalid_input, overflow, unsupported_kind };
enum class WorldVillageActivityDenial { none, no_quarter_slots50, insufficient_points12 };
std::optional<std::vector<int>>
catalogue_world_village_activities(const std::vector<WorldVillageActivityDefinition> &definitions);
struct WorldVillageActivityGate {
    WorldVillageActivityError error{WorldVillageActivityError::none};
    WorldVillageActivityDenial denial{WorldVillageActivityDenial::none};
};
// raw51只检查q再检查点数，不扣点、不增加F；raw52返回不重新做这两个玩法门槛。
WorldVillageActivityGate
check_world_village_activity(const WorldVillageActivityDefinition &activity, int quarter_slots,
                             int village_points);
struct WorldVillageActivityStart {
    WorldVillageActivityDefinition activity;
    int village_points{};
    int events_held{};
};
struct WorldVillageActivityStartResult {
    WorldVillageActivityError error{WorldVillageActivityError::none};
    std::optional<WorldVillageActivityStart> candidate;
};
// raw52开展候选：点数扣款、m++、bit4、F++；q和所有人物效果留给raw53完成。
WorldVillageActivityStartResult
prepare_world_village_activity_start(const WorldVillageActivityDefinition &activity,
                                     int village_points, int events_held);
struct WorldVillageActivityAnimation {
    bool sound5{};   // 更新恰70，只由更新消费者领取，不由确认重放声音。
    bool complete{}; // 确认且>=120；早确认不快进。
};
std::optional<WorldVillageActivityAnimation> world_village_activity_animation(int counter,
                                                                              bool confirm);
struct WorldVillageHumanEffect {
    HumanDefinitionStatsInput definition;
    int satisfaction{};
    int previous_value{}; // 原ao；54显示时再读取当前值，不冻结另一个最终值。
    std::optional<HumanDerivedStats> stats;
};
struct WorldVillageHumanEffectResult {
    WorldVillageActivityError error{WorldVillageActivityError::none};
    std::optional<WorldVillageHumanEffect> candidate;
};
// 独立消费一个已开放定义；资格、全局人气、两次抽签及结果页由唯一Owner原子组合。
// 类型0不是共用赠礼奖励，类型1按当前职业系数增加extra后重算，不能直接增加最终属性。
WorldVillageHumanEffectResult
prepare_world_village_human_effect(const WorldVillageActivityDefinition &activity,
                                   const HumanDefinitionStatsInput &definition,
                                   const std::vector<HumanProfessionRule> &professions,
                                   const HumanDerivedStats &cached, int satisfaction);
} // namespace ark::simulation::rules
