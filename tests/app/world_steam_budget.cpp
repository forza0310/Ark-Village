#include "world_steam_budget.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/village/rules/world_calendar_tasks.hpp"
#include "ark/simulation/village/rules/world_village_activity.hpp"
#include "world_steam_layout.hpp"
#include "world_steam_people.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(std::string("Steam budget: ") + message);
}
std::vector<int> activity_catalogue(const State &s) {
    std::vector<ref::WorldVillageActivityDefinition> definitions;
    for (const auto &a : s.rules->activities)
        definitions.push_back({a.identity, s.scripts.activities.at(a.identity).status,
                               s.activity_flags.at(a.identity), s.activity_counts.at(a.identity),
                               a.parameters[2], a.parameters[3], a.parameters[4], a.parameters[5]});
    const auto available = ref::catalogue_world_village_activities(definitions);
    require(available.has_value(), "point budget needs the source activity qualifications");
    return *available;
}
} // namespace

std::int64_t steam_operating_reserve(const State &s) {
    // Two months of maintenance and current profession fee endpoints form a cash
    // buffer, not a prediction of monthly profit.
    std::int64_t total{};
    for (const auto &[id, f] : s.scene.world.world.facilities) {
        (void)f;
        const auto values = sim::startup_world_facility_values(s, id);
        require(values.has_value(), "missing maintenance quote");
        total += std::max<std::int64_t>(0, values->definition_attributes[3]);
    }
    for (const auto &[id, present] : s.human_presence)
        if (present) {
            const auto &job = s.rules->jobs.at(
                s.scene.world.world.ai.growth.at(id).definition.current_profession);
            total += std::max({0, job.fee[0], job.fee[1]});
        }
    return total * 2;
}

std::int64_t steam_residence_reserve(const State &s, std::optional<std::uint64_t> replacing_plot) {
    const auto house = sim::startup_world_build_quote(s, 25);
    require(house.has_value(), "missing admission house maintenance quote");
    auto reserve =
        steam_operating_reserve(s) + 2 * std::max<std::int64_t>(0, house->definition_attributes[3]);
    if (replacing_plot) {
        require(s.scene.world.world.facilities.at(*replacing_plot).kind == 13,
                "admission budget must consume a real recruitment plot");
        const auto plot = sim::startup_world_facility_values(s, *replacing_plot);
        require(plot.has_value(), "missing retiring plot maintenance quote");
        reserve -= 2 * std::max<std::int64_t>(0, plot->definition_attributes[3]);
    }
    return reserve;
}

std::int64_t steam_residence_plot_reserve(const State &s) {
    const auto plot = sim::startup_world_build_quote(s, 24);
    const auto house = sim::startup_world_build_quote(s, 25);
    require(plot && house, "missing source plot or house quote");
    return steam_operating_reserve(s) + plot->construction_cost +
           2 * std::max({std::int64_t{0}, plot->definition_attributes[3],
                         house->definition_attributes[3]});
}

bool steam_task_affordable(const State &s, std::uint64_t id) {
    const auto f = s.tasks.find(id);
    return f != s.tasks.end() && s.rules->tasks.at(f->second.definition).recruitment_fee <=
                                     cash(s) - steam_operating_reserve(s);
}

bool steam_task_renewable(const State &s, int displayed_fee) {
    // raw33 quotes once; its parent later consumes the current participants' cached
    // fees. Keep both checks without copying the profession-level fee formula.
    std::int64_t current_fee{};
    for (const int human : s.participants)
        current_fee += s.human_calendar.at(human).continuation_cost;
    return cash(s) - steam_operating_reserve(s) >=
           std::max<std::int64_t>(displayed_fee, current_fee);
}

