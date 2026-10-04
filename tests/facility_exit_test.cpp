// Exit contracts use real product map binding. Cases are conditional fixtures, not new-game data.
#include "ark/economy/cash.hpp"
#include "facility_test_support.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
using namespace ark;
using namespace facilities;
int checks{};
void check(bool ok, const char *message) {
    ++checks;
    if (!ok)
        throw std::runtime_error(message);
}
void completion_and_satisfaction() {
    FacilityUseProgress state;
    for (int n = 0; n < 10; ++n) {
        const auto c = prepare_facility_use_completion(28, {2, 10}, state);
        check(c.candidate && c.candidate->progress.completed_uses == n + 1 &&
                  c.candidate->progress.level == 1 &&
                  c.candidate->progress.upgrade_pending == (n >= 1),
              "shared use increments and latches prompt without automatically leveling");
        state = c.candidate->progress;
    }
    state.completed_uses = std::numeric_limits<int>::max();
    check(!prepare_facility_use_completion(28, {2, 10}, state).candidate,
          "use overflow refuses a partial result");
    for (int satisfaction = 0; satisfaction <= 100; ++satisfaction)
        for (int ticket = 0; ticket < 10; ++ticket) {
            FacilitySatisfactionInput i{{1}, 2, 33, satisfaction, {10, 100}, 5, ticket};
            const auto c = prepare_facility_satisfaction(i);
            check(
                c.candidate && c.candidate->satisfaction >= satisfaction &&
                    c.candidate->satisfaction <= 100 &&
                    c.candidate->popularity.delta ==
                        (10 + satisfaction * 90 / 100 + ticket - 5 <= 5 ? 1 : 0),
                "threshold equality succeeds; popularity request differs from capped satisfaction");
        }
    const auto tail = prepare_ordinary_exit_tail(1, 0, {{0, 1}, {4, 2}}, 1);
    check(tail.candidate && tail.candidate->requests.size() == 3 &&
              tail.candidate->requests[0].action == ExitDeferredAction::choose_activity &&
              tail.candidate->requests[2].parameter == 4 && tail.candidate->requests[2].value == 2,
          "next activity precedes selected delayed attribute");
    check(!prepare_ordinary_exit_tail(1, 0, {{0, 1}}, 2).candidate,
          "invalid effect ticket cannot produce a partial tail");
}
void exits() {
    for (int shape : {0, 1, 2})
        for (int orientation : {0, 1}) {
            Placement p{1, 33, shape, orientation, {2, 2}};
            const auto parts = footprint(shape, orientation, p.anchor);
            world::RouteMap terrain{5, 5, std::vector<world::RouteCell>(25)};
            for (auto &c : terrain.cells) {
                c.legacy_state = 5;
                c.category = world::RouteCategory::blocked;
            }
            const auto map = *test::bind_fixture_map(terrain, {{p, 3}}).map;
            for (const auto &part : parts) {
                const world::WorldPosition current{part.cell.x * 100.0F + 17,
                                                   part.cell.y * 100.0F + 88};
                const auto retained = prepare_facility_exit_position(map, p, current);
                check(retained.candidate &&
                          retained.candidate->status == FacilityExitPositionStatus::retained &&
                          retained.candidate->position.x == current.x &&
                          retained.candidate->position.z == current.z,
                      "single cell or fully blocked multi-cell exit retains continuous position");
                auto broken = map;
                broken.cells[broken.index(parts.front().cell)].facility.reset();
                check(!prepare_facility_exit_position(broken, p, current).candidate,
                      "partial binding rejected even when the current fragment still exists");
            }
            if (shape == 1 && orientation == 0) {
                auto open = map;
                const world::Cell exit{3, 3};
                open.cells[open.index(exit)].legacy_state = 4;
                const auto c = prepare_facility_exit_position(open, p, {250, 250});
                check(c.candidate && c.candidate->status == FacilityExitPositionStatus::relocated &&
                          c.candidate->logical_cell == exit && c.candidate->position.x == 350,
                      "multi-cell source binding order finds reachable external ground");
            }
        }
}
void cash() {
    using namespace economy;
    CashLedger ledger(5000);
    economy::CashEntry income{1, 1, CashCategory::facilities, CashDirection::income, 300};
    check(ledger.post_cash(income) == CashError::none &&
              ledger.post_cash(income) == CashError::none && ledger.funds() == 5300 &&
              ledger.entries().size() == 1,
          "arrival event retries are idempotent");
    income.amount = 400;
    check(ledger.post_cash(income) == CashError::event_conflict && ledger.funds() == 5300,
          "same identity with different cash is rejected");
    income.event_id = 2;
    income.amount = std::numeric_limits<std::int64_t>::max();
    check(ledger.post_cash(income) == CashError::numeric_overflow && ledger.entries().size() == 1,
          "cash overflow cannot create a ledger entry");
}
} // namespace
int main() {
    completion_and_satisfaction();
    exits();
    cash();
    std::cout << checks << " checks passed\n";
}
