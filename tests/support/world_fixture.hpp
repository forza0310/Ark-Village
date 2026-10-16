#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"

#include <cstdint>
#include <stdexcept>

namespace ark::test {
// Each caller owns a copy of the real new-world state. The common seed shares only an
// immutable initial snapshot per process; no fixture advances time or confirms a page.
inline simulation::StartupWorldRuntimeState initial_world(std::uint64_t seed = 1) {
    const auto create = [](std::uint64_t value) {
        simulation::StartupSession startup;
        simulation::StartupWorldRuntimeSession session(
            startup.state(), simulation::rules::WorldRandomStream::from_java_seed(value));
        return session.state();
    };
    if (seed == 1) {
        static const auto initial = create(1);
        return initial;
    }
    return create(seed);
}

// A notice callsite fixture, not a naturally unlocked encounter. The real source consumer
// initializes its clock and outbreak rows; UI tests must never synthesize those payloads.
inline simulation::StartupWorldRuntimeState monster_notice_fixture(int raw, int definition = 3) {
    if (raw != 99 && raw != 100)
        throw std::invalid_argument("Notice fixture requires99/100");
    auto state = initial_world();
    state.scripts.pages.front().lifecycle = 3;
    simulation::rules::WorldScriptPage page;
    page.id = state.scripts.next_page_id++;
    page.kind = simulation::rules::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    page.monster_definition = definition;
    state.scripts.pages.push_back(page);
    state.scripts.executing_page = page.id;
    if (simulation::acknowledge_startup_world_runtime_page(state, page.id) !=
        simulation::StartupWorldRuntimeError::none)
        throw std::runtime_error("Notice fixture source initialization failed");
    auto ready = simulation::prepare_startup_world_runtime(state);
    if (!ready.candidate)
        throw std::runtime_error("Notice fixture framework Update failed");
    return std::move(*ready.candidate);
}

// A partial read-only invariant, not whole-owner equality. Callers additionally compare
// the cash, random stream, actor, map, or page fields relevant to their own contract.
inline bool same_world_clock(const simulation::StartupWorldRuntimeState &state,
                             const simulation::StartupWorldRuntimeState &before) {
    const auto &now = state.scene.calendar;
    const auto &old = before.scene.calendar;
    return now.year == old.year && now.month == old.month && now.subperiod == old.subperiod &&
           now.units == old.units && now.previous_units == old.previous_units &&
           now.month_ticks == old.month_ticks &&
           state.scene.world.updates == before.scene.world.updates &&
           state.simulation_steps == before.simulation_steps &&
           state.arrival_counter == before.arrival_counter;
}
} // namespace ark::test
