#include "ark/simulation/village/rules/world_month_report.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
WorldMonthReportState world() {
    WorldMonthReportState state;
    WorldScriptFinance finance;
    finance.cash = 5000;
    finance.cash_peak = 8000;
    finance.month = 0;
    finance.monthly_totals[0] = {{{300, 70}, {80, 20}, {25, 5}, {1000, 2000}, {500, 100}}};
    state.presentation.finance = finance;
    WorldScriptPage scene;
    scene.id = 1;
    state.presentation.pages.push_back(scene);
    state.presentation.next_page_id = 2;
    state.village_points = 10;
    state.records = {0, 1, 400, 1000, 10, 8};
    state.maximum_income = 400;
    state.monsters = {{3, 2}, {2, 4}};
    state.kill_display = {9, 8, 7, 6, 5, 4};
    state.humans = {{7, {5, {105, 305}}}, {2, {10, {500, 500}}}};
    state.active_humans = {7, 2, 7};
    state.facility_fee_slot3 = {{3, 30}};
    state.facilities = {{10, 3, {}}, {11, 3, {}}};
    state.facilities[0].monthly_cash[0] = {400, 9};
    return state;
}
WorldMonthReportInput input() { return {1457, 80, 0, true, false}; }
void prepare_snapshot() {
    const auto state = world();
    const auto result = prepare_world_month_report(state, input());
    check(result.candidate.has_value(), "real oldt1457 prepares monthly overlay");
    const auto &next = result.candidate->state;
    check(next.display_state == 1 && next.display_counter == 0 &&
              next.snapshot == std::array<int, 6>{5, 14, 405, 1035, -630, 7},
          "K includes bw.u*l, fees before categories0/1/2, first active definition not max effort");
    check(next.presentation.finance->cash == 4060 && next.village_points == 10 &&
              next.presentation.finance->cash_peak == 8000,
          "fees immediately subtract940 with no early points/no net repayment/no peak mutation");
    check(next.presentation.finance->monthly_totals[0][0][1] == 130 &&
              next.presentation.finance->monthly_totals[0][2][1] == 885 &&
              next.presentation.finance->monthly_totals[0][3][1] == 2000,
          "active duplicate human billed twice; facility and adventurer categories charged "
          "separately");
    check(next.facilities[0].monthly_cash[0] == std::array<int, 2>{400, 39} &&
              next.facilities[1].monthly_cash[0][1] == 30 &&
              next.monster_portraits == std::array<int, 5>{9, 8, 7, 6, 5} &&
              next.kill_display == state.kill_display,
          "current g all instances update v expense only and N first5 copied without clearing");
    check(next.maximum_income == 405 &&
              next.new_records == std::array<bool, 6>{false, true, true, true, false, false} &&
              next.records == std::array<int, 6>{5, 14, 405, 1035, 10, 8},
          "first record never displays new-record mark; all six legacy slots compare original "
          "values");
    check(result.candidate->inserted_notices.size() == 1 &&
              next.presentation.notices.back().message == 3 &&
              next.presentation.notices.back().counter == -1 &&
              next.presentation.notices.back().duration == 80 &&
              next.presentation.notices.back().replacement == "940Ｇ" &&
              next.presentation.notices.back().text == "支付维护费 940Ｇ" &&
              next.presentation.pages.size() == 1 && next.presentation.next_page_id == 2,
          "actual S notice usesaz3=1/duration80; monthly overlay does not invent page stack "
          "insertion");
    check(result.candidate->stages ==
              std::vector<MonthReportStage>{
                  MonthReportStage::start_overlay, MonthReportStage::human_fees,
                  MonthReportStage::facility_fees, MonthReportStage::fee_notice,
                  MonthReportStage::snapshot_totals, MonthReportStage::income_record,
                  MonthReportStage::first_human, MonthReportStage::records},
          "preparation source sequence not date cross-month event");
    check(state.presentation.finance->cash == 5000 && state.facilities[0].monthly_cash[0][1] == 9 &&
              state.presentation.notices.empty(),
          "all domains remain unchanged outside private candidate");
}
void gates_and_progression() {
    for (int tick = 0; tick < 1601; ++tick) {
        auto request = input();
        request.old_month_tick = tick;
        const auto result = prepare_world_month_report(world(), request);
        check(result.candidate && result.candidate->state.display_state == (tick == 1457 ? 1 : 0),
              "exact old month tick trigger; not1456/1458/calendar1600");
    }
    auto state = prepare_world_month_report(world(), input()).candidate->state;
    // 报表期间世界仍可记账：快照不再重汇总，也不把周期封账。
    state.presentation.finance->cash += 50;
    state.presentation.finance->monthly_totals[0][0][0] += 50;
    for (int frame = 1; frame <= 140; ++frame) {
        state = prepare_world_month_report(state, input()).candidate->state;
        const int expected_state = frame < 70 ? 1 : frame < 140 ? 2 : 0;
        check(state.display_state == expected_state && state.snapshot[2] == 405 &&
                  state.presentation.finance->cash == 4110 &&
                  state.village_points == (frame < 140 ? 10 : 24),
              "70+70 admitted calls advance overlay; cash50 remains; points only on second close");
    }
    auto request = input();
    request.old_month_tick = 1600;
    const auto closed = prepare_world_month_report(state, request);
    check(closed.candidate->state.village_points == 24 &&
              closed.candidate->state.presentation.notices.size() == 1,
          "ordinary post-close tick cannot re-award or re-charge");
    for (int mode : {1, 2, 3}) {
        state.display_state = mode;
        state.display_counter = 0;
        state.snapshot[1] = 2000;
        state.village_points = 900;
        request.skip_display = true;
        const auto skipped = prepare_world_month_report(state, request);
        check(skipped.candidate && skipped.candidate->state.display_state == (mode == 1 ? 2 : 0) &&
                  skipped.candidate->state.display_counter == 0 &&
                  skipped.candidate->state.village_points == (mode == 1 ? 900 : 999),
              "skip sets duration then increments; state1 advances,2/3 close and clamp points999");
        if (mode > 1)
            check(skipped.candidate->stages ==
                      std::vector<MonthReportStage>{MonthReportStage::close_overlay,
                                                    MonthReportStage::award_points},
                  "n(0) resets overlay before point helper");
        request.admitted = false;
        const auto paused = prepare_world_month_report(state, request);
        check(paused.candidate && paused.candidate->state.display_state == mode &&
                  paused.candidate->state.display_counter == 0 && paused.candidate->stages.empty(),
              "unadmitted e does not consume skip or mutate presentation");
        request.admitted = true;
    }
    for (int counter = 0; counter <= 90; ++counter) {
        state.display_state = 3;
        state.display_counter = counter;
        request.skip_display = false;
        const auto third = prepare_world_month_report(state, request);
        check(third.candidate && third.candidate->state.display_state == (counter >= 89 ? 0 : 3),
              "existing special state3 uses90 counters without inventing its unknown entrance");
    }
}
void rollback_and_edges() {
    for (int failure = 0; failure < 6; ++failure) {
        auto state = world();
        if (failure == 0)
            state.humans.erase(7);
        if (failure == 1)
            state.facility_fee_slot3.clear();
        if (failure == 2)
            state.facilities[1].identity = 10;
        if (failure == 3)
            state.facilities[1].monthly_cash[0][1] = std::numeric_limits<int>::max();
        if (failure == 4)
            state.presentation.finance->monthly_totals[0][0][0] = std::numeric_limits<int>::max();
        if (failure == 5)
            state.presentation.finance->cash = std::numeric_limits<std::int64_t>::min();
        const auto result = prepare_world_month_report(state, input());
        check(!result.candidate && state.display_state == 0 &&
                  state.facilities[0].monthly_cash[0][1] == 9 && state.presentation.notices.empty(),
              "missing records/late arithmetic failure publishes no partial fee/notice/overlay");
    }
    auto empty = world();
    empty.active_humans.clear();
    empty.facilities.clear();
    empty.monsters.clear();
    const auto result = prepare_world_month_report(empty, input());
    check(result.candidate &&
              result.candidate->state.snapshot == std::array<int, 6>{0, 0, 405, 95, 310, 0} &&
              result.candidate->state.monster_portraits == std::array<int, 5>{-1, -1, -1, -1, -1} &&
              result.candidate->inserted_notices[0].replacement == "0Ｇ",
          "empty actual rosters still create zero maintenance notice, no fake initial actor or "
          "monster");
    auto debt = world();
    debt.presentation.finance->cash = 100;
    auto charged = prepare_world_month_report(debt, input());
    check(charged.candidate && charged.candidate->state.presentation.finance->cash == -840,
          "insufficient funds do not suppress original fees");
    auto grouped = world();
    grouped.facility_fee_slot3[3] = 1234567;
    charged = prepare_world_month_report(grouped, input());
    check(charged.candidate && charged.candidate->inserted_notices[0].replacement == "2,470,014Ｇ",
          "fee notice source groups decimal bythree and preserves fullwidthＧ");
    for (const int level : {-1, 0, 1, 5, 10, 11, 100}) {
        auto fees = world();
        fees.active_humans = {7};
        fees.facilities.clear();
        fees.humans.at(7).profession_level = level;
        const auto candidate = prepare_world_month_report(fees, input());
        const int fee = level < 1     ? 100
                        : level > 10  ? 300
                        : level == 5  ? 190
                        : level == 10 ? 300
                                      : 100;
        check(candidate.candidate &&
                  candidate.candidate->state.presentation.finance->cash == 5000 - fee,
              "profession fee clamps1..10 at original endpoints then truncates decimal towardzero");
    }
    auto signed_fee = world();
    signed_fee.active_humans = {7};
    signed_fee.facilities.clear();
    signed_fee.humans.at(7) = {1, {-105, -105}};
    const auto refund = prepare_world_month_report(signed_fee, input());
    check(refund.candidate && refund.candidate->state.presentation.finance->cash == 5100 &&
              refund.candidate->inserted_notices[0].replacement == "-100Ｇ",
          "signed original integer helper truncates negative fee towardzero; not unsigned ledger "
          "rewrite");
}
} // namespace
int main() {
    try {
        prepare_snapshot();
        gates_and_progression();
        rollback_and_edges();
        std::cout << "world_month_report: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "world_month_report: " << error.what() << '\n';
        return 1;
    }
}
