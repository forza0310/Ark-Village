#pragma once

// Immediate cash subset of research/accounting's independent owner contract. No month reports.
#include <cstdint>
#include <map>
namespace ark::economy {
enum class CashCategory { facilities, monsters, adventurers, shop, other };
enum class CashDirection { income, expense };
enum class CashError { none, invalid_input, event_conflict, numeric_overflow };
struct CashEntry {
    std::uint64_t event_id{};
    std::uint64_t sequence{}; // Caller tag; the initial AI session uses its admitted round number.
    CashCategory category{CashCategory::facilities};
    CashDirection direction{CashDirection::income};
    std::int64_t amount{};
};
bool operator==(const CashEntry &a, const CashEntry &b);
class CashLedger {
  public:
    explicit CashLedger(std::int64_t opening_funds = 0) : funds_(opening_funds) {}
    // Exact event retry is idempotent; conflicting retry/overflow leaves funds and entries intact.
    CashError post_cash(const CashEntry &entry);
    std::int64_t funds() const { return funds_; }
    const std::map<std::uint64_t, CashEntry> &entries() const { return entries_; }

  private:
    std::int64_t funds_{};
    std::map<std::uint64_t, CashEntry> entries_;
};
} // namespace ark::economy
