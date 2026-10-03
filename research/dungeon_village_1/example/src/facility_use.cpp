// Bounded wait protocol, not a translation of the original control interpreter.
// Evidence, phase ordering and unsupported branches: ../rules/FACILITY_USE.md#use-timing.
#include "dungeon_village_reference/facility_use.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
FacilityUseError validate(const FacilityUseInput &input) {
    if (input.character_id.value == 0 || input.instance_id.value == 0 || input.definition_id < 0 ||
        input.legacy_category < 0 || input.legacy_detail < 0 || input.legacy_activity < 0 ||
        input.definition_wait < 0 ||
        input.definition_wait == std::numeric_limits<std::int32_t>::max())
        return FacilityUseError::invalid_input;
    if ((input.legacy_category != 1 && input.legacy_category != 2) || input.legacy_detail != 0 ||
        input.legacy_activity > 1)
        return FacilityUseError::unsupported_branch;
    return FacilityUseError::none;
}
std::int32_t duration(const FacilityUseInput &input) {
    // Even a zero wait consumes one interpreter visit; the decrement is tested afterwards.
    return input.legacy_category == 2 ? 200 : std::max(input.definition_wait, 1);
}
} // namespace

FacilityUseTimingResult prepare_facility_use(const FacilityUseInput &input) {
    const auto error = validate(input);
    if (error != FacilityUseError::none)
        return {error, std::nullopt};
    auto flags = input.legacy_flags & ~16U; // Entering state 14 clears bit16.
    if (input.legacy_category == 2)
        flags &= ~(512U | 1024U | 32768U);
    const auto wait = duration(input);
    return {FacilityUseError::none,
            FacilityUseStep{
                {input, FacilityUsePhase::queued, wait, 0, wait, flags}, false, false, false}};
}

FacilityUseTimingResult advance_facility_use(const FacilityUseState &state) {
    const auto error = validate(state.input);
    if (error != FacilityUseError::none)
        return {error, std::nullopt};
    if (state.duration != duration(state.input) || state.elapsed < 0 ||
        state.elapsed > state.duration || state.remaining != state.duration - state.elapsed ||
        (state.phase == FacilityUsePhase::queued && state.elapsed != 0) ||
        (state.phase == FacilityUsePhase::in_use && (state.elapsed == 0 || state.remaining == 0)) ||
        (state.phase == FacilityUsePhase::exit_ready && state.remaining != 0) ||
        (state.phase != FacilityUsePhase::queued && state.phase != FacilityUsePhase::in_use &&
         state.phase != FacilityUsePhase::exit_ready))
        return {FacilityUseError::invalid_state, std::nullopt};
    if (state.phase == FacilityUsePhase::exit_ready)
        return {FacilityUseError::exit_pending, std::nullopt};

    FacilityUseStep next{state, state.phase == FacilityUsePhase::queued,
                         state.input.legacy_category == 2 && state.elapsed == 170, false};
    // Recovery is requested above using c()'s old B. d() then increments B, runs HP animation,
    // registers occupation on its first visit and decrements the wait in that same visit.
    ++next.state.elapsed;
    --next.state.remaining;
    next.state.legacy_flags |= 1U;
    if (state.input.legacy_category == 2)
        next.state.legacy_flags |= 32U;
    next.request_exit = next.state.remaining == 0;
    next.state.phase = next.request_exit ? FacilityUsePhase::exit_ready : FacilityUsePhase::in_use;
    return {FacilityUseError::none, next};
}

} // namespace dungeon_village_reference
