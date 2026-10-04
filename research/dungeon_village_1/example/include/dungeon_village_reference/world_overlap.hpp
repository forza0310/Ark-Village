#pragma once

// UserData.L在已准入世界轮的最后执行，位于全部名单和设施遍历之后。
#include "dungeon_village_reference/actor_ai.hpp"
#include <functional>

namespace dungeon_village_reference {
struct WorldOverlapActor {
    CharacterId id; // 身份按人物/怪物名单分域，零有效。
    int state{};
    Position cell;                   // 缓存s，本次任何分离后均不重投影。
    bool inside_town{};              // 缓存ax，仅人物/人物段读取。
    bool decision_area{};            // b.a资格读取aB[0]，不读aB[1]。
    int monster_shape{};             // 怪物g+3索引原矩形表，人物不用。
    WorldPosition position;          // 实时n.x/z，分离只写这两项。
    WorldPosition previous_position; // bu.x/z，近轴重叠的回退目标。
};
struct WorldOverlapRect {
    int x_offset{};
    int z_offset{};
    int width{};
    int height{};
};
struct WorldOverlapIdentity {
    ActorKind kind{ActorKind::human};
    CharacterId id;
};
struct WorldOverlapAttempt {
    WorldOverlapIdentity moved;
    WorldOverlapIdentity other;
    int ticket{};                                     // 原d.a(2)，在相邻格/矩形守卫之前消费。
    std::optional<WorldOverlapRect> shared_rectangle; // 相邻时才查询，双方共用br[0]。
    bool adjacent{};
    bool overlapping{};
    bool rolled_back{};
};
struct WorldOverlapInput {
    std::vector<WorldOverlapActor> humans; // 当前原名单顺序。
    std::vector<WorldOverlapActor> monsters;
    int boundary_y{};                   // h.l[n.o][1][1]，不以城内判断替代。
    std::vector<int> direction_tickets; // 按原pair遍历顺序提供[0,2)抽号。
    std::size_t attempt_limit{1000000}; // 维护保护预算，不是APK规则。
    std::function<std::optional<int>(int)> draw{};
};
enum class WorldOverlapError { none, invalid_input, duplicate_id, missing_ticket, attempt_limit };
struct WorldOverlapCandidate {
    std::vector<WorldOverlapActor> humans;
    std::vector<WorldOverlapActor> monsters;
    std::vector<WorldOverlapAttempt> attempts;
    std::size_t consumed_tickets{};
};
struct WorldOverlapResult {
    WorldOverlapError error{WorldOverlapError::none};
    std::optional<WorldOverlapCandidate> candidate;
};
// ah和ai[*][0]给出的实际n.a(0,shape)结果，不猜显示/居中矩形。
std::optional<WorldOverlapRect> reference_world_overlap_rectangle(ActorKind kind, int shape);
// 人物/人物一次、人物/怪物一次、有序怪物/怪物每无序对两次。
// 不更新s/t/u/ax、不推进路径/HP/实例删除，不拥有PRNG。
// 晚期缺号/非法号拒绝整批私有候选，不返回已经分离的半状态。
WorldOverlapResult prepare_world_overlap(const WorldOverlapInput &input);
} // namespace dungeon_village_reference
