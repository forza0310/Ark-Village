#include "dungeon_village_reference/world_calendar_maintenance.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
WorldCalendarMaintenanceState fixture() {
    WorldCalendarMaintenanceState state;
    state.random = WorldRandomStream::from_raw({-1, 0, 2});
    for (int i = 0; i < 3; ++i) {
        CalendarMaintenanceHuman human;
        human.definition = i;
        human.presence = i == 2 ? 0 : i + 1;
        human.leave_months = i == 0 ? 2 : -1;
        human.absent_months = i + 7;
        human.equipment = {2, 0, -1};
        human.yearly_totals = {3, 4, 5};
        human.residence_status = 1;
        human.profession_type = i == 0 ? 7 : 0;
        human.legacy_F = 999;
        human.legacy_G = 7;
        state.humans.push_back(human);
    }
    state.active_human_definitions = {0, 0};
    state.facility_definitions = {{0, 2, -1}, {1, 3, -1}, {2, 12, -1}};
    state.facilities = {{1, 0, 9, {{{10, 5}}, {{20, 7}}}}, {2, 1, 0, {{{30, 3}}}}, {3, 2, 4, {}}};
    state.monster_month_kills = {1, 7, -9};
    state.item_flags = {4, 5, 0xFFFFFFFFu};
    state.kill_display = {7, 8, 7};
    for (auto &month : state.monthly_cash) {
        month[0] = {100, 50};
        month[1] = {70, 90};
        month[2] = {15, 5};
        month[3] = {999, 111};
    }
    state.yearly_statistics.resize(3);
    state.yearly_statistics[1].fill(7);
    state.rank = 3;
    state.popularity = 234;
    state.legacy_v = 4;
    state.legacy_D = 5;
    state.legacy_n0 = 6;
    state.quarter_base = 9;
    state.quarter_counter = -1;
    return state;
}
CalendarMaintenanceResult step(const WorldCalendarMaintenanceState &state, WorldCalendarStage stage,
                               int month = 0, int year = 1) {
    return prepare_world_calendar_maintenance(state, {year, month, 0, 0, 0, 0}, stage);
}
void annual() {
    const auto original = fixture();
    auto result = step(original, WorldCalendarStage::year_statistics);
    const std::array<std::int32_t, 10> expected{10, 241, 9, 9, 11, 12, 13, 607, -233, 127};
    check(result.candidate && result.candidate->state.yearly_statistics[1] == expected &&
              result.candidate->state.yearly_statistics[0][0] == 0 &&
              original.yearly_statistics[1][0] == 7,
          "D accumulates in NEW raw-year row, counts all present definitions and category2/3 "
          "facilities");
    check(result.candidate->state.monthly_cash[0][3][0] == 999,
          "D reads only categories0/1/2 and never clears all cash itself");
    result = step(result.candidate->state, WorldCalendarStage::year_characters);
    for (const auto &human : result.candidate->state.humans)
        check(human.yearly_totals == std::vector<std::int32_t>({0, 0, 0}),
              "all bv.B clears, including p0 definitions");
    result = step(result.candidate->state, WorldCalendarStage::year_facilities);
    for (const auto &facility : result.candidate->state.facilities)
        for (const auto &month : facility.yearly_cash)
            check(month == std::array<std::int32_t, 2>({0, 0}), "all g.v rows clear");
    result = step(result.candidate->state, WorldCalendarStage::year_refresh);
    for (const auto &month : result.candidate->state.monthly_cash)
        for (const auto &category : month)
            check(category == std::array<std::int32_t, 2>({0, 0}),
                  "all z rows and five categories clear");
    check(result.candidate->state.yearly_statistics[1] == expected,
          "i clearing z does not clear annual accumulated C");
    auto out_of_range = original;
    out_of_range.monthly_cash[0][0][0] = std::numeric_limits<std::int32_t>::max();
    check(step(out_of_range, WorldCalendarStage::year_statistics, 0, 3).candidate.has_value(),
          "D returns before reading annual sums if year>=C.length, does not invent a row");
    check(step(out_of_range, WorldCalendarStage::year_statistics).error ==
              CalendarMaintenanceError::overflow,
          "annual int accumulation overflow rejected without corrupting input");
}
void scalar_month() {
    auto original = fixture();
    auto result = step(original, WorldCalendarStage::month_kill_reset);
    check(result.candidate &&
              result.candidate->state.monster_month_kills == std::vector<std::int32_t>({0, 0, 0}),
          "bw.u clears without changing other catalogs");
    result = step(original, WorldCalendarStage::month_resident_countdown);
    check(result.candidate->state.humans[0].leave_months == 1 &&
              result.candidate->state.humans[1].leave_months == -1 &&
              result.candidate->state.humans[2].leave_months == -1,
          "resident m decrements positive values only when p!=0");
    result = step(original, WorldCalendarStage::month_resident_presence);
    check(result.candidate->state.humans[0].absent_months == 0 &&
              result.candidate->state.humans[1].absent_months == 9 &&
              result.candidate->state.humans[2].absent_months == 9,
          "presence uses actor-definition membership, repeated active refs do not double count");
    result = step(original, WorldCalendarStage::month_equipment);
    check(result.candidate->state.humans[0].equipment[0] == 1 &&
              result.candidate->state.humans[1].equipment[0] == 1 &&
              result.candidate->state.humans[2].equipment[0] == 2,
          "all present bv get A decrement, including absent actors; p0 does not");
    result = step(original, WorldCalendarStage::month_facility_definitions);
    check(result.candidate->state.facility_definitions[0].month_counter == 30 &&
              result.candidate->state.facility_definitions[1].month_counter == 20 &&
              result.candidate->state.facility_definitions[2].month_counter == 20,
          "all br.N reset to30 only category2, otherwise20");
    result = step(original, WorldCalendarStage::month_facility_age);
    check(result.candidate->state.facilities[0].month_age == 10 &&
              result.candidate->state.facilities[2].month_age == 5,
          "every g.y1 increments without facility status or occupancy filter");
    result = step(original, WorldCalendarStage::month_kill_display_clear);
    check(result.candidate->state.kill_display.empty() &&
              result.candidate->state.monster_month_kills[1] == 7,
          "G only clears N, does not redo u reset or rewards");
    for (int month = 0; month < 12; ++month) {
        result = step(original, WorldCalendarStage::month_quarter_items, month);
        const bool quarter = month == 2 || month == 5 || month == 8 || month == 11;
        check(result.candidate->state.quarter_counter == (quarter ? 12 : -1) &&
                  result.candidate->state.item_flags[0] == (quarter ? 0u : 4u) &&
                  result.candidate->state.item_flags[1] == (quarter ? 1u : 5u) &&
                  result.candidate->state.item_flags[2] == (quarter ? 0xFFFFFFFBu : 0xFFFFFFFFu),
              "only quarterly months rewrite q and clear flag4 while preserving all other bits");
    }
    original.humans[1].absent_months = std::numeric_limits<std::int32_t>::max();
    check(step(original, WorldCalendarStage::month_resident_presence).error ==
                  CalendarMaintenanceError::overflow &&
              original.humans[0].absent_months == 7,
          "late aq overflow returns no partial first-person reset");
    original = fixture();
    original.facilities.back().month_age = std::numeric_limits<std::int32_t>::max();
    check(step(original, WorldCalendarStage::month_facility_age).error ==
                  CalendarMaintenanceError::overflow &&
              original.facilities.front().month_age == 9,
          "late facility age overflow keeps all input ages");
}
void housing_and_scripts() {
    auto original = fixture();
    auto result = step(original, WorldCalendarStage::month_housing_tax, 3);
    check(result.candidate && result.candidate->state.humans[0].legacy_G == 590 &&
              result.candidate->state.humans[1].legacy_G == 290 &&
              result.candidate->state.humans[2].legacy_G == 7,
          "resident job7 uses60%, others30%, toward-zero tens; p0 keeps oldG");
    const auto &requests = result.candidate->pending_requests;
    check(requests.size() == 3 && requests[0].kind == CalendarMaintenanceRequestKind::script &&
              requests[0].code == 122 && requests[1].kind == CalendarMaintenanceRequestKind::page &&
              requests[1].code == 90 && requests[2].code == 123 && requests[2].number == 880,
          "tax has exact script122/page90/script123 pending sequence, not eager cash payment");
    check(result.candidate->state.monthly_cash == original.monthly_cash,
          "housing G calculation does not pay monthly ledger or global cash");
    for (int month = 0; month < 12; ++month)
        check(step(original, WorldCalendarStage::month_housing_tax, month, 0)
                  .candidate->pending_requests.empty(),
              "year0 never queues tax display");
    original.humans[0].legacy_F = 1;
    original.humans[1].legacy_F = 1;
    check(step(original, WorldCalendarStage::month_housing_tax, 3)
                  .candidate->pending_requests.size() == 3,
          "eligible resident queues scripts even when totalG rounds to0");
    result = step(original, WorldCalendarStage::month_all_residents_script);
    check(result.candidate->pending_requests.size() == 1 &&
              result.candidate->pending_requests[0].code == 203 &&
              !result.candidate->state.event203_seen,
          "203 request is not fake event-registration completion");
    original.humans[2].residence_status = 0;
    check(step(original, WorldCalendarStage::month_all_residents_script)
              .candidate->pending_requests.empty(),
          "p0 definition still blocks all-D2==1 script203");
    original = fixture();
    original.event203_seen = true;
    check(step(original, WorldCalendarStage::month_all_residents_script)
              .candidate->pending_requests.empty(),
          "registered203 does not queue again");
    original = fixture();
    original.humans[1].legacy_F = std::numeric_limits<std::int32_t>::max();
    check(step(original, WorldCalendarStage::month_housing_tax, 3).error ==
                  CalendarMaintenanceError::overflow &&
              original.humans[0].legacy_G == 7,
          "tax multiplication overflow rejects whole candidate and earlierG writes");
}
void restock() {
    auto original = fixture();
    original.shop_refresh_enabled = true;
    original.shop_items = {{0, 0, 10, 0, 0, 8, false},  {1, 0, 2, 0, 1, 9, false},
                           {2, 9, 10, 0, 0, 8, false},  {3, -1, 10, 0, 0, 8, false},
                           {4, 0, 10, 10, 1, 8, false}, {5, 0, 10, 0, 1, 8, false}};
    const auto result = step(original, WorldCalendarStage::month_shop_restock, 1);
    check(result.candidate && result.candidate->state.random.draws() == 3 &&
              result.candidate->state.shop_items[0].quantity == 4 &&
              result.candidate->state.shop_items[1].quantity == 0 &&
              result.candidate->state.shop_items[5].quantity == 5,
          "H consumes one draw3 per eligible shortage, including clamp-to0, skipping rank/-1/full");
    check(result.candidate->state.shop_items[0].presence == 1 &&
              result.candidate->state.shop_items[0].legacy_q == 0 &&
              result.candidate->state.shop_items[0].newly_available &&
              result.candidate->state.shop_items[5].legacy_q == 8 &&
              !result.candidate->state.shop_items[5].newly_available,
          "new shop definition b() writes p1/q0/rtrue; existing item preserves q/r");
    check(result.candidate->pending_requests.size() == 1 &&
              result.candidate->pending_requests[0].kind == CalendarMaintenanceRequestKind::hint &&
              result.candidate->pending_requests[0].code == 17 &&
              result.candidate->pending_requests[0].delay == 1 && original.random.draws() == 0,
          "new availability takes hint17 precedence over ordinary restock16, owner not consumed");
    for (int month = 0; month < 12; ++month) {
        const auto inactive = step(original, WorldCalendarStage::month_shop_restock, month);
        check(inactive.candidate->state.random.draws() == ((month + 1) % 3 == 2 ? 3u : 0u),
              "H calendar qualification matches final raw-month modulo, not quarterly stock flags");
    }
    original.shop_refresh_enabled = false;
    check(
        step(original, WorldCalendarStage::month_shop_restock, 1).candidate->state.random.draws() ==
            0,
        "UserData bit16 required before any stock randomness");
    original.shop_refresh_enabled = true;
    original.random = WorldRandomStream::from_raw({1});
    check(step(original, WorldCalendarStage::month_shop_restock, 1).error ==
                  CalendarMaintenanceError::random_failed &&
              original.shop_items[0].quantity == 0 && original.random.draws() == 0,
          "late raw exhaustion rolls back earlier new unlock and raw cursor");
    original = fixture();
    original.shop_refresh_enabled = true;
    original.shop_items = {{0, 0, 10, 0, 1, 8, false}};
    const auto ordinary = step(original, WorldCalendarStage::month_shop_restock, 1);
    check(ordinary.candidate && ordinary.candidate->pending_requests.size() == 1 &&
              ordinary.candidate->pending_requests[0].code == 16 &&
              ordinary.candidate->pending_requests[0].delay == 1 &&
              ordinary.candidate->state.shop_items[0].legacy_q == 8,
          "ordinary restock emits delayed hint16 and does not re-unlock present item");
    original.shop_items[0].maximum_quantity = 1;
    original.random = WorldRandomStream::from_raw({0});
    const auto no_change = step(original, WorldCalendarStage::month_shop_restock, 1);
    check(no_change.candidate && no_change.candidate->state.random.draws() == 1 &&
              no_change.candidate->pending_requests.empty() &&
              no_change.candidate->state.shop_items[0].quantity == 0,
          "eligible tiny shortage consumes raw but clamped0 produces no hint");
}
void strict_owner() {
    auto original = fixture();
    original.active_human_definitions.push_back(999);
    check(step(original, WorldCalendarStage::month_equipment).error ==
              CalendarMaintenanceError::invalid_owner,
          "stale actor definition reference rejected");
    original = fixture();
    original.facilities.back().definition = 999;
    check(step(original, WorldCalendarStage::year_statistics).error ==
              CalendarMaintenanceError::invalid_owner,
          "facility must reference current shared definition");
    original = fixture();
    check(step(original, WorldCalendarStage::month_rank_check).error ==
                  CalendarMaintenanceError::unsupported_stage &&
              step(original, WorldCalendarStage::subperiod_task_generation).error ==
                  CalendarMaintenanceError::unsupported_stage,
          "unimplemented page/quest stages cannot silently count as month closure");
}
} // namespace
int main() {
    try {
        annual();
        scalar_month();
        housing_and_scripts();
        restock();
        strict_owner();
        std::cout << "world_calendar_maintenance: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_calendar_maintenance: " << e.what() << '\n';
        return 1;
    }
}
