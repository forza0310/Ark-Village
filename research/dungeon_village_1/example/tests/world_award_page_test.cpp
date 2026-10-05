#include "dungeon_village_reference/world_award_page.hpp"

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
void rules() {
    WorldAwardPageState original;
    original.humans = {{0, 1, {0, 0, 0}, 91}, {1, 0, {0, 900, 900}, 92}, {2, 2, {0, 20, 20}, 93}};
    auto r = prepare_world_award_page_initialization(original);
    check(r.candidate && r.candidate->state.medal_count == 1 &&
              r.candidate->state.ranked_definitions == std::vector<int>({2, 0}),
          "raw87 initializes medal once and sorts original active definitions by contribution");
    auto state = r.candidate->state;
    check(state.humans[0].contribution == 25 && state.humans[1].contribution == 92 &&
              state.humans[2].contribution == 75 && original.medal_count == 0 &&
              original.humans[0].contribution == 91,
          "B1/B2 mean and variance exclude p0, input untouched");
    r = prepare_world_award_page_initialization(state);
    check(r.candidate && r.candidate->state.medal_count == 1 && r.candidate->effects.empty(),
          "initialization resume never repeats j increment");
    state.page_counter = 1;
    r = prepare_world_award_page(state, WorldAwardAction::update);
    check(r.candidate && r.candidate->effects.size() == 1 &&
              r.candidate->effects[0].kind == WorldAwardEffectKind::sound &&
              r.candidate->effects[0].value == 3 && r.candidate->state.announced,
          "first update emits sound3 only, not event23");
    state = r.candidate->state;
    r = prepare_world_award_page(state, WorldAwardAction::update);
    check(r.candidate && r.candidate->effects.size() == 2 &&
              r.candidate->effects[1].kind == WorldAwardEffectKind::event &&
              r.candidate->effects[1].value == 23 && r.candidate->effects[1].event_argument == 1,
          "counter1 reentry with w true emits event23 with actual medal count");
    state.termination_pending = true;
    r = prepare_world_award_page(state, WorldAwardAction::confirm_termination);
    check(r.candidate && !r.candidate->state.closed && r.candidate->effects.size() == 2,
          "event23 early return precedes pending termination confirmation");
    state.page_counter = 2;
    state.termination_pending = false;
    check(prepare_world_award_page(state, WorldAwardAction::confirm_termination).error ==
              WorldAwardError::invalid_owner,
          "no pending prompt cannot be accepted to bypass original confirmation");
    r = prepare_world_award_page(state, WorldAwardAction::request_termination);
    check(r.candidate && !r.candidate->state.closed && r.candidate->effects.size() == 1 &&
              r.candidate->effects[0].kind == WorldAwardEffectKind::termination_prompt,
          "termination request opens confirmation without discarding medal");
    state = r.candidate->state;
    r = prepare_world_award_page(state, WorldAwardAction::reject_termination);
    check(r.candidate && !r.candidate->state.closed && r.candidate->effects.empty() &&
              r.candidate->state.medal_count == 1,
          "negative answer leaves ceremony and medals intact");
    r = prepare_world_award_page(state, WorldAwardAction::confirm_termination);
    check(r.candidate && r.candidate->state.closed && r.candidate->state.medal_count == 1 &&
              r.candidate->effects.size() == 3 &&
              r.candidate->effects[0].kind == WorldAwardEffectKind::event &&
              r.candidate->effects[0].value == 22 &&
              r.candidate->effects[1].kind == WorldAwardEffectKind::refresh &&
              r.candidate->effects[2].kind == WorldAwardEffectKind::close,
          "explicit termination preserves medal and emits event22 then refresh then close");
    check(prepare_world_award_page(r.candidate->state, WorldAwardAction::update).error ==
              WorldAwardError::invalid_owner,
          "closed page cannot replay effects");
    state.medal_count = 0;
    r = prepare_world_award_page(state, WorldAwardAction::update);
    check(r.candidate && r.candidate->state.closed && r.candidate->effects.size() == 3 &&
              r.candidate->effects[0].kind == WorldAwardEffectKind::refresh &&
              r.candidate->effects[1].kind == WorldAwardEffectKind::event &&
              r.candidate->effects[1].value == 22,
          "zero medals automatic branch refreshes before event22");
    state.medal_count = 1;
    state.termination_pending = false;
    check(prepare_world_award_page(state, WorldAwardAction::confirm_award).error ==
              WorldAwardError::invalid_owner,
          "award cannot skip actual yes/no request");
    r = prepare_world_award_page(state, WorldAwardAction::request_award, 1);
    check(r.candidate && r.candidate->state.pending_award == 0 &&
              r.candidate->state.medal_count == 1 &&
              r.candidate->effects.front().kind == WorldAwardEffectKind::award_prompt,
          "request binds actual selected ranked definition without spending medal");
    state = r.candidate->state;
    auto rejected = prepare_world_award_page(state, WorldAwardAction::reject_award);
    check(rejected.candidate && !rejected.candidate->state.pending_award &&
              rejected.candidate->state.medal_count == 1,
          "no answer preserves medal, clears only award prompt");
    r = prepare_world_award_page(state, WorldAwardAction::confirm_award);
    check(r.candidate && r.candidate->state.medal_count == 0 &&
              r.candidate->state.page_counter == 0 && r.candidate->state.announced &&
              r.candidate->effects.front().kind == WorldAwardEffectKind::reward &&
              r.candidate->effects.front().value == 0,
          "yes consumes once, requests bound reward, retains w and resets parent counter");
    check(prepare_world_award_page(r.candidate->state, WorldAwardAction::confirm_award).error ==
              WorldAwardError::invalid_owner,
          "same answer cannot award twice");
    check(prepare_world_award_page(state, static_cast<WorldAwardAction>(99)).error ==
              WorldAwardError::unsupported_action,
          "unknown action never falls through as ordinary update");
    original.humans = {{0, 1, {0, 10, 10}, 0}};
    r = prepare_world_award_page_initialization(original);
    check(r.candidate && r.candidate->state.humans[0].contribution == 25,
          "zero variance returns mapping lower bound25, not invented neutral50");
    original.humans = {{0, 1, {0, 0, 0}, 0}, {1, 1, {0, 1, 1}, 0}, {2, 1, {0, 1, 1}, 0}};
    r = prepare_world_award_page_initialization(original);
    check(r.candidate && r.candidate->state.humans[0].contribution == 25 &&
              r.candidate->state.humans[1].contribution == 25,
          "integer variance division occurs before sqrt");
    original.humans = {{0, 1, {0, 0, 0}, 0}, {1, 1, {0, 0, 0}, 0}, {2, 1, {0, 20, 20}, 0}};
    r = prepare_world_award_page_initialization(original);
    check(r.candidate && r.candidate->state.ranked_definitions == std::vector<int>({2, 1, 0}),
          "source reverse-swap loop does not promise stable ties");
    original.medal_count = std::numeric_limits<int>::max();
    check(prepare_world_award_page_initialization(original).error == WorldAwardError::overflow,
          "medal increment overflow rejects atomically");
    original.medal_count = 0;
    original.humans = {{0, 1, {0, std::numeric_limits<int>::max(), 0}, 0}, {1, 1, {0, 1, 0}, 0}};
    check(prepare_world_award_page_initialization(original).error == WorldAwardError::overflow,
          "source integer sum overflow rejects atomically");
    original.humans = {{0, 1, {0, 0, 0}, 0}, {1, 1, {0, 100000, 0}, 0}};
    check(prepare_world_award_page_initialization(original).error == WorldAwardError::overflow,
          "source square overflow rejects atomically");
    original.humans = {{0, 0, {}, 0}};
    check(prepare_world_award_page_initialization(original).error == WorldAwardError::invalid_owner,
          "zero roster rejects source division by zero, never injects adventurer");
}
void displays() {
    const auto random = WorldRandomStream::from_raw({-1});
    struct Case {
        int counter;
        int phase;
        bool confirm;
        int next_counter;
        int next_phase;
        bool event;
    };
    for (const auto c : {Case{134, 0, true, 134, 0, false}, Case{135, 0, true, 212, 1, true},
                         Case{201, 1, true, 202, 1, false}, Case{202, 1, true, 212, 1, false},
                         Case{211, 1, true, 212, 1, false}, Case{212, 1, true, 212, 1, true},
                         Case{500, 1, false, 500, 1, false}}) {
        const auto r = prepare_world_award_display({c.counter, c.phase}, c.confirm, random);
        check(r && r->state.counter == c.next_counter && r->state.phase == c.next_phase &&
                  r->event.has_value() == c.event && r->random.draws() == (c.event ? 1U : 0U),
              "raw88 old-local transition and fast-forward boundaries preserve single random draw");
        if (c.event)
            check(r->event == 25, "source signed modulo random selects actual event25");
    }
    check(!prepare_world_award_display({212, 1}, true, WorldRandomStream::from_raw({})),
          "exhausted RNG cannot return partial display or close");
    for (const int n : {0, 2, 41, 42, 66, 67, 72, 73}) {
        const auto r = prepare_world_effort_display(n, true, {});
        check(r && r->counter == (n < 67 ? 67 : n) && r->closed == (n >= 73),
              "effort confirm preserves 67..72 hold before closing at73");
    }
    check(prepare_world_effort_display(2, false, {})->counter == 8 &&
              prepare_world_effort_display(2, false, {1, 0, 0, 0})->counter == 2 &&
              prepare_world_effort_display(14, false, {1, 0, 0, 0})->counter == 20,
          "only actual nonzero delta window suppresses extra six counter steps");
}
} // namespace
int main() {
    try {
        rules();
        displays();
        std::cout << checks << " award page checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
