#include "world_management_inspection.hpp"
#include "ark/simulation/startup_world_building.hpp"
#include "world_build_placement.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Error = simulation::StartupWorldRuntimeError;
using Kind = simulation::rules::WorldScriptPageKind;
namespace rules = simulation::rules;
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
           mode == "world-build-preview" || mode == "world-build-rotated" ||
           mode == "world-award-granted";
}
void begin_management_inspection(State &state, const std::string &mode,
                                 WorldManagementInspection &) {
    if (mode == "world-building" || mode == "world-built" || mode == "world-build-preview" ||
        mode == "world-build-rotated")
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
    if (mode == "world-building")
        return page->legacy_page == 21 && state.build_page_catalogs.count(page->id);
    if (mode == "world-details")
        return page->legacy_page == 74 && state.facility_page_bindings.count(page->id);
    if (mode == "world-build-preview" || mode == "world-build-rotated")
        return inspection.selection && inspection.preview_anchor &&
               state.build_definition == inspection.selection && page->kind == Kind::scene &&
               state.scene.scene_state == 1;
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
    const bool preview = mode == "world-build-preview" || mode == "world-build-rotated";
    if ((mode == "world-built" || preview) && !inspection.created) {
        if (page.legacy_page == 21) {
            const auto &groups = state.build_page_catalogs.at(page.id);
            for (int group : {1, 2, 0})
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
            const auto result = simulation::confirm_startup_world_build(
                state, empty_site(state, *inspection.selection), rules::FacilityOrientation::first);
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
