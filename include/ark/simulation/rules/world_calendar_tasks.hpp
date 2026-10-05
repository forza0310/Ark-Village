#pragma once

#include "ark/simulation/rules/world_calendar.hpp"
#include "ark/simulation/rules/world_dungeon_finish.hpp"
#include "ark/simulation/rules/world_random.hpp"
#include "ark/simulation/rules/world_scripts.hpp"

namespace ark::simulation::rules {
struct CalendarTaskRankCriterion {
    int type{};
    int threshold{};
};
using CalendarTaskRankTerms = std::vector<CalendarTaskRankCriterion>;
// 固定rankUpTerm.txt五行四条件；不是维护演示阈值。
std::map<int, CalendarTaskRankTerms> fixed_calendar_task_rank_terms();
struct CalendarTaskHuman {
    std::vector<int> profession_levels; // bv.M，期限追加费按全槽逐个求和。
    int continuation_cost{};            // bv.an，n()先重置200，末端仅上限3000。
};
struct CalendarTaskDefinitionDetails {
    std::string title;          // f68c，对话参数。
    std::string displayed_name; // 当时a.m.b()真实解析结果，不补“普通任务”名称。
};
struct CalendarTaskMonsterDetails {
    std::string name;
    int family{};    // f59f，首次脚本family+139。
    int victories{}; // v，特殊任务重演升级与165/166守卫。
};
// 全部结构是唯一外层Owner临时投影，不另存任务/人物/页栈/随机的持久副本。
struct WorldCalendarTasksState {
    DungeonFinishState finish;
    WorldScriptState scripts;
    WorldRandomStream random;
    std::map<int, CalendarTaskHuman> human_details;
    std::map<int, CalendarTaskDefinitionDetails> task_details;
    std::map<int, CalendarTaskMonsterDetails> monster_details;
    std::map<int, int> facility_definition_presence; // br.p，容量提示统计全部开放定义。
    std::map<int, CalendarTaskRankTerms> rank_terms;
    std::vector<std::uint64_t> facility_order;  // g原序，可重复引用，数量不能改唯一计数。
    int rank{};                                 // UserData.k，不是P[3]。
    int highest_month_income{};                 // A，rank条件0。
    int popularity{};                           // f214d，不能从待结算队列或现金推算。
    int events_held{};                          // F，rank条件6。
    int rank_bypass{};                          // static n.au，固定默认0；恰1才绕过未满足条件。
    std::array<bool, 4> rank_met{};             // a.n.i真实显示缓存。
    std::array<int, 4> rank_values{};           // a.n.j，类型3仍保留0。
    int task_subperiods{};                      // UserData.y，不是每任务独立计时。
    bool generation_retry{};                    // 主场景Y；资格成立时先清，即使之后容量满也已清。
    std::optional<std::uint64_t> deadline_page; // 主场景aI最新页33真实引用。
    int completion_mode{};                      // P[14]。
    int system_completion_mode{};               // J[9]。
    std::array<std::vector<std::uint8_t>, 2> system_unlock_data; // UserData.C后的J.e[0/1]原字节。
};
struct CalendarTaskRankStatus {
    std::array<bool, 4> met{};
    std::array<int, 4> values{};
    bool qualified{};
};
// a/n.a(rank)共享纯查询；不插晋级页、不升rank、不消费随机。
// rank5在调用方先返回；保留原设施名单重复与施工中的计数资格。
std::optional<CalendarTaskRankStatus>
prepare_world_rank_status(const WorldCalendarTasksState &state);
struct WorldRankPromotion {
    int rank{};
    bool promoted{};
    bool mark_user_flag{};
    std::vector<WorldScriptInput> before_promotion;
    std::vector<WorldScriptInput> after_promotion;
};
// raw48读取初始化后的四项缓存；条件解释不标u8，拒绝与真正晋级分开。
std::optional<WorldRankPromotion>
prepare_world_rank_promotion(int rank, const CalendarTaskRankTerms &terms,
                             const std::array<bool, 4> &met, int selection, bool manual,
                             const std::string &village, int bypass = 0);
struct WorldRankCelebration {
    std::vector<std::array<int, 5>> participants; // raw50 X：定义、x、y、朝向、层。
    WorldRandomStream random;
};
// 原全名单交换洗牌，每个人抽一次（包括只有一人），之后截取十个静态布局槽。
std::optional<WorldRankCelebration>
prepare_world_rank_celebration(const std::vector<int> &present_definitions,
                               const WorldRandomStream &random);
enum class CalendarTaskExternalKind { endgame_checkpoint, create_task };
struct CalendarTaskExternalRequest {
    CalendarTaskExternalKind kind{};
    int task_kind{}; // 原f(i26)；只在create_task时有效。
};
struct CalendarTaskExternalCandidate {
    WorldCalendarTasksState state;
    std::optional<std::uint64_t> created_task; // null是已执行真实factory后的普通无地点拒绝。
};
// 回调须实际完成UserData.C()/f(kind)及其共享地图/设施/挑战/目录/随机提交。
// 只在私有Owner值上准备，不能修改外部真实对象；缺回调明确失败，不排ID替代。
using CalendarTaskExternalConsumer = std::function<std::optional<CalendarTaskExternalCandidate>(
    const WorldCalendarTasksState &, const CalendarTaskExternalRequest &)>;
enum class CalendarTaskError {
    none,
    invalid_owner,
    unsupported_stage,
    missing_consumer,
    consumer_failed,
    script_failed,
    overflow,
    random_failed
};
struct CalendarTaskCandidate {
    WorldCalendarTasksState state;
    std::vector<WorldScriptTrace> scripts;
    std::vector<WorldScriptPage> pages;
    std::vector<CalendarTaskExternalRequest> external_calls;
    std::optional<int> generation_kind;
    std::optional<std::uint64_t> created_task;
    std::size_t random_draws{};
};
struct CalendarTaskResult {
    CalendarTaskError error{CalendarTaskError::none};
    std::optional<CalendarTaskCandidate> candidate;
};
// 已归一化最终date的一个真实stage，不再次推进日期/人物/设施计数。
// 支持month特殊脚本/等级、subperiod期限/中段/生成/容量提示；其余域留既有maintenance。
CalendarTaskResult prepare_world_calendar_tasks(const WorldCalendarTasksState &state,
                                                const WorldCalendarState &date,
                                                WorldCalendarStage stage,
                                                const WorldScriptCatalog &catalog,
                                                const CalendarTaskExternalConsumer &external = {});
} // namespace ark::simulation::rules
