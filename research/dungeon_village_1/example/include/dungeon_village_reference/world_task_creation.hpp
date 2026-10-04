#pragma once

#include "dungeon_village_reference/world_dungeon_finish.hpp"
#include "dungeon_village_reference/world_map_refresh.hpp"
#include "dungeon_village_reference/world_random.hpp"

namespace dungeon_village_reference {
struct TaskCreationDefinition {
    int identity{}; // a.m.n，目录按bD原序，不按map key重排。
    int kind{};
    int exploration_stage{}; // f71f，普通kind1同样要求等于当前x。
    std::uint32_t flags{};
    int completions{};
    int monster{};
    std::vector<int> facilities; // h，仍按原表顺序抽选。
    int minimum_rewards{};
    int maximum_rewards{};
    int pending_completion_value{}; // a.m.m，第14列；累加f215e后经脚本22转延迟人气，不是金币。
};
struct TaskCreationMonster {
    int victories{};          // v，G>=500分支确实会按首个flag2怪物的值上限钳制。
    bool replay_available{};  // y，不等于p解锁状态。
    int challenge_strength{}; // m，挑战条目最后字段。
};
struct TaskCreationHuman {
    int status{};    // bv.p，所有非零定义参与知识均值，不只实时人物名单。
    int knowledge{}; // bv.x[5]
};
struct TaskCreationReward {
    int kind{}; // 0物品、1武器、2防具、3饰品。
    int identity{};
    int difficulty{}; // g.j / p.g / b.f / a.f。
    std::uint32_t flags{};
    int status{}; // 装备仅排除恰1，物品不检查p。
};
struct TaskCreationFacilityDefinition {
    WorldMapDefinition map;
    int kind{};     // o.e
    int category{}; // o.f，kind0使用的真实定义应为5。
    int detail{};
    int price{};
    int wait{};
    LevelEndpoints upgrade_uses;
};
// 唯一Owner的临时值投影；地图只在finish.dungeon.world.map保存，地图元数据不另持地图。
struct WorldTaskCreationState {
    DungeonFinishState finish;
    WorldRandomStream random;
    std::vector<TaskCreationDefinition> definitions;
    std::map<int, TaskCreationMonster> monsters;
    std::vector<TaskCreationHuman> humans;
    std::vector<TaskCreationReward> rewards;            // 按by→bt→bz→bA原序输入。
    std::array<std::vector<int>, 6> challenge_monsters; // a.k.O，保留真实原序。
    std::map<int, TaskCreationFacilityDefinition> facility_definitions;
    std::vector<std::uint64_t> facility_order;          // 原g。
    std::map<std::uint64_t, int> facility_original_ids; // 原f206b，可为0；稳定ID独立。
    std::map<std::uint64_t, int> facility_ordinals;     // 同定义f207c，从0首个空号。
    std::map<std::uint64_t, int> facility_difficulties; // Tenant.l，工厂末端设置。
    std::map<std::uint64_t, int> facility_residents;    // Tenant.t，新建为-1，不是住宅所有权。
    std::map<std::uint64_t, int> task_original_ids;     // 原c.k.b，不能用定义ID替代。
    int task_sequence{};                                // static c.k.m。
    std::array<Position, 2> generation_bounds;          // h.m[0]，左下/右上，与town不同。
    std::vector<int> base_variants;
    int special_ground_definition{-1};
    int fence_level{-1};
    std::vector<std::array<Position, 2>> fence_levels;
    std::vector<std::array<bool, 2>> road_patches;
    bool refresh_pending{};
    int rank{};                   // UserData.k
    int special_selection_mode{}; // a.m.v，恰1且kind1使用u[w]。
    std::vector<int> special_selection;
    int special_selection_index{};
    std::vector<int> replay_order;           // a.m.x，逆序筛最多6个。
    int first_reward_weapon{};               // a.p.J.n，第一次成功任务的末条强制奖励。
    std::uint64_t next_facility_identity{1}; // 维护稳定ID单调分配；原rawID仍允许复用。
    std::uint64_t next_task_identity{1};     // 页面/退休引用不能被后续对象复用。
};
enum class TaskCreationError {
    none,
    invalid_input,
    missing_definition,
    empty_pool,
    random_failed,
    map_failed,
    numeric_overflow
};
struct TaskCreationCandidate {
    WorldTaskCreationState state;
    std::optional<std::uint64_t> created_task; // null是20个地点都拒绝，不是接口缺失。
    std::optional<int> selected_definition;
    int difficulty{};
    std::vector<int> random_bounds; // 实际消费顺序，含最终被97覆盖的draw10。
    std::vector<WorldMapRefreshStep> map_steps;
};
struct TaskCreationResult {
    TaskCreationError error{TaskCreationError::none};
    std::optional<TaskCreationCandidate> candidate;
};
// c/n.f→a(kind,definition)→c/k.c→a/o.a→c/k.a完整工厂，不抽外层kind/人物/事件脚本。
// 成功/普通无落点均返回候选；目录/随机/地图错误不提交任何输入变化。
TaskCreationResult prepare_world_task_creation(const WorldTaskCreationState &state, int kind);
} // namespace dungeon_village_reference