std::optional<int> steam_task_extra_human(const State &s, std::uint64_t page) {
    const auto list = s.task_extra_pages.find(page);
    const auto reserve = steam_operating_reserve(s);
    if (list == s.task_extra_pages.end() || cash(s) < reserve)
        return {};
    const auto budget = cash(s) - reserve;
    std::optional<std::tuple<bool, int, std::int64_t, int>> best;
    for (const int human : list->second) {
        if (!s.human_presence.at(human) ||
            std::find(s.participants.begin(), s.participants.end(), human) != s.participants.end())
            continue;
        const auto fee = s.human_calendar.at(human).continuation_cost;
        require(fee >= 0, "initialized extra-member quote cannot be negative");
        if (fee > budget)
            continue;
        const auto &growth = s.scene.world.world.ai.growth.at(human);
        std::int64_t strength{};
        for (const int value : growth.derived.combat)
            strength += value;
        const int job = growth.definition.current_profession;
        const int level = growth.definition.profession_levels.at(job);
        // Test-player policy: retain the first five combat members, then train holders of
        // unopened professions. Opening still requires their actual source mastery consumer.
        const bool cultivate = s.participants.size() >= 5 &&
                               s.scripts.professions.at(job).status == 0 && level < 10;
        const auto priority = std::make_tuple(!cultivate, cultivate ? -level : 0, -strength, human);
        if (!best || priority < *best)
            best = priority;
    }
    return best ? std::optional<int>{std::get<3>(*best)} : std::nullopt;
}

bool steam_optional_activity_affordable(const State &s) {
    return cash(s) >= steam_operating_reserve(s);
}
std::optional<int> steam_required_facility(const State &s) {
    const auto table = ref::fixed_calendar_task_rank_terms();
    const auto terms = table.find(s.rank);
    if (terms == table.end())
        return {};
    for (const auto &term : terms->second)
        if (term.type == 3 &&
            std::none_of(
                s.scene.world.world.facilities.begin(), s.scene.world.world.facilities.end(),
                [&](const auto &f) { return f.second.placement.definition_id == term.threshold; }))
            return term.threshold;
    return {};
}
std::optional<SteamProgressPointGoal> steam_progress_point_goal(const State &s) {
    if (s.rank < 4)
        return {};
    // Formal PROGRESSION_ROUTES: actual promotion to rank4 directly opens activity19.
    // A real source catalogue still decides whether it is offered this quarter.
    if (s.scripts.activities.at(19).status != 0 && s.activity_counts.at(19) == 0)
        return SteamProgressPointGoal{SteamProgressPurchase::activity, 19,
                                      s.rules->activities.at(19).parameters[4]};
    if (const auto facility = steam_required_facility(s);
        facility && s.facility_presence.at(*facility) == 0)
        return SteamProgressPointGoal{SteamProgressPurchase::blueprint, *facility,
                                      s.rules->facility_initial.at(*facility).capacity};
    // Museum completion opens activity21 through the source event75. Reserve its
    // first real quote; catalogue, quarter slots and payment remain source gates.
    if (s.scripts.activities.at(21).status == 1 && s.activity_counts.at(21) == 0)
        return SteamProgressPointGoal{SteamProgressPurchase::activity, 21,
                                      s.rules->activities.at(21).parameters[4]};
    // Formal MAGIC_POT: 28/29 are separately opened type5 consumers. Protect
    // their first actual quote from optional events; raw53 still owns the level.
    for (const int id : {28, 29})
        if (s.scripts.activities.at(id).status == 1 && s.activity_counts.at(id) == 0 &&
            s.legacy_n[11] < id - 26)
            return SteamProgressPointGoal{SteamProgressPurchase::activity, id,
                                          s.rules->activities.at(id).parameters[4]};
    return {};
}
std::optional<SteamProgressPointGoal> steam_unheld_activity_goal(const State &s) {
    std::optional<SteamProgressPointGoal> selected;
    for (const int id : activity_catalogue(s)) {
        const auto &a = s.rules->activities.at(id);
        if (a.parameters[2] < 0 || a.parameters[2] > 2 || s.activity_counts.at(id) != 0)
            continue;
        if (!selected || std::make_pair(a.parameters[4], id) <
                             std::make_pair(selected->points, selected->definition))
            selected = SteamProgressPointGoal{SteamProgressPurchase::activity, id, a.parameters[4]};
    }
    return selected;
}
std::optional<SteamShopExpansion> steam_select_shop_expansion(const State &s,
                                                              const std::vector<int> &entries,
                                                              std::int64_t cash_budget) {
    std::optional<SteamShopExpansion> best;
    for (const int id : entries) {
        if (id == -1 || id == -2)
            continue; // Source21's demolition/move tools have no facility definition.
        require(id >= 0 && static_cast<std::size_t>(id) < s.rules->facilities.size(),
                "expansion catalogue contains an unknown definition");
        const auto &d = s.rules->facilities.at(id);
        if (d.kind != 3 || d.detail == 1 || d.detail == 4 || d.detail == 5 || d.detail == 6 ||
            std::any_of(s.scene.world.world.facilities.begin(),
                        s.scene.world.world.facilities.end(),
                        [&](const auto &f) { return f.second.placement.definition_id == id; }))
            continue;
        const auto quote = sim::startup_world_build_quote(s, id);
        if (!quote || quote->definition_attributes[0] <= 0 ||
            cash_budget < quote->construction_cost + 2 * quote->definition_attributes[3])
            continue;
        const SteamShopExpansion choice{id, s.rules->facility_initial.at(id).capacity,
                                        quote->definition_attributes[0], quote->construction_cost,
                                        quote->definition_attributes[3]};
        // Stable ties prefer the lower recurring/point cost, then source identity.
        if (!best ||
            std::make_tuple(choice.price, -choice.maintenance, -choice.points, -choice.definition) >
                std::make_tuple(best->price, -best->maintenance, -best->points, -best->definition))
            best = choice;
    }
    return best;
}

