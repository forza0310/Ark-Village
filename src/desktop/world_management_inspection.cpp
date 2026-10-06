#include "world_management_inspection.hpp"
#include "ark/simulation/rules/world_village_activity.hpp"
#include "ark/simulation/startup_world_building.hpp"
#include "ark/simulation/startup_world_human.hpp"
#include "ark/simulation/startup_world_village_activity.hpp"
#include "world_build_placement.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Error = simulation::StartupWorldRuntimeError;
using Kind = simulation::rules::WorldScriptPageKind;
namespace rules = simulation::rules;
bool build_preview_mode(const std::string &mode) {
    return mode == "world-build-preview" || mode == "world-build-preview-hidden" ||
           mode == "world-build-rotated";
}
bool village_mode(const std::string &mode) {
    return mode == "world-village" || mode == "world-village-start" ||
           mode == "world-village-results";
}
std::optional<int> affordable_activity(const State &state, bool result) {
    std::vector<rules::WorldVillageActivityDefinition> definitions;
    for (const auto &a : state.rules->activities)
        definitions.push_back({a.identity, state.scripts.activities.at(a.identity).status,
                               state.activity_flags.at(a.identity),
                               state.activity_counts.at(a.identity), a.parameters[2],
                               a.parameters[3], a.parameters[4], a.parameters[5]});
    const auto list = rules::catalogue_world_village_activities(definitions);
    if (!list)
        throw std::runtime_error("Village inspection cannot inspect actual catalogue");
    std::optional<int> fallback;
    for (const int id : *list) {
        const auto &a = definitions.at(id);
        const auto gate =
            rules::check_world_village_activity(a, state.quarter_counter, state.village_points);
        if (a.kind > 2 || gate.error != rules::WorldVillageActivityError::none ||
            gate.denial != rules::WorldVillageActivityDenial::none)
            continue;
        if (!result || a.kind < 2)
            return id;
        // The initial clean-up activity and real quests can earn the popularity that unlocks
        // the first attribute activity. These are player choices, not injected unlocks/points.
        if (state.popularity < 200)
            fallback = id;
    }
    return fallback;
}
const rules::WorldScriptPage *top(const State &state) {
    const auto found = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                    [](const auto &page) { return page.lifecycle != 4; });
    return found == state.scripts.pages.rend() ? nullptr : &*found;
}
void require(Error error, const char *action) {
    if (error != Error::none)
        throw std::runtime_error(std::string("Management inspection ") + action +
                                 " failed: source=" + std::to_string(static_cast<int>(error)));
}
void require_build(const simulation::StartupBuildResult &result, const char *action) {
    require(result.error, action);
    if (result.denial != simulation::StartupBuildDenial::none)
        throw std::runtime_error(
            std::string("Management inspection ") + action +
            " denied by current world: " + std::to_string(static_cast<int>(result.denial)));
}
rules::Position
empty_site(const State &state, int definition,
           rules::FacilityOrientation orientation = rules::FacilityOrientation::first) {
    const auto bounds = state.rules->fences.at(state.fence_level);
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
            if (world_build_preview(state, definition, {x, y}, orientation).valid())
                return {x, y};
        }
    throw std::runtime_error("Management inspection found no vacant source building footprint");
}
} // namespace
bool management_inspection_mode(const std::string &mode) {
    return mode == "world-building" || mode == "world-details" || mode == "world-built" ||
           build_preview_mode(mode) || mode == "world-award-granted" || village_mode(mode) ||
           mode == "world-reward95";
}
void begin_management_inspection(State &state, const std::string &mode,
                                 WorldManagementInspection &) {
    if (mode == "world-building" || mode == "world-built" || build_preview_mode(mode))
        require(simulation::open_startup_world_build_menu(state), "open building catalogue");
    if (mode == "world-details") {
        // Definition28 is the maintained startup inn, selected through its actual stable instance.
        const auto found = std::find_if(
            state.scene.world.facility_order.begin(), state.scene.world.facility_order.end(),
            [&](auto id) {
                return state.scene.world.world.facilities.at(id).placement.definition_id == 28;
            });
        if (found == state.scene.world.facility_order.end())
            throw std::runtime_error("Management inspection cannot find the actual startup inn");
        require(simulation::open_startup_world_facility_page(state, *found), "open inn details");
    }
}
bool management_inspection_ready(const State &state, const std::string &mode,
                                 const WorldManagementInspection &inspection) {
    const auto *page = top(state);
    if (!page)
        return false;
    if (mode == "world-reward95") {
        const auto counter = state.page_counters.find(page->id);
        return page->legacy_page == 95 && counter != state.page_counters.end() &&
               counter->second >= 40;
    }
    if (village_mode(mode)) {
        const auto view = simulation::inspect_startup_world_village_activity_page(state, page->id);
        if (!view)
            return false;
        if (mode == "world-village")
            return view->raw == 51;
        return view->activity == inspection.activity &&
               (mode == "world-village-start"
                    ? view->raw == 53 && view->counter >= 70 && inspection.activity_started
                    : view->raw == 54 && inspection.activity_completed);
    }
    if (mode == "world-building")
        return page->legacy_page == 21 && state.build_page_catalogs.count(page->id);
    if (mode == "world-details")
        return page->legacy_page == 74 && state.facility_page_bindings.count(page->id);
    if (build_preview_mode(mode)) {
        if (!inspection.selection || !inspection.preview_anchor ||
            state.build_definition != inspection.selection || page->kind != Kind::scene ||
            state.scene.scene_state != 1)
            return false;
        const auto preview =
            world_build_preview(state, *inspection.selection, *inspection.preview_anchor,
                                inspection.preview_orientation);
        return mode == "world-build-preview-hidden"
                   ? preview.cursor_in_map && state.scene.scene_counter % 20 >= 10 &&
                         !preview.graphic_visible
                   : preview.graphic_visible;
    }
    if (mode == "world-built")
        return inspection.created && page->kind == Kind::scene && state.scene.scene_state == 0 &&
               state.scene.world.world.facilities.at(*inspection.created).status == 1;
    const auto human = state.page_human_bindings.find(page->id);
    return mode == "world-award-granted" && inspection.award_applied &&
           (page->legacy_page == 88 || page->legacy_page == 67) &&
           human != state.page_human_bindings.end() && human->second == inspection.awarded_human &&
           state.page_counters.count(page->id);
}
bool apply_management_inspection_input(State &state, const std::string &mode,
                                       WorldManagementInspection &inspection) {
    const auto *current = top(state);
    if (!current)
        throw std::runtime_error("Management inspection lost the source page stack");
    const auto page = *current; // Source commands may replace/reallocate the stack.
    if (village_mode(mode) || mode == "world-reward95") {
        using A = simulation::StartupVillageActivityAction;
        if (mode == "world-reward95" && page.legacy_page == 95)
            return true; // Natural counter only; capture before the actual reward claim.
        if (page.kind == Kind::scene && state.scene.scene_state == 0 &&
            !state.scene.framework_paused && !state.scene.world.world.ai.human_order.empty()) {
            const auto selected = affordable_activity(state, mode == "world-village-results" ||
                                                                 mode == "world-reward95");
            if (mode == "world-village" || selected) {
                inspection.activity = selected;
                require(simulation::open_startup_world_village_activities(state),
                        "open actual village activities");
                return true;
            }
        }
        if (page.legacy_page >= 51 && page.legacy_page <= 54) {
            const auto view =
                simulation::inspect_startup_world_village_activity_page(state, page.id);
            if (!view || state.activity_page_answers.count(page.id))
                return true;
            if (page.legacy_page == 51) {
                const auto selected = inspection.activity
                                          ? std::find(view->entries.begin(), view->entries.end(),
                                                      *inspection.activity)
                                          : view->entries.end();
                if (selected == view->entries.end())
                    require(simulation::act_startup_world_village_activity_page(state, page.id,
                                                                                A::cancel),
                            "leave unavailable village catalogue");
                else {
                    const int index = static_cast<int>(selected - view->entries.begin());
                    require(simulation::act_startup_world_village_activity_page(
                                state, page.id, view->selection == index ? A::confirm : A::select,
                                index),
                            "select actual activity");
                }
            } else if (page.legacy_page == 52) {
                require(
                    simulation::act_startup_world_village_activity_page(state, page.id, A::confirm),
                    "start real activity");
                inspection.activity_started = true;
                inspection.activity_completed = false;
            } else if (page.legacy_page == 53 && view->counter >= 120) {
                require(
                    simulation::act_startup_world_village_activity_page(state, page.id, A::confirm),
                    "complete real activity");
                inspection.activity_completed = true;
            } else if (page.legacy_page == 54)
                require(
                    simulation::act_startup_world_village_activity_page(state, page.id, A::confirm),
                    "close actual activity results");
            return true;
        }
        if (page.legacy_page == 70) {
            // A naturally earned mastery is not an implicit profession change: this explicit
            // diagnostic player keeps the profession at the final source choice.
            if (!simulation::startup_world_human_page_ready(state, page.id))
                return true;
            const bool choose_stay =
                state.page_phases.at(page.id) == 2 && state.human_page_selections.at(page.id) != 1;
            require(simulation::act_startup_world_human_page(
                        state, page.id,
                        choose_stay ? simulation::StartupHumanPageAction::select
                                    : simulation::StartupHumanPageAction::confirm,
                        1),
                    "keep mastered profession");
            return true;
        }
        if (page.legacy_page == 31) {
            if (state.crew_summaries.count(page.id))
                require(simulation::acknowledge_startup_world_runtime_page(state, page.id),
                        "close real crew result");
            return true;
        }
        if (page.kind == Kind::scene &&
            (state.scene.scene_state != 0 || state.scene.framework_paused))
            return true;
        if (mode == "world-reward95" || mode == "world-village-results")
            return apply_world_task_inspection_input(state, inspection.task_policy);
        if (page.kind == Kind::scene)
            return true; // Wait for natural points/quarter slots; never freeze on an unaffordable
                         // offer.
    }
    const bool preview = build_preview_mode(mode);
    if ((mode == "world-built" || preview) && !inspection.created) {
        if (page.legacy_page == 21) {
            const auto &groups = state.build_page_catalogs.at(page.id);
            // Start with the actual resident plot: inn-only diagnostics miss single-frame
            // artwork. Both preview orientations and the installed rotated surface are checked.
            for (int group : {0, 1, 2})
                for (int definition : groups.at(group)) {
                    const auto quote = simulation::startup_world_build_quote(state, definition);
                    if (!quote ||
                        quote->construction_cost > state.scene.world.world.ai.accounting.funds())
                        continue;
                    require_build(
                        simulation::select_startup_world_build_menu(state, page.id, definition),
                        "select actual affordable building");
                    inspection.selection = definition;
                    if (preview) {
                        inspection.preview_orientation = mode == "world-build-rotated"
                                                             ? rules::FacilityOrientation::second
                                                             : rules::FacilityOrientation::first;
                        inspection.preview_anchor =
                            empty_site(state, definition, inspection.preview_orientation);
                    }
                    return true;
                }
            throw std::runtime_error(
                "Management inspection has no affordable building in actual catalogue");
        }
        if (!preview && page.kind == Kind::scene && state.scene.scene_state == 1 &&
            inspection.selection) {
            const auto orientation = rules::FacilityOrientation::second;
            const auto result = simulation::confirm_startup_world_build(
                state, empty_site(state, *inspection.selection, orientation), orientation);
            require_build(result, "place selected building");
            if (!result.created)
                throw std::runtime_error(
                    "Management inspection placement returned no stable identity");
            inspection.created = result.created;
            require(simulation::cancel_startup_world_build(state), "leave continuous placement");
            return true; // Subsequent ordinary source world rounds perform construction.
        }
    }
    if (mode == "world-award-granted" && page.legacy_page == 87) {
        const auto ranking = state.award_rankings.find(page.id);
        if (ranking == state.award_rankings.end())
            return true; // Real annual source update must initialize the actual rankings.
        if (ranking->second.empty() || state.medal_count <= 0)
            throw std::runtime_error(
                "Management inspection annual page has no awardable candidate");
        using Action = rules::WorldAwardAction;
        const auto pending = state.award_pending_humans.find(page.id);
        if (pending == state.award_pending_humans.end()) {
            inspection.selection = 0;
            inspection.awarded_human = ranking->second.front();
            require(simulation::act_startup_world_runtime_award_page(state, page.id,
                                                                     Action::request_award, 0),
                    "request annual award");
        } else {
            if (pending->second != inspection.awarded_human)
                throw std::runtime_error(
                    "Management inspection award prompt changed its bound human");
            require(simulation::act_startup_world_runtime_award_page(state, page.id,
                                                                     Action::confirm_award),
                    "confirm actual annual award");
            inspection.award_applied = true;
        }
        return true;
    }
    return false;
}
} // namespace ark::desktop
