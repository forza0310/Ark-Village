#include "world_steam_budget.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/village/rules/world_calendar_tasks.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

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

std::int64_t steam_residence_reserve(const State &s) {
    const auto house = sim::startup_world_build_quote(s, 25);
    require(house.has_value(), "missing admission house maintenance quote");
    return steam_operating_reserve(s) +
           2 * std::max<std::int64_t>(0, house->definition_attributes[3]);
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
    std::optional<int> best;
    std::int64_t score{};
    for (const int human : list->second) {
        if (!s.human_presence.at(human) ||
            std::find(s.participants.begin(), s.participants.end(), human) != s.participants.end())
            continue;
        const auto fee = s.human_calendar.at(human).continuation_cost;
        require(fee >= 0, "initialized extra-member quote cannot be negative");
        if (fee > budget)
            continue;
        std::int64_t strength{};
        for (const int value : s.scene.world.world.ai.growth.at(human).derived.combat)
            strength += value;
        if (!best || strength > score || (strength == score && human < *best)) {
            best = human;
            score = strength;
        }
    }
    return best;
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
        if (term.type == 3 && std::none_of(s.scene.world.world.facilities.begin(),
                                          s.scene.world.world.facilities.end(), [&](const auto &f) {
                                              return f.second.placement.definition_id == term.threshold;
                                          }))
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
    return {};
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
    require(!steam_progress_point_goal(progression) && steam_required_facility(progression) == 64,
            "an acquired blueprint reserves no second purchase but still needs actual construction");
    progression.scripts.activities.at(21).status = 1;
    progression.village_points = 75;
    const auto museum_activity = steam_progress_point_goal(progression);
    require(museum_activity && museum_activity->kind == SteamProgressPurchase::activity &&
                museum_activity->definition == 21 && museum_activity->points == 160 &&
                progression.village_points < museum_activity->points,
            "the open unheld museum activity must preserve its real160-point budget while unaffordable");
    progression.activity_counts.at(21) = 1;
    require(!steam_progress_point_goal(progression),
            "a held museum activity must release its reserved points");
    progression.rank = 3;
    require(!steam_progress_point_goal(progression), "rank4 input goals must not be injected into a rank3 town");
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
}
} // namespace ark::test
