#pragma once

// Immediate cash and deferred village points with event/report identity; this is an independent
// safe contract.

#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace dungeon_village_reference {

enum class CashCategory { facilities, monsters, adventurers, shop, other };
enum class CashDirection { income, expense };
enum class AccountingError {
    none,
    invalid_input,
    event_conflict,
    sealed_period,
    numeric_overflow,
    report_conflict,
    report_not_found
};

struct CashEntry {
    std::uint64_t event_id{};
    std::uint64_t period{};
    CashCategory category{CashCategory::facilities};
    CashDirection direction{CashDirection::income};
    std::int64_t amount{};
};

bool operator==(const CashEntry &left, const CashEntry &right);

struct DefeatTally {
    std::uint64_t definition_id{};
    std::uint64_t defeats{};
    std::uint64_t points_per_defeat{};
};

bool operator==(const DefeatTally &left, const DefeatTally &right);

struct ReportInput {
    std::uint64_t period{};
    std::vector<CashEntry> charges;
    std::vector<DefeatTally> defeats;
};

bool operator==(const ReportInput &left, const ReportInput &right);

struct CashTotals {
    std::int64_t income{};
    std::int64_t expense{};
};

struct AccountingReport {
    ReportInput input;
    std::array<CashTotals, 5> categories{};
    CashTotals displayed;
    std::int64_t displayed_net{};
    std::uint64_t pending_points{};
    std::uint16_t awarded_points{};
    bool claimed{};
};

// Independent Ark contract: events affect cash immediately, reports never pay their net again.
class PeriodAccounting {
  public:
    explicit PeriodAccounting(std::int64_t opening_funds = 0, std::uint16_t opening_points = 0);

    // Post cash immediately; identical event retries are harmless, conflicting event payloads fail.
    AccountingError post_cash(const CashEntry &entry);
    // Stage the entire fee batch and seal its period; conflicting retries leave the ledger intact.
    AccountingError prepare_report(const ReportInput &input);
    // Award deferred points once; the displayed net is never paid into cash again.
    AccountingError claim_report(std::uint64_t period);

    std::int64_t funds() const;
    std::uint16_t village_points() const;
    const std::map<std::uint64_t, CashEntry> &entries() const;
    const std::map<std::uint64_t, AccountingReport> &reports() const;

  private:
    AccountingError prepare_in_place(const ReportInput &input);

    std::int64_t funds_{};
    std::uint16_t village_points_{};
    std::map<std::uint64_t, CashEntry> entries_;
    std::map<std::uint64_t, AccountingReport> reports_;
};

} // namespace dungeon_village_reference
