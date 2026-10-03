#include "dungeon_village_reference/character_hp.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

using namespace dungeon_village_reference;

namespace {

int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool same(const CharacterHpState &left, const CharacterHpState &right) {
    return left.requested_delta == right.requested_delta && left.displayed == right.displayed &&
           left.origin == right.origin && left.target == right.target &&
           left.animating == right.animating && left.legacy_tick == right.legacy_tick;
}

CharacterHpState success(const CharacterHpResult &result) {
    check(result.error == CharacterHpError::none && result.candidate.has_value(),
          "valid hp transition has complete candidate");
    return *result.candidate;
}

void timelines() {
    const CharacterHpState original{0, 100, 100, 100, false, 0};
    const auto damage = success(prepare_hp_change(original, -50, 100));
    check(damage.target == 50 && damage.origin == 100 && damage.displayed == 100 &&
              damage.animating && damage.requested_delta == -50 && damage.legacy_tick == 0,
          "damage changes actual target immediately but not displayed value");
    const auto recovery = success(prepare_hp_change({0, 100, 100, 100, false, 0}, 50, 200));
    auto damaged = damage;
    auto recovered = recovery;
    for (int tick = 0; tick < 30; ++tick) {
        damaged = success(advance_hp_animation(damaged));
        recovered = success(advance_hp_animation(recovered));
        const auto elapsed = std::clamp(tick - 10, 0, 9);
        check(damaged.displayed == 100 - elapsed * 50 / 9 && damaged.target == 50 &&
                  damaged.legacy_tick == tick + 1 && damaged.animating == (tick < 29),
              "negative change uses denominator nine then holds exact target");
        check(recovered.displayed == 100 + elapsed * 50 / 10 && recovered.target == 150 &&
                  recovered.legacy_tick == tick + 1 && recovered.animating == (tick < 29),
              "positive change uses original amount over ten, no invented final snap");
    }
    check(recovered.displayed == 145 && recovered.target == 150,
          "uncapped recovery helper stops before full display value");
    check(same(success(advance_hp_animation(recovered, 1000000)), recovered),
          "inactive transition ignores additional logical steps");
    const auto full = success(prepare_hp_change({0, 50, 50, 50, false, 0}, 100, 100));
    check(full.requested_delta == 100 && full.target == 100,
          "full recovery preserves requested amount despite target cap");
    check(success(advance_hp_animation(full, 20)).displayed == 100,
          "capped full recovery can reach target before animation ends");
    const auto dead = success(prepare_hp_change(original, -150, 100));
    check(dead.target == -50 && success(advance_hp_animation(dead, 30)).displayed == -50,
          "damage can remain below zero without invented floor");
    const auto zero = success(prepare_hp_change(original, 0, 50));
    check(zero.target == 100 && zero.animating &&
              success(advance_hp_animation(zero, 30)).displayed == 100,
          "zero request starts animation and does not apply positive capacity cap");
    check(same(original, {0, 100, 100, 100, false, 0}), "hp preparation leaves input unchanged");
}

void interruption_and_assignment() {
    const auto initial = success(prepare_hp_change({0, 100, 100, 100, false, 0}, -50, 100));
    const auto halfway = success(advance_hp_animation(initial, 15));
    check(halfway.displayed > halfway.target, "damage display remains behind actual target");
    const auto restarted = success(prepare_hp_change(halfway, 20, 100));
    check(restarted.origin == 50 && restarted.displayed == 50 && restarted.target == 70 &&
              restarted.requested_delta == 20 && restarted.legacy_tick == 0,
          "new change restarts from actual target not halfway display");
    const auto assigned = success(prepare_hp_assignment(halfway, 100));
    check(assigned.displayed == 100 && assigned.origin == 100 && assigned.target == 100 &&
              assigned.animating == halfway.animating &&
              assigned.legacy_tick == halfway.legacy_tick &&
              assigned.requested_delta == halfway.requested_delta,
          "direct assignment preserves old animation metadata");
    const auto resumed = success(advance_hp_animation(assigned, 30));
    check(resumed.displayed == 100 && resumed.target == 100 && !resumed.animating,
          "negative interpolation after direct assignment keeps equal endpoints");
    const auto unchanged = success(advance_hp_animation(halfway, 0));
    check(same(halfway, unchanged), "zero animation steps preserve all fields");
    for (std::uint32_t split = 0; split <= 40; ++split) {
        const auto first = success(advance_hp_animation(initial, split));
        const auto second = success(advance_hp_animation(first, 40 - split));
        check(same(second, success(advance_hp_animation(initial, 40))),
              "damage animation logical-step partition is equivalent");
        const auto first_recovery = success(advance_hp_animation(restarted, split));
        check(same(success(advance_hp_animation(first_recovery, 40 - split)),
                   success(advance_hp_animation(restarted, 40))),
              "recovery animation logical-step partition is equivalent");
    }
}

void extrema_and_rejections() {
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    const auto minimum = std::numeric_limits<std::int32_t>::min();
    const CharacterHpState low{0, minimum, minimum, minimum, false, 0};
    const auto wide = success(prepare_hp_change(low, maximum, 0));
    check(wide.target == -1 && success(advance_hp_animation(wide, 30)).displayed < -1,
          "large positive animation multiplication uses wide integer without wrap");
    const CharacterHpState large_damage{minimum, maximum, maximum, minimum, true, 10};
    check(success(advance_hp_animation(large_damage, 10)).displayed == minimum,
          "large negative interpolation endpoint difference uses wide arithmetic");
    const auto negative_overflow = prepare_hp_change(low, -1, 100);
    check(negative_overflow.error == CharacterHpError::numeric_overflow &&
              !negative_overflow.candidate,
          "target underflow rejected with no partial animation");
    const auto positive_overflow =
        prepare_hp_change({0, maximum, maximum, maximum, false, 0}, 1, 100);
    check(positive_overflow.error == CharacterHpError::numeric_overflow &&
              !positive_overflow.candidate,
          "uncapped sum overflow rejected before applying positive cap");
    check(success(prepare_hp_assignment(low, maximum)).target == maximum,
          "direct assignment accepts signed int32 endpoints");
    const CharacterHpState ordinary;
    const auto capacity = prepare_hp_change(ordinary, 1, -1);
    check(capacity.error == CharacterHpError::invalid_input && !capacity.candidate,
          "negative capacity rejected");
    const auto budget = advance_hp_animation(ordinary, 1000001);
    check(budget.error == CharacterHpError::invalid_input && !budget.candidate,
          "step resource budget rejected even for inactive state");
    for (const auto state :
         {CharacterHpState{0, 0, 0, 0, false, -1}, CharacterHpState{0, 0, 0, 0, false, 31},
          CharacterHpState{0, 0, 0, 0, true, 30}}) {
        for (const auto result : {prepare_hp_change(state, 0, 100), advance_hp_animation(state, 0),
                                  prepare_hp_assignment(state, 100)}) {
            check(result.error == CharacterHpError::invalid_input && !result.candidate,
                  "every hp operation rejects invalid input with no state");
        }
    }
}

using Slots = std::array<std::int32_t, 6>;

void oracle_step(Slots &slots) {
    if (slots[4] != 1) {
        return;
    }
    if (slots[5] >= 10 && slots[5] < 20) {
        if (slots[0] < 0) {
            slots[1] = slots[2] + (slots[5] - 10) * (slots[3] - slots[2]) / 9;
        } else {
            slots[1] = std::min(slots[2] + (slots[5] - 10) * slots[0] / 10, slots[3]);
        }
    }
    ++slots[5];
    if (slots[5] >= 30) {
        slots[4] = 0;
    }
}

CharacterHpState from_slots(const Slots &slots) {
    return {slots[0], slots[1], slots[2], slots[3], slots[4] == 1, slots[5]};
}

void mixed_operation_oracle() {
    std::mt19937 random(0xB100DA7U);
    for (int trial = 0; trial < 1000; ++trial) {
        Slots slots{0, 200, 200, 200, 0, 0};
        auto state = from_slots(slots);
        for (int operation = 0; operation < 100; ++operation) {
            switch (random() % 3U) {
            case 0: {
                const auto delta = static_cast<std::int32_t>(random() % 601U) - 300;
                const auto capacity = static_cast<std::int32_t>(random() % 501U);
                slots[0] = delta;
                slots[4] = 1;
                slots[5] = 0;
                slots[2] = slots[3];
                slots[3] += delta;
                slots[1] = slots[2];
                if (delta > 0 && slots[3] >= capacity) {
                    slots[3] = capacity;
                }
                state = success(prepare_hp_change(state, delta, capacity));
                break;
            }
            case 1: {
                const auto value = static_cast<std::int32_t>(random() % 601U) - 100;
                slots[1] = value;
                slots[2] = value;
                slots[3] = value;
                state = success(prepare_hp_assignment(state, value));
                break;
            }
            case 2: {
                const auto steps = random() % 41U;
                for (std::uint32_t step = 0; step < steps; ++step) {
                    oracle_step(slots);
                }
                state = success(advance_hp_animation(state, steps));
                break;
            }
            }
            check(same(state, from_slots(slots)),
                  "mixed restart, assignment and animation match independent six-slot oracle");
        }
    }
}

} // namespace

int main() {
    timelines();
    interruption_and_assignment();
    extrema_and_rejections();
    mixed_operation_oracle();
    std::cout << checks << " checks passed\n";
}
