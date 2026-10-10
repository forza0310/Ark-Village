// Bounded source inspections and capture checks, separated from ordinary player input.
#include "world_inspection.hpp"
#include "ark/app/session/world_report.hpp"
#include "../ui/village/world_award.hpp"
#include "../ui/common/world_panels.hpp"
#include "../ui/tasks/world_tasks.hpp"
#include "../scene/world_build_placement.hpp"
#include "../scene/world_editing.hpp"
#include "world_human_inspection.hpp"
#include "ark/presentation/world_rest_visuals.hpp"
#include "../scene/world_scene.hpp"
#include "world_task_inspection.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace ark::desktop {
namespace {
namespace rules = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
const rules::WorldScriptPage *active_page(const State &s) {
    const auto found = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                    [](const auto &page) { return page.lifecycle != 4; });
    if (found == s.scripts.pages.rend() || found->kind == rules::WorldScriptPageKind::scene)
        return nullptr;
    return &*found;
}
bool advance(State &s) {
    auto candidate = simulation::prepare_startup_world_runtime(s);
    if (!candidate.candidate ||
        !simulation::update_startup_world_render_cache(*candidate.candidate)) {
        std::cerr << "World update rejected: runtime=" << static_cast<int>(candidate.error)
                  << " scene=" << static_cast<int>(candidate.scene_error)
                  << " world=" << static_cast<int>(candidate.world_error)
                  << " rounds=" << s.simulation_steps << '\n';
        return false;
    }
    s = std::move(*candidate.candidate);
    return true;
}
// Inspections supply explicit user confirmations to a real new game, never a fabricated battle
// or payment. Transient inspections stop at a naturally produced source display record.
bool inspection_ready(const State &s, const std::string &mode) {
    const auto &ai = s.scene.world.world.ai;
    if (mode == "world-save" || mode == "world-load" || mode == "world-load-error")
        return app::world_save_eligible(s);
    if (mode == "world-active" || mode == "world-menu" || mode == "world-village-menu")
        return ai.human_order.size() >= 3 && !active_page(s) && s.scene.scene_state == 0;
    if (mode == "world-month-defeats")
        return app::world_report_visible(s) && s.report_state == 1 && s.report_counter >= 6 &&
               s.report_snapshot[0] > 0;
    if (mode == "world-month" || mode == "world-month-income")
        return app::world_report_visible(s) && s.report_state == (mode == "world-month" ? 1 : 2) &&
               (s.report_state != 1 || s.report_counter >= 6);
    if (const auto *page = active_page(s)) {
        if (mode == "world-award")
            return page->legacy_page == 87 && s.award_rankings.count(page->id);
        if (mode == "world-rank")
            return page->legacy_page == 49 && s.page_counters.count(page->id);
        if (mode == "world-news")
            return page->kind == rules::WorldScriptPageKind::newspaper;
        if (mode == "world-break")
            return std::any_of(
                page->paragraphs.begin(), page->paragraphs.end(),
                [](const auto &body) { return body.find("<br>") != std::string::npos; });
        return false; // Scene overlays must not be captured behind an unrelated modal.
    }
    if (mode == "world-task-added" || mode == "world-level-up") {
        const auto &notices = s.scripts.notices;
        for (std::size_t i = 0; i < std::min<std::size_t>(2, notices.size()); ++i)
            if (notices[i].counter >= 8 && notices[i].counter <= notices[i].duration &&
                (mode == "world-level-up" ? notices[i].message == 0
                                          : notices[i].message == 13 || notices[i].message == 14))
                return true;
        return false;
    }
    if (mode == "world-combat")
        for (const auto &entry : ai.battle.actors)
            if (entry.second.label_timer > 0 && !entry.second.miss_label &&
                !(entry.second.control.flags & 1U))
                return true;
    if (mode == "world-reward")
        for (const auto &effect : s.visual_effects)
            if (effect.size() >= 2 && effect[0] == 3 && effect[1] >= 0)
                return true;
    if (mode == "world-exp")
        for (const auto id : ai.human_order)
            for (const auto &effect : ai.contexts.at(id).effects.display)
                if (effect.size() >= 2 && effect[0] == 24 && effect[1] >= 0 && effect[1] < 55)
                    return true;
    if (mode == "world-rest" || mode == "world-rest-hp")
        for (const auto id : s.scene.world.facility_order)
            for (const auto &row : world_rest_rows(s, id))
                if ((mode == "world-rest" && row.counter > 50 && row.counter < 170) ||
                    (mode == "world-rest-hp" && row.counter >= 180))
                    return true;
    return false;
}
void focus_inspection(State &s, const std::string &mode) {
    // A camera change after the inspection target is reached is a screenshot policy, not AI.
    const auto &ai = s.scene.world.world.ai;
    for (const auto &entry : ai.battle.actors) {
        const auto &a = entry.second;
        bool focus = mode == "world-combat" && a.label_timer > 0 && !a.miss_label;
        focus =
            focus || ((mode == "world-rest" || mode == "world-rest-hp") && (a.control.flags & 32U));
        if (mode == "world-exp" && a.kind == rules::ActorKind::human)
            for (const auto &effect : ai.contexts.at(entry.first).effects.display)
                focus = focus || (effect.size() >= 2 && effect[0] == 24 && effect[1] >= 0);
        if (focus) {
            const auto p = s.actor_metadata.at(entry.first).render_position;
            s.camera = {(p.x + p.z) * .3F, (p.z - p.x) * .15F + p.height};
            return;
        }
    }
    if (mode == "world-reward")
        for (const auto &effect : s.visual_effects)
            if (effect.size() >= 4 && effect[0] == 3 && effect[1] >= 0) {
                s.camera = {static_cast<float>(effect[2]), static_cast<float>(effect[3])};
                return;
            }
}
} // namespace

