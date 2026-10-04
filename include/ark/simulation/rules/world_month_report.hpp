#pragma once

#include "ark/simulation/rules/world_scripts.hpp"

namespace ark::simulation::rules {
struct MonthReportHumanDefinition {
    int profession_level{};              // a.e.g()，当前职业M[t]，不是人物全职业最高等级。
    std::array<int, 2> profession_fee{}; // 当前a.h.l。
};
struct MonthReportMonsterDefinition {
    int month_defeats{};     // bw.u，永久击败数v不参与。
    int points_per_defeat{}; // bw.l，不是即时金币奖励。
};
struct MonthReportFacility {
    std::uint64_t identity{};
    int definition{};
    std::array<std::array<int, 2>, 12> monthly_cash{}; // 实例v，保留销售槽。
};
// 唯一Owner的临时投影：scripts.finance是这里唯一现金/z事实，不能持久双写另一份。
struct WorldMonthReportState {
    WorldScriptState presentation;
    int display_state{};               // UserData.r：0无叠层，1讨伐，2收支，3既有特殊展示。
    int display_counter{};             // s，n(r)先清零。
    std::array<int, 6> snapshot{};     // K：击败数、点数、收入、经费、差额、首名定义n。
    std::array<int, 6> records{};      // L。
    std::array<bool, 6> new_records{}; // M：旧记录>0且本次更高才置true。
    std::array<int, 5> monster_portraits{{-1, -1, -1, -1, -1}}; // O。
    std::vector<int> kill_display;                      // N，只截前min(5,N.size,K0)项；不清原N。
    int maximum_income{};                               // A。
    int village_points{};                               // f213c，0..999，关闭报告才增加。
    std::vector<MonthReportMonsterDefinition> monsters; // 全bw，原序。
    std::map<int, MonthReportHumanDefinition> humans;
    std::vector<int> active_humans;              // bl投影的n，保留名单次序/重复。
    std::map<int, int> facility_fee_slot3;       // 已刷新共享a.o.I[3]，不读实例邻接修正。
    std::vector<MonthReportFacility> facilities; // 当前g全实例，无施工/通行过滤。
};
struct WorldMonthReportInput {
    int old_month_tick{};  // 本次UserData.e看到的旧t，不能传随后日历递增结果。
    int clock_parameter{}; // d.a.Q，初局80必须由实际Owner提供。
    int month{};           // P[2]，0..11。
    bool admitted{true};   // 已取得UserData.e资格；r1/2不是暂停全世界的理由。
    bool skip_display{};   // aw.c(1048576)，原位置输入。
};
enum class MonthReportError { none, invalid_owner, missing_definition, numeric_overflow };
enum class MonthReportStage {
    start_overlay,
    human_fees,
    facility_fees,
    fee_notice,
    snapshot_totals,
    income_record,
    first_human,
    records,
    next_overlay,
    close_overlay,
    award_points
};
struct WorldMonthReportCandidate {
    WorldMonthReportState state;
    std::vector<MonthReportStage> stages;
    std::vector<WorldScriptNotice> inserted_notices;
};
struct WorldMonthReportResult {
    MonthReportError error{MonthReportError::none};
    std::optional<WorldMonthReportCandidate> candidate;
};
// 只还原UserData.e的月报前段，不执行更早的V/aK/怪物生成，也不推进日期/共同世界。
// 实际消费现金、z、v、K/L/M/O与提示S；报告不是独立页栈，之后现金仍可继续记账。
// 数值溢出返回失败候选属于维护安全边界，不模拟Java溢出造成的损坏。
WorldMonthReportResult prepare_world_month_report(const WorldMonthReportState &state,
                                                  const WorldMonthReportInput &input);
} // namespace ark::simulation::rules