int steam_point_reserve(const State &s, const std::vector<int> *offered_activities) {
    const auto buildings = sim::startup_world_build_catalog(s);
    require(buildings.has_value(), "point reserve needs the real construction catalogue");
    int reserve{};
    bool needs_expansion = steam_housing_expansion_needed(s);
    for (const auto &target : steam_village_layout()) {
        const bool inside = steam_layout_inside_fence(s, target);
        needs_expansion |= !inside;
        const bool buildable =
            std::any_of(buildings->begin(), buildings->end(), [&](const auto &group) {
                return std::find(group.begin(), group.end(), target.definition) != group.end();
            });
        const auto &definition = s.rules->facilities.at(target.definition);
        if (target.role != SteamLayoutRole::residence && inside && !buildable &&
            definition.unlock_rank >= 0 && definition.unlock_rank <= s.rank)
            reserve = std::max(reserve, s.rules->facility_initial.at(target.definition).capacity);
    }
    std::vector<int> queried;
    if (needs_expansion) {
        if (!offered_activities) {
            queried = activity_catalogue(s);
            offered_activities = &queried;
        }
        std::optional<int> cost;
        for (const int id : *offered_activities) {
            const auto &a = s.rules->activities.at(id);
            if (a.parameters[2] == 3 && (!cost || a.parameters[4] < *cost))
                cost = a.parameters[4];
        }
        if (cost)
            reserve = std::max(reserve, *cost);
    }
    if (const auto goal = steam_progress_point_goal(s))
        reserve = std::max(reserve, goal->points);
    else if ((s.scripts.user_flags & 16U) && steam_reference_layout_complete(s)) {
        // This is an upper point budget from known table offers, not an85 list.
        // Keep that reserve while an owned shop is under construction or waiting
        // for its first sale. Discovery/payment still waits for expansion readiness.
        // Discover the actual frozen85 entries before choosing or buying a shop.
        std::vector<int> prospective;
        for (const auto &d : s.rules->facilities)
            if (s.facility_presence.at(d.id) == 0 && d.unlock_rank >= 0 && d.unlock_rank <= s.rank)
                prospective.push_back(d.id);
        if (const auto shop =
                steam_select_shop_expansion(s, prospective, cash(s) - steam_operating_reserve(s)))
            reserve = std::max(reserve, shop->points);
    }
    return reserve;
}
int steam_task_recovery_month(const State &s) { return month(s) + 2; }

void inspect_steam_expenses(const State &before, const State &after, std::string_view phase,
                            std::ostream &out) {
    const bool same_month = month(before) == month(after);
    const int bucket = after.scene.calendar.month;
    for (int category = 0; category < 5; ++category) {
        const auto old = same_month ? before.monthly_cash.at(bucket).at(category)[1] : 0;
        const auto delta = std::int64_t(after.monthly_cash.at(bucket).at(category)[1]) - old;
        if (delta > 0)
            out << "BUSINESS_EXPENSE phase=" << phase << " tick=" << after.simulation_steps
                << " month=" << month(after) << " category=" << category << " amount=" << delta
                << " cash_before=" << cash(before) << " cash_after=" << cash(after)
                << " reserve_after=" << steam_operating_reserve(after)
                << " active_task=" << after.active_task.value_or(0)
                << " participants=" << after.participants.size() << '\n';
    }
}

