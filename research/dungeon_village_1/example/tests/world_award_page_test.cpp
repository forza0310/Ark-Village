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
    for (const auto action : {WorldAwardAction::request_award, WorldAwardAction::confirm_award})
        check(prepare_world_award_page(state, action).error == WorldAwardError::unsupported_action,
              "unimplemented award and raw88 explicitly reject without mutation");
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
} // namespace
int main() {
    try {
        rules();
        std::cout << checks << " award page checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