void prepare_world_inspection(State &state, const app::LaunchOptions &options, bool transient,
                              WorldManagementInspection &management_inspection,
                              std::optional<State> *checkpoint,
                              WorldManagementInspection *inspection_checkpoint) {
    if (options.inspect_page.rfind("world-", 0) == 0) {
        double next_inspection_draw{};
        WorldTaskInspection task_inspection;
        if (checkpoint && *checkpoint && inspection_checkpoint) {
            management_inspection = *inspection_checkpoint;
            // A credit catalogue may be opened with less than its 800G quote. A reused
            // rebuild branch leaves that modal through the real cancel before earning more.
            if (options.inspect_page == "world-home-rebuilt" &&
                state.scene.world.world.ai.accounting.funds() <
                    management_inspection.home_build_quote) {
                const auto *page = active_page(state);
                if (!page || simulation::cancel_startup_world_build_menu(state, page->id) !=
                                 simulation::StartupWorldRuntimeError::none)
                    throw std::runtime_error("Home suite cannot leave an unaffordable catalogue");
                management_inspection.home_rebuild_stage = 1;
            }
        } else {
            begin_management_inspection(state, options.inspect_page, management_inspection);
        }
        WorldHumanInspection human_inspection;
        begin_human_inspection(state, options.inspect_page, human_inspection);
        bool reached =
            (management_inspection_mode(options.inspect_page) &&
             management_inspection_ready(state, options.inspect_page, management_inspection)) ||
            (human_inspection_mode(options.inspect_page) &&
             human_inspection_ready(state, options.inspect_page, human_inspection));
        const int limit = human_inspection_mode(options.inspect_page) ? 180000
                          : options.inspect_page == "world-award" ||
                                  management_inspection_mode(options.inspect_page) ||
                                  world_task_inspection_mode(options.inspect_page)
                              ? 120000
                              : 20000;
        for (int step = 0; step < limit && !reached; ++step) {
            if (step % 5000 == 0) {
                const auto *page = active_page(state);
                std::cout << "World inspection progress: target=" << options.inspect_page
                          << " step=" << step << " rounds=" << state.simulation_steps
                          << " page=" << (page ? page->legacy_page : -1)
                          << " date=" << state.scene.calendar.year << '/'
                          << state.scene.calendar.month << std::endl;
            }
            // Long natural preparations must pump the OS window. This affects neither source
            // ticks nor player commands; default gameplay never enters this diagnostic loop.
            if (GetTime() >= next_inspection_draw) {
                BeginDrawing();
                ClearBackground({32, 38, 42, 255});
                DrawText("Preparing inspection from a real new world...", 24, 28, 20, RAYWHITE);
                DrawText(TextFormat("%s: %d / %d", options.inspect_page.c_str(), step, limit), 24,
                         62, 18, RAYWHITE);
                EndDrawing();
                next_inspection_draw = GetTime() + .1;
                if (WindowShouldClose())
                    throw std::runtime_error("World inspection closed during preparation");
            }
            before_human_inspection_update(state, options.inspect_page, human_inspection);
            if (!advance(state))
                throw std::runtime_error("World inspection failed before its real target state");
            after_human_inspection_update(state, human_inspection);
            // This synchronous diagnostic is its own Owner; normal windows consume outputs
            // in WorldSession. No sound playback is implemented in either adapter yet.
            state.sound_requests.clear();
            // Reports advance automatically alongside the world, including in diagnostics.
            // No synthetic confirmation or report-specific clock is introduced here.
            reached =
                human_inspection_mode(options.inspect_page)
                    ? human_inspection_ready(state, options.inspect_page, human_inspection)
                : management_inspection_mode(options.inspect_page)
                    ? management_inspection_ready(state, options.inspect_page,
                                                  management_inspection)
                : world_task_inspection_mode(options.inspect_page)
                    ? world_task_inspection_ready(state, options.inspect_page, task_inspection)
                    : inspection_ready(state, options.inspect_page);
            if (reached)
                break;
            if (human_inspection_mode(options.inspect_page) &&
                apply_human_inspection_input(state, options.inspect_page, human_inspection)) {
                // raw86 retires on the next framework update. Observe the committed transaction
                // page before another update; rendering never initializes or holds its consumer.
                reached = human_inspection_ready(state, options.inspect_page, human_inspection);
                if (reached)
                    break;
                continue;
            }
            if (management_inspection_mode(options.inspect_page) &&
                apply_management_inspection_input(state, options.inspect_page,
                                                  management_inspection))
                continue;
            if (world_task_inspection_mode(options.inspect_page) &&
                apply_world_task_inspection_input(state, task_inspection))
                continue;
            // Stop at each fully expanded victory phase using source counters. Only the
            // popularity inspection supplies the one phase0->phase1 confirmation.
            if (const auto *page = active_page(state);
                page && page->legacy_page == 30 &&
                (options.inspect_page == "world-task-victory" ||
                 options.inspect_page == "world-task-popularity")) {
                const auto counter = state.page_counters.find(page->id);
                if (counter == state.page_counters.end() || counter->second < 40)
                    continue;
            }
            if (const auto *page = active_page(state);
                page && ui::world_page_regular_confirmation(*page) &&
                ui::world_task_related_confirmation(state, *page) &&
                !(options.inspect_page == "world-rank" && page->legacy_page == 49)) {
                if (simulation::acknowledge_startup_world_runtime_page(state, page->id) !=
                    simulation::StartupWorldRuntimeError::none)
                    throw std::runtime_error("World inspection page consumer rejected input");
                if (human_inspection_mode(options.inspect_page) &&
                    human_inspection_ready(state, options.inspect_page, human_inspection)) {
                    reached = true;
                    break;
                }
            }
        }
        if (!reached)
            throw std::runtime_error("World inspection did not reach its bounded target");
        if (checkpoint && !*checkpoint) {
            if (options.inspect_page != "world-commerce" &&
                options.inspect_page != "world-home-credit")
                throw std::runtime_error("Inspection suite must first capture its natural entry");
            *checkpoint = state; // Capture before screenshot-only camera/pause changes.
            if (inspection_checkpoint)
                *inspection_checkpoint = management_inspection;
        }
        if (human_inspection_mode(options.inspect_page))
            std::cout << "World human inspection: target=" << options.inspect_page
                      << " human=" << human_inspection.human.value_or(-1)
                      << " equipment=" << human_inspection.equipment.value_or(-1)
                      << " gift=" << human_inspection.gift_confirmed
                      << " cancelled=" << human_inspection.cancelled
                      << " profession=" << human_inspection.target_profession.value_or(-1)
                      << " changed=" << human_inspection.profession_confirmed
                      << " completed=" << human_inspection.profession_completed
                      << " home=" << human_inspection.home.value_or(0)
                      << " tax=" << human_inspection.expected_tax
                      << " collected=" << human_inspection.tax_collected << '\n';
        if (world_task_inspection_mode(options.inspect_page))
            std::cout << "World task inspection: target=" << options.inspect_page
                      << " accepted=" << task_inspection.accepted_task.value_or(0)
                      << " departed=" << task_inspection.departed_task.value_or(0)
                      << " page=" << active_page(state)->legacy_page
                      << " participants=" << state.participants.size()
                      << " successes=" << state.task_progress.successes << '\n';
        if (management_inspection_mode(options.inspect_page)) {
            if (management_inspection.preview_anchor) {
                const auto p = *management_inspection.preview_anchor;
                state.camera = {30.F * (p.x + p.y) + 30, 15.F * (p.y - p.x)};
            }
            if (management_inspection.created)
                if (const auto camera = simulation::startup_world_runtime_facility_target(
                        state, *management_inspection.created))
                    state.camera = *camera;
            std::cout << "World management inspection: target=" << options.inspect_page
                      << " created=" << management_inspection.created.value_or(0)
                      << " definition=" << management_inspection.selection.value_or(-1)
                      << " awarded=" << management_inspection.awarded_human.value_or(-1)
                      << " applied=" << management_inspection.award_applied
                      << " activity=" << management_inspection.activity.value_or(-1)
                      << " activity_started=" << management_inspection.activity_started
                      << " activity_completed=" << management_inspection.activity_completed
                      << " scene_counter=" << state.scene.scene_counter;
            if (options.inspect_page.rfind("world-expansion-", 0) == 0) {
                const auto &town = state.scene.world.town;
                std::cout << " expansion_fixture=1 fence_level="
                          << management_inspection.expansion_level_before << '/'
                          << state.fence_level << " town=" << town.left << ',' << town.right << ','
                          << town.top << ',' << town.bottom
                          << " map_cells=" << state.scene.world.world.map.cells.size();
            }
            if (options.inspect_page.rfind("world-road-", 0) == 0 ||
                options.inspect_page == "world-demolished")
                std::cout << " edit_cells=" << management_inspection.edit_cells
                          << " edit_cash=" << management_inspection.edit_cash_before << '/'
                          << management_inspection.edit_cash_after
                          << " edit_random=" << management_inspection.edit_draws_before << '/'
                          << management_inspection.edit_draws_after
                          << " edited_old=" << management_inspection.edited_old.value_or(0)
                          << " edit_completed=" << management_inspection.edit_completed;
            if (options.inspect_page == "world-home-credit" ||
                options.inspect_page == "world-home-rebuilt")
                std::cout << " home_stage=" << management_inspection.home_rebuild_stage
                          << " home_resident=" << management_inspection.home_resident.value_or(-1)
                          << " home_credit=" << management_inspection.home_credit_before << '/'
                          << management_inspection.home_credit_after << '/'
                          << management_inspection.home_credit_remaining
                          << " home_quote=" << management_inspection.home_build_quote
                          << " home_cash=" << management_inspection.home_build_cash_before << '/'
                          << management_inspection.home_build_cash_after
                          << " home_random=" << management_inspection.home_build_draws_before << '/'
                          << management_inspection.home_build_draws_after;
            if (management_inspection.selection && management_inspection.preview_anchor)
                std::cout << " ghost_visible="
                          << world_build_preview(state, *management_inspection.selection,
                                                 *management_inspection.preview_anchor,
                                                 management_inspection.preview_orientation)
                                 .graphic_visible;
            std::cout << '\n';
        }
        if (transient)
            focus_inspection(state, options.inspect_page);
    }
}

