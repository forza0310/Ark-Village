#pragma once

#include "ark/simulation/rules/world_calendar_tasks.hpp"

namespace ark::simulation::rules {
// raw33独有i/d/c/K/ak；任务引用与显示费用在真实页面对象中，不重复持有任务世界。
struct WorldTaskDeadlinePageState {
    bool initialized{};
    int phase{};
    int counter{};
    int cursor{};
    int returned{-1};
    int grade{};
};
enum class WorldTaskDeadlinePageAction { initialize, update, confirm };
struct WorldTaskDeadlinePageInput {
    WorldTaskDeadlinePageAction action{WorldTaskDeadlinePageAction::update};
    int selection{};
    int displayed_fee{}; // 创建页33时f126f的费用快照；确认不重算an。
    std::int64_t funds{};
    int initial_grade{}; // n.a(aa,q)的0..4结果；来源输入，不影响真实任务进度。
};
enum class WorldTaskDeadlineError {
    none,
    invalid_input,
    stale_page,
    missing_human,
    missing_consumer,
    consumer_failed,
    accounting_failed,
    overflow
};
struct WorldTaskDeadlinePageCandidate {
    WorldTaskDeadlinePageState state;
    bool insufficient_funds{};
    bool closed{};
};
struct WorldTaskDeadlinePageResult {
    WorldTaskDeadlineError error{WorldTaskDeadlineError::none};
    std::optional<WorldTaskDeadlinePageCandidate> candidate;
};
// update自己执行通用d++；confirm是当前帧边沿输入，不重复d++。
// 确认动画仅快进到50，下次update先检测门槛并关页；无取消分支。
WorldTaskDeadlinePageResult prepare_world_task_deadline_page(const WorldTaskDeadlinePageState &,
                                                             const WorldTaskDeadlinePageInput &);

enum class WorldTaskDeadlineEffectKind { abort_task, event, notice };
struct WorldTaskDeadlineEffect {
    WorldTaskDeadlineEffectKind kind;
    int id{};
};
using WorldTaskDeadlineConsumer = std::function<std::optional<WorldCalendarTasksState>(
    const WorldCalendarTasksState &, const WorldTaskDeadlineEffect &)>;
struct WorldTaskDeadlineCandidate {
    WorldCalendarTasksState state;
    std::vector<WorldTaskDeadlineEffect> effects;
    int current_fee{};
    bool renewed{};
    bool aborted{};
};
struct WorldTaskDeadlineResult {
    WorldTaskDeadlineError error{WorldTaskDeadlineError::none};
    std::optional<WorldTaskDeadlineCandidate> candidate;
};
// 仅真正返回获准主场景b()入口消费：先清aI，再合计当时an；不在页关闭时扣费。
// abort回调须实际执行n.o（人物/遭遇/全占地恢复/任务清理）；脚本与通知在原调用点同步。
// 费用、脚本、地图、随机皆在私有候选提交，晚期失败不留下扣款/游标或部分中止。
WorldTaskDeadlineResult prepare_world_task_deadline_result(
    const WorldCalendarTasksState &, int returned, std::uint64_t cash_period,
    const WorldTaskDeadlineConsumer & = {}, const std::optional<WorldScriptPage> &closed_page = {});
} // namespace ark::simulation::rules
