#include "world_management_inspection.hpp"
#include "ark/simulation/rules/world_village_activity.hpp"
#include "ark/simulation/startup_world_building.hpp"
#include "ark/simulation/startup_world_editing.hpp"
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
bool expansion_mode(const std::string &mode) {
    return mode == "world-expansion-catalogue" || mode == "world-expansion-start" ||
           mode == "world-expansion-completed";
}
bool road_mode(const std::string &mode) {
    return mode == "world-road-start" || mode == "world-road-end" || mode == "world-road-built" ||
           mode == "world-road-remove";
}
bool home_mode(const std::string &mode) {
    return mode == "world-home-credit" || mode == "world-home-rebuilt";
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
void check_edit(bool condition, const char *contract) {
    if (!condition)
        throw std::runtime_error(std::string("Management inspection map edit: ") + contract);
}
bool in_town(const State &state, rules::Position p) {
    const auto &f = state.rules->fences.at(state.fence_level);
    return p.x > f[0].x && p.x < f[1].x && p.y > f[1].y && p.y < f[0].y;
}
std::size_t tile_index(const State &state, rules::Position p) {
    return static_cast<std::size_t>(p.y * state.scene.world.world.map.width + p.x);
}
void edit_observations(const State &state, WorldManagementInspection &i) {
    i.edit_cash_after = state.scene.world.world.ai.accounting.funds();
    i.edit_draws_after = state.scene.random.draws();
    check_edit(i.edit_draws_before == i.edit_draws_after, "player editing draws no random values");
}
void begin_road_inspection(State &state, const std::string &mode, WorldManagementInspection &i) {
    const auto d = std::find_if(
        state.rules->facilities.begin(), state.rules->facilities.end(), [&](const auto &v) {
            return v.kind == 6 && (v.flags & 4) && state.facility_presence.at(v.id) != 0;
        });
    check_edit(d != state.rules->facilities.end(), "actual new world has no available road");
    i.selection = d->id;
    const auto quote = simulation::startup_world_build_quote(state, *i.selection);
    check_edit(quote.has_value(), "actual road has no construction quote");
    // Use three actual empty cells, independent of the segment consumer's own calculation.
    // The fixture never replaces terrain or inserts a fake facility to obtain a suitable site.
    const auto &map = state.scene.world.world.map;
    for (int y = 0; y < map.height && !i.preview_anchor; ++y)
        for (int x = 0; x + 2 < map.width && !i.preview_anchor; ++x) {
            bool vacant = true;
            for (int dx = 0; dx < 3; ++dx) {
                const rules::Position p{x + dx, y};
                const auto &cell = map.cells.at(tile_index(state, p));
                vacant = vacant && in_town(state, p) && cell.legacy_state == 4 && !cell.facility;
            }
            if (vacant) {
                i.preview_anchor = rules::Position{x, y};
                i.edit_endpoint = rules::Position{x + 2, y};
                i.edit_cells = 3;
            }
        }
    check_edit(i.preview_anchor.has_value(), "no three real vacant town road cells");
    i.edit_cash_before = state.scene.world.world.ai.accounting.funds();
    i.edit_draws_before = state.scene.random.draws();
    require_build(simulation::begin_startup_world_road(state, *i.selection), "enter actual road");
    if (mode != "world-road-start") {
        require_build(
            simulation::confirm_startup_world_edit(state, *i.preview_anchor, i.preview_orientation),
            "select actual road start");
        check_edit(state.build_mode == 2 && state.build_anchor == i.preview_anchor,
                   "road first confirmation only selects start");
    }
    if (mode == "world-road-built" || mode == "world-road-remove") {
        require_build(
            simulation::confirm_startup_world_edit(state, *i.edit_endpoint, i.preview_orientation),
            "commit actual three-cell road");
        check_edit(state.build_mode == 1 && !state.build_anchor,
                   "road commit returns to continuous start mode");
        for (int dx = 0; dx < i.edit_cells; ++dx) {
            const auto n = tile_index(state, {i.preview_anchor->x + dx, i.preview_anchor->y});
            check_edit(state.scene.world.world.map.cells.at(n).legacy_state == 3 &&
                           state.surface.at(n).definition == *i.selection,
                       "road commit binds all three actual input cells");
        }
        check_edit(state.scene.world.world.ai.accounting.funds() ==
                       i.edit_cash_before - quote->construction_cost * i.edit_cells,
                   "road charges the quote once per changed input cell");
        i.edit_completed = true;
        if (mode == "world-road-remove") {
            const auto paid = state.scene.world.world.ai.accounting.funds();
            require(simulation::cancel_startup_world_edit(state), "leave actual road");
            require_build(simulation::begin_startup_world_edit(state, false), "enter road removal");
            require_build(simulation::confirm_startup_world_edit(state, *i.preview_anchor,
                                                                 i.preview_orientation),
                          "select actual road removal start");
            check_edit(state.build_mode == 5, "road removal selects the segment first");
            require_build(simulation::confirm_startup_world_edit(state, *i.edit_endpoint,
                                                                 i.preview_orientation),
                          "remove actual three-cell road");
            for (int dx = 0; dx < i.edit_cells; ++dx) {
                const auto n = tile_index(state, {i.preview_anchor->x + dx, i.preview_anchor->y});
                check_edit(state.scene.world.world.map.cells.at(n).legacy_state == 4 &&
                               state.surface.at(n).definition == state.ground_definition,
                           "road removal restores all input cells to ground");
            }
            check_edit(state.scene.world.world.ai.accounting.funds() == paid,
                       "road removal returns no synthetic refund");
        }
    } else
        check_edit(state.scene.world.world.ai.accounting.funds() == i.edit_cash_before,
                   "road previews neither charge nor commit a tile");
    edit_observations(state, i);
}
void begin_demolition_inspection(State &state, WorldManagementInspection &i) {
    // Pick the actual startup bun shop; retain its references and AI exactly as supplied.
    const auto found = std::find_if(state.scene.world.facility_order.begin(),
                                    state.scene.world.facility_order.end(), [&](auto id) {
                                        const auto &f = state.scene.world.world.facilities.at(id);
                                        return f.placement.definition_id == 33 && f.status == 1;
                                    });
    check_edit(found != state.scene.world.facility_order.end(), "no actual startup bun shop33");
    i.edited_old = *found;
    const auto placement = state.scene.world.world.facilities.at(*found).placement;
    i.selection = placement.definition_id;
    i.preview_anchor = i.edited_old_anchor = placement.anchor;
    i.preview_orientation = placement.orientation;
    i.edit_old_raw = state.facility_original_ids.at(*found);
    i.edit_old_ordinal = state.facility_ordinals.at(*found);
    i.edit_cash_before = state.scene.world.world.ai.accounting.funds();
    i.edit_draws_before = state.scene.random.draws();
    require_build(simulation::begin_startup_world_edit(state, false), "enter actual demolition");
    require_build(
        simulation::confirm_startup_world_edit(state, placement.anchor, placement.orientation),
        "demolish actual startup bun shop");
    check_edit(!state.scene.world.world.facilities.count(*i.edited_old) &&
                   std::none_of(state.scene.world.world.map.cells.begin(),
                                state.scene.world.world.map.cells.end(),
                                [&](const auto &cell) {
                                    return cell.facility &&
                                           cell.facility->instance_id.value == *i.edited_old;
                                }),
               "demolition retires actual instance and all map bindings");
    check_edit(state.scene.world.world.ai.accounting.funds() == i.edit_cash_before,
               "demolition does not refund the original building cost");
    i.edit_completed = true;
    edit_observations(state, i);
}
bool home_rebuild_input(State &state, const std::string &mode, WorldManagementInspection &i,
                        const rules::WorldScriptPage &page) {
    if (i.home_rebuild_stage == 0) {
        // Do not wait for the tax date. The public policy has already paid/gifted/admitted
        // through real source pages; only ordinary updates complete that actual home.
        if (page.kind == Kind::scene && state.scene.scene_state == 0 &&
            !state.scene.framework_paused && i.housing_policy.home &&
            state.scene.world.world.facilities.at(*i.housing_policy.home).status == 1) {
            const auto old = *i.housing_policy.home;
            const auto placement = state.scene.world.world.facilities.at(old).placement;
            const int resident = state.facility_residents.at(old);
            check_edit(resident >= 0 && i.housing_policy.resident == resident,
                       "completed natural home retains its admitted resident");
            const auto definition =
                std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                             [](const auto &d) { return d.kind == 12; });
            check_edit(definition != state.rules->facilities.end(),
                       "no published residence definition");
            check_edit(definition->id == 25, "published first residence retains definition25");
            i.selection = definition->id;
            i.home_resident = resident;
            i.edited_old = old;
            i.preview_anchor = i.edited_old_anchor = placement.anchor;
            i.preview_orientation = placement.orientation;
            i.edit_old_raw = state.facility_original_ids.at(old);
            i.edit_old_ordinal = state.facility_ordinals.at(old);
            i.home_credit_before = state.facility_free_builds.at(*i.selection);
            const int legacy_d3 = state.human_homes.at(resident)[3];
            i.edit_cash_before = state.scene.world.world.ai.accounting.funds();
            i.edit_draws_before = state.scene.random.draws();
            require_build(simulation::begin_startup_world_edit(state, false),
                          "enter natural home demolition");
            require_build(simulation::confirm_startup_world_edit(state, placement.anchor,
                                                                 placement.orientation),
                          "demolish actual admitted home");
            i.home_credit_after = state.facility_free_builds.at(*i.selection);
            i.home_credit_remaining = i.home_credit_after;
            check_edit(!state.scene.world.world.facilities.count(old) &&
                           state.human_homes.at(resident)[0] == 0 &&
                           state.human_homes.at(resident)[1] == 0 &&
                           state.human_homes.at(resident)[2] == 2 &&
                           state.human_homes.at(resident)[3] == legacy_d3 &&
                           i.home_credit_after == std::min(99, i.home_credit_before + 1),
                       "actual home demolition preserves D3 and returns one capped H entitlement");
            check_edit(state.scene.world.world.ai.accounting.funds() == i.edit_cash_before,
                       "actual home demolition refunds no admission or construction fee");
            edit_observations(state, i);
            i.home_rebuild_stage = 1;
            return true;
        }
        return apply_human_inspection_input(state, "world-tax", i.housing_policy);
    }
    if (page.legacy_page == 63 || page.legacy_page == 98)
        return true; // Automatic profession/tax consumers never receive a generic confirmation.
    if (page.legacy_page == 70)
        throw std::runtime_error(
            "Home rebuild inspection reached an unrelated profession decision");
    if (i.home_rebuild_stage == 1) {
        // Source135/136 belong to demolition and must actually be acknowledged. Returning
        // false lets the existing generic script consumer handle their real page identities.
        if (page.kind != Kind::scene)
            return false;
        if (state.scene.scene_state == 1)
            require(simulation::cancel_startup_world_edit(state), "leave actual home demolition");
        if (state.scene.scene_state != 0 || state.scene.framework_paused)
            return true;
        const auto quote = simulation::startup_world_build_quote(state, *i.selection);
        check_edit(quote.has_value() && quote->construction_cost == 800,
                   "published home25 quote remains actual800G despite its H entitlement");
        i.home_build_quote = quote->construction_cost;
        if (mode == "world-home-rebuilt" &&
            state.scene.world.world.ai.accounting.funds() < i.home_build_quote)
            return true; // Let real business earn the fee before opening a modal catalogue.
        require(simulation::open_startup_world_build_menu(state),
                "open actual rebuild entitlement");
        i.home_rebuild_stage = 2;
        return true;
    }
    if (i.home_rebuild_stage == 2) {
        if (mode == "world-home-credit")
            return true;
        if (page.legacy_page != 21)
            return false;
        check_edit(state.facility_free_builds.at(*i.selection) > 0 &&
                       state.human_homes.at(*i.home_resident)[2] == 2,
                   "rebuild consumes a genuine returned entitlement and displaced resident");
        i.home_build_cash_before = state.scene.world.world.ai.accounting.funds();
        i.home_build_draws_before = state.scene.random.draws();
        require_build(simulation::select_startup_world_build_menu(state, page.id, *i.selection),
                      "select actual paid residence rebuild");
        const auto anchor = empty_site(state, *i.selection, i.preview_orientation);
        const auto built =
            simulation::confirm_startup_world_build(state, anchor, i.preview_orientation);
        require_build(built, "place actual returned-entitlement residence");
        check_edit(built.created.has_value(), "actual residence rebuild returned no instance");
        i.created = built.created;
        i.housing_policy.home =
            built.created; // Retired home never leaks into subsequent diagnostics.
        i.preview_anchor = anchor;
        i.home_credit_remaining = state.facility_free_builds.at(*i.selection);
        i.home_build_cash_after = state.scene.world.world.ai.accounting.funds();
        i.home_build_draws_after = state.scene.random.draws();
        check_edit(i.home_build_cash_after == i.home_build_cash_before - i.home_build_quote &&
                       i.home_build_draws_after == i.home_build_draws_before &&
                       i.home_credit_remaining == i.home_credit_after - 1 &&
                       state.facility_residents.at(*i.created) == *i.home_resident &&
                       state.facility_details.at(*i.created).residence_mode == 0 &&
                       state.human_homes.at(*i.home_resident) ==
                           std::array<int, 4>{anchor.x, anchor.y, 1, 0},
                   "rebuild pays construction once, consumes H and binds the same resident without "
                   "admission");
        // The actual final H consumer already returns to scene0. If more H remains, leave
        // continuous placement through its normal source cancel rather than editing the mode.
        if (state.scene.scene_state == 1)
            require(simulation::cancel_startup_world_build(state), "leave actual paid rebuilding");
        i.home_rebuild_stage = 3;
        i.edit_completed = true;
        return true;
    }
    return page.kind ==
           Kind::scene; // Ordinary updates finish construction; real pages keep their consumers.
}
} // namespace
bool management_inspection_mode(const std::string &mode) {
    return mode == "world-building" || mode == "world-details" ||
           mode == "world-facility-bonuses" || mode == "world-built" || build_preview_mode(mode) ||
           mode == "world-award-granted" || village_mode(mode) || mode == "world-reward95" ||
           road_mode(mode) || mode == "world-demolished" || home_mode(mode) || expansion_mode(mode);
}
void begin_management_inspection(State &state, const std::string &mode,
                                 WorldManagementInspection &inspection) {
    if (expansion_mode(mode)) {
        // Explicit source-callsite window fixture, not a natural unlock. Only eligibility,
        // points and the quarter slot are prepared; source51/52/53 own all map changes,
        // payments, counters and retirement. No terrain/entity/popularity is injected.
        state.scripts.event_calls[100] = 1;
        for (auto &[id, activity] : state.scripts.activities)
            activity.status = id == 25 ? 1 : 0;
        state.village_points = 100;
        state.quarter_counter = 3;
        inspection.activity = 25;
        inspection.expansion_level_before = state.fence_level;
        require(simulation::open_startup_world_village_activities(state),
                "open expansion callsite fixture");
    }
    if (road_mode(mode))
        begin_road_inspection(state, mode, inspection);
    if (mode == "world-demolished")
        begin_demolition_inspection(state, inspection);
    if (home_mode(mode))
        begin_human_inspection(state, "world-tax", inspection.housing_policy);
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
    if (mode == "world-facility-bonuses") {
        const auto found =
            std::find_if(state.scene.world.facility_order.begin(),
                         state.scene.world.facility_order.end(), [&](auto id) {
                             return !state.neighbourhood_details.at(id).sources.empty() &&
                                    state.scene.world.world.facilities.at(id).status != 0;
                         });
        if (found == state.scene.world.facility_order.end())
            throw std::runtime_error("Initial world has no actual facility bonus sources");
        require(simulation::open_startup_world_facility_page(state, *found),
                "open real bonus target");
        require(
            simulation::act_startup_world_facility_page(
                state, state.scripts.pages.back().id, simulation::StartupFacilityPageAction::next),
            "open actual facility bonus page");
    }
}
bool management_inspection_ready(const State &state, const std::string &mode,
                                 const WorldManagementInspection &inspection) {
    const auto *page = top(state);
    if (!page)
        return false;
    if (expansion_mode(mode)) {
        if (mode == "world-expansion-completed")
            return inspection.activity_completed && page->kind == Kind::scene &&
                   state.scene.scene_state == 0 &&
                   state.fence_level == inspection.expansion_level_before + 1;
        const auto view = simulation::inspect_startup_world_village_activity_page(state, page->id);
        return view && (mode == "world-expansion-catalogue"
                            ? view->raw == 51 && view->entries == std::vector<int>{25}
                            : view->raw == 53 && view->activity == 25 && view->counter >= 70 &&
                                  inspection.activity_started &&
                                  state.fence_level == inspection.expansion_level_before);
    }
    if (home_mode(mode)) {
        if (mode == "world-home-credit")
            return inspection.home_rebuild_stage == 2 && page->legacy_page == 21 &&
                   inspection.selection && inspection.home_resident &&
                   state.facility_free_builds.at(*inspection.selection) ==
                       inspection.home_credit_after &&
                   inspection.home_credit_after > 0 &&
                   state.human_homes.at(*inspection.home_resident)[2] == 2;
        return inspection.home_rebuild_stage == 3 && inspection.created &&
               page->kind == Kind::scene && state.scene.scene_state == 0 &&
               state.scene.world.world.facilities.at(*inspection.created).status == 1 &&
               state.facility_residents.at(*inspection.created) == inspection.home_resident;
    }
    if (road_mode(mode) || mode == "world-demolished")
        return page->kind == Kind::scene && state.scene.scene_state == 1 &&
               state.build_mode == (mode == "world-road-end" ? 2
                                    : mode == "world-road-remove" || mode == "world-demolished"
                                        ? 3
                                        : 1) &&
               (mode == "world-road-start" || mode == "world-road-end" ||
                inspection.edit_completed);
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
    if (mode == "world-facility-bonuses")
        return page->legacy_page == 74 && state.page_phases.at(page->id) == 1 &&
               simulation::startup_world_facility_bonus_rows(state, page->id).has_value();
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
    if (home_mode(mode))
        return home_rebuild_input(state, mode, inspection, page);
    if (expansion_mode(mode)) {
        using A = simulation::StartupVillageActivityAction;
        const auto view = simulation::inspect_startup_world_village_activity_page(state, page.id);
        if (!view || state.activity_page_answers.count(page.id))
            return true; // First initialization and real parent resumption belong to updates.
        if (view->raw == 51) {
            require(simulation::act_startup_world_village_activity_page(
                        state, page.id, inspection.activity_completed ? A::cancel : A::confirm),
                    "select expansion or close its resumed catalogue");
        } else if (view->raw == 52) {
            require(simulation::act_startup_world_village_activity_page(state, page.id, A::confirm),
                    "start expansion through actual52 payment");
            inspection.activity_started = true;
        } else if (view->raw == 53 && view->counter >= 120) {
            require(simulation::act_startup_world_village_activity_page(state, page.id, A::confirm),
                    "complete expansion through actual53 map transaction");
            inspection.activity_completed = true;
        } else if (view->raw == 54)
            throw std::runtime_error("Expansion incorrectly inserted human-result54");
        return true;
    }
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
