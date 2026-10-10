#pragma once

#include "ark/simulation/combat/rules/rescue_commit.hpp"

#include <algorithm>

namespace ark::simulation::rules {
// 原MainScene.W是UID -1的独立人物，不属于bl/bm；普通live入口仍要求名单身份。
inline bool valid_detached_human(const RescueWorldState &s, CharacterId id) {
    const auto actor = s.ai.battle.actors.find(id);
    return id.value && actor != s.ai.battle.actors.end() && actor->second.id == id &&
           actor->second.kind == ActorKind::human && actor->second.legacy_id == -1 &&
           s.actors.count(id) && s.ai.contexts.count(id) && !s.ai.retired_actors.count(id) &&
           std::count(s.ai.human_order.begin(), s.ai.human_order.end(), id) == 0 &&
           std::count(s.ai.monster_order.begin(), s.ai.monster_order.end(), id) == 0;
}
} // namespace ark::simulation::rules
