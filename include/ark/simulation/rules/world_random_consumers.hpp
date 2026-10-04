#pragma once

#include "ark/simulation/rules/world_facilities.hpp"
#include "ark/simulation/rules/world_random.hpp"

namespace ark::simulation::rules {
struct WorldExpressionDrawResult {
    WorldRandomError random_error{WorldRandomError::none};
    ActorEffectError expression_error{ActorEffectError::none};
    std::optional<WorldExpressionTicket> ticket;
    std::optional<ActorExpressionCandidate> candidate;
};
// 两个表对应kairo.android.i.h.a()的真实结果，不能按截图语言/平台名称猜选表。
int reference_world_expression_variants(int expression, bool primary_table);
// 一律先抽1000；纯规则判定真正需要变体后才抽变体数，抑制/概率失败不预抽。
// 参数stream必须是外层私有随机候选；错误时本函数局部也回滚游标。
WorldExpressionDrawResult prepare_world_random_expression(WorldRandomStream &stream,
                                                          const ActorEffectState &state,
                                                          int expression, int delay,
                                                          bool primary_table);
} // namespace ark::simulation::rules
