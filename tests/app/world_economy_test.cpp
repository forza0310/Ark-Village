// Bounded source-consumer economy comparison. This is not the active-player campaign:
// all variants use the same ordinary-page policy and differ only in legal construction.
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/village/startup_world_tax.hpp"
#include "../support/world_fixture.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
namespace sim = ark::simulation;
namespace ref = sim::rules;
using State = sim::StartupWorldRuntimeState;
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}
void good(sim::StartupWorldRuntimeError error) {
    require(error == sim::StartupWorldRuntimeError::none,
            "Runtime consumer rejected: " + std::to_string(static_cast<int>(error)));
}
const ref::WorldScriptPage &top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return *p;
    throw std::runtime_error("Missing live page");
}
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
struct Build {
    int definition, x, y;
};
const std::vector<std::vector<Build>> plans{{},
                                            {{35, 12, 6}},
                                            {{35, 10, 7}, {66, 9, 6}},
                                            {{35, 12, 6}, {66, 9, 6}},
                                            {{35, 12, 6}, {66, 9, 6}, {45, 12, 7}}};

struct Comparison {
    State state = ark::test::initial_world(1);
    std::int64_t opening = cash(state), minimum = opening, construction{};
    int steps{};
    std::set<std::uint64_t> built;
    std::map<std::uint64_t, int> income_updates;
    std::set<int> seen_pages;
    std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();

