#include "ark/simulation/village/rules/accounting.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

using namespace ark::simulation::rules;

namespace {

int checks = 0;

void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool equal_state(const PeriodAccounting &left, const PeriodAccounting &right) {
    if (left.funds() != right.funds() || left.village_points() != right.village_points() ||
        left.entries() != right.entries() || left.reports().size() != right.reports().size()) {
        return false;
    }
    for (const auto &item : left.reports()) {
        const auto found = right.reports().find(item.first);
        if (found == right.reports().end()) {
            return false;
        }
        const auto &a = item.second;
        const auto &b = found->second;
        if (!(a.input == b.input) || a.displayed.income != b.displayed.income ||
            a.displayed.expense != b.displayed.expense || a.displayed_net != b.displayed_net ||
            a.pending_points != b.pending_points || a.awarded_points != b.awarded_points ||
            a.claimed != b.claimed) {
            return false;
        }
        for (std::size_t index = 0; index < a.categories.size(); ++index) {
            if (a.categories[index].income != b.categories[index].income ||
                a.categories[index].expense != b.categories[index].expense) {
                return false;
            }
        }
    }
    return true;
}

CashEntry entry(std::uint64_t id, CashCategory category, CashDirection direction,
                std::int64_t amount, std::uint64_t period = 1) {
    return {id, period, category, direction, amount};
}

void expect_post_error(PeriodAccounting &state, const CashEntry &input, AccountingError error) {
    const auto before = state;
    check(state.post_cash(input) == error, "expected posting refusal");
    check(equal_state(before, state), "refused posting changes nothing");
}

void expect_report_error(PeriodAccounting &state, const ReportInput &input, AccountingError error) {
    const auto before = state;
    check(state.prepare_report(input) == error, "expected report refusal");
    check(equal_state(before, state), "refused report rolls back every charge");
}

void resource_and_report_sequence() {
    PeriodAccounting state(1000, 10);
    const auto sale = entry(1, CashCategory::facilities, CashDirection::income, 300);
    const auto bounty = entry(2, CashCategory::monsters, CashDirection::income, 200);
    check(state.post_cash(sale) == AccountingError::none, "sale income immediate");
    check(state.funds() == 1300, "sale is not deferred until monthly report");
    check(state.post_cash(bounty) == AccountingError::none, "bounty income immediate");
    check(state.post_cash(entry(3, CashCategory::shop, CashDirection::expense, 50)) ==
              AccountingError::none,
          "shop expense affects cash");
    check(state.post_cash(entry(4, CashCategory::other, CashDirection::income, 80)) ==
              AccountingError::none,
          "other income affects cash");
    const ReportInput report{1,
                             {entry(5, CashCategory::facilities, CashDirection::expense, 240),
                              entry(6, CashCategory::adventurers, CashDirection::expense, 100)},
                             {{1, 2, 5}, {2, 1, 3}}};
    check(state.prepare_report(report) == AccountingError::none, "prepare report");
    check(state.funds() == 1190 && state.village_points() == 10,
          "fees deducted but points not granted yet");
    const auto &snapshot = state.reports().at(1);
    check(snapshot.displayed.income == 500 && snapshot.displayed.expense == 340 &&
              snapshot.displayed_net == 160,
          "report includes only first three categories");
    check(snapshot.categories[3].expense == 50 && snapshot.categories[4].income == 80,
          "excluded categories remain in full snapshot");
    check(snapshot.pending_points == 13 && !snapshot.claimed,
          "defeat points are a separate delayed resource");
    const auto prepared = state;
    check(state.prepare_report(report) == AccountingError::none && equal_state(state, prepared),
          "preparing same report never charges twice");
    check(state.claim_report(1) == AccountingError::none, "claim report");
    check(state.funds() == 1190 && state.village_points() == 23,
          "claim does not pay monthly net or duplicate bounty cash");
    check(state.reports().at(1).awarded_points == 13, "receipt records actual points");
    const auto claimed = state;
    check(state.claim_report(1) == AccountingError::none && equal_state(state, claimed),
          "claim is idempotent");
    check(state.prepare_report(report) == AccountingError::none && equal_state(state, claimed),
          "preparing claimed report preserves claim receipt");
    check(state.post_cash(sale) == AccountingError::none && equal_state(state, claimed),
          "known event retry accepted after sealing");
    expect_post_error(state, entry(7, CashCategory::facilities, CashDirection::income, 1),
                      AccountingError::sealed_period);
    auto conflicting_report = report;
    std::reverse(conflicting_report.charges.begin(), conflicting_report.charges.end());
    expect_report_error(state, conflicting_report, AccountingError::report_conflict);
    conflicting_report = report;
    ++conflicting_report.defeats[0].defeats;
    expect_report_error(state, conflicting_report, AccountingError::report_conflict);
    check(state.claim_report(0) == AccountingError::report_not_found && equal_state(state, claimed),
          "missing report claim changes nothing");
}

void invalid_input_and_conflicts() {
    PeriodAccounting state;
    const auto income = entry(1, CashCategory::facilities, CashDirection::income, 10);
    check(state.post_cash(income) == AccountingError::none, "seed event");
    auto different = income;
    different.amount = 11;
    expect_post_error(state, different, AccountingError::event_conflict);
    different = income;
    different.period = 2;
    expect_post_error(state, different, AccountingError::event_conflict);
    different = income;
    different.category = CashCategory::shop;
    expect_post_error(state, different, AccountingError::event_conflict);
    different = income;
    different.direction = CashDirection::expense;
    expect_post_error(state, different, AccountingError::event_conflict);
    for (int invalid = 0; invalid < 5; ++invalid) {
        auto bad = entry(2, CashCategory::facilities, CashDirection::income, 1);
        if (invalid == 0) {
            bad.event_id = 0;
        } else if (invalid == 1) {
            bad.period = 0;
        } else if (invalid == 2) {
            bad.amount = -1;
        } else if (invalid == 3) {
            bad.category = static_cast<CashCategory>(5);
        } else {
            bad.direction = static_cast<CashDirection>(2);
        }
        expect_post_error(state, bad, AccountingError::invalid_input);
    }
    expect_post_error(state, entry(2, static_cast<CashCategory>(-1), CashDirection::income, 1),
                      AccountingError::invalid_input);
    const auto fee = entry(2, CashCategory::facilities, CashDirection::expense, 20);
    const ReportInput valid{1, {fee}, {}};
    auto bad = valid;
    bad.charges.push_back(fee);
    expect_report_error(state, bad, AccountingError::invalid_input);
    bad = valid;
    bad.charges.push_back(entry(3, CashCategory::adventurers, CashDirection::expense, -1));
    expect_report_error(state, bad, AccountingError::invalid_input);
    bad = valid;
    bad.charges.push_back(entry(3, CashCategory::adventurers, CashDirection::expense, 1, 2));
    expect_report_error(state, bad, AccountingError::invalid_input);
    bad = valid;
    bad.charges.push_back(income);
    expect_report_error(state, bad, AccountingError::invalid_input);
    bad = valid;
    bad.charges.push_back(entry(3, CashCategory::shop, CashDirection::expense, 1));
    expect_report_error(state, bad, AccountingError::invalid_input);
    bad = valid;
    bad.charges.push_back(entry(1, CashCategory::facilities, CashDirection::expense, 1));
    expect_report_error(state, bad, AccountingError::event_conflict);
    bad = valid;
    bad.defeats = {{0, 1, 1}};
    expect_report_error(state, bad, AccountingError::invalid_input);
    bad.defeats = {{1, 1, 1}, {1, 2, 1}};
    expect_report_error(state, bad, AccountingError::invalid_input);
    expect_report_error(state, {0, {}, {}}, AccountingError::invalid_input);
    check(state.post_cash(fee) == AccountingError::none && state.funds() == -10,
          "maintenance may make funds negative");
    check(state.prepare_report(valid) == AccountingError::none && state.funds() == -10,
          "already posted identical fee not deducted twice");
    check(state.claim_report(1) == AccountingError::none && state.village_points() == 0,
          "zero point report still gets a claim receipt");
    bool rejected = false;
    try {
        const PeriodAccounting invalid(0, 1000);
        (void)invalid;
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "constructor rejects invalid opening point balance");
}

void integer_edges_and_rollback() {
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    const auto minimum = std::numeric_limits<std::int64_t>::min();
    const auto unsigned_maximum = std::numeric_limits<std::uint64_t>::max();
    PeriodAccounting high(maximum);
    expect_post_error(high, entry(1, CashCategory::other, CashDirection::income, 1),
                      AccountingError::numeric_overflow);
    PeriodAccounting low(minimum);
    expect_post_error(low, entry(1, CashCategory::other, CashDirection::expense, 1),
                      AccountingError::numeric_overflow);
    check(low.post_cash(entry(1, CashCategory::other, CashDirection::income, maximum)) ==
                  AccountingError::none &&
              low.funds() == -1,
          "extreme income has no signed overflow");
    PeriodAccounting batch(-maximum);
    expect_report_error(batch,
                        {1,
                         {entry(1, CashCategory::facilities, CashDirection::expense, 1),
                          entry(2, CashCategory::adventurers, CashDirection::expense, 1)},
                         {}},
                        AccountingError::numeric_overflow);
    PeriodAccounting points;
    const auto charge = entry(1, CashCategory::facilities, CashDirection::expense, 10);
    expect_report_error(points, {1, {charge}, {{1, unsigned_maximum, 2}}},
                        AccountingError::numeric_overflow);
    expect_report_error(points, {1, {charge}, {{1, unsigned_maximum, 1}, {2, 1, 1}}},
                        AccountingError::numeric_overflow);
    check(points.prepare_report({1, {charge}, {{1, unsigned_maximum, 1}}}) == AccountingError::none,
          "full unsigned point award allowed without addition overflow");
    check(points.claim_report(1) == AccountingError::none && points.village_points() == 999 &&
              points.funds() == -10,
          "point cap avoids adding huge award to point balance");
    PeriodAccounting totals;
    check(totals.post_cash(entry(1, CashCategory::facilities, CashDirection::income, maximum)) ==
              AccountingError::none,
          "maximum income");
    check(totals.post_cash(entry(2, CashCategory::other, CashDirection::expense, maximum)) ==
              AccountingError::none,
          "offset maximum income");
    check(totals.post_cash(entry(3, CashCategory::facilities, CashDirection::income, 1)) ==
              AccountingError::none,
          "cash remains valid even when report category total overflows");
    expect_report_error(totals, {1, {}, {}}, AccountingError::numeric_overflow);
    PeriodAccounting combined;
    check(combined.post_cash(entry(1, CashCategory::facilities, CashDirection::income, maximum)) ==
              AccountingError::none,
          "combined first income");
    check(combined.post_cash(entry(2, CashCategory::other, CashDirection::expense, maximum)) ==
              AccountingError::none,
          "combined offset");
    check(combined.post_cash(entry(3, CashCategory::monsters, CashDirection::income, 1)) ==
              AccountingError::none,
          "combined second income");
    expect_report_error(combined, {1, {}, {}}, AccountingError::numeric_overflow);
    PeriodAccounting negative;
    check(negative.prepare_report(
              {1, {entry(1, CashCategory::facilities, CashDirection::expense, maximum)}, {}}) ==
                  AccountingError::none &&
              negative.reports().at(1).displayed_net == -maximum,
          "maximum expense produces valid negative report net");
    check(negative.post_cash(entry(unsigned_maximum, CashCategory::other, CashDirection::income, 0,
                                   unsigned_maximum)) == AccountingError::none,
          "full width stable event and period identities");
}

void period_isolation_and_point_cap() {
    PeriodAccounting state(0, 990);
    check(state.post_cash(entry(1, CashCategory::facilities, CashDirection::income, 30, 2)) ==
              AccountingError::none,
          "another period can post before first report");
    check(state.prepare_report({1, {}, {{1, 4, 5}}}) == AccountingError::none &&
              state.reports().at(1).displayed_net == 0,
          "report excludes another period");
    check(state.prepare_report({2, {}, {{1, 1, 1}}}) == AccountingError::none &&
              state.reports().at(2).displayed_net == 30,
          "second report has its own totals");
    check(state.claim_report(2) == AccountingError::none && state.village_points() == 991,
          "claim order independent of UI report order");
    check(state.claim_report(1) == AccountingError::none && state.village_points() == 999 &&
              state.reports().at(1).awarded_points == 8,
          "records only actual awarded points below cap");
    check(state.prepare_report({3, {}, {{1, 1, 5}}}) == AccountingError::none &&
              state.claim_report(3) == AccountingError::none &&
              state.reports().at(3).awarded_points == 0 && state.funds() == 30,
          "at cap claim succeeds once without changing cash");
}

void deterministic_ledger_differential() {
    std::mt19937 random(0xAC2026U);
    for (std::uint64_t trial = 1; trial <= 100; ++trial) {
        PeriodAccounting state(10000, 10);
        std::int64_t expected_funds = 10000;
        std::array<CashTotals, 5> totals{};
        for (std::uint64_t index = 1; index <= 40; ++index) {
            const auto category = static_cast<CashCategory>(random() % 5);
            const auto direction =
                random() % 2 == 0 ? CashDirection::income : CashDirection::expense;
            const auto amount = static_cast<std::int64_t>(random() % 1000);
            const auto posting = entry(index, category, direction, amount, trial);
            auto &category_total = totals[static_cast<std::size_t>(category)];
            if (direction == CashDirection::income) {
                expected_funds += amount;
                category_total.income += amount;
            } else {
                expected_funds -= amount;
                category_total.expense += amount;
            }
            check(state.post_cash(posting) == AccountingError::none &&
                      state.funds() == expected_funds,
                  "random posting matches independent signed accumulator");
            check(state.post_cash(posting) == AccountingError::none &&
                      state.funds() == expected_funds,
                  "random posting retry is inert");
        }
        const ReportInput input{trial, {}, {{1, trial, 3}}};
        check(state.prepare_report(input) == AccountingError::none, "random report prepared");
        const auto &report = state.reports().at(trial);
        std::int64_t net = 0;
        for (std::size_t index = 0; index < totals.size(); ++index) {
            check(report.categories[index].income == totals[index].income &&
                      report.categories[index].expense == totals[index].expense,
                  "random categorized snapshot matches independent aggregate");
            if (index < 3) {
                net += totals[index].income - totals[index].expense;
            }
        }
        check(report.displayed_net == net && state.funds() == expected_funds,
              "snapshot never reapplies reported cash");
        check(state.claim_report(trial) == AccountingError::none &&
                  state.village_points() == 10 + trial * 3 && state.funds() == expected_funds,
              "claim affects only point resource");
    }
}

} // namespace

int main() {
    resource_and_report_sequence();
    invalid_input_and_conflicts();
    integer_edges_and_rollback();
    period_isolation_and_point_cap();
    deterministic_ledger_differential();
    std::cout << checks << " checks passed\n";
}
