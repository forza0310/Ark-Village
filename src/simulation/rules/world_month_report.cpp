#include "ark/simulation/rules/world_month_report.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation::rules {
namespace {
bool add(int &value, std::int64_t delta) {
    const auto next = static_cast<std::int64_t>(value) + delta;
    if (next < std::numeric_limits<int>::min() || next > std::numeric_limits<int>::max())
        return false;
    value = static_cast<int>(next);
    return true;
}
bool charge(WorldScriptFinance &finance, int month, int category, int amount) {
    if ((amount > 0 && finance.cash < std::numeric_limits<std::int64_t>::min() + amount) ||
        (amount < 0 && finance.cash > std::numeric_limits<std::int64_t>::max() + amount) ||
        !add(finance.monthly_totals[month][category][1], amount))
        return false;
    finance.cash -= amount; // av.g允许负余额，报告净额不再入现金。
    return true;
}
std::optional<int> human_fee(const MonthReportHumanDefinition &human) {
    // a.e.j(): c.d.a(g(),1,10,l0,l1,true)，再i-i%10（Java向零取整）。
    std::int64_t mapped = human.profession_fee[0];
    if (human.profession_level > 10)
        mapped = human.profession_fee[1];
    else if (human.profession_level >= 1)
        mapped += static_cast<std::int64_t>(human.profession_level - 1) *
                  (static_cast<std::int64_t>(human.profession_fee[1]) - human.profession_fee[0]) /
                  9;
    mapped -= mapped % 10;
    if (mapped < std::numeric_limits<int>::min() || mapped > std::numeric_limits<int>::max())
        return {};
    return static_cast<int>(mapped);
}
std::string grouped_gold(int amount) {
    std::string value = std::to_string(amount);
    const std::size_t start = amount < 0 ? 1 : 0;
    for (std::size_t offset = value.size(); offset > start + 3;) {
        offset -= 3;
        value.insert(offset, ",");
    }
    return value + "Ｇ"; // 固定来源b.d.a("Ｇ",long)，不是locale猜测。
}
} // namespace
WorldMonthReportResult prepare_world_month_report(const WorldMonthReportState &state,
                                                  const WorldMonthReportInput &input) {
    const auto fail = [](MonthReportError error) { return WorldMonthReportResult{error, {}}; };
    if (state.display_state < 0 || state.display_state > 3 || state.display_counter < 0 ||
        state.village_points < 0 || state.village_points > 999 || input.month < 0 ||
        input.month >= 12 || input.clock_parameter <= 0)
        return fail(MonthReportError::invalid_owner);
    WorldMonthReportCandidate candidate{state, {}, {}};
    if (!input.admitted)
        return {MonthReportError::none, std::move(candidate)};
    auto &next = candidate.state;
    const auto stage = [&](MonthReportStage value) { candidate.stages.push_back(value); };
    const auto transition = [&](int value) {
        next.display_state = value;
        next.display_counter = 0;
    };
    if (state.display_state == 0) {
        const auto trigger = static_cast<std::int64_t>(input.clock_parameter) * 20 - 140 - 3;
        if (input.old_month_tick != trigger)
            return {MonthReportError::none, std::move(candidate)};
        if (!next.presentation.finance || next.presentation.finance->month != input.month)
            return fail(MonthReportError::invalid_owner);
        auto &finance = *next.presentation.finance;
        transition(1);
        next.snapshot.fill(0);
        next.monster_portraits.fill(-1);
        stage(MonthReportStage::start_overlay);
        for (auto monster = next.monsters.rbegin(); monster != next.monsters.rend(); ++monster) {
            if (monster->month_defeats < 0 || !add(next.snapshot[0], monster->month_defeats) ||
                !add(next.snapshot[1], static_cast<std::int64_t>(monster->points_per_defeat) *
                                           monster->month_defeats))
                return fail(MonthReportError::numeric_overflow);
        }
        for (std::size_t i = 0; i < next.monster_portraits.size() && i < next.kill_display.size() &&
                                i < static_cast<std::size_t>(next.snapshot[0]);
             ++i)
            next.monster_portraits[i] = next.kill_display[i];
        int human_total{};
        for (const auto definition : next.active_humans) {
            const auto human = next.humans.find(definition);
            if (human == next.humans.end())
                return fail(MonthReportError::missing_definition);
            const auto fee = human_fee(human->second);
            if (!fee || !add(human_total, *fee))
                return fail(MonthReportError::numeric_overflow);
        }
        if (!charge(finance, input.month, 2, human_total))
            return fail(MonthReportError::numeric_overflow);
        stage(MonthReportStage::human_fees);
        int total = human_total;
        std::set<std::uint64_t> identities;
        for (auto &facility : next.facilities) {
            if (facility.identity == 0 || !identities.insert(facility.identity).second)
                return fail(MonthReportError::invalid_owner);
            const auto fee = next.facility_fee_slot3.find(facility.definition);
            if (fee == next.facility_fee_slot3.end())
                return fail(MonthReportError::missing_definition);
            if (!add(total, fee->second) ||
                !add(facility.monthly_cash[input.month][1], fee->second))
                return fail(MonthReportError::numeric_overflow);
        }
        const auto facilities = static_cast<std::int64_t>(total) - human_total;
        if (facilities < std::numeric_limits<int>::min() ||
            facilities > std::numeric_limits<int>::max() ||
            !charge(finance, input.month, 0, static_cast<int>(facilities)))
            return fail(MonthReportError::numeric_overflow);
        stage(MonthReportStage::facility_fees);
        const auto replacement = grouped_gold(total);
        WorldScriptNotice notice{3, -1, 80, replacement, "支付维护费 " + replacement};
        next.presentation.notices.push_back(notice);
        candidate.inserted_notices.push_back(std::move(notice));
        stage(MonthReportStage::fee_notice);
        for (int category : {0, 1, 2}) {
            const auto &cash = finance.monthly_totals[input.month][category];
            if (!add(next.snapshot[2], cash[0]) || !add(next.snapshot[3], cash[1]) ||
                !add(next.snapshot[4], static_cast<std::int64_t>(cash[0]) - cash[1]))
                return fail(MonthReportError::numeric_overflow);
        }
        stage(MonthReportStage::snapshot_totals);
        next.maximum_income = std::max(next.maximum_income, next.snapshot[2]);
        stage(MonthReportStage::income_record);
        if (!next.active_humans.empty())
            next.snapshot[5] = next.active_humans.front();
        stage(MonthReportStage::first_human);
        for (std::size_t i = 0; i < next.records.size(); ++i) {
            next.new_records[i] = false;
            if (next.records[i] < next.snapshot[i]) {
                next.new_records[i] = next.records[i] > 0;
                next.records[i] = next.snapshot[i];
            }
        }
        stage(MonthReportStage::records);
    } else {
        const int duration = state.display_state == 3 ? 90 : 70;
        if (input.skip_display)
            next.display_counter = duration;
        if (!add(next.display_counter, 1))
            return fail(MonthReportError::numeric_overflow);
        if (next.display_counter >= duration) {
            if (state.display_state == 1) {
                transition(2);
                stage(MonthReportStage::next_overlay);
            } else {
                transition(0); // 必须先清r/s，之后才调用点数helper。
                stage(MonthReportStage::close_overlay);
                if (next.snapshot[1] > 0) {
                    next.village_points = static_cast<int>(std::min<std::int64_t>(
                        999, static_cast<std::int64_t>(next.village_points) + next.snapshot[1]));
                    stage(MonthReportStage::award_points);
                }
            }
        }
    }
    return {MonthReportError::none, std::move(candidate)};
}
} // namespace ark::simulation::rules