    void build(const Build &b) {
        require(top(state).kind == ref::WorldScriptPageKind::scene,
                "Construction must start from live scene");
        const auto quote = sim::startup_world_build_quote(state, b.definition);
        require(quote.has_value(), "Missing current quote");
        const auto before = cash(state);
        const auto random = state.scene.random.draws();
        std::vector<std::pair<std::size_t, int>> roads;
        const auto &initial_map = state.scene.world.world.map;
        for (std::size_t n = 0; n < initial_map.cells.size(); ++n)
            if (initial_map.cells[n].legacy_state == 3)
                roads.push_back({n, initial_map.cells[n].legacy_state});
        good(sim::open_startup_world_build_menu(state));
        const auto select =
            sim::select_startup_world_build_menu(state, top(state).id, b.definition);
        good(select.error);
        require(select.denial == sim::StartupBuildDenial::none, "Catalog selection denied");
        const auto placed =
            sim::confirm_startup_world_build(state, {b.x, b.y}, ref::FacilityOrientation::first);
        good(placed.error);
        require(placed.denial == sim::StartupBuildDenial::none && placed.created,
                "Placement denied definition=" + std::to_string(b.definition) +
                    " denial=" + std::to_string(static_cast<int>(placed.denial)));
        require(before - cash(state) == quote->construction_cost,
                "Construction must debit the actual quote exactly once");
        require(state.scene.random.draws() == random, "Construction changed random stream");
        for (const auto &[n, value] : roads)
            require(state.scene.world.world.map.cells[n].legacy_state == value &&
                        !state.scene.world.world.map.cells[n].facility,
                    "Construction altered an existing road");
        const auto &f = state.scene.world.world.facilities.at(*placed.created);
        require(f.placement.anchor.x == b.x && f.placement.anchor.y == b.y &&
                    f.placement.definition_id == b.definition,
                "Real placement identity differs from requested plan");
        construction += before - cash(state);
        minimum = std::min(minimum, cash(state));
        built.insert(*placed.created);
        std::cout << "BUILD definition=" << b.definition << " instance=" << *placed.created
                  << " x=" << b.x << " y=" << b.y << " paid=" << before - cash(state)
                  << " cash=" << cash(state) << std::endl;
        good(sim::cancel_startup_world_build(state));
    }
    void input() {
        const auto page = top(state);
        const int raw = page.legacy_page;
        if (seen_pages.insert(raw).second)
            std::cout << "PAGE raw=" << raw << " month=" << month(state) << std::endl;
        if (page.kind == ref::WorldScriptPageKind::scene || raw == 16 || raw == 24 || raw == 56 ||
            raw == 57 || raw == 97 || raw == 98)
            return;
        if (raw == 87) {
            good(sim::act_startup_world_runtime_award_page(
                state, page.id, ref::WorldAwardAction::request_termination));
            good(sim::act_startup_world_runtime_award_page(
                state, page.id, ref::WorldAwardAction::confirm_termination));
        } else if (raw == 48) {
            good(sim::act_startup_world_runtime_rank_page(state, page.id, 0, true));
        } else if (raw == 83) {
            good(sim::cancel_startup_world_runtime_page(state, page.id));
        } else if (raw == 90) {
            good(sim::act_startup_world_tax_page(state, page.id,
                                                 sim::StartupWorldTaxAction::confirm));
        } else {
            const std::set<int> ordinary{0,  1,  15, 30, 31, 32, 49, 50, 59,
                                         67, 88, 89, 94, 95, 96, 99, 100};
            require(ordinary.count(raw), "Uncovered ordinary page raw=" + std::to_string(raw));
            good(sim::acknowledge_startup_world_runtime_page(state, page.id));
        }
    }
    void step() {
        require(steps < 25000, "Economy comparison exceeded 25000 calls");
        require(std::chrono::steady_clock::now() - started < std::chrono::minutes(10),
                "Economy comparison exceeded ten minutes");
        const auto before_month = month(state);
        const auto random = state.scene.random.draws();
        auto next = sim::prepare_startup_world_runtime(state);
        require(next.candidate.has_value(),
                "Runtime update rejected at call=" + std::to_string(steps));
        require(sim::update_startup_world_render_cache(*next.candidate), "Render cache rejected");
        for (const auto &[id, f] : next.candidate->scene.world.world.facilities) {
            const auto old = state.scene.world.world.facilities.find(id);
            if (old != state.scene.world.world.facilities.end() && f.sales > old->second.sales)
                ++income_updates[id]; // Positive-sales updates, deliberately not claimed as visits.
        }
        state = std::move(*next.candidate);
        state.sound_requests.clear(); // Consume presentation output without affecting simulation.
        input();
        ++steps;
        minimum = std::min(minimum, cash(state));
        require(cash(state) >= 0, "Plan became insolvent");
        require(state.scene.random.draws() >= random, "Random stream rewound");
        require(month(state) >= before_month && month(state) <= before_month + 1 &&
                    ref::valid_world_calendar_state(state.scene.calendar),
                "Calendar discontinuity");
        if (month(state) != before_month || steps % 2000 == 0)
            std::cout << "PROGRESS month=" << month(state) << " calls=" << steps
                      << " cash=" << cash(state) << " minimum=" << minimum << std::endl;
    }
    void report(int plan) {
        std::int64_t income{}, maintenance{};
        std::cout << "RESULT {\"plan\":\"P" << plan << "\",\"seed\":1,\"start_month\":3,"
                  << "\"end_month\":" << month(state) << ",\"calls\":" << steps
                  << ",\"simulation_steps\":" << state.simulation_steps
                  << ",\"random_draws\":" << state.scene.random.draws()
                  << ",\"opening_cash\":" << opening << ",\"cash\":" << cash(state)
                  << ",\"net_cash\":" << cash(state) - opening << ",\"minimum_cash\":" << minimum
                  << ",\"construction\":" << construction << ",\"facilities\":[";
        bool first = true;
        for (const auto id : state.scene.world.facility_order) {
            const auto &f = state.scene.world.world.facilities.at(id);
            const auto values = sim::startup_world_facility_values(state, id);
            require(values.has_value(), "Facility values unavailable");
            if (f.kind == 3 || f.kind == 9)
                require(f.price == values->instance_attributes[0],
                        "Arrival price cache differs from current neighbour/profession quote: " +
                            std::to_string(id));
            std::int64_t sales{}, costs{};
            for (const auto &m : state.facility_monthly_cash.at(id)) {
                sales += m[0];
                costs += m[1];
            }
            income += sales;
            maintenance += costs;
            if (built.count(id)) {
                require(f.status == 1, "Planned building did not finish construction");
                if (f.kind == 3)
                    require(sales > 0, "Planned shop never made an actual sale");
            }
            if (!first)
                std::cout << ',';
            first = false;
            const auto &use = state.scene.world.world.facility_uses.at(f.placement.definition_id);
            std::cout << "{\"instance\":" << id << ",\"definition\":" << f.placement.definition_id
                      << ",\"x\":" << f.placement.anchor.x << ",\"y\":" << f.placement.anchor.y
                      << ",\"status\":" << f.status << ",\"sales\":" << sales
                      << ",\"maintenance\":" << costs << ",\"net_operating\":" << sales - costs
                      << ",\"positive_sales_updates\":" << income_updates[id]
                      << ",\"shared_level\":" << use.level
                      << ",\"shared_completed_uses\":" << use.completed_uses
                      << ",\"price\":" << values->instance_attributes[0]
                      << ",\"arrival_price_cache\":" << f.price
                      << ",\"quality\":" << values->instance_attributes[1]
                      << ",\"charm\":" << values->instance_attributes[2] << '}';
        }
        require(income > 0, "No actual business income");
        std::cout << "],\"facility_income\":" << income
                  << ",\"facility_maintenance\":" << maintenance
                  << ",\"facility_net_operating\":" << income - maintenance
                  << ",\"cash_categories\":[";
        std::int64_t accounted_net{};
        for (int category = 0; category < 5; ++category) {
            std::int64_t in{}, out{};
            for (const auto &m : state.monthly_cash) {
                in += m[category][0];
                out += m[category][1];
            }
            accounted_net += in - out;
            if (category == 0)
                require(in == income && out == maintenance + construction,
                        "Instance operating accounts do not reconcile with construction and "
                        "village facilities");
            if (category)
                std::cout << ',';
            std::cout << "{\"category\":" << category << ",\"income\":" << in
                      << ",\"expense\":" << out << '}';
        }
        require(accounted_net == cash(state) - opening,
                "Cash categories do not reconcile with actual funds");
        std::cout << "],\"months\":[";
        for (int m = 3; m < 6; ++m) {
            if (m != 3)
                std::cout << ',';
            std::int64_t in{}, out{};
            for (const auto &category : state.monthly_cash[m]) {
                in += category[0];
                out += category[1];
            }
            std::cout << "{\"month\":" << m << ",\"income\":" << in << ",\"expense\":" << out
                      << ",\"net\":" << in - out << '}';
        }
        std::cout << "]}" << std::endl;
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--contract") {
            // No world ticks: a fast construction-contract check, distinct from business proof.
            for (int plan = 1; plan <= 4; ++plan) {
                Comparison run;
                for (const auto &building : plans.at(plan))
                    run.build(building);
                require(run.state.simulation_steps == 0 &&
                            cash(run.state) == run.opening - run.construction,
                        "Construction-only contract changed world clock or lost charges");
            }
            std::cout
                << "PASS actual initial construction contracts P1-P4 (no business-duration claim)"
                << std::endl;
            return 0;
        }
        require(argc == 2 && std::string(argv[1]).size() == 2 && argv[1][0] == 'P' &&
                    argv[1][1] >= '0' && argv[1][1] <= '4',
                "Expected one variant P0..P4");
        const int plan = argv[1][1] - '0';
        Comparison run;
        require(month(run.state) == 3, "Unexpected original initial month");
        for (const auto &building : plans.at(plan))
            run.build(building);
        while (month(run.state) < 6)
            run.step();
        run.report(plan);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Economy comparison failed: " << e.what() << std::endl;
        return 1;
    }
}
