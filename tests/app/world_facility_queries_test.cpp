// Real empty startup for integration; explicit geometry/economy fixtures test source
// boundaries without simulating visits or claiming natural construction acceptance.
#include "ark/app/world_facility_queries.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>

namespace {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace rules = sim::rules;
using State = sim::StartupWorldRuntimeState;
using Error = app::WorldFacilityQueryError;
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
const sim::StartupDefinition &definition(const State &s, int id) {
    const auto it = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                 [id](const auto &d) { return d.id == id; });
    if (it == s.rules->facilities.end())
        throw std::runtime_error("Test source definition missing");
    return *it;
}
std::uint64_t instance(const State &s, int def) {
    for (const auto &f : s.scene.world.world.facilities)
        if (f.second.placement.definition_id == def)
            return f.first;
    throw std::runtime_error("Test startup instance missing");
}
app::WorldFacilityDetail query(const State &s, std::uint64_t id) {
    const auto result = app::query_world_facility_detail(s, id);
    check(result.error == Error::none && result.detail.has_value(), "Facility query failed");
    return *result.detail;
}
void rejected(const State &s, std::uint64_t id, Error error) {
    const auto result = app::query_world_facility_detail(s, id);
    check(result.error == error && !result.detail, "Incorrect query rejection or partial result");
}
// e8f66d9 initializes the full neighbour cache at startup. A pure query must preserve
// every field and source order, regardless of whether its caller supplied an empty cache.
bool same_neighbourhood_details(
    const std::map<std::uint64_t, rules::WorldMapNeighbourCache> &actual,
    const std::map<std::uint64_t, rules::WorldMapNeighbourCache> &before) {
    if (actual.size() != before.size())
        return false;
    return std::equal(
        actual.begin(), actual.end(), before.begin(), [](const auto &a, const auto &b) {
            if (a.first != b.first)
                return false;
            const auto &left = a.second;
            const auto &right = b.second;
            return left.current == right.current && left.previous == right.previous &&
                   left.visited == right.visited && left.notices == right.notices &&
                   left.sources.size() == right.sources.size() &&
                   std::equal(left.sources.begin(), left.sources.end(), right.sources.begin(),
                              [](const auto &source, const auto &saved) {
                                  return source.instance_id.value == saved.instance_id.value &&
                                         source.definition_id == saved.definition_id;
                              });
        });
}
State geometry_fixture() {
    auto s = ark::test::initial_world();
    auto &world = s.scene.world.world;
    world.facilities.clear();
    s.scene.world.facility_order.clear();
    s.neighbourhood.clear();
    s.neighbourhood_details.clear();
    s.facility_monthly_cash.clear();
    world.map = {8, 8, std::vector<rules::LegacyMapCell>(64)};
    world.map.cells[2 * 8 + 3].legacy_state = 3;
    world.map.cells[2 * 8 + 3].category = rules::RouteCategory::road;
    std::vector<rules::BoundFacility> bound;
    const auto add = [&](std::uint64_t id, int def, rules::Position at) {
        const auto &d = definition(s, def);
        rules::RescueFacility f;
        f.placement = {{id},
                       def,
                       static_cast<rules::FacilityShape>(d.shape),
                       rules::FacilityOrientation::first,
                       at};
        f.kind = d.kind;
        f.category = d.category;
        f.detail = d.detail;
        world.facilities.emplace(id, f);
        s.scene.world.facility_order.push_back(id);
        s.facility_monthly_cash.emplace(id, std::array<std::array<int, 2>, 12>{});
        bound.push_back({f.placement, d.kind});
    };
    add(101, 28, {3, 3});
    add(102, 29, {4, 2}); // Two occupied cells border the target; contributes once.
    add(103, 66, {2, 2});
    add(104, 66, {2, 4}); // Same definition, different instance: both contribute.
    const auto map = rules::bind_facility_map(world.map, bound);
    check(map.map.has_value(), "Geometry fixture binding rejected");
    world.map = *map.map;
    s.neighbourhood[101] = {40, 10, 12}; // Two sunflowers + one inn + one road cell.
    return s;
}
void startup_and_current_economy() {
    auto s = ark::test::initial_world();
    const auto inn = instance(s, 28);
    const auto before = s;
    const auto first = query(s, inn);
    check(first.type == app::WorldFacilityTemplate::ordinary && first.definition == 28 &&
              first.level == 1 && first.status == 1 && !first.construction,
          "Real inn classification/progress incorrect");
    check(!first.neighbours.empty(), "Startup source neighbours were misrepresented as no bonuses");
    check(first.monthly_income == 0 && first.monthly_expense == 0 && first.cumulative_profit == 0,
          "Empty real startup has invented sales");
    check(query(s, instance(s, 30)).type == app::WorldFacilityTemplate::equipment &&
              query(s, instance(s, 66)).type == app::WorldFacilityTemplate::booster,
          "Special source templates lost classification");
    check(same_neighbourhood_details(s.neighbourhood_details, before.neighbourhood_details) &&
              s.neighbourhood == before.neighbourhood &&
              s.scene.random.draws() == before.scene.random.draws() &&
              s.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds() &&
              s.facility_monthly_cash == before.facility_monthly_cash &&
              s.scripts.event_calls == before.scripts.event_calls &&
              ark::test::same_world_clock(s, before),
          "Pure query mutated world/caches/cash/random");

    auto &use = s.scene.world.world.facility_uses.at(28);
    use.level = 2;
    use.completed_uses = 1000; // Keep negative d()-K; do not clamp it to zero.
    use.upgrade_pending = false;
    s.scripts.facilities.at(28).level = 5; // Deliberately stale projections must not win.
    s.scripts.facilities.at(28).attributes = {-123, -456, -789, -111};
    s.scene.world.world.facilities.at(inn).price = -999;
    s.scripts.facilities.at(28).improvements = {37, 2, 4, 13};
    s.scripts.job_counts[2] = 3;
    s.scripts.job_counts[0] = 2;
    rules::FacilityEconomyInput input;
    input.level = 2;
    input.completed_definition_uses = 1000;
    input.definition_improvements = s.scripts.facilities.at(28).improvements;
    input.legacy_job_counts = s.scripts.job_counts;
    std::copy(s.neighbourhood.at(inn).begin(), s.neighbourhood.at(inn).end(),
              input.instance_modifiers.begin());
    const auto expected = rules::derive_facility_economy(definition(s, 28).economy, input);
    check(expected.values.has_value(), "Explicit economy fixture invalid");
    const auto current = query(s, inn);
    check(current.level == 2 && current.attributes == expected.values->instance_attributes &&
              current.attributes != first.attributes,
          "Query used initial values or cached price/level instead of current source rules");
    check(current.remaining_uses == expected.values->upgrade_uses - 1000 &&
              *current.remaining_uses < 0 && !current.upgrade_pending,
          "Readiness replaced pending flag or remaining uses were clamped");
    check(current.attributes[3] == expected.values->definition_attributes[3],
          "Neighbourhood incorrectly increased maintenance");
    use.level = 5;
    check(!query(s, inn).remaining_uses, "MAX level displays remaining uses");
}
void sources_and_staleness() {
    auto s = geometry_fixture();
    const auto d = query(s, 101);
    check(d.neighbours.size() == 3 && d.neighbours[0].instance == 102 &&
              d.neighbours[1].instance == 103 && d.neighbours[2].instance == 104,
          "Source order, footprint deduplication or same-definition instances lost");
    check(d.neighbour_modifiers == std::array<int, 3>{40, 10, 12},
          "Source neighbours/road charm incorrect");
    s.scene.world.world.facility_uses.at(66).level = 3;
    check(query(s, 101).neighbours[1].level == 3 && query(s, 101).neighbours[2].level == 3,
          "Neighbour labels do not use shared definition level");
    rules::WorldMapNeighbourCache cache;
    cache.current = d.neighbour_modifiers;
    for (const auto &source : d.neighbours)
        cache.sources.push_back({{source.instance}, source.definition});
    s.neighbourhood_details.emplace(101, cache);
    check(query(s, 101).neighbours.size() == 3, "Consistent source cache rejected");
    s.neighbourhood_details.at(101).sources.clear();
    rejected(s, 101, Error::inconsistent_neighbourhood);
    s.neighbourhood_details.at(101) = cache;
    std::swap(s.neighbourhood_details.at(101).sources[0],
              s.neighbourhood_details.at(101).sources[1]);
    rejected(s, 101, Error::inconsistent_neighbourhood);
    s.neighbourhood_details.clear();
    s.neighbourhood.at(101)[0] = 0;
    rejected(s, 101, Error::inconsistent_neighbourhood);
    s.neighbourhood.at(101)[0] = 40;
    s.scene.world.world.map.cells[2 * 8 + 3].legacy_state = 4;
    rejected(s, 101, Error::inconsistent_neighbourhood);
}
void instance_finance_and_failures() {
    auto s = ark::test::initial_world();
    const auto inn = instance(s, 28);
    auto &f = s.scene.world.world.facilities.at(inn);
    f.status = 0;
    s.dungeon_facilities.at(inn).updates = 19;
    s.facility_details.at(inn).construction_limit = 280;
    s.scripts.job_counts[9] = 3; // Later carpenter count cannot shorten this instance's contract.
    f.occupants = {{8}, {8}, {12}};
    f.sales = 7000;
    s.scene.calendar.month = 3;
    auto &cash = s.facility_monthly_cash.at(inn);
    cash[0] = {100, 400};
    cash[3] = {50, 80};
    cash[4] = {9000, 0};
    const auto d = query(s, inn);
    check(d.construction && d.construction->updates == 19 && d.construction->limit == 280,
          "Construction query re-quoted or confused counters");
    check(d.occupants.size() == 3 && d.occupants[0].value == 8 && d.occupants[1].value == 8,
          "Occupation sequence was silently deduplicated");
    check(d.monthly_income == 50 && d.monthly_expense == 80 && d.cumulative_profit == -330,
          "Monthly cash confused with cumulative sales/future month rows");
    s.scene.calendar.month = 0;
    check(query(s, inn).cumulative_profit == -300, "New year query included future month rows");
    rejected(s, 99999, Error::missing_instance);
    auto bad = s;
    bad.rules = nullptr;
    rejected(bad, inn, Error::missing_source);
    bad = s;
    bad.neighbourhood.erase(inn);
    rejected(bad, inn, Error::missing_source);
    bad = s;
    bad.facility_monthly_cash.erase(inn);
    rejected(bad, inn, Error::missing_source);
    bad = s;
    bad.facility_details.erase(inn);
    rejected(bad, inn, Error::missing_source);
    bad = s;
    bad.scene.calendar.month = 12;
    rejected(bad, inn, Error::invalid_state);
    bad = s;
    bad.scene.world.facility_order.push_back(inn);
    rejected(bad, inn, Error::invalid_state);
    bad = s;
    for (auto &cell : bad.scene.world.world.map.cells)
        if (cell.facility && cell.facility->instance_id.value == inn) {
            cell.facility->definition_id = 66;
            break;
        }
    rejected(bad, inn, Error::invalid_state);
    bad = s;
    bad.scene.world.world.facility_uses.at(28).completed_uses = -1;
    rejected(bad, inn, Error::invalid_state);

    auto catalog = *s.rules;
    auto &raw = *std::find_if(catalog.facilities.begin(), catalog.facilities.end(),
                              [](const auto &x) { return x.id == 28; });
    raw.economy.attributes[0] = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
    raw.economy.legacy_flags = 4096U | 131072U;
    s.rules = &catalog;
    s.scripts.job_counts[2] = std::numeric_limits<int>::max();
    s.scripts.job_counts[0] = std::numeric_limits<int>::max();
    rejected(s, inn, Error::numeric_overflow);
}
} // namespace
int main() {
    try {
        startup_and_current_economy();
        sources_and_staleness();
        instance_finance_and_failures();
        std::cout << checks << " world facility query checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
