// Immediate cash and deferred village points with event/report identity; this is an independent
// safe contract. Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/rules/accounting.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace ark::simulation::rules {
namespace {

bool valid_entry(const CashEntry &entry) {
    const auto category = static_cast<int>(entry.category);
    return entry.event_id != 0 && entry.period != 0 && entry.amount >= 0 && category >= 0 &&
           category < 5 &&
           (entry.direction == CashDirection::income || entry.direction == CashDirection::expense);
}

bool add_nonnegative(std::int64_t &total, std::int64_t value) {
    if (total > std::numeric_limits<std::int64_t>::max() - value) {
        return false;
    }
    total += value;
    return true;
}

} // namespace

bool operator==(const CashEntry &left, const CashEntry &right) {
    return left.event_id == right.event_id && left.period == right.period &&
           left.category == right.category && left.direction == right.direction &&
           left.amount == right.amount;
}

bool operator==(const DefeatTally &left, const DefeatTally &right) {
    return left.definition_id == right.definition_id && left.defeats == right.defeats &&
           left.points_per_defeat == right.points_per_defeat;
}

bool operator==(const ReportInput &left, const ReportInput &right) {
    return left.period == right.period && left.charges == right.charges &&
           left.defeats == right.defeats;
}

PeriodAccounting::PeriodAccounting(std::int64_t opening_funds, std::uint16_t opening_points)
    : funds_(opening_funds), village_points_(opening_points) {
    if (opening_points > 999) {
        throw std::invalid_argument("village points must be in [0,999]");
    }
}

AccountingError PeriodAccounting::post_cash(const CashEntry &entry) {
    if (!valid_entry(entry)) {
        return AccountingError::invalid_input;
    }
    const auto previous = entries_.find(entry.event_id);
    if (previous != entries_.end()) {
        return previous->second == entry ? AccountingError::none : AccountingError::event_conflict;
    }
    if (reports_.count(entry.period) != 0) {
        return AccountingError::sealed_period;
    }
    if ((entry.direction == CashDirection::income &&
         funds_ > std::numeric_limits<std::int64_t>::max() - entry.amount) ||
        (entry.direction == CashDirection::expense &&
         funds_ < std::numeric_limits<std::int64_t>::min() + entry.amount)) {
        return AccountingError::numeric_overflow;
    }
    entries_.emplace(entry.event_id, entry);
    funds_ += entry.direction == CashDirection::income ? entry.amount : -entry.amount;
    return AccountingError::none;
}

AccountingError PeriodAccounting::prepare_report(const ReportInput &input) {
    const auto previous = reports_.find(input.period);
    if (previous != reports_.end()) {
        return previous->second.input == input ? AccountingError::none
                                               : AccountingError::report_conflict;
    }
    auto staged = *this;
    const auto error = staged.prepare_in_place(input);
    if (error == AccountingError::none) {
        *this = std::move(staged);
    }
    return error;
}

AccountingError PeriodAccounting::prepare_in_place(const ReportInput &input) {
    if (input.period == 0) {
        return AccountingError::invalid_input;
    }
    std::set<std::uint64_t> charge_ids;
    for (const auto &charge : input.charges) {
        if (charge.period != input.period || charge.direction != CashDirection::expense ||
            (charge.category != CashCategory::facilities &&
             charge.category != CashCategory::adventurers) ||
            !charge_ids.insert(charge.event_id).second) {
            return AccountingError::invalid_input;
        }
        const auto error = post_cash(charge);
        if (error != AccountingError::none) {
            return error;
        }
    }
    AccountingReport report;
    report.input = input;
    std::set<std::uint64_t> definition_ids;
    for (const auto &tally : input.defeats) {
        if (tally.definition_id == 0 || !definition_ids.insert(tally.definition_id).second) {
            return AccountingError::invalid_input;
        }
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        if (tally.points_per_defeat != 0 && tally.defeats > maximum / tally.points_per_defeat) {
            return AccountingError::numeric_overflow;
        }
        const auto award = tally.defeats * tally.points_per_defeat;
        if (report.pending_points > maximum - award) {
            return AccountingError::numeric_overflow;
        }
        report.pending_points += award;
    }
    for (const auto &item : entries_) {
        const auto &entry = item.second;
        if (entry.period != input.period) {
            continue;
        }
        auto &category = report.categories[static_cast<std::size_t>(entry.category)];
        auto &total = entry.direction == CashDirection::income ? category.income : category.expense;
        if (!add_nonnegative(total, entry.amount)) {
            return AccountingError::numeric_overflow;
        }
    }
    for (std::size_t category = 0; category < 3; ++category) {
        if (!add_nonnegative(report.displayed.income, report.categories[category].income) ||
            !add_nonnegative(report.displayed.expense, report.categories[category].expense)) {
            return AccountingError::numeric_overflow;
        }
    }
    report.displayed_net = report.displayed.income - report.displayed.expense;
    reports_.emplace(input.period, std::move(report));
    return AccountingError::none;
}

