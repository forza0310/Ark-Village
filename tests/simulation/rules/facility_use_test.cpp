// Boundary checks are active in Release; input rejection never exposes a partial candidate.
#include "ark/simulation/actors/rules/character_hp.hpp"
#include "ark/simulation/facilities/rules/facility_use.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace ref = ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
ref::FacilityUseInput input(int category = 2) {
    return {ref::CharacterId{1},
            ref::BuildingId{4},
            category == 2 ? 28 : 33,
            category,
            0,
            0,
            60,
            2U | 16U | 8192U | 512U | 1024U | 32768U};
}
void timing() {
    for (int activity : {0, 1}) {
        auto data = input();
        data.legacy_activity = activity;
        const auto start = ref::prepare_facility_use(data);
        check(start.candidate && start.candidate->state.duration == 200 &&
                  start.candidate->state.phase == ref::FacilityUsePhase::queued &&
                  !start.candidate->register_occupation,
              "inn ignores definition wait; preparation does not occupy");
        auto state = start.candidate->state;
        check(state.legacy_flags == (2U | 8192U), "immediate inn clear, queued flag commands");
        int occupations = 0, recoveries = 0, exits = 0;
        ref::CharacterHpState hp{0, 7, 7, 7, false, 0}; // Synthetic injured actor.
        for (int step = 1; step <= 200; ++step) {
            const auto result = ref::advance_facility_use(state);
            check(result.error == ref::FacilityUseError::none && result.candidate,
                  "valid uninterrupted pair");
            const auto &next = *result.candidate;
            check(next.register_occupation == (step == 1) &&
                      next.request_capacity_hp == (step == 171) &&
                      next.request_exit == (step == 200),
                  "first d occupies; c reads B170 on pair171; pair200 requests same-d exit");
            occupations += next.register_occupation;
            recoveries += next.request_capacity_hp;
            exits += next.request_exit;
            if (next.request_capacity_hp) {
                const auto change = ref::prepare_hp_change(hp, 22, 22);
                check(change.candidate && change.candidate->target == 22 &&
                          change.candidate->requested_delta == 22,
                      "request full capacity, not missing HP, clip target");
                hp = *change.candidate;
            }
            hp = *ref::advance_hp_animation(hp).candidate;
            if (step == 171)
                check(hp.legacy_tick == 1 && hp.target == 22 && hp.displayed == 7,
                      "same-d HP animation follows c recovery");
            if (step == 199)
                check(hp.animating && hp.legacy_tick == 29, "animation still active before exit");
            if (step == 200)
                check(!hp.animating && hp.legacy_tick == 30, "animation finishes on exit d");
            state = next.state;
            check(state.elapsed == step && state.remaining == 200 - step &&
                      state.legacy_flags == (2U | 8192U | 1U | 32U),
                  "B and wait counters are distinct; no premature exit flag clearing");
        }
        check(occupations == 1 && recoveries == 1 && exits == 1,
              "one request each per committed use");
        check(state.phase == ref::FacilityUsePhase::exit_ready, "boundary is not completed exit");
        const auto retry = ref::advance_facility_use(state);
        check(retry.error == ref::FacilityUseError::exit_pending && !retry.candidate,
              "cannot repeat completion by advancing boundary");
    }
    for (const int wait : {0, 1, 2, 60, 170, 200, 201}) {
        auto data = input(1);
        data.definition_wait = wait;
        auto state = ref::prepare_facility_use(data).candidate->state;
        check((state.legacy_flags & (512U | 1024U | 32768U)) == (512U | 1024U | 32768U),
              "food does not apply inn-specific clearing");
        const int expected = wait == 0 ? 1 : wait;
        for (int step = 1; step <= expected; ++step) {
            const auto next = *ref::advance_facility_use(state).candidate;
            check(!next.request_capacity_hp && next.request_exit == (step == expected) &&
                      next.register_occupation == (step == 1),
                  "food wait has no inn recovery even at B170");
            state = next.state;
        }
        check((state.legacy_flags & 16U) == 0 && (state.legacy_flags & 1U) != 0,
              "entry clear and control-set flags");
    }
}
void rejection() {
    const auto reject = [](auto edit, ref::FacilityUseError expected) {
        auto data = input();
        edit(data);
        const auto result = ref::prepare_facility_use(data);
        check(result.error == expected && !result.candidate,
              "unsupported/invalid use has no candidate");
    };
    reject([](auto &v) { v.character_id.value = 0; }, ref::FacilityUseError::invalid_input);
    reject([](auto &v) { v.instance_id.value = 0; }, ref::FacilityUseError::invalid_input);
    reject([](auto &v) { v.definition_id = -1; }, ref::FacilityUseError::invalid_input);
    reject([](auto &v) { v.definition_wait = -1; }, ref::FacilityUseError::invalid_input);
    reject([](auto &v) { v.definition_wait = std::numeric_limits<int>::max(); },
           ref::FacilityUseError::invalid_input);
    for (const int category : {0, 3, 5, 9})
        reject([&](auto &v) { v.legacy_category = category; },
               ref::FacilityUseError::unsupported_branch);
    for (const int detail : {1, 2, 4, 5})
        reject([&](auto &v) { v.legacy_detail = detail; },
               ref::FacilityUseError::unsupported_branch);
    reject([](auto &v) { v.legacy_activity = 2; }, ref::FacilityUseError::unsupported_branch);
    const auto broken = [](auto edit) {
        auto state = ref::prepare_facility_use(input()).candidate->state;
        edit(state);
        const auto result = ref::advance_facility_use(state);
        check(result.error == ref::FacilityUseError::invalid_state && !result.candidate,
              "reject inconsistent phase or counter without mutation");
    };
    broken([](auto &v) { v.remaining = 199; });
    broken([](auto &v) { v.elapsed = -1; });
    broken([](auto &v) { v.elapsed = 201; });
    broken([](auto &v) { v.duration = 60; });
    broken([](auto &v) { v.phase = ref::FacilityUsePhase::in_use; });
    broken([](auto &v) { v.phase = ref::FacilityUsePhase::exit_ready; });
    broken([](auto &v) { v.phase = static_cast<ref::FacilityUsePhase>(99); });
    auto last = ref::prepare_facility_use(input(1)).candidate->state;
    last.input.definition_wait = std::numeric_limits<std::int32_t>::max() - 1;
    last.duration = last.input.definition_wait;
    last.elapsed = last.duration - 1;
    last.remaining = 1;
    last.phase = ref::FacilityUsePhase::in_use;
    const auto final = ref::advance_facility_use(last);
    check(final.candidate && final.candidate->request_exit &&
              final.candidate->state.elapsed == last.duration,
          "largest supported counter completes without signed overflow or Java modulo");
}
} // namespace
int main() {
    try {
        timing();
        rejection();
        std::cout << "facility use timing checks=" << checks << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
