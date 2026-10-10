#include "ark/simulation/combat/rules/encounter_lifecycle.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
bool has(const EncounterStepCandidate &c, EncounterRequestKind kind, int parameter = -1) {
    for (const auto &r : c.requests)
        if (r.kind == kind && (parameter == -1 || r.parameter == parameter))
            return true;
    return false;
}
std::vector<EncounterRequest> requests(const EncounterStepCandidate &c, EncounterRequestKind kind) {
    std::vector<EncounterRequest> v;
    for (const auto &r : c.requests)
        if (r.kind == kind)
            v.push_back(r);
    return v;
}
EncounterStepInput normal() {
    EncounterStepInput i;
    i.state = {1, {5, 5}, 0, 0, 0, 1, 0, 100};
    i.tickets = {{1000, 999}, {100, 99}};
    i.humans = {{{1}, 10, {5, 5}, false, 1, 2, 128 | 2048, 25, 1}};
    return i;
}
void normal_lifecycle() {
    auto i = normal();
    auto c = prepare_encounter_step(i).candidate;
    check(c && c->victory && !c->remove && c->state.state == 1 && c->state.counter == 0 &&
              c->humans[0].state == 10 && c->humans[0].attack_count == 0 &&
              c->humans[0].flags == (128 | 2048) && c->consumed_tickets == 2,
          "normal victory state10 only as>=2, keeps boost, consumes empty spawn probe");
    const auto displays = requests(*c, EncounterRequestKind::reward_display);
    const auto rewards = requests(*c, EncounterRequestKind::reward_accumulation);
    check(displays.size() == 1 && displays[0].value == 75 && displays[0].delay == 22 &&
              rewards[0].value == 75 && rewards[0].delay == 50,
          "normal personalJ plus half eventf, separate display22/growth50");
    check(c->requests.front().kind == EncounterRequestKind::update_group &&
              c->requests[1].kind == EncounterRequestKind::snapshot_influence &&
              has(*c, EncounterRequestKind::event, 91),
          "normal group/field precede victory event");
    for (int old = 0; old <= 100; ++old) {
        i.state.state = 1;
        i.state.counter = old;
        c = prepare_encounter_step(i).candidate;
        check(c && c->remove == (old >= 99) && !c->victory && c->requests.empty(),
              "retiring old99 reaches100, does not issue duplicate victory");
    }
    i = normal();
    i.humans.push_back({{2}, 20, {6, 6}, false, 5, 0, 0, 7, 0});
    i.humans.push_back({{3}, 30, {7, 5}, false, 1, 3, 0, 100, 2});
    i.humans.push_back({{4}, 40, {4, 5}, true, 1, 3, 0, 100, 2});
    i.humans.push_back({{5}, 50, {5, 5}, false, 2, 3, 0, 100, 2});
    i.tickets.push_back({100, 99});
    c = prepare_encounter_step(i).candidate;
    check(c && c->nearby_humans == 4 &&
              requests(*c, EncounterRequestKind::reward_accumulation).size() == 2,
          "near count includes town/down, reward list excludes those and outside square");
    check(c->humans[1].state == 5 && c->humans[1].recent_reward == 0,
          "normal eligible nonattacker receives reward without forced state10");
    const auto ds = requests(*c, EncounterRequestKind::reward_display);
    check(ds[1].delay == 26, "ordinary selected ordinal staggers reward by4");
    i = normal();
    i.monsters = {{{2}, 1}, {{3}, std::nullopt}};
    for (int idle : {0, 598, 599, 600}) {
        i.state.idle = idle;
        c = prepare_encounter_step(i).candidate;
        check(c && c->cancelled == (idle >= 599) && !c->victory && c->linked_monsters == 1,
              "normal idle not implicitly reset by linked monster, cancel at new600");
        if (idle >= 599) {
            const auto cancelled = requests(*c, EncounterRequestKind::cancel_monster);
            check(cancelled.size() == 1 && cancelled[0].actor == CharacterId{2} &&
                      cancelled[0].parameter == 3 && cancelled[0].value == 1,
                  "cancel c3/C1 only matching db, not unbound monster");
        }
    }
    i.town_overlap = true;
    i.tickets.clear();
    c = prepare_encounter_step(i).candidate;
    check(c && c->cancelled && !c->consumed_tickets &&
              !has(*c, EncounterRequestKind::update_group) &&
              !has(*c, EncounterRequestKind::snapshot_influence),
          "town overlap cancels before group/count/probe/field");
    i = normal();
    i.state.spawned = 5;
    i.tickets = {{100, 99}};
    check(prepare_encounter_step(i).candidate->consumed_tickets == 1,
          "spawned5 skips otherwise empty1000 branch draw");
    i.state.state = 2;
    i.tickets.clear();
    check(prepare_encounter_step(i).candidate->state.counter == 1 &&
              prepare_encounter_step(i).candidate->requests.empty(),
          "k2 retains, still increments l");
}
void task_lifecycle() {
    auto i = normal();
    i.state.state = 3;
    i.state.quota = 1;
    i.quest.participants = {10};
    i.tickets = {{100, 99}, {1, 0}, {1, 0}};
    auto c = prepare_encounter_step(i).candidate;
    check(c && c->victory && c->humans[0].state == 10 && !(c->humans[0].flags & 2048) &&
              has(*c, EncounterRequestKind::reset_hp) && c->consumed_tickets == 3,
          "quest eligible winner fullHP/state10, clearboost and two page31 draws");
    const auto display = requests(*c, EncounterRequestKind::reward_display);
    const auto reward = requests(*c, EncounterRequestKind::reward_accumulation);
    check(display[0].delay == 16 && reward[0].delay == 44 && reward[0].value == 75,
          "quest fixed display16 and accumulation44, no normal ordinal staggering");
    const std::vector<EncounterRequestKind> end = {EncounterRequestKind::refresh_map,
                                                   EncounterRequestKind::page30,
                                                   EncounterRequestKind::page31,
                                                   EncounterRequestKind::event,
                                                   EncounterRequestKind::completion_delta,
                                                   EncounterRequestKind::mark_task_complete,
                                                   EncounterRequestKind::clear_task,
                                                   EncounterRequestKind::refresh_global,
                                                   EncounterRequestKind::refresh_task_catalog,
                                                   EncounterRequestKind::event,
                                                   EncounterRequestKind::event};
    const auto start = c->requests.size() - end.size();
    for (std::size_t n = 0; n < end.size(); ++n)
        check(c->requests[start + n].kind == end[n], "quest page/reward/clear/global order");
    for (int state : {0, 2, 16}) {
        i.humans[0].state = state;
        i.tickets = {{1, 0}, {1, 0}};
        c = prepare_encounter_step(i).candidate;
        check(c && c->humans[0].state == state && !has(*c, EncounterRequestKind::reset_hp) &&
                  !has(*c, EncounterRequestKind::task_victory_expression) &&
                  requests(*c, EncounterRequestKind::reward_accumulation)[0].value == 75 &&
                  !(c->humans[0].flags & 2048),
              "quest0/2/16 still receive pending reward and boost clear, no HP/victory pose draw");
    }
    i = normal();
    i.state.state = 3;
    i.state.quota = 1;
    i.quest.participants = {10, 10, 99};
    i.tickets = {{100, 99}, {100, 99}, {3, 0}, {3, 2}};
    c = prepare_encounter_step(i).candidate;
    const auto rs = requests(*c, EncounterRequestKind::reward_accumulation);
    check(
        c && rs.size() == 2 && rs[0].value == 75 && rs[1].value == 50 &&
            requests(*c, EncounterRequestKind::page31)[0].parameter == 10 &&
            requests(*c, EncounterRequestKind::page31)[0].value == 99,
        "duplicate member processed twice with J/K reset, absent actor can still appear on page31");
    i.quest.participants.clear();
    i.tickets = {{1, 0}};
    check(prepare_encounter_step(i).error == EncounterAiError::no_candidates,
          "empty participants cannot silently omit mandatory page31 selection");
    i = normal();
    i.state.state = 3;
    i.state.quota = 1;
    i.task_exists = false;
    i.tickets.clear();
    c = prepare_encounter_step(i).candidate;
    check(c && c->cancelled && c->requests[0].kind == EncounterRequestKind::clear_task &&
              !has(*c, EncounterRequestKind::update_group),
          "missing task clears before cancel/refresh");
    i = normal();
    i.state.state = 3;
    i.state.quota = 1;
    i.quest.participants = {10};
    i.state.reward = 0;
    i.quest.flags = 2;
    i.quest.boss_reward = 99;
    i.quest.event_offset = 4;
    i.tickets = {{100, 99}, {1, 0}, {1, 0}};
    c = prepare_encounter_step(i).candidate;
    check(c && c->state.reward == 99 && requests(*c, EncounterRequestKind::page30)[0].value == 49 &&
              has(*c, EncounterRequestKind::event, 149) &&
              has(*c, EncounterRequestKind::event, 152),
          "special zero-reward task fallback99, floor half49, both special events");
    i.event128_present = true;
    i.feature16 = false;
    i.event205_present = true;
    c = prepare_encounter_step(i).candidate;
    check(c && has(*c, EncounterRequestKind::set_feature16) &&
              !has(*c, EncounterRequestKind::refresh_task_catalog) &&
              !has(*c, EncounterRequestKind::event, 205),
          "post-task first/existing128 and205 guards");
}
void spawning_and_gates() {
    auto i = normal();
    i.state.state = 3;
    i.state.quota = 2;
    i.state.spawned = 0;
    i.quest.normal_definitions = {3, 8};
    i.quest.boss_definition = 9;
    i.quest_cells = {{{4, 6}, 4, true}, {{5, 6}, 4, false}};
    i.tickets = {{2, 0}};
    auto c = prepare_encounter_step(i).candidate;
    check(c && !c->spawn && c->consumed_tickets == 1 && c->state.spawned == 0,
          "town spawn candidate consumes first draw, no retry or monster-definition draw");
    i.tickets = {{2, 1}, {2, 1}};
    c = prepare_encounter_step(i).candidate;
    check(c && c->spawn && c->spawn->definition == 8 && !c->spawn->boss_instance &&
              c->state.spawned == 1 && c->linked_monsters == 1 && !c->victory,
          "task normal spawn only when current linked count below nearby people");
    i.state.spawned = 1;
    i.quest.flags = 2;
    i.tickets = {{2, 1}};
    c = prepare_encounter_step(i).candidate;
    check(c && c->spawn && c->spawn->definition == 9 && c->spawn->boss_instance &&
              c->state.spawned == 2 && c->consumed_tickets == 1,
          "last task monster boss no definition-selection draw, adds16384 only special");
    i.monsters = {{{2}, 1}};
    i.tickets.clear();
    c = prepare_encounter_step(i).candidate;
    check(c && !c->spawn && !c->consumed_tickets,
          "count includes death-state linked monsters until later removal, no spawn when n==o");
    i = normal();
    for (int kills : {0, 1, 2})
        for (int ticket = 0; ticket < 100; ++ticket)
            for (bool task : {false, true}) {
                i.state.state = task ? 3 : 0;
                i.state.quota = 1;
                i.quest.participants = {10};
                i.humans[0].recent_kills = kills;
                i.tickets = task ? std::vector<EncounterRandomTicket>{{100, ticket}, {1, 0}, {1, 0}}
                                 : std::vector<EncounterRandomTicket>{{1000, 999}, {100, ticket}};
                c = prepare_encounter_step(i).candidate;
                const int threshold = kills == 0 ? 15 : kills == 1 ? (task ? 30 : 40) : 60;
                check(c &&
                          has(*c, task ? EncounterRequestKind::task_victory_expression
                                       : EncounterRequestKind::expression5) == (ticket < threshold),
                      "normal/task recent K gates15/40or30/60, strict less");
            }
    i = normal();
    i.tickets = {{1000, 0}, {100, 100}};
    check(!prepare_encounter_step(i).candidate && i.humans[0].recent_reward == 25,
          "invalid later gate rejects whole candidate, no partly reset J/K");
    i.tickets = {{1000, 0}, {100, 0}};
    i.humans[0].recent_reward = std::numeric_limits<int>::max();
    check(prepare_encounter_step(i).error == EncounterAiError::invalid_input,
          "reward overflow rejects entire plan without wrapping");
}
void delayed_reward() {
    for (int amount = 0; amount <= 100; ++amount)
        for (int delay : {0, 1, 16, 44, 50}) {
            auto state = prepare_delayed_reward({}, amount, delay);
            int accumulated{};
            for (int tick = 0; tick < delay + 9; ++tick) {
                const auto step = advance_delayed_reward(*state);
                check(step.has_value(), "delayed reward valid bounded advance");
                accumulated += step->increment;
                state = complete_delayed_reward_step(*step, false);
            }
            check(accumulated == amount && state->amount == 0,
                  "nine integer increments conserve reward after exact negative-delay wait");
        }
    check(prepare_delayed_reward({100, 4}, 50, 44)->amount == 106,
          "new reward merges only old unconsumed56, not entire original100");
    auto step = advance_delayed_reward({100, 2});
    check(step && step->increment == 11 && complete_delayed_reward_step(*step, true)->amount == 0,
          "actual level-up clears still-pending gain immediately, not guaranteed full payout");
    check(advance_delayed_reward({0, -44})->state.counter == -44,
          "zero N short-circuits before O increment");
    check(!prepare_delayed_reward({std::numeric_limits<int>::max(), -1}, 1, 0),
          "pending merge overflow returns no candidate");
}
} // namespace
int main() {
    try {
        normal_lifecycle();
        task_lifecycle();
        spawning_and_gates();
        delayed_reward();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
