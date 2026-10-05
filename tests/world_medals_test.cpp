// Product regression for the canonical UserData.j bridge. Published scripts and annual-page
// consumers are real; eligible presence and the annual page are explicit integration fixtures.
#include "ark/simulation/startup_world_runtime.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
namespace sim = ark::simulation;
namespace rules = sim::rules;
using State = sim::StartupWorldRuntimeState;
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
void unchanged_business(const State &state, const State &before) {
    check(state.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds() &&
              state.scene.world.world.ai.accounting.entries().size() ==
                  before.scene.world.world.ai.accounting.entries().size() &&
              ark::test::same_world_clock(state, before) &&
              state.scene.random.draws() == before.scene.random.draws(),
          "Medal/page input must not mutate cash, date, actor updates or shared random");
}
void consistent_projection(const State &state, int expected) {
    const auto projection = sim::startup_world_runtime_scripts(state);
    check(state.medal_count == expected && state.scripts.medal_count == 0 &&
              projection.medal_count == expected,
          "Only the canonical owner persists medals; temporary script projection reads that owner");
}
void script(State &state, int event) {
    const auto before = state;
    const auto result =
        rules::prepare_world_script(sim::startup_world_runtime_catalog(),
                                    sim::startup_world_runtime_scripts(state), {event, {}, {}});
    check(result.candidate.has_value(), "Published medal event must prepare successfully");
    check(state.medal_count == before.medal_count &&
              state.scripts.event_calls == before.scripts.event_calls,
          "Preparing a script must not mutate the live medal owner");
    check(sim::write_startup_world_runtime_scripts(state, result.candidate->state),
          "Published medal event must write back through the canonical projection");
    check(state.scripts.event_calls.at(event) ==
              (before.scripts.event_calls.count(event) ? before.scripts.event_calls.at(event) : 0) +
                  1,
          "Medal event executes exactly once");
    consistent_projection(state, before.medal_count + 1);
    unchanged_business(state, before);
}
std::uint64_t annual_page(State &state) {
    // This tests the raw87 integration boundary without pretending to run an entire year or
    // introducing a new adventurer. Ranking legitimately reads in-village definition presence.
    state.human_presence.at(state.rules->humans.front().identity) = 1;
    rules::WorldScriptPage page;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 87;
    const auto added =
        rules::prepare_world_script_page(sim::startup_world_runtime_scripts(state), page);
    check(added.candidate && added.candidate->inserted_pages.size() == 1 &&
              sim::write_startup_world_runtime_scripts(state, added.candidate->state),
          "Explicit annual fixture is installed through the real framework page consumer");
    return added.candidate->inserted_pages.front().id;
}
void initialize_annual(State &state) {
    const auto result = sim::prepare_startup_world_runtime(state);
    check(result.candidate.has_value(), "Eligible annual page must initialize through runtime");
    state = *result.candidate;
}
void interleaved_consumers() {
    auto state = ark::test::initial_world();
    consistent_projection(state, 0);
    script(state, 53);
    script(state, 54);
    consistent_projection(state, 2);
    const auto page = annual_page(state);
    const auto before = state;
    initialize_annual(state);
    consistent_projection(state, 3);
    check(state.award_rankings.count(page) && state.award_announced.at(page),
          "Annual initialization sees prior script medals and initializes real rankings");
    unchanged_business(state, before);
    initialize_annual(state);
    initialize_annual(state);
    consistent_projection(state, 3);
    check(sim::act_startup_world_runtime_award_page(state, page,
                                                    rules::WorldAwardAction::request_termination) ==
                  sim::StartupWorldRuntimeError::none &&
              state.award_termination_pending.at(page),
          "Actual termination prompt must be requested before answering");
    check(sim::act_startup_world_runtime_award_page(state, page,
                                                    rules::WorldAwardAction::reject_termination) ==
                  sim::StartupWorldRuntimeError::none &&
              !state.award_termination_pending.at(page),
          "Rejecting termination keeps the ceremony active");
    consistent_projection(state, 3);
    check(
        std::any_of(state.scripts.pages.begin(), state.scripts.pages.end(),
                    [page](const auto &entry) { return entry.id == page && entry.lifecycle != 4; }),
        "Rejecting termination must not close the annual page");
    check(sim::act_startup_world_runtime_award_page(state, page,
                                                    rules::WorldAwardAction::request_termination) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::act_startup_world_runtime_award_page(
                  state, page, rules::WorldAwardAction::confirm_termination) ==
                  sim::StartupWorldRuntimeError::none,
          "Explicit yes closes through the real annual consumer");
    consistent_projection(state, 3);
    check(state.scripts.event_calls.at(22) == 1 &&
              std::any_of(
                  state.scripts.pages.begin(), state.scripts.pages.end(),
                  [page](const auto &entry) { return entry.id == page && entry.lifecycle == 4; }),
          "Annual termination emits event22 once while retaining all unused medals");
    unchanged_business(state, before);
    script(state, 108); // Published direct medal event after annual close must retain all three.
    consistent_projection(state, 4);
    const auto projection = sim::startup_world_runtime_scripts(state);
    check(sim::write_startup_world_runtime_scripts(state, projection),
          "Unchanged script roundtrip remains a valid projection write");
    consistent_projection(state, 4);
}
void overflow_rollback() {
    auto state = ark::test::initial_world();
    state.medal_count = std::numeric_limits<int>::max();
    const auto before = state;
    const auto result =
        rules::prepare_world_script(sim::startup_world_runtime_catalog(),
                                    sim::startup_world_runtime_scripts(state), {53, {}, {}});
    check(!result.candidate && result.error == rules::WorldScriptError::numeric_overflow,
          "Canonical max medal count must reach opcode29 and reject overflow atomically");
    consistent_projection(state, std::numeric_limits<int>::max());
    check(state.scripts.event_calls == before.scripts.event_calls &&
              state.scripts.pages.size() == before.scripts.pages.size() &&
              state.scripts.next_page_id == before.scripts.next_page_id &&
              state.scripts.notices.size() == before.scripts.notices.size(),
          "Late opcode29 overflow must not leave an event53 message, notice or event count");
    unchanged_business(state, before);
    const auto page = annual_page(state);
    const auto annual_before = state;
    const auto tick = sim::prepare_startup_world_runtime(state);
    check(!tick.candidate, "Annual initialization must also reject canonical medal overflow");
    check(sim::act_startup_world_runtime_award_page(state, page,
                                                    rules::WorldAwardAction::request_termination) ==
              sim::StartupWorldRuntimeError::missing_source,
          "Early annual input cannot bypass initialization overflow");
    consistent_projection(state, std::numeric_limits<int>::max());
    check(state.award_rankings.empty() && state.award_announced.empty() &&
              state.award_termination_pending.empty() && state.page_counters.empty() &&
              state.scripts.event_calls == annual_before.scripts.event_calls &&
              state.scripts.pages.back().id == page && state.scripts.pages.back().lifecycle != 4,
          "Rejected annual candidate must not publish partial counters/rankings/prompt/close");
    unchanged_business(state, annual_before);
}
} // namespace
int main() {
    interleaved_consumers();
    overflow_rollback();
    std::cout << "PASS canonical world medals " << checks << " checks\n";
}
