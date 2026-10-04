#include "ark/simulation/rules/actor_control.hpp"
#include "ark/simulation/rules/actor_effects.hpp"

#include <array>
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
void every_duration() {
    constexpr std::array<int, 27> expected{32, 10, 10, 20, 27, 16, 16, 24, 67, 10, 5,  30, 13, 40,
                                           48, 40, 19, 9,  9,  11, 8,  22, 22, 6,  72, 20, 20};
    for (int effect = 0; effect < 27; ++effect)
        for (int delay : {0, 1, 6, 16, 44}) {
            ActorEffectState s;
            s.display = {{effect, -delay, effect == 16 ? 4 : 13, 99, 7}};
            for (int tick = 1; tick <= delay + expected[effect]; ++tick) {
                const auto c = advance_actor_effects(s);
                check(c.candidate.has_value(), "all27 valid display scheduling records");
                s = c.candidate->state;
                check(
                    s.display.empty() == (tick == delay + expected[effect]),
                    "display expiry uses new counter with exact duration including negative delay");
                if (!s.display.empty())
                    check(s.display[0][1] == tick - delay && s.display[0][3] == 99,
                          "counter alone changes, opaque rendering payload preserved");
            }
        }
    for (int spell = 0; spell < 7; ++spell) {
        const int duration = spell < 4 ? 0 : spell == 4 ? 19 : spell == 5 ? 24 : 11;
        ActorEffectState s;
        s.display = {{16, duration - 1, spell}};
        check(advance_actor_effects(s).candidate->state.display.empty(),
              "cd16 duration sums4/5/6 frame sequences19/24/11; empty sequences expire");
    }
    ActorEffectState s;
    s.display = {{10, 4}, {10, 4}, {10, 4}};
    auto c = advance_actor_effects(s);
    check(c.candidate && c.candidate->state.display == std::vector<ActorEffectRecord>{{10, 4}} &&
              c.candidate->removed_display_indices == std::vector<std::size_t>{0, 1},
          "forward cd erase skips shifted middle, does not advance all original effects");
    check(advance_actor_effects(c.candidate->state).candidate->state.display.empty(),
          "skipped adjacent effect expires next round");
}
void late_dispatch() {
    ActorEffectState s;
    s.delayed = {{4, 0, 10, 20}, {5, 1, 30, 40}, {6, -1, 50, 60}};
    s.display = {{24, 0, 0, 100}};
    auto c = advance_actor_effects(s);
    check(c.candidate && c.candidate->sounds.size() == 2 && c.candidate->sounds[0].sound == 19 &&
              c.candidate->sounds[1].sound == 17,
          "ce reverse dispatch gives sound6 before4, at cached actor location");
    check(c.candidate->state.delayed == std::vector<ActorEffectRecord>{{5, 0, 30, 40}} &&
              c.candidate->state.display == std::vector<ActorEffectRecord>{{16, 1, 4, 10, 20},
                                                                           {16, 1, 6, 50, 60},
                                                                           {24, 1, 0, 100}},
          "old1 delay decrements0 without firing; each due16 inserts front then increments same "
          "round");
    c = advance_actor_effects(c.candidate->state);
    check(c.candidate && c.candidate->sounds[0].sound == 18 && c.candidate->state.delayed.empty() &&
              c.candidate->state.display[0][1] == 1,
          "next round delay0 fires; inserted display starts new1");
    for (int delay = 0; delay <= 10; ++delay) {
        s = {};
        s.delayed = {{4, delay, 0, 0}};
        for (int tick = 0; tick <= delay; ++tick) {
            c = advance_actor_effects(s);
            check(c.candidate && c.candidate->sounds.empty() == (tick < delay),
                  "ce delay6 fires on seventh update, not sixth");
            s = c.candidate->state;
        }
    }
}
void expressions() {
    constexpr std::array<int, 19> gate{200, 200, 1000, 3,    1000, 1000, 0,    1000, 2,   1000,
                                       300, 300, 1000, 1000, 1000, 1000, 1000, 3,    1000};
    ActorExpressionInput i;
    i.delay = 5;
    i.variant_count = 3;
    i.variant_ticket = 2;
    for (int expression = 0; expression < 19; ++expression)
        for (int ticket = 0; ticket < 1000; ++ticket) {
            i.expression = expression;
            i.probability_ticket = ticket;
            const auto c = prepare_actor_expression(i);
            check(c.candidate && c.candidate->probability_passed == (ticket < gate[expression]) &&
                      c.candidate->inserted == (ticket < gate[expression]) &&
                      c.candidate->consumed_variant == (ticket < gate[expression]),
                  "all19 expression probability1000 thresholds, strict less");
        }
    i.expression = 5;
    i.probability_ticket = 999;
    i.variant_ticket.reset();
    for (int blocker : {12, 24}) {
        i.state.display = {{blocker, -50, 30}};
        const auto c = prepare_actor_expression(i);
        check(c.candidate && c.candidate->probability_passed && c.candidate->suppressed &&
                  !c.candidate->consumed_variant && c.candidate->state.display == i.state.display,
              "probability draw happened before12/24 suppresses, negative counter also blocks");
    }
    i.state.display = {{13, 0, 0}};
    check(prepare_actor_expression(i).error == ActorEffectError::missing_ticket,
          "attribute display13 does not suppress; success needs variant draw");
    i.variant_ticket = 3;
    check(prepare_actor_expression(i).error == ActorEffectError::invalid_ticket,
          "variant equals bound invalid, no partial front insert");
    i.variant_ticket = 0;
    const auto c = prepare_actor_expression(i);
    check(c.candidate && c.candidate->state.display[0] == ActorEffectRecord{12, -5, 30, 5, 0},
          "new expression inserts at front with negative delay, duration30, ID/variant intact");
    i.probability_ticket = 1000;
    check(prepare_actor_expression(i).error == ActorEffectError::invalid_ticket,
          "probability ticket1000 out of bound even if later blocked");
}
void counter_order() {
    ActorCounterState s{0, 0, 0, 7, 1, true, 99, 2};
    const auto prefix = advance_actor_counters(s);
    check(prefix && prefix->alternate == 1 && prefix->action == 1 && prefix->state == 1 &&
              prefix->hit_flash == 6 && prefix->hit_label == 1 && prefix->damage_total == 99,
          "d prefix advances i/l/B/aw, does not expire label before effects");
    const auto label = expire_actor_hit_label(*prefix);
    check(label && !label->miss_label && label->hit_label == 0 && label->damage_total == 0 &&
              label->hits == 0,
          "aq reaching0 clears ar/ao/ap after display pass");
    s.hit_label = 0;
    check(expire_actor_hit_label(s)->damage_total == 99,
          "already aq0 does not reset lingering hit stats");
    s.state = s.action = s.alternate = std::numeric_limits<int>::max() - 1;
    const auto wrap = advance_actor_counters(s);
    check(wrap && wrap->state == 0 && wrap->action == 0 && wrap->alternate == 0,
          "modulo max at max-1 supported without C++ overflow");
    s.state = std::numeric_limits<int>::max();
    check(!advance_actor_counters(s), "out-of-maintenance counter rejects Java overflow emulation");
    ActorEffectState e;
    e.display = {{24, 71, 0, 10}};
    e = advance_actor_effects(e).candidate->state;
    ActorExpressionInput expression{e, 9, 0, 0, 1, 0};
    const auto shown = prepare_actor_expression(expression);
    check(shown.candidate && shown.candidate->inserted && shown.candidate->state.display[0][1] == 0,
          "control expression after display expiry inserts counter0, waits next d to advance");
    e = advance_actor_effects(shown.candidate->state).candidate->state;
    check(e.display[0][1] == 1, "new control-generated expression advances only next round");
}
void errors() {
    for (const ActorEffectRecord &r : {ActorEffectRecord{},
                                       {27, 0},
                                       {12, 0},
                                       {16, 0, 7},
                                       {0, std::numeric_limits<int>::max()}}) {
        ActorEffectState s;
        s.display = {{10, 0}, r};
        check(!advance_actor_effects(s).candidate && s.display[0][1] == 0,
              "malformed display tail does not partly advance front");
    }
    ActorEffectState s;
    s.delayed = {{4, 0, 1, 2}, {7, 0, 1, 2}};
    check(!advance_actor_effects(s).candidate && s.display.empty(),
          "invalid delayed spell index rejects entire dispatch plan");
}
} // namespace
int main() {
    try {
        every_duration();
        late_dispatch();
        expressions();
        counter_order();
        errors();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
