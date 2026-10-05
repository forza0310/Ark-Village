#pragma once

#include "ark/simulation/rules/world_dungeon_finish.hpp"
#include "ark/simulation/rules/world_random.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ark::simulation::rules {
struct TaskCommandDefinition {
    int identity{};
    int recruitment_fee{};    // a.m.g，第7列；不是实例难度或完成奖励。
    int strength{};           // a.m.s，第16列；页28预测使用。
    std::string notice_title; // av.b(15,a.m.b())调用点的当前动态名称。
};
struct TaskCommandHuman {
    int identity{};
    int status{}; // bv.p，非零均可参加，不要求已有bl实例。
    int legacy_u{};
    int event_participations{};         // e.E，n.a(e,...,true)递增；不是授勋数，页24概率项。
    std::vector<int> profession_levels; // e.M，页27报价和页28预测。
    int extra_fee{};                    // e.an，只有页27初始化才重新计算。
    int absence{};                      // e.m，真正出发才按参与定义清0。
};
enum class TaskCommandPhase { offer, recruiting, team, extra, departure_prompt, departing, closed };
struct TaskRecruitmentEntry {
    int human{};
    int counter{};  // 原X[i][1]，负数为入场延迟。
    int phrase{-1}; // 原X[i][2]；n.aw原6句的索引。
};
struct TaskRecruitmentAnimation {
    std::vector<TaskRecruitmentEntry> entries;
    std::vector<int> portraits; // 原Y，按过零时点加入/按ar切换。
    int arrival_extent{};       // static ap。
    int completion_tick{};      // static aq=ap+130。
    int portrait_timer{};       // static ar。
    int displayed_count{};      // static as，不直接用X.size替代消息参数。
};
// 唯一Owner的临时聚合投影；现金、H/I、人物位2、设施状态与队伍仍只在finish中保存。
struct WorldTaskCommandState {
    DungeonFinishState finish;
    WorldRandomStream random;
    std::map<int, TaskCommandDefinition> definitions;
    std::vector<TaskCommandHuman> humans;               // bv原序，不能按稳定人物实例ID排序。
    std::map<std::uint64_t, int> facility_difficulties; // Tenant.l。
    std::optional<std::uint64_t> selected_task;
    TaskCommandPhase phase{TaskCommandPhase::offer};
    TaskRecruitmentAnimation recruitment;
    std::vector<int> extra_candidates; // 页27 X；页25展示m后追加-1由表现层处理。
    int page_counter{};                // f124d；每个页面调用先统一+1，转入出发动画时清0。
    int secondary_counter{};           // f125e，页24held额外+1。
    bool accelerate_departure{};       // 页28 g=1。
    int predicted_result{};            // 页28 f126f，0..6，不是难度或实际胜率。
    int active_task_updates{};         // UserData.p，安装h时写0。
    int task_subperiods{};             // UserData.y，正式出发最后写0。
    int clock_parameter{};             // d/a.Q，必须由实际Owner提供，不补初值。
    std::uint64_t cash_period{};       // 维护台账调用身份；不是Java逻辑tick换算。
    bool event9_seen{};                // 页24真实8/9提示选择。
};
enum class TaskCommandEffectKind {
    event,               // first为消息ID，text为真实参数；消息64携带j1/k定义。
    reset_main_scene,    // 原MainScene.a0，先于页24初始化。
    activate_main_scene, // 原ax.a(av.l)，先回主场景并关闭其余页，才正式安装h。
    open_page,           // first为raw页24/25/27/28，task保持稳定引用。
    close_page,          // first为被关闭raw页23/24/25/27/28。
    notice15             // 原av.b(15,a.m.b())消息调用，不是声音/音乐；先位2/m0，再y0。
};
struct TaskCommandEffect {
    TaskCommandEffectKind kind;
    int first{};
    int second{};
    std::string text;
    std::optional<std::uint64_t> task;
    std::optional<int> human;
};
using TaskCommandConsumer = std::function<std::optional<WorldTaskCommandState>(
    const WorldTaskCommandState &, const TaskCommandEffect &)>;
enum class TaskCommandError {
    none,
    invalid_input,
    stale_task,
    stale_human,
    missing_consumer,
    consumer_failed,
    random_failed,
    accounting_failed,
    numeric_overflow
};
enum class TaskCommandDenial { none, insufficient_funds, team_full, no_extra_candidates };
struct TaskCommandCandidate {
    WorldTaskCommandState state;
    std::vector<TaskCommandEffect> effects;
    std::vector<int> random_bounds;
    TaskCommandDenial denial{TaskCommandDenial::none};
    bool accepted{};
    bool departed{};
};
struct TaskCommandResult {
    TaskCommandError error{TaskCommandError::none};
    std::optional<TaskCommandCandidate> candidate;
};
struct TaskCommandInput {
    bool confirm{}; // 边沿确认，和页24held不同。
    bool cancel{};
    int selection{}; // 页28恰0确认出发，其余确认退出；页23不读取该值。
};
// 页23确认：费用4→a0→页24初始化随机→关闭23；取消不抽号、不扣款、不清m。
TaskCommandResult prepare_world_task_offer(const WorldTaskCommandState &, std::uint64_t task,
                                           TaskCommandInput, const TaskCommandConsumer & = {});
// 页24一个真实页面tick；held额外推进页面/各入场记录，完成后事件8/9→清m→H/I0→页25。
TaskCommandResult prepare_world_task_recruitment(const WorldTaskCommandState &, bool held,
                                                 const TaskCommandConsumer & = {});
// 页25附加征集入口：先9人上限，再页27当前定义候选与报价；无候选发63并关闭27。
TaskCommandResult prepare_world_task_extra_candidates(const WorldTaskCommandState &,
                                                      const TaskCommandConsumer & = {});
TaskCommandResult prepare_world_task_hire(const WorldTaskCommandState &, int human,
                                          TaskCommandInput, const TaskCommandConsumer & = {});
// 页25→28只初始化预测，不安装h；真正提交发生于页28动画计数>=96。
TaskCommandResult prepare_world_task_departure_page(const WorldTaskCommandState &,
                                                    const TaskCommandConsumer & = {});
TaskCommandResult prepare_world_task_departure(const WorldTaskCommandState &, TaskCommandInput,
                                               const TaskCommandConsumer & = {});
} // namespace ark::simulation::rules
