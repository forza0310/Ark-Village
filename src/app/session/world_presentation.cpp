#include "world_presentation.hpp"

namespace ark::app::detail {
simulation::StartupPresentationResult
prepare_world_presentation(const simulation::StartupWorldRuntimeState &state,
                           const WorldSystemState &system, std::uint64_t ordinal) {
    namespace sim = simulation;
    using Error = sim::StartupWorldRuntimeError;
    const sim::rules::WorldScriptPage *top{};
    for (auto p = state.scripts.pages.rbegin(); p != state.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4) {
            top = &*p;
            break;
        }
    const auto is_clear = [](const auto &p) {
        return p.kind == sim::rules::WorldScriptPageKind::raw_page && p.legacy_page == 17;
    };
    if (system.clear) {
        const auto &clear = *system.clear;
        const auto counter = state.page_counters.find(clear.page);
        const auto phase = state.page_phases.find(clear.page);
        if (!top || !is_clear(*top) || top->id != clear.page || top->lifecycle != 2 ||
            !sim::valid_startup_world_clear_page(state, *top) ||
            counter == state.page_counters.end() || phase == state.page_phases.end() ||
            counter->second != clear.score.counter || phase->second != clear.score.stage ||
            clear.score.captured_high_score != system.records.high_score ||
            sim::validate_startup_clear_score_page(clear.rows, clear.score) !=
                sim::StartupClearScoreError::none)
            return {Error::invalid_page, {}, {}};
    } else if (top && is_clear(*top) &&
               (top->lifecycle != 0 || !sim::valid_startup_world_clear_page(state, *top)))
        return {Error::invalid_page, {}, {}};
    for (const auto &page : state.scripts.pages)
        if (is_clear(page) && page.lifecycle != 0 && page.lifecycle != 4 &&
            (!system.clear || !top || top->id != page.id))
            return {Error::invalid_page, {}, {}};
    sim::StartupPresentationRequest request;
    request.ordinal = ordinal;
    request.mode = sim::StartupPresentationMode::full_redraw;
    const auto pages = sim::startup_world_presentation_pages(state, request.mode);
    if (!pages)
        return {Error::invalid_page, {}, {}};
    request.expected_pages = *pages;
    // PC pause does not pause audio. The unproved source66 wrapper remains disabled.
    request.gift_wrapper_ready = false;
    return sim::prepare_startup_world_presentation(state, request);
}
} // namespace ark::app::detail
