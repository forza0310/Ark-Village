#pragma once

// FIFO续行前消费已发出的商店27/29请求，追加原cd载荷。
#include "ark/simulation/facilities/rules/world_shop.hpp"

namespace ark::simulation::rules {
struct WorldEquipmentDisplayInput {
    CharacterId actor;
    ShopWorldRequest request;            // 原27/29控制已移除后的请求，不再删除下一命令。
    std::optional<Position> cached_view; // 旧u，仅27必需；不以新n/s重建。
};
enum class WorldEquipmentDisplayError { none, invalid_input, stale_actor, missing_cached_view };
struct WorldEquipmentDisplayCandidate {
    RescueWorldState state;
    std::vector<ActorEffectRecord> appended; // 原追加顺序，插入时不推进年龄。
};
struct WorldEquipmentDisplayResult {
    WorldEquipmentDisplayError error{WorldEquipmentDisplayError::none};
    std::optional<WorldEquipmentDisplayCandidate> candidate;
};
// 27依次追加15与年龄-8的7；29追加21/22，不写ce、不装配、不抽随机。
// 不再次删控制，不推进B/cd/ce/HP、不投影u或播放平台效果。
// 请求寿命由调用方管理，每条只消费一次，不按装备ID额外去重。
WorldEquipmentDisplayResult
prepare_world_equipment_display(const RescueWorldState &state,
                                const WorldEquipmentDisplayInput &input);
} // namespace ark::simulation::rules
