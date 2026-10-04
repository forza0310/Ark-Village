// Conditional owner composition, not default new-game AI, inventory or profession fixtures.
#include "ark/app/ai_schedule.hpp"
#include "ark/app/game.hpp"
#include "ark/people/actor_control.hpp"
#include "ark/people/human_growth.hpp"
#include "ark/people/weapon_choice.hpp"
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>

namespace {
using namespace ark::people;
using namespace ark::app;
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
void rewards() {
    for (int amount = 1; amount <= 100; ++amount)
        for (int delay = 0; delay <= 10; ++delay) {
            auto state = *prepare_delayed_reward({}, amount, delay);
            int delivered{};
            for (int tick = 0; tick < delay + 9; ++tick) {
                const auto next = advance_delayed_reward(state);
                check(next.has_value(), "valid delayed reward progresses");
                delivered += next->increment;
                state = *complete_delayed_reward_step(*next, false);
                if (tick < delay)
                    check(delivered == 0, "delay does not award XP early");
            }
            check(delivered == amount && state.amount == 0,
                  "nine truncated differences award the exact total once");
        }
    const auto merged = prepare_delayed_reward({90, 4}, 20, 3);
    check(merged && merged->amount == 70 && merged->counter == -3,
          "merge preserves unconsumed50 plus new20 and replaces delay");
    const auto step = advance_delayed_reward({90, 0});
    check(complete_delayed_reward_step(*step, true)->amount == 0,
          "real level gain discards the remaining reward");
    const auto zero = advance_delayed_reward({0, -7});
    check(zero && zero->state.counter == -7 && zero->increment == 0,
          "zero reward cannot advance its delay");
    check(!prepare_delayed_reward({std::numeric_limits<int>::max(), 0}, 1, 0) &&
              !prepare_delayed_reward({}, 1, -1) &&
              !advance_delayed_reward({1, std::numeric_limits<int>::max()}),
          "reward overflow and malformed values reject without a candidate");
}
struct Owner {
    HumanGrowthInput definition;
    std::map<std::uint64_t, ActorEffectState> effects;
    std::map<std::uint64_t, int> hp;
    std::vector<HumanGrowthRequest> requests;
};
// The schedule never rolls back a consumer's mutations. Its handler must operate on this
// private owner copy; only a fully accepted plan replaces the live value.
bool advance_owner(Owner &live, bool reject_finalize) {
    Owner next = live;
    AiScheduleInput input;
    input.rosters[0] = {1, 2};
    const auto plan = prepare_ai_schedule(input, [&](const auto &visit, const auto &) {
        AiScheduleResponse response;
        if (visit.phase == AiSchedulePhase::human_execution) {
            const auto effects = advance_actor_effects(next.effects.at(*visit.id));
            if (!effects.candidate) {
                response.accepted = false;
                return response;
            }
            next.definition.effects = effects.candidate->state;
            const auto growth = prepare_human_growth(next.definition);
            if (!growth.candidate) {
                response.accepted = false;
                return response;
            }
            const auto &c = *growth.candidate;
            next.definition.definition = c.definition;
            next.definition.experience = c.experience;
            next.definition.pending = c.pending;
            next.definition.notice_pending = c.notice_pending;
            next.definition.notice_attributes = c.notice_attributes;
            next.effects.at(*visit.id) = c.effects;
            for (const auto &request : c.requests) {
                next.requests.push_back(request);
                if (request.kind == HumanGrowthRequestKind::event109)
                    next.definition.event109_seen = true;
            }
        } else if (visit.phase == AiSchedulePhase::finalize && reject_finalize)
            response.accepted = false;
        return response;
    });
    if (!plan.candidate)
        return false;
    live = std::move(next);
    return true;
}
Owner owner() {
    Owner value;
    value.definition.definition.base = {22, 2, 2, 2, 2, 2};
    value.definition.definition.profession_levels = {1};
    HumanProfessionRule profession;
    profession.attribute_percent.fill(100);
    profession.maximum_growth.fill(9);
    value.definition.professions = {profession};
    value.definition.experience = 11;
    value.definition.pending = {9, 0};
    value.effects = {{1, {}}, {2, {{{24, 0}}, {}}}};
    value.hp = {{1, 7}, {2, 9}};
    return value;
}
void shared_growth() {
    auto live = owner();
    check(!advance_owner(live, true) && live.definition.definition.profession_levels[0] == 1 &&
              live.definition.experience == 11 && live.definition.pending.counter == 0 &&
              live.effects.at(2).display == std::vector<ActorEffectRecord>{{24, 0}} &&
              live.requests.empty(),
          "finalize refusal cannot leak earlier private growth, effects or requests");
    check(advance_owner(live, false) && live.definition.definition.profession_levels[0] == 2 &&
              live.definition.experience == 0 && live.definition.pending.amount == 0 &&
              live.effects.at(2).display == std::vector<ActorEffectRecord>{{14, 0}} &&
              live.effects.at(1).display.empty() && live.hp.at(1) == 7 && live.hp.at(2) == 9,
          "reverse-last actor upgrades shared definition; new14 waits next round, HP unchanged");
    check(live.requests.size() == 2 && live.requests[0].kind == HumanGrowthRequestKind::event109 &&
              live.requests[1].kind == HumanGrowthRequestKind::report_growth,
          "event109 precedes report; second actor cannot duplicate the discarded reward");
    check(advance_owner(live, false) &&
              live.effects.at(2).display == std::vector<ActorEffectRecord>{{14, 1}} &&
              live.requests.size() == 2,
          "next effects pass advances newly added growth display once");
    live = owner();
    live.definition.experience = 0;
    check(advance_owner(live, false) && live.definition.experience == 2 &&
              live.definition.pending.counter == 2,
          "two instances sharing one definition each advance N/O in the same world round");
}
void equipment_timeline() {
    const auto choice = prepare_weapon_choice({{0, 0, true}, {7, 2, true}}, 0, 0, 1);
    check(choice.candidate && choice.candidate->weapon_id == 7,
          "explicit catalogue ticket chooses the researched candidate");
    EquipmentExitTailInput exit;
    exit.new_weapon = choice.candidate->weapon_id;
    ActorControlState control;
    control.queue = *prepare_equipment_exit_tail(exit);
    auto prefix = prepare_local_control_prefix(control, {{}, true});
    check(prefix.candidate && prefix.candidate->flow == ActorControlFlow::departure_started &&
              prefix.candidate->state.queue.front()[0] == 20,
          "activity0 starts first and stops before equipment display/commit");
    control = prefix.candidate->state;
    for (int tick = 0; tick < 5; ++tick) {
        prefix = prepare_local_control_prefix(control);
        check(prefix.candidate &&
                  prefix.candidate->flow ==
                      (tick < 4 ? ActorControlFlow::waiting : ActorControlFlow::delegated),
              "initial wait counts down before weapon presentation");
        control = prefix.candidate->state;
    }
    check(control.queue.front()[0] == 27 && !prepare_equipment_commit(control.queue.front()),
          "weapon display cannot equip or reset the definition counter");
    control.queue.erase(control.queue.begin()); // Explicit external display handler acceptance.
    for (int tick = 0; tick < 42; ++tick) {
        prefix = prepare_local_control_prefix(control);
        check(prefix.candidate &&
                  prefix.candidate->flow ==
                      (tick < 41 ? ActorControlFlow::waiting : ActorControlFlow::delegated),
              "42-step presentation wait precedes actual equipment commit");
        control = prefix.candidate->state;
    }
    const auto commit = prepare_equipment_commit(control.queue.front());
    check(commit && commit->slot == 0 && commit->equipment == 7 && commit->reselect_counter == 6 &&
              !(control.flags & 64),
          "28 equips selected weapon after clearing64 and resets reselect count");
    HumanDefinitionStatsInput definition;
    definition.base = {22, 2, 2, 2, 2, 2};
    definition.profession_levels = {1};
    definition.equipment[commit->slot] = std::array<int, 4>{0, 5, 0, 0};
    HumanProfessionRule rule;
    rule.attribute_percent.fill(100);
    const auto stats = derive_human_stats(definition, {rule});
    check(stats.candidate && stats.candidate->combat == std::array<int, 4>{22, 7, 2, 2} &&
              stats.candidate->attributes == definition.base,
          "equipment adjusts combat after definition attributes, not actor HP");
    Game game;
    check(!game.state().adventurer && game.state().money == 5000,
          "pure conditional composition cannot alter the real new-game aggregate");
}
} // namespace
int main() {
    rewards();
    shared_growth();
    equipment_timeline();
    std::cout << checks << " checks passed\n";
}
