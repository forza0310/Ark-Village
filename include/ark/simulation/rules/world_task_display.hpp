#pragma once

#include "ark/simulation/rules/world_random.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace ark::simulation::rules {
// n.bd为共享演出表，不是八个实际战斗实例；新100页E也不清其未写列。
struct WorldTaskDisplayState {
    int legacy_page{99};
    bool initialized{}; // 按页身份保存；另一个100页必须重新执行E。
    std::array<std::array<std::int32_t, 9>, 8> bd{};
};
enum class WorldTaskDisplayAction { initialize, update, confirm };
struct WorldTaskDisplayInput {
    WorldTaskDisplayAction action{WorldTaskDisplayAction::update};
    int counter{}; // 框架已经推进的f124d，领域不加计数、不换算秒。
};
enum class WorldTaskDisplayError { none, invalid_input, random_failed, overflow };
struct WorldTaskDisplayCandidate {
    WorldTaskDisplayState state;
    WorldRandomStream random; // 临时候选游标，回写同一全局流，不能长期另存页面随机。
    bool closed{};
};
struct WorldTaskDisplayResult {
    WorldTaskDisplayError error{WorldTaskDisplayError::none};
    std::optional<WorldTaskDisplayCandidate> candidate;
};
// b/g init100:n.E；update100:n.F先于确认。99完全不抽；两页确认须counter>=40，
// 早确认不快进。confirm只检查门槛，不重复本帧已经消费的F；未初始化时先真实初始化。
WorldTaskDisplayResult prepare_world_task_display_page(const WorldTaskDisplayState &state,
                                                       const WorldTaskDisplayInput &input,
                                                       const WorldRandomStream &random);
} // namespace ark::simulation::rules
