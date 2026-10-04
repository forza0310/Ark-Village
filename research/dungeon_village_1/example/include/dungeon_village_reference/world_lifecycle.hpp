#pragma once

#include "dungeon_village_reference/world_departure.hpp"
#include "dungeon_village_reference/world_facilities.hpp"
#include "dungeon_village_reference/world_misc_control.hpp"

namespace dungeon_village_reference {
struct WorldLifecycleInput {
    CharacterId actor;
    std::vector<WorldExpressionTicket> expressions; // 状态2按expression3、4实际调用顺序读取。
    WorldExpressionDraw expression_draw{};
};
enum class WorldLifecycleError {
    none,
    stale_actor,
    invalid_input,
    unsupported_state,
    missing_ticket,
    preparation_failed
};
struct WorldLifecycleCandidate {
    RescueWorldState state;
    std::size_t consumed_expressions{};
    std::size_t consumed_variants{};
    bool restored_baseline{};
    bool transitioned{};
    bool cleaned_up{};
};
struct WorldLifecycleResult {
    WorldLifecycleError error{WorldLifecycleError::none};
    std::optional<WorldLifecycleCandidate> candidate;
};
// 共同c前段/引用修复以后，只执行2/4/6/7/10/12/14/16/19；不再次感知或运行d。
// 6/7/19在原c没有独立分支，但仍保留原控制队列给d；其他状态明确交接。
// 状态16只复制真实bl成员的n/高度，不重复引用修复，也不刷新s/t/u。
WorldLifecycleResult prepare_world_lifecycle_c(const RescueWorldState &state,
                                               const WorldLifecycleInput &input);

struct WorldMonsterActInput {
    CharacterId actor;
    std::optional<WorldPathInput> path; // 只在T1/4要求；不用伪造P返回值替代实际消费者。
};
struct WorldMonsterActCandidate {
    RescueWorldState state;                 // 最终候选，包含P之后可能执行的r；外层只提交此世界。
    std::optional<WorldPathCandidate> path; // P原时点审计及真实任务/地图/表现请求，非持久所有者。
    bool called_battle_gate{};
    bool entered_battle{};
    bool cleaned_up{};
};
struct WorldMonsterActResult {
    WorldLifecycleError error{WorldLifecycleError::none};
    std::optional<WorldMonsterActCandidate> candidate;
};
// 状态17完整模式0..4：G→c1或实际P→按旧L500决定r，不传播P=true为删除。
// T0的M1500空分支保持无副作用；不创建T2/3/4入口，也不运行第二次共同c前段。
WorldMonsterActResult prepare_world_monster_act_c(const RescueWorldState &state,
                                                  const WorldMonsterActInput &input);
} // namespace dungeon_village_reference
