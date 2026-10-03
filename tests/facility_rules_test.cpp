// Real-table golden values and independent ring-set checks. Synthetic layouts are test-only,
// never player startup data. Release uses explicit checks rather than disabled assertions.
#include "ark/app/game.hpp"
#include <climits>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace ark;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F run) {
    bool rejected = false;
    try {
        run();
    } catch (const std::exception &) {
        rejected = true;
    }
    require(rejected, "invalid query accepted");
}
void economy() {
    app::Game game;
    auto d = game.definition(28).economy;
    const std::int64_t prices[] = {300, 337, 375, 412, 450};
    const std::int64_t fees[] = {240, 300, 360, 420, 480};
    const std::int64_t uses[] = {50, 162, 275, 387, 500};
    for (int level = 1; level <= 5; ++level) {
        facilities::EconomyInput input;
        input.level = level;
        const auto v = facilities::derive_economy(d, input);
        require(v.definition[0] == prices[level - 1] && v.definition[3] == fees[level - 1] &&
                    v.upgrade_uses == uses[level - 1],
                "inn level interpolation");
    }
    facilities::EconomyInput input;
    d.flags = 4096 | 8192 | 131072 | 262144;
    input.improvements = {4, 3, -9, 9};
    input.modifiers = {17, 4, 5};
    input.job_counts[2] = input.job_counts[0] = 1;
    input.job_counts[3] = input.job_counts[1] = 2;
    auto v = facilities::derive_economy(d, input);
    require(v.definition == std::array<std::int64_t, 4>{400, 8, -4, 240} &&
                v.instance == std::array<std::int64_t, 4>{417, 12, 1, 240},
            "flag priority/separate truncations/signed modifiers");
    input.improvements = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
    input.modifiers = {INT_MAX, INT_MAX, INT_MAX};
    v = facilities::derive_economy(d, input);
    require(v.instance == std::array<std::int64_t, 4>{900, 100, 100, 960},
            "definition and instance caps");
    input = {};
    input.improvements[3] = -365;
    require(facilities::derive_economy(game.definition(28).economy, input).instance[3] == -120,
            "negative fee rounds toward zero without invented floor");
    input = {};
    input.job_counts = app::startup_data().initial_job_counts;
    v = facilities::derive_economy(game.definition(66).economy, input);
    require(v.construction_cost == 200, "two unlocked farmer definitions discount");
    require(facilities::derive_economy(game.definition(28).economy, input).construction_ticks ==
                280,
            "one unlocked carpenter accelerates");
    input.completed_uses = 50;
    require(facilities::derive_economy(game.definition(28).economy, input).upgrade_ready &&
                input.level == 1,
            "readiness is not an upgrade");
    input.level = 5;
    require(!facilities::derive_economy(game.definition(28).economy, input).upgrade_ready,
            "max level guard");
    input.level = 0;
    rejects([&] { facilities::derive_economy(d, input); });
    input = {};
    input.job_counts[0] = -1;
    rejects([&] { facilities::derive_economy(d, input); });
    input = {};
    d.attributes[0] = {INT_MAX, INT_MAX};
    input.improvements[0] = input.job_counts[2] = INT_MAX;
    rejects([&] { facilities::derive_economy(d, input); });
}
void rings() {
    const world::SourceMap map{8, 8, {}};
    const std::vector<world::Cell> single = {{3, 4}, {4, 4}, {4, 3}, {4, 2},
                                             {3, 2}, {2, 2}, {2, 3}, {2, 4}};
    require(facilities::surroundings(0, 0, {3, 3}, map) == single, "ordered single ring");
    for (int shape = 0; shape < 3; ++shape)
        for (int orientation = 0; orientation < 2; ++orientation)
            for (int y = 0; y < 8; ++y)
                for (int x = 0; x < 8; ++x) {
                    const auto footprint = facilities::footprint(shape, orientation, {x, y});
                    bool valid = true;
                    for (const auto &part : footprint)
                        valid = valid && map.contains(part.cell);
                    if (!valid)
                        continue;
                    std::set<world::Cell> occupied, expected;
                    for (const auto &part : footprint)
                        occupied.insert(part.cell);
                    for (const auto &part : footprint)
                        for (int dy = -1; dy <= 1; ++dy)
                            for (int dx = -1; dx <= 1; ++dx) {
                                const world::Cell p{part.cell.x + dx, part.cell.y + dy};
                                if (map.contains(p) && !occupied.count(p))
                                    expected.insert(p);
                            }
                    const auto ring = facilities::surroundings(shape, orientation, {x, y}, map);
                    require(std::set<world::Cell>(ring.begin(), ring.end()) == expected &&
                                ring.size() == expected.size(),
                            "ring differs from independent eight-neighbour set");
                }
}
void neighbours() {
    const world::SourceMap map{8, 8, {}};
    auto definitions = app::startup_data().definitions;
    std::map<facilities::InstanceId, facilities::Instance> layout = {
        {1, {1, 29, {3, 3}, 0, 0, false}},
        {2, {2, 66, {4, 3}, 0, 0, false}},
        {3, {3, 66, {4, 4}, 0, 0, false}},
        {4, {4, 28, {3, 5}, 0, 0, false}}};
    const std::vector<world::Cell> roads = {{2, 3}, {2, 4}};
    const auto values = facilities::derive_neighbourhood(definitions, layout, roads, map);
    require(values.at(1).modifiers == std::array<std::int64_t, 3>{40, 10, 14} &&
                values.at(1).sources.size() == 3 && values.at(1).road_cells == 2,
            "pair target deduplication/same-definition stacking/diagonal roads");
    require(values.at(4).modifiers == std::array<std::int64_t, 3>{20, 5, 12},
            "source pair counted once");
    const auto again = facilities::derive_neighbourhood(definitions, layout, roads, map);
    require(again.at(1).modifiers == values.at(1).modifiers, "recompute does not accumulate");
    facilities::EconomyInput input;
    input.modifiers = {999, 999, 999};
    const auto replaced = facilities::with_neighbours(input, values.at(1));
    require(replaced.modifiers == std::array<std::int32_t, 3>{40, 10, 14}, "snapshot replacement");
    layout.erase(3);
    require(facilities::derive_neighbourhood(definitions, layout, roads, map).at(1).modifiers[0] ==
                20,
            "removing source reduces next derived view");
    auto bad = layout;
    bad.at(2).id = 99;
    rejects([&] { facilities::derive_neighbourhood(definitions, bad, roads, map); });
    rejects([&] { facilities::derive_neighbourhood(definitions, layout, {{2, 3}, {2, 3}}, map); });
    rejects([&] { facilities::derive_neighbourhood(definitions, layout, {{3, 3}}, map); });
    rejects([&] { facilities::derive_neighbourhood(definitions, layout, {{-1, 0}}, map); });
    bad = layout;
    bad.at(2).anchor = {3, 4};
    rejects([&] { facilities::derive_neighbourhood(definitions, bad, roads, map); });
    for (auto &d : definitions)
        if (d.id == 66)
            d.neighbours = {{0, INT64_MAX}, {0, 1}};
    rejects([&] { facilities::derive_neighbourhood(definitions, layout, roads, map); });
    facilities::Neighbourhood too_large;
    too_large.modifiers[0] = static_cast<std::int64_t>(INT_MAX) + 1;
    rejects([&] { facilities::with_neighbours(input, too_large); });
}
void queries() {
    app::Game game;
    const auto seed = *game.facility_at({10, 5});
    require(game.facility_values(28).definition == std::array<std::int64_t, 4>{300, 5, 5, 240},
            "definition preview");
    require(game.facility_values(28, seed).instance == std::array<std::int64_t, 4>{300, 5, 16, 240},
            "source inn values");
    require(game.neighbourhood(seed).road_cells == 3 &&
                game.neighbourhood(seed).sources.size() == 1,
            "source snapshot identity and roads");
    rejects([&] { game.facility_values(30, seed); });
    rejects([&] { game.neighbourhood(999); });
    game.open_catalog();
    game.select(28);
    game.confirm({7, 3});
    game.cancel();
    const auto target = *game.facility_at({7, 3});
    const auto base = game.facility_values(28, target).instance;
    game.open_catalog();
    game.select(66);
    game.confirm({8, 3});
    game.cancel();
    const auto modified = game.facility_values(28, target).instance;
    require(modified[0] == base[0] + 20 && modified[1] == base[1] + 5 && modified[3] == base[3],
            "plant affects instance, not fee");
    require(game.facility_values(28).definition[0] == 300 &&
                game.facility_values(28, seed).instance[0] == 300,
            "definition and other instance isolated");
    const auto money = game.state().money;
    const auto count = game.state().facilities.size();
    for (int i = 0; i < 10; ++i)
        game.facility_values(28, target);
    require(game.state().money == money && game.state().facilities.size() == count &&
                game.state().definition_progress.at(28).completed_uses == 0 &&
                !game.state().definition_progress.at(28).upgrade_pending,
            "queries have no business side effects");
    for (const auto &d : app::startup_data().definitions)
        if (d.tab >= 0) {
            const auto values = game.facility_values(d.id);
            require(values.construction_cost == d.price &&
                        ((d.economy.flags & 64) ? values.construction_ticks : 0) ==
                            d.construction_ticks,
                    "initial published quotes agree with pure derivation");
        }
    require(game.definition(54).effect_icons.size() == 1 &&
                game.definition(54).effect_markers.size() == 2,
            "unopened nonparallel source lists retained");
}
} // namespace
int main() {
    economy();
    rings();
    neighbours();
    queries();
    std::cout
        << "PASS facility arithmetic, ordered rings, source identities and read-only queries\n";
}