AccountingError PeriodAccounting::claim_report(std::uint64_t period) {
    const auto found = reports_.find(period);
    if (found == reports_.end()) {
        return AccountingError::report_not_found;
    }
    auto &report = found->second;
    if (!report.claimed) {
        const auto room = static_cast<std::uint64_t>(999 - village_points_);
        report.awarded_points = static_cast<std::uint16_t>(std::min(room, report.pending_points));
        village_points_ = static_cast<std::uint16_t>(village_points_ + report.awarded_points);
        report.claimed = true;
    }
    return AccountingError::none;
}

std::int64_t PeriodAccounting::funds() const { return funds_; }

std::uint16_t PeriodAccounting::village_points() const { return village_points_; }

const std::map<std::uint64_t, CashEntry> &PeriodAccounting::entries() const { return entries_; }

const std::map<std::uint64_t, AccountingReport> &PeriodAccounting::reports() const {
    return reports_;
}

PeriodAccountingSnapshot PeriodAccounting::snapshot() const {
    return {funds_, village_points_, entries_, reports_};
}
std::optional<PeriodAccounting>
PeriodAccounting::from_snapshot(const PeriodAccountingSnapshot &snapshot) {
    if (snapshot.village_points > 999)
        return {};
    for (const auto &[id, entry] : snapshot.entries)
        if (id != entry.event_id || !valid_entry(entry))
            return {};
    for (const auto &[period, report] : snapshot.reports) {
        if (period == 0 || report.input.period != period || report.awarded_points > 999 ||
            report.awarded_points > report.pending_points ||
            (!report.claimed && report.awarded_points != 0))
            return {};
        // 用原有报告规则核验快照，不把原现金余额再支付一次。
        PeriodAccounting checked;
        for (const auto &[id, entry] : snapshot.entries)
            if (entry.period == period)
                checked.entries_.emplace(id, entry);
        checked.funds_ = snapshot.funds;
        if (checked.prepare_report(report.input) != AccountingError::none)
            return {};
        const auto &expected = checked.reports_.at(period);
        if (expected.pending_points != report.pending_points ||
            expected.displayed.income != report.displayed.income ||
            expected.displayed.expense != report.displayed.expense ||
            expected.displayed_net != report.displayed_net)
            return {};
        for (std::size_t n = 0; n < report.categories.size(); ++n)
            if (expected.categories[n].income != report.categories[n].income ||
                expected.categories[n].expense != report.categories[n].expense)
                return {};
        // prepare_report不得补写快照中缺失的原费用事件。
        for (const auto &charge : report.input.charges) {
            const auto found = snapshot.entries.find(charge.event_id);
            if (found == snapshot.entries.end() || !(found->second == charge))
                return {};
        }
    }
    PeriodAccounting result;
    result.funds_ = snapshot.funds;
    result.village_points_ = snapshot.village_points;
    result.entries_ = snapshot.entries;
    result.reports_ = snapshot.reports;
    return result;
}

} // namespace ark::simulation::rules
