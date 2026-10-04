// Immediate entry contract from research d7ca763 example/accounting; no period sealing inferred.
#include "ark/economy/cash.hpp"
#include <limits>
namespace ark::economy {
bool operator==(const CashEntry &a, const CashEntry &b) {
    return a.event_id == b.event_id && a.sequence == b.sequence && a.category == b.category &&
           a.direction == b.direction && a.amount == b.amount;
}
CashError CashLedger::post_cash(const CashEntry &entry) {
    const auto category = static_cast<int>(entry.category);
    if (!entry.event_id || !entry.sequence || entry.amount < 0 || category < 0 || category >= 5 ||
        (entry.direction != CashDirection::income && entry.direction != CashDirection::expense))
        return CashError::invalid_input;
    const auto prior = entries_.find(entry.event_id);
    if (prior != entries_.end())
        return prior->second == entry ? CashError::none : CashError::event_conflict;
    if ((entry.direction == CashDirection::income &&
         funds_ > std::numeric_limits<std::int64_t>::max() - entry.amount) ||
        (entry.direction == CashDirection::expense &&
         funds_ < std::numeric_limits<std::int64_t>::min() + entry.amount))
        return CashError::numeric_overflow;
    entries_.emplace(entry.event_id, entry);
    funds_ += entry.direction == CashDirection::income ? entry.amount : -entry.amount;
    return CashError::none;
}
} // namespace ark::economy
