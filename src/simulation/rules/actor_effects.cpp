#include "ark/simulation/rules/actor_effects.hpp"

#include <array>
#include <limits>

namespace ark::simulation::rules {
namespace {
constexpr std::array<int, 27> durations{32, 10, 10, 20, 27, 16, 16, 24, 67, 10, 5,  30, -1, 40,
                                        48, 40, -1, 9,  9,  11, 8,  22, 22, 6,  72, 20, 20};
constexpr std::array<int, 7> spell_durations{0, 0, 0, 0, 19, 24, 11};
constexpr std::array<int, 19> probability{200, 200, 1000, 3,    1000, 1000, 0,    1000, 2,   1000,
                                          300, 300, 1000, 1000, 1000, 1000, 1000, 3,    1000};
constexpr std::array<int, 19> expression_duration{8,  12, 30, 30, 30, 30, 12, 12, 30, 30,
                                                  30, 30, 30, 30, 30, 30, 30, 30, 30};
bool valid_display(const ActorEffectRecord &r) {
    if (r.size() < 2 || r[0] < 0 || r[0] >= 27 || r[1] == std::numeric_limits<int>::max())
        return false;
    if (r[0] == 12 && (r.size() < 3 || r[2] < 0))
        return false;
    if (r[0] == 16 && (r.size() < 3 || r[2] < 0 || r[2] >= 7))
        return false;
    return true;
}
} // namespace
bool valid_actor_effect_state(const ActorEffectState &s) {
    if (s.display.size() > 1000000 || s.delayed.size() > 1000000 ||
        s.display.size() + s.delayed.size() > 1000000)
        return false;
    for (const auto &r : s.display)
        if (!valid_display(r))
            return false;
    for (const auto &r : s.delayed)
        if (r.size() < 4 || r[0] < 0 || r[0] >= 7)
            return false;
    return true;
}
namespace {
bool valid_counters(const ActorCounterState &s) {
    const auto count = [](int value) {
        return value >= 0 && value < std::numeric_limits<int>::max();
    };
    return count(s.alternate) && count(s.action) && count(s.state) && s.hit_flash >= 0 &&
           s.hit_label >= 0 && s.damage_total >= 0 && s.hits >= 0;
}
} // namespace
ActorEffectStepResult advance_actor_effects(const ActorEffectState &s) {
    if (!valid_actor_effect_state(s))
        return {ActorEffectError::invalid_input, std::nullopt};
    ActorEffectStepCandidate c{s, {}, {}};
    for (std::size_t n = c.state.delayed.size(); n > 0; --n) {
        auto &r = c.state.delayed[n - 1];
        if (r[1] > 0) {
            --r[1];
        } else {
            if (r[0] >= 4)
                c.sounds.push_back({r[0] + 13});
            c.state.display.insert(c.state.display.begin(), {16, 0, r[0], r[2], r[3]});
            c.state.delayed.erase(c.state.delayed.begin() + static_cast<std::ptrdiff_t>(n - 1));
        }
    }
    for (std::size_t n = 0; n < c.state.display.size(); ++n) {
        auto &r = c.state.display[n];
        const int duration = r[0] == 12   ? r[2]
                             : r[0] == 16 ? spell_durations[r[2]]
                                          : durations[r[0]];
        ++r[1];
        if (r[1] >= duration) {
            c.removed_display_indices.push_back(n);
            c.state.display.erase(c.state.display.begin() + static_cast<std::ptrdiff_t>(n));
        }
        // No index correction: the shifted next cd is deliberately not advanced this round.
    }
    return {ActorEffectError::none, c};
}
ActorExpressionResult prepare_actor_expression(const ActorExpressionInput &i) {
    if (!valid_actor_effect_state(i.state) || i.expression < 0 || i.expression >= 19 ||
        i.delay < 0 || i.variant_count <= 0 || i.state.display.size() >= 1000000)
        return {ActorEffectError::invalid_input, std::nullopt};
    if (i.probability_ticket < 0 || i.probability_ticket >= 1000)
        return {ActorEffectError::invalid_ticket, std::nullopt};
    ActorExpressionCandidate c{i.state, false, false, false, false};
    if (i.probability_ticket >= probability[i.expression])
        return {ActorEffectError::none, c};
    c.probability_passed = true;
    for (const auto &r : c.state.display)
        if (r[0] == 12 || r[0] == 24) {
            c.suppressed = true;
            return {ActorEffectError::none, c};
        }
    if (!i.variant_ticket)
        return {ActorEffectError::missing_ticket, std::nullopt};
    if (*i.variant_ticket < 0 || *i.variant_ticket >= i.variant_count)
        return {ActorEffectError::invalid_ticket, std::nullopt};
    c.state.display.insert(
        c.state.display.begin(),
        {12, -i.delay, expression_duration[i.expression], i.expression, *i.variant_ticket});
    c.inserted = c.consumed_variant = true;
    return {ActorEffectError::none, c};
}
std::optional<ActorCounterState> advance_actor_counters(const ActorCounterState &s) {
    if (!valid_counters(s))
        return std::nullopt;
    ActorCounterState c = s;
    c.alternate = (s.alternate + 1) % std::numeric_limits<int>::max();
    c.action = (s.action + 1) % std::numeric_limits<int>::max();
    c.state = (s.state + 1) % std::numeric_limits<int>::max();
    if (c.hit_flash > 0)
        --c.hit_flash;
    return c;
}
std::optional<ActorCounterState> expire_actor_hit_label(const ActorCounterState &s) {
    if (!valid_counters(s))
        return std::nullopt;
    ActorCounterState c = s;
    if (c.hit_label > 0 && --c.hit_label == 0) {
        c.miss_label = false;
        c.damage_total = c.hits = 0;
    }
    return c;
}
} // namespace ark::simulation::rules
