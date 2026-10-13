// Count, cumulative score and captured record remain separate; painting cannot commit any of them.
#include "world_clear_score.hpp"
#include "../common/steam_plan_draw.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
std::optional<simulation::SteamClearScoreInput>
world_clear_score_input(const simulation::StartupWorldRuntimeState &state,
                        const app::WorldClearPage &clear) {
    namespace sim = simulation;
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &page) { return page.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->id != clear.page ||
        top->kind != sim::rules::WorldScriptPageKind::raw_page || top->legacy_page != 17 ||
        top->lifecycle != 2 || !sim::valid_startup_world_clear_page(state, *top) ||
        sim::validate_startup_clear_score_page(clear.rows, clear.score) !=
            sim::StartupClearScoreError::none)
        return {};
    // The shared validator requires all three fields; never substitute a stage clock for frame_.
    const auto stage = state.page_phases.at(clear.page);
    const auto counter = state.page_counters.at(clear.page);
    if (stage != clear.score.stage || counter != clear.score.counter)
        return {};
    return sim::SteamClearScoreInput{
        stage,           counter,         state.page_secondary_counters.at(clear.page),
        clear.score.row, clear.score.sum, clear.score.captured_high_score,
        clear.rows};
}
std::optional<simulation::SteamClearScorePlan>
prepare_world_clear_score(const simulation::SteamClearScoreInput &input, const Skin &skin) {
    namespace sim = simulation;
    sim::SteamClearScoreOptions options;
    options.view_y = 23;
    options.title_widths = std::array<int, 2>{int(skin.text.width("通关点数发表")),
                                              int(skin.text.width("通关点数发表"))};
    if (input.stage == 3 && input.stage_counter >= 15) {
        if (!input.rows || input.row < 0 || input.row >= int(input.rows->size()))
            return {};
        const auto number = sim::steam_clear_score_number((*input.rows)[input.row].score);
        if (!number)
            return {};
        // Source has two Font calls; the fixed suffix allowance remains 48 in its plan.
        options.score_widths = {int(skin.text.width(*number)), int(skin.text.width(*number))};
    }
    return sim::steam_clear_score_skin(input, options);
}
void draw_world_clear_score(const simulation::SteamClearScorePlan &plan, Extent extent,
                            const Skin &skin) {
    namespace sim = simulation;
    SteamPlanDraw draw({(extent.width - 240) / 2.F, (extent.height - 256) / 2.F}, skin, plan.title);
    for (const auto &operation : plan.operations) {
        if (const auto *frame = std::get_if<sim::SteamStartupFrameDraw>(&operation))
            draw.frame(*frame);
        else if (const auto *image = std::get_if<sim::SteamHumanImage>(&operation))
            draw.human_image(*image);
        else if (const auto *text = std::get_if<sim::SteamHumanOverviewText>(&operation))
            draw.text(*text, text->value);
        else if (!std::holds_alternative<sim::SteamStartupTouch>(operation))
            throw std::invalid_argument("Unsupported clear-score paint operation");
        // Global Touch2 has no local rectangle. Input still uses the application source consumer.
    }
    draw.finish();
}
} // namespace ark::desktop::ui
