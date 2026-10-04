// Six-slot target/display protocol using legacy logical counts, not wall-clock seconds.
// Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/rules/character_hp.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
namespace {

bool valid(const CharacterHpState &state) {
    return state.legacy_tick >= 0 && state.legacy_tick <= 30 &&
           (!state.animating || state.legacy_tick < 30);
}

bool fits(std::int64_t value) {
    return value >= std::numeric_limits<std::int32_t>::min() &&
           value <= std::numeric_limits<std::int32_t>::max();
}

} // namespace

CharacterHpResult prepare_hp_change(const CharacterHpState &state, std::int32_t delta,
                                    std::int32_t capacity) {
    if (!valid(state) || capacity < 0) {
        return {CharacterHpError::invalid_input, std::nullopt};
    }
    const auto target = static_cast<std::int64_t>(state.target) + delta;
    if (!fits(target)) {
        return {CharacterHpError::numeric_overflow, std::nullopt};
    }
    return {CharacterHpError::none,
            CharacterHpState{delta, state.target, state.target,
                             static_cast<std::int32_t>(
                                 delta > 0 ? std::min<std::int64_t>(target, capacity) : target),
                             true, 0}};
}

CharacterHpResult advance_hp_animation(const CharacterHpState &state, std::uint32_t steps) {
    if (!valid(state) || steps > 1000000U) {
        return {CharacterHpError::invalid_input, std::nullopt};
    }
    auto next = state;
    for (std::uint32_t step = 0; step < steps && next.animating; ++step) {
        if (next.legacy_tick >= 10 && next.legacy_tick < 20) {
            const auto elapsed = static_cast<std::int64_t>(next.legacy_tick - 10);
            const auto value =
                next.requested_delta < 0
                    ? next.origin +
                          elapsed * (static_cast<std::int64_t>(next.target) - next.origin) / 9
                    : std::min<std::int64_t>(next.origin + elapsed * next.requested_delta / 10,
                                             next.target);
            if (!fits(value)) {
                return {CharacterHpError::numeric_overflow, std::nullopt};
            }
            next.displayed = static_cast<std::int32_t>(value);
        }
        ++next.legacy_tick;
        if (next.legacy_tick >= 30) {
            next.animating = false;
        }
    }
    return {CharacterHpError::none, next};
}

CharacterHpResult prepare_hp_assignment(const CharacterHpState &state, std::int32_t value) {
    if (!valid(state)) {
        return {CharacterHpError::invalid_input, std::nullopt};
    }
    auto next = state;
    next.displayed = value;
    next.origin = value;
    next.target = value;
    return {CharacterHpError::none, next};
}

} // namespace ark::simulation::rules