void steam_budget_contract() {
    // Explicit cash/fee qualifications test player decisions, not natural acquisition.
    auto s = initial_world();
    const auto buffer = steam_operating_reserve(s);
    require(buffer > 0 && steam_residence_reserve(s) >= buffer,
            "real new town must reserve its recurring costs");
    const auto plot = sim::startup_world_build_quote(s, 24);
    const auto house = sim::startup_world_build_quote(s, 25);
    require(
        plot && house && plot->construction_cost == 100 && plot->definition_attributes[3] == 120 &&
            house->definition_attributes[3] == 180 &&
            steam_residence_plot_reserve(s) == buffer + 460,
        "source plot construction and transitional house upkeep must remain separately budgeted");
    auto admission = s;
    for (auto &[id, present] : admission.human_presence)
        present = id == 1 ? 1 : 0;
    admission.human_homes.at(1)[2] = 0;
    admission.shop_humans.at(1).satisfaction = admission.rules->humans.at(1).residence_threshold;
    const auto admission_buffer = steam_residence_plot_reserve(admission);
    const auto admission_fee = admission.rules->humans.at(1).residence_fee;
    for (const int spare : {-1, 0, 1}) {
        admission.scene.world.world.ai.accounting =
            ref::PeriodAccounting(admission_buffer + admission_fee + spare);
        require(
            SteamPeopleStrategy::eligible_homeless(admission, admission_buffer).has_value() ==
                (spare >= 0),
            "one extra plot must reserve its real construction, upkeep and candidate fee exactly");
    }
    const auto destination = steam_connected_extension(admission, 24);
    require(destination.has_value(), "admission budget fixture needs a legal connected plot");
    const auto started = sim::begin_startup_world_build(admission, 24);
    const auto built =
        sim::confirm_startup_world_build(admission, destination->anchor, destination->orientation);
    require(started.error == sim::StartupWorldRuntimeError::none &&
                built.error == sim::StartupWorldRuntimeError::none && built.created &&
                steam_residence_reserve(admission, *built.created) ==
                    admission_buffer - plot->construction_cost,
            "paid plot-to-house replacement must preserve the preconstruction budget at admission");
    for (const int spare : {-1, 0}) {
        const auto reserve = steam_residence_reserve(admission, *built.created);
        admission.scene.world.world.ai.accounting =
            ref::PeriodAccounting(reserve + admission_fee + spare);
        require(
            SteamPeopleStrategy::eligible_homeless(admission, reserve).has_value() == (spare >= 0),
            "retiring plot upkeep must not make an exactly funded admission wait for more cash");
    }
    auto progression = s;
    progression.rank = 4;
    progression.scripts.activities.at(19).status = 1;
    progression.scripts.activities.at(21).status = 1; // Qualification fixture for goal priority.
    const auto fireworks = steam_progress_point_goal(progression);
    require(fireworks && fireworks->kind == SteamProgressPurchase::activity &&
                fireworks->definition == 19 && fireworks->points == 150,
            "rank4 must reserve the real150-point fireworks offer before cheap optional events");
    progression.activity_counts.at(19) = 1;
    const auto museum = steam_progress_point_goal(progression);
    require(steam_required_facility(progression) == 64 && museum &&
                museum->kind == SteamProgressPurchase::blueprint && museum->definition == 64 &&
                museum->points == 200,
            "after fireworks the next rank's actual museum blueprint requires200 points");
    progression.facility_presence.at(64) = 2;
    progression.scripts.activities.at(21).status = 0;
    require(
        !steam_progress_point_goal(progression) && steam_required_facility(progression) == 64,
        "an acquired blueprint reserves no second purchase but still needs actual construction");
    progression.scripts.activities.at(21).status = 1;
    progression.village_points = 75;
    const auto museum_activity = steam_progress_point_goal(progression);
    require(museum_activity && museum_activity->kind == SteamProgressPurchase::activity &&
                museum_activity->definition == 21 && museum_activity->points == 160 &&
                progression.village_points < museum_activity->points,
            "the open unheld museum activity must preserve its real160-point budget while "
            "unaffordable");
    progression.activity_counts.at(21) = 1;
    require(!steam_progress_point_goal(progression),
            "a held museum activity must release its reserved points");
    for (const int id : {28, 29}) {
        progression.scripts.activities.at(id).status = 1;
        progression.legacy_n[11] = id - 27;
        const auto upgrade = steam_progress_point_goal(progression);
        require(upgrade && upgrade->kind == SteamProgressPurchase::activity &&
                    upgrade->definition == id && upgrade->points == 100 &&
                    steam_point_reserve(progression) >= 100,
                "an open unheld pot upgrade must protect its actual100-point quote");
        progression.activity_counts.at(id) = 1;
        require(!steam_progress_point_goal(progression),
                "source payment releases the goal without pretending raw53 has expanded the pot");
        progression.activity_counts.at(id) = 0;
        ++progression.legacy_n[11];
        require(!steam_progress_point_goal(progression),
                "an already sufficient pot level must not reserve a repeated upgrade");
        progression.scripts.activities.at(id).status = 0;
        --progression.legacy_n[11];
        require(!steam_progress_point_goal(progression),
                "experience or prospective capacity must not invent an unopened activity");
        progression.activity_counts.at(id) = 1;
    }
    progression.rank = 3;
    require(!steam_progress_point_goal(progression),
            "rank4 input goals must not be injected into a rank3 town");
    auto unheld = s;
    // Source eligibility only. The actual layout gate and spending are exercised
    // by the primary player from its verified business prefix.
    for (auto &[id, a] : unheld.scripts.activities) {
        a.status = 0;
        unheld.activity_counts.at(id) = 0;
        unheld.activity_flags.at(id) = 0;
    }
    for (const int id : {0, 6, 8, 25, 27, 28})
        unheld.scripts.activities.at(id).status = 1;
    unheld.activity_counts.at(0) = 1;
    const auto first = steam_unheld_activity_goal(unheld);
    require(
        first && first->kind == SteamProgressPurchase::activity && first->definition == 6 &&
            first->points == 60,
        "unheld regular goals need the source quote and stable ID tie-break, not cheap repeats");
    unheld.activity_counts.at(6) = 1;
    const auto second = steam_unheld_activity_goal(unheld);
    require(second && second->definition == 8 && second->points == 60,
            "source payment releases the first goal for the next unheld definition");
    unheld.activity_flags.at(8) = 4U;
    require(!steam_unheld_activity_goal(unheld),
            "source-excluded regular rows or special consumers must not create coverage goals");
    unheld.activity_flags.at(8) = 0;
    unheld.scripts.activities.at(8).status = 0;
    require(!steam_unheld_activity_goal(unheld), "unopened activities must stay unavailable");
    unheld.scripts.activities.at(8).status = 1;
    unheld.activity_counts.at(8) = 1;
    require(!steam_unheld_activity_goal(unheld),
            "an already paid activity is not a first-use goal");
    // Cached raw27 quotes and strength are explicit player-choice qualifications.
    // Frozen task rules own the fee formula, reset effects and full-party rejection.
    auto extras = s;
    extras.task_extra_pages[91] = {2, 1, 0};
    for (int human = 0; human < 3; ++human) {
        extras.human_presence.at(human) = 1;
        extras.human_calendar.at(human).continuation_cost = human == 2 ? 201 : 200;
        extras.scene.world.world.ai.growth.at(human).derived.combat =
            human == 2 ? std::array<int, 4>{500, 100, 100, 100}
                       : std::array<int, 4>{100, 30, 30, 10};
    }
    const auto crew_buffer = steam_operating_reserve(extras);
    extras.scene.world.world.ai.accounting = ref::PeriodAccounting(crew_buffer + 200);
    require(steam_task_extra_human(extras, 91) == 0,
            "paid crew must respect source quote and stable strength tie order");
    extras.participants = {0};
    require(steam_task_extra_human(extras, 91) == 1,
            "an existing participant cannot be hired twice");
    extras.scene.world.world.ai.accounting = ref::PeriodAccounting(crew_buffer + 201);
    require(steam_task_extra_human(extras, 91) == 2,
            "strongest affordable candidate must be selected by definition identity");
    extras.scene.world.world.ai.accounting = ref::PeriodAccounting(crew_buffer + 199);
    require(!steam_task_extra_human(extras, 91) && !steam_task_extra_human(extras, 92),
            "unaffordable or uninitialized recruitment must wait");
    // Qualification fixtures only: source mastery, actual fees and task outcomes are not
    // simulated by this read-only selection policy. Keep the preceding combat/budget checks.
    auto &trainee = extras.scene.world.world.ai.growth.at(1).definition;
    trainee.current_profession = 8;
    for (const auto &[members, level, open, fee, expected] :
         std::vector<std::tuple<int, int, int, int, int>>{
             {4, 9, 0, 200, 2}, {5, 9, 0, 200, 1}, {5, 9, 1, 200, 2},
             {5, 10, 0, 200, 2}, {5, 9, 0, 202, 2}}) {
        extras.participants = {3, 4, 5, 6};
        if (members == 5)
            extras.participants.push_back(7);
        trainee.profession_levels.at(8) = level;
        extras.scripts.professions.at(8).status = open;
        extras.human_calendar.at(1).continuation_cost = fee;
        // This fixture changes current profession, which also changes the wage buffer.
        extras.scene.world.world.ai.accounting =
            ref::PeriodAccounting(steam_operating_reserve(extras) + 201);
        const auto digest = sim::startup_world_state_digest(extras);
        const auto selected = steam_task_extra_human(extras, 91);
        const auto context = "training members=" + std::to_string(members) +
                             " level=" + std::to_string(level) + " open=" + std::to_string(open) +
                             " fee=" + std::to_string(fee) + " selected=" +
                             std::to_string(selected.value_or(-1)) +
                             " expected=" + std::to_string(expected);
        require(selected == expected &&
                    sim::startup_world_state_digest(extras) == digest,
                context.c_str());
    }
    const std::uint64_t task = 1;
    ref::DungeonFinishTask offered;
    offered.identity = task;
    offered.definition = s.rules->tasks.front().factory.identity;
    s.tasks.emplace(task, offered);
    const auto fee = s.rules->tasks.at(s.tasks.at(task).definition).recruitment_fee;
    for (const int spare : {-1, 0, 1}) {
        s.scene.world.world.ai.accounting = ref::PeriodAccounting(buffer + fee + spare);
        require(steam_task_affordable(s, task) == (spare >= 0),
                "fixed recruitment quote must preserve the two-month buffer exactly");
    }
    require(!steam_task_affordable(s, 0), "missing task must not be affordable");
    s.participants = {0, 1};
    s.human_calendar.at(0).continuation_cost = 200;
    s.human_calendar.at(1).continuation_cost = 300;
    for (const auto fees : {std::array<int, 2>{450, 500}, {600, 600}})
        for (const int spare : {-1, 0}) {
            s.scene.world.world.ai.accounting = ref::PeriodAccounting(buffer + fees[1] + spare);
            require(steam_task_renewable(s, fees[0]) == (spare >= 0),
                    "renewal must reserve both displayed and current participant costs");
        }
    for (const int spare : {-1, 0}) {
        s.scene.world.world.ai.accounting = ref::PeriodAccounting(buffer + spare);
        require(steam_optional_activity_affordable(s) == (spare >= 0),
                "optional activities must wait for operating liquidity");
    }
    s.scene.calendar.year = 5;
    s.scene.calendar.month = 11;
    require(steam_task_recovery_month(s) == 73, "business recovery must cross the year boundary");
    auto after = s;
    after.monthly_cash[11][4][1] += 500;
    std::ostringstream log;
    inspect_steam_expenses(s, after, "command", log);
    require(log.str().find("category=4 amount=500") != std::string::npos,
            "expense diagnostic must retain the actual category and amount");
    after = s;
    after.scene.calendar.year = 6;
    after.scene.calendar.month = 0;
    s.monthly_cash[0][0][1] = 999; // Ignore the previous year's January bucket.
    after.monthly_cash = {};
    after.monthly_cash[0][0][1] = 150;
    log.str("");
    inspect_steam_expenses(s, after, "update", log);
    require(log.str().find("month=72 category=0 amount=150") != std::string::npos,
            "expense diagnostic must handle the annual bucket reset");

    // Explicit rank qualification freezes the source85 offer; no natural unlock,
    // payment or revenue is claimed by this test-player selection query.
    auto offers = initial_world();
    offers.rank = 4;
    ref::WorldScriptPage commerce_page;
    commerce_page.kind = ref::WorldScriptPageKind::raw_page;
    commerce_page.legacy_page = 85;
    const auto commerce_inserted =
        ref::prepare_world_script_page(sim::startup_world_runtime_scripts(offers), commerce_page);
    require(
        commerce_inserted.candidate &&
            sim::write_startup_world_runtime_scripts(offers, commerce_inserted.candidate->state) &&
            sim::initialize_startup_world_commerce_pages(offers),
        "shop expansion qualification needs the real frozen85 entries");
    const auto offer_page = offers.scripts.pages.back().id;
    const auto &shop_offers = offers.commerce_page_lists.at(offer_page);
    const auto expansion = steam_select_shop_expansion(offers, shop_offers, 10000);
    require(expansion && expansion->definition == 54 && expansion->points == 250,
            "actual rank4 offers must prioritize the highest quoted ordinary shop");
    const auto quote = sim::startup_world_build_quote(offers, 54);
    require(quote && expansion->price == quote->definition_attributes[0] &&
                expansion->construction == quote->construction_cost &&
                expansion->maintenance == quote->definition_attributes[3],
            "shop choice must retain the current source quote, not a revenue forecast");
    const auto exact_budget = expansion->construction + 2 * expansion->maintenance;
    for (const int spare : {-1, 0}) {
        const auto selected = steam_select_shop_expansion(offers, {54}, exact_budget + spare);
        require(selected.has_value() == (spare == 0),
                "a shop must preserve construction and two months of its maintenance exactly");
    }

    // The higher-priced definition remains known, but an owned blueprint no
    // longer belongs to the source85 offer. The selector cannot add its own row.
    auto restricted = initial_world();
    restricted.rank = 4;
    restricted.facility_presence.at(54) = 2;
    const auto restricted_inserted = ref::prepare_world_script_page(
        sim::startup_world_runtime_scripts(restricted), commerce_page);
    require(restricted_inserted.candidate &&
                sim::write_startup_world_runtime_scripts(restricted,
                                                         restricted_inserted.candidate->state) &&
                sim::initialize_startup_world_commerce_pages(restricted),
            "restricted expansion qualification needs a newly frozen source85 offer");
    const auto &restricted_entries =
        restricted.commerce_page_lists.at(restricted.scripts.pages.back().id);
    const auto restricted_choice =
        steam_select_shop_expansion(restricted, restricted_entries, 10000);
    require(std::find(restricted_entries.begin(), restricted_entries.end(), 54) ==
                    restricted_entries.end() &&
                restricted_choice && restricted_choice->definition != 54,
            "a known higher quote outside the actual85 entries cannot be selected");
    restricted.village_points = 0;
    const auto construction = sim::startup_world_build_catalog(restricted);
    require(construction.has_value(), "owned blueprint needs the real21 catalogue");
    std::vector<int> construction_entries;
    for (const auto &group : *construction)
        construction_entries.insert(construction_entries.end(), group.begin(), group.end());
    const auto owned = steam_select_shop_expansion(restricted, construction_entries, exact_budget);
    require(owned && owned->definition == 54 && restricted.village_points == 0,
            "an owned real21 blueprint can be selected without reserving a second point payment");

    // Only instance presence is qualified here; source construction/payment and
    // catalogue filtering retain their existing rule-suite ownership.
    const auto instance = restricted.scene.world.world.facilities.begin()->second;
    auto installed = instance;
    installed.placement.definition_id = 54;
    const auto instance_id = restricted.next_facility_identity++;
    installed.placement.instance_id.value = instance_id;
    restricted.scene.world.world.facilities.emplace(instance_id, installed);
    require(!steam_select_shop_expansion(restricted, {54}, exact_budget),
            "an existing shop instance must prevent another expansion of the same definition");
}
} // namespace ark::test
