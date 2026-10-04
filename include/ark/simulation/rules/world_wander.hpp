#pragma once

// 同一世界中的单次控制10/12/13事务，不重复d前段、不当场寻路或运动。
#include "ark/simulation/rules/rescue_commit.hpp"
#include "ark/simulation/rules/world_perception.hpp"

namespace ark::simulation::rules {
struct WorldWanderInput {
    CharacterId actor;
    WorldMapFacts facts;      // 当前逻辑状态4/地图位2/开边界，不是路径资格。
    std::vector<int> tickets; // 原序抽号；扩展返回实际消费量。
    std::function<std::optional<int>(int)> draw{};
};
struct WorldWanderCandidate {
    RescueWorldState state;
    int opcode{};
    std::optional<Position> center; // 10旧s、12的db中心、13的S旧s；空引用仍为空。
    std::vector<Position> cells;    // 当前地图过滤后的原四/八格顺序。
    std::vector<LegacyActorControl> append;
    std::size_t consumed_tickets{};
};
struct WorldWanderResult {
    RescueWorldError error{RescueWorldError::none};
    std::optional<WorldWanderCandidate> candidate;
};
// 只接受当前队首10/12/13：先移除，再把新计划接在所有旧尾部之后。
// db/S为空时零随机且只移除，不强迫提供不会用到的地图投影。
// 非空引用在当前/退休存储中均找不到则拒绝，不伪装为空引用。
// 不改变n/s/t/动作/计数/路径/设施；调用方继续同次FIFO。
WorldWanderResult prepare_world_wander(const RescueWorldState &state,
                                       const WorldWanderInput &input);
} // namespace ark::simulation::rules