WorldSaveInspection::WorldSaveInspection(const app::LaunchOptions &options)
    : mode_(options.inspect_page), directory_(options.save_directory),
      enabled_(mode_ == "world-save" || mode_ == "world-load" || mode_ == "world-load-error"),
      ready_(!enabled_) {
    if (mode_ == "world-load" && directory_.empty())
        throw std::runtime_error("Cold-load inspection requires an explicit isolated --save-dir");
    if (enabled_ && directory_.empty()) {
        if (options.screenshot.empty())
            throw std::runtime_error(
                "Save inspection requires a screenshot path for isolated files");
        const std::filesystem::path capture(options.screenshot);
        directory_ = capture.parent_path() /
                     (capture.stem().string() + "-files-" +
                      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    }
}

void WorldSaveInspection::observe(const app::WorldFrame &publication, app::WorldSession &session,
                                  std::uint64_t &pending_menu, double elapsed) {
    const auto &current = *publication.state;
    const bool failed = publication.failed;
    if (enabled_ && !ready_ && !pending_menu && !publication.save_busy) {
        if (stage_ == 0 && publication.main_menu_open) {
            frozen_ = publication.state;
            pending_menu = session.open_save_menu();
            stage_ = 1;
        } else if (stage_ == 1 && publication.save_menu_open) {
            // Explicit error probe uses a nonexistent isolated slot through the worker.
            // Normal UI keeps empty-slot Load disabled; no player file is touched.
            if (mode_ == "world-load") {
                load_metadata_ = publication.save_slots[0].metadata;
                if (!load_metadata_)
                    throw std::runtime_error("Cold-load inspection has no valid source slot");
                pending_menu = session.load_slot(0);
            } else {
                if (mode_ == "world-load-error" && publication.save_slots[1].exists)
                    throw std::runtime_error(
                        "Load-error inspection requires an empty isolated second slot");
                pending_menu = mode_ == "world-save" ? session.save_slot(0) : session.load_slot(1);
            }
            stage_ = 2;
        } else if (stage_ == 2 && !publication.save_message.empty()) {
            const bool saved =
                publication.save_slots[0].exists && publication.save_slots[0].metadata.has_value();
            const bool loading = mode_ == "world-load";
            const bool expected_message =
                mode_ == "world-save" ? publication.save_message == "保存完毕"
                : loading             ? publication.save_message == "读取完毕"
                                      : publication.save_message.rfind("读取失败", 0) == 0;
            if (!expected_message ||
                (loading ? publication.save_menu_open || publication.generation != 2 ||
                               publication.state == frozen_
                         : !publication.save_menu_open || (mode_ == "world-save" && !saved) ||
                               publication.state != frozen_))
                throw std::runtime_error(
                    "Save inspection did not preserve the expected file/overlay result");
            if (loading) {
                const auto &metadata = *load_metadata_;
                const auto &date = current.scene.calendar;
                if (date.year != metadata.year || date.month != metadata.month ||
                    date.subperiod != metadata.week || date.units != metadata.units ||
                    current.scene.world.world.ai.accounting.funds() != metadata.funds ||
                    !current.scene.framework_paused)
                    throw std::runtime_error("Cold-load inspection disagrees with saved "
                                             "date/funds or explicit pause");
                auto before = frozen_->scene.random, after = current.scene.random;
                if (before.draws() != after.draws())
                    throw std::runtime_error(
                        "Cold-load inspection reset the current random cursor");
                for (int sample = 0; sample < 8; ++sample)
                    if (before.draw(197).ticket != after.draw(197).ticket)
                        throw std::runtime_error(
                            "Cold-load inspection replaced the current random stream");
            }
            ready_ = true;
            std::cout << "World save inspection: target=" << mode_
                      << " directory=" << directory_.string() << " saved=" << saved
                      << " generation=" << publication.generation
                      << " message=" << publication.save_message << '\n';
        }
    }
    if (enabled_ && !ready_ && (failed || elapsed > 30))
        throw std::runtime_error("Save inspection FIFO did not finish within its bound");
}

void capture_world_screenshot(const app::LaunchOptions &options, int frames) {
    if (!options.screenshot.empty()) {
        if (frames != options.frames)
            throw std::runtime_error("World window closed before bounded capture");
        auto screenshot = LoadImageFromScreen();
        auto pixels = screenshot.data ? LoadImageColors(screenshot) : nullptr;
        bool nonblank{};
        if (pixels)
            for (std::int64_t i = 0;
                 i < static_cast<std::int64_t>(screenshot.width) * screenshot.height; ++i)
                if (pixels[i].r || pixels[i].g || pixels[i].b) {
                    nonblank = true;
                    break;
                }
        if (pixels)
            UnloadImageColors(pixels);
        const bool saved = nonblank && ExportImage(screenshot, options.screenshot.c_str());
        if (screenshot.data)
            UnloadImage(screenshot);
        if (!saved)
            throw std::runtime_error("World screenshot is blank or cannot be exported");
    }
}
} // namespace ark::desktop
