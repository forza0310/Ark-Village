#include "dungeon_village_prototype/village.hpp"

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace dungeon_village_prototype;
using namespace dungeon_village_reference;

namespace {
int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

LegacyMap terrain() {
    LegacyMap map{7, 7, std::vector<LegacyMapCell>(49)};
    for (int y = 0; y < 7; ++y) {
        for (int x = 0; x < 7; ++x) {
            if (x == 3 || y == 3)
                map.cells[static_cast<std::size_t>(y * 7 + x)] = {3, RouteCategory::road,
                                                                  std::nullopt};
        }
    }
    return map;
}

std::string snapshot(const Village &village) {
    std::ostringstream out;
    const auto &state = village.state();
    out << state.accounting.funds() << ',' << state.accounting.village_points() << ','
        << state.next_instance << ',' << state.next_activity << ',' << state.next_event << ','
        << state.random_state << ',' << state.ticks << ',' << state.period << ','
        << state.remainder_ms << ',' << state.paused;
    for (const auto &entry : state.facilities) {
        const auto &p = entry.second;
        out << ';' << entry.first.value << ',' << p.definition_id << ',' << p.anchor.x << ','
            << p.anchor.y << ',' << static_cast<int>(p.shape) << ','
            << static_cast<int>(p.orientation);
    }
    for (const auto &entry : state.actors) {
        const auto &a = entry.second;
        out << ';' << a.id.value << ',' << a.cell.x << ',' << a.cell.y << ',' << a.exit_cell.x
            << ',' << a.exit_cell.y << ',' << static_cast<int>(a.activity) << ','
            << (a.target ? a.target->value : 0) << ',' << a.activity_id << ',' << a.cursor << ','
            << a.phase_ticks << ',' << a.wait_ticks << ',' << a.flags << ',' << a.arrivals << ','
            << a.completed << ',' << a.cancelled << ',' << a.arrival.legacy_actor_total << ','
            << a.arrival.current_month_facility_sales << ',' << a.arrival.legacy_category_one_count
            << ',' << a.arrival.legacy_category_six_counter << ','
            << (a.arrival.last_visited_instance ? a.arrival.last_visited_instance->value : 0);
        for (const auto v : a.arrival.legacy_visit_counts)
            out << ',' << v;
        for (const auto p : a.path)
            out << ',' << p.x << ',' << p.y;
    }
    for (const auto &entry : state.monthly_sales)
        out << ';' << entry.first.value << ',' << entry.second;
    for (const auto &entry : state.completed_definition_uses)
        out << ';' << entry.first << ',' << entry.second;
    for (const auto &entry : state.accounting.entries()) {
        const auto &e = entry.second;
        out << ';' << e.event_id << ',' << e.period << ',' << static_cast<int>(e.category) << ','
            << static_cast<int>(e.direction) << ',' << e.amount;
    }
    for (const auto &entry : state.accounting.reports()) {
        const auto &r = entry.second;
        out << ';' << entry.first << ',' << r.claimed << ',' << r.displayed.income << ','
            << r.displayed.expense << ',' << r.displayed_net << ',' << r.pending_points << ','
            << r.awarded_points;
        for (const auto &c : r.categories)
            out << ',' << c.income << ',' << c.expense;
        for (const auto &c : r.input.charges)
            out << ',' << c.event_id << ',' << c.amount;
    }
    return out.str();
}

Village populated(const std::vector<PrototypeDefinition> &catalog, PrototypeConfig config = {}) {
    Village village(catalog, terrain(), 10000, config);
    check(village.place(28, {2, 3}, FacilityOrientation::first).error == VillageError::none,
          "place inn");
    check(village.place(29, {4, 3}, FacilityOrientation::first).error == VillageError::none,
          "place pair");
    check(village.place(36, {3, 2}, FacilityOrientation::first).error == VillageError::none,
          "place cafe");
    check(village.add_actor({1}, {0, 3}) == VillageError::none, "add actor one");
    check(village.add_actor({2}, {6, 3}) == VillageError::none, "add actor two");
    return village;
}

void catalog_geometry_and_commands(const std::vector<PrototypeDefinition> &catalog) {
    check(catalog.size() == 3, "three recovered definitions");
    auto village = populated(catalog);
    check(village.state().accounting.funds() == 7300, "actual construction prices");
    check(village.values({1}).instance_attributes[0] == 300 &&
              village.values({3}).definition_attributes[3] == 190,
          "actual price and rounded upkeep");
    check(village.facility_at({4, 4}) == std::optional<BuildingId>{{2}} &&
              village.facility_at({4, 3}) == std::optional<BuildingId>{{2}},
          "pair owns both cells");
    const auto before = snapshot(village);
    check(village.place(36, {4, 4}, FacilityOrientation::first).error ==
                  VillageError::invalid_placement &&
              snapshot(village) == before,
          "overlap rejects with full state unchanged");
    check(village.place(29, {0, 0}, FacilityOrientation::second).error ==
                  VillageError::invalid_placement &&
              snapshot(village) == before,
          "pair left boundary rejects");
    check(village.place(28, {0, 3}, FacilityOrientation::first).error ==
                  VillageError::actor_occupied &&
              snapshot(village) == before,
          "actor overlap rejects");
    check(village.place(99, {0, 0}, FacilityOrientation::first).error == VillageError::not_found &&
              snapshot(village) == before,
          "unknown definition rejects");
    check(village.relocate({2}, {4, 3}, FacilityOrientation::first) == VillageError::none &&
              snapshot(village) == before,
          "no-op relocation has no cost or cancellation");
    check(village.relocate({2}, {5, 5}, FacilityOrientation::second) == VillageError::none &&
              village.state().accounting.funds() == 7000 &&
              village.facility_at({4, 5}) == std::optional<BuildingId>{{2}},
          "stable ID, second orientation and move cost");
    const auto funds = village.state().accounting.funds();
    const auto entries = village.state().accounting.entries().size();
    check(village.demolish({2}) == VillageError::none &&
              village.state().accounting.funds() == funds &&
              village.state().accounting.entries().size() == entries &&
              !village.facility_at({4, 5}),
          "demolition has zero refund and releases every cell");
    Village poor(catalog, terrain(), 100);
    const auto poor_before = snapshot(poor);
    check(poor.place(28, {1, 1}, FacilityOrientation::first).error ==
                  VillageError::insufficient_funds &&
              snapshot(poor) == poor_before,
          "insufficient construction funds atomic");
    check(village.add_actor({1}, {0, 0}) == VillageError::invalid_input, "duplicate actor rejects");
}

void autonomous_periods_and_atomic_time(const std::vector<PrototypeDefinition> &catalog) {
    PrototypeConfig config{100, 1, 2, 1, 100};
    auto one = populated(catalog, config);
    auto many = one;
    check(one.advance(25000) == VillageError::none, "batched advance");
    for (int i = 0; i < 250; ++i) {
        check(many.advance(100) == VillageError::none, "fixed advance");
        const auto &first = many.state().actors.at({1});
        const auto &second = many.state().actors.at({2});
        check(!first.target || !second.target || !(first.target == second.target),
              "exclusive fixture reservation");
    }
    check(snapshot(one) == snapshot(many),
          "complete state, RNG and ledger invariant to frame batching");
    check(one.state().actors.at({1}).completed > 0 && one.state().actors.at({2}).completed > 0,
          "both actors choose, arrive, use and repeat without commands");
    check(one.state().accounting.reports().size() == 2 && one.state().period == 3,
          "two fixture months");
    std::int64_t expected = 10000;
    for (const auto &entry : one.state().accounting.entries()) {
        expected += entry.second.direction == CashDirection::income ? entry.second.amount
                                                                    : -entry.second.amount;
    }
    check(expected == one.state().accounting.funds(),
          "cash equals individual events, no report net repayment");
    for (const auto &entry : one.state().accounting.reports()) {
        check(entry.second.displayed.expense == 670 && entry.second.claimed,
              "three actual fees, once per month");
    }
    const auto stable = snapshot(one);
    check(one.advance(0) == VillageError::none && snapshot(one) == stable, "zero delta no replay");
    check(one.advance(-1) == VillageError::invalid_input && snapshot(one) == stable,
          "invalid delta preserves state");
    one.set_paused(true);
    const auto paused = snapshot(one);
    check(one.advance(60000) == VillageError::none && snapshot(one) == paused,
          "pause freezes time and ledger");
    check(one.advance(60001) == VillageError::invalid_input && snapshot(one) == paused,
          "paused invalid input still rejects");
}

void arrival_free_and_cancel(const std::vector<PrototypeDefinition> &catalog) {
    Village village(catalog, terrain(), 10000, {100, 1, 5, 1, 1000});
    const auto id = village.place(28, {2, 3}, FacilityOrientation::first).instance;
    check(village.add_actor({1}, {0, 3}) == VillageError::none, "single paid actor");
    for (int i = 0; i < 100 && village.state().actors.at({1}).activity != ActivityState::in_use;
         ++i)
        check(village.advance(100) == VillageError::none, "advance to paid arrival");
    check(village.state().actors.at({1}).activity == ActivityState::in_use &&
              village.state().actors.at({1}).arrivals == 1 &&
              village.state().monthly_sales.at(*id) == 300 &&
              village.state().accounting.funds() == 9300,
          "charge at arrival, not at finish");
    const auto &actor = village.state().actors.at({1});
    const auto cell = actor.cell;
    const auto stable = snapshot(village);
    check(village.relocate(*id, cell, FacilityOrientation::second) ==
                  VillageError::actor_occupied &&
              snapshot(village) == stable,
          "failed edit does not cancel or charge again");
    check(village.demolish(*id) == VillageError::none &&
              village.state().actors.at({1}).cancelled == 1 &&
              village.state().actors.at({1}).completed == 0 &&
              village.state().accounting.funds() == 9300,
          "cancel after paid arrival does not refund or grant completion");
    check(village.advance(10000) == VillageError::none &&
              village.state().actors.at({1}).completed == 0,
          "no stale completion after demolition");
    Village free(catalog, terrain(), 10000, {100, 1, 2, 1, 1000});
    const auto free_id = free.place(28, {2, 3}, FacilityOrientation::first).instance;
    check(free.add_actor({1}, {0, 3}, 512) == VillageError::none, "free guard actor");
    check(free.advance(5000) == VillageError::none && free.state().actors.at({1}).arrivals > 1 &&
              free.state().actors.at({1}).completed > 1 &&
              free.state().accounting.funds() == 9000 &&
              free.state().monthly_sales.at(*free_id) == 0 &&
              free.state().actors.at({1}).arrival.legacy_actor_total == 0,
          "free arrivals count and repeat with zero income, without replay");
    auto empty = Village(catalog, terrain());
    check(empty.add_actor({1}, {0, 0}) == VillageError::none &&
              empty.advance(2000) == VillageError::none &&
              empty.state().actors.at({1}).activity == ActivityState::idle,
          "bounded no-candidate retry");
}

void overflow_and_month_edge(const std::vector<PrototypeDefinition> &catalog) {
    auto large = catalog;
    large[0].economy.attributes[0] = {std::numeric_limits<std::int32_t>::max(),
                                      std::numeric_limits<std::int32_t>::max()};
    Village overflow(large, terrain(), 10000, {100, 1, 1, 1, 1000});
    check(overflow.place(28, {2, 3}, FacilityOrientation::first).error == VillageError::none,
          "large-price fixture construction");
    check(overflow.add_actor({1}, {0, 3}) == VillageError::none &&
              overflow.advance(300) == VillageError::none &&
              overflow.state().actors.at({1}).activity == ActivityState::in_use,
          "first exact int32 price is supported");
    const auto before = snapshot(overflow);
    check(overflow.advance(1000) == VillageError::numeric_overflow && snapshot(overflow) == before,
          "later actor total overflow rolls back whole batch, RNG, counters and cash");

    Village edge(catalog, terrain(), 10000, {100, 1, 10, 1, 3});
    check(edge.place(28, {2, 3}, FacilityOrientation::first).error == VillageError::none &&
              edge.add_actor({1}, {0, 3}) == VillageError::none &&
              edge.advance(300) == VillageError::none,
          "arrival on fixture month boundary");
    const auto &report = edge.state().accounting.reports().at(1);
    check(report.displayed.income == 300 && report.displayed.expense == 240 &&
              edge.state().accounting.funds() == 9060 && edge.state().period == 2 &&
              edge.state().monthly_sales.at({1}) == 0 &&
              edge.state().actors.at({1}).activity == ActivityState::in_use,
          "arrival belongs to closing period, fee follows, sales reset without cancelling use");
}
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 2)
            throw std::invalid_argument("需要设施表路径");
        const auto catalog = load_prototype_catalog(argv[1]);
        catalog_geometry_and_commands(catalog);
        autonomous_periods_and_atomic_time(catalog);
        arrival_free_and_cancel(catalog);
        overflow_and_month_edge(catalog);
        std::cout << checks << " prototype village checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
