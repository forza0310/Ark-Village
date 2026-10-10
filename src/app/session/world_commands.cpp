#include "world_commands.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"

#include <algorithm>

namespace ark::app::detail {
namespace {
using Kind = WorldCommandKind;
using Error = simulation::StartupWorldRuntimeError;
void build_result(const simulation::StartupBuildResult &source, WorldCommandResult &result) {
    result.runtime_error = source.error;
    result.build_denial = source.denial;
    result.created = source.created;
}
bool valid_task_action(simulation::StartupWorldTaskAction action) {
    using Action = simulation::StartupWorldTaskAction;
    switch (action) {
    case Action::confirm:
    case Action::cancel:
    case Action::add_member:
    case Action::depart:
    case Action::hire:
    case Action::inspect:
    case Action::request_abort:
        return true;
    }
    return false;
}
bool valid_human_action(simulation::StartupHumanPageAction action) {
    using Action = simulation::StartupHumanPageAction;
    switch (action) {
    case Action::previous:
    case Action::next:
    case Action::select:
    case Action::confirm:
    case Action::cancel:
    case Action::professions:
    case Action::gifts:
    case Action::inspect_equipment:
    case Action::view_tab:
    case Action::equipment_slot:
        return true;
    }
    return false;
}
bool valid_tax_action(simulation::StartupWorldTaxAction action) {
    using Action = simulation::StartupWorldTaxAction;
    return action == Action::previous || action == Action::next || action == Action::select ||
           action == Action::confirm;
}
bool valid_village_action(simulation::StartupVillageActivityAction action) {
    using Action = simulation::StartupVillageActivityAction;
    return action == Action::previous || action == Action::next || action == Action::select ||
           action == Action::confirm || action == Action::cancel;
}
bool valid_commerce_action(simulation::StartupCommerceAction action) {
    using Action = simulation::StartupCommerceAction;
    return action == Action::confirm || action == Action::cancel || action == Action::previous ||
           action == Action::next || action == Action::select || action == Action::previous_tab ||
           action == Action::next_tab || action == Action::inspect;
}
bool valid_facility_item_action(simulation::StartupFacilityItemAction action) {
    using Action = simulation::StartupFacilityItemAction;
    return action == Action::confirm || action == Action::cancel || action == Action::previous ||
           action == Action::next || action == Action::select;
}
bool unsupported_village_choice(const WorldState &state, const WorldCommand &command) {
    using Action = simulation::StartupVillageActivityAction;
    if (command.village_activity_action != Action::confirm)
        return false;
    const auto view = simulation::inspect_startup_world_village_activity_page(state, command.page);
    if (!view || view->raw != 51 || view->selection < 0 ||
        view->selection >= static_cast<int>(view->entries.size()))
        return false; // A damaged initialized payload still reaches the source failure path.
    const int id = view->entries[view->selection];
    const auto definition =
        std::find_if(state.rules->activities.begin(), state.rules->activities.end(),
                     [id](const auto &v) { return v.identity == id; });
    // Type4 has no standalone effect; published type5/6 use the real magic-pot Owner.
    // A stale UI choice cannot turn an
    // intentionally disabled feature into a fatal missing-source error for the whole world.
    return definition != state.rules->activities.end() &&
           (definition->parameters[2] == 4 || definition->parameters[2] > 6);
}
bool human_page(const WorldState &state, std::uint64_t id) {
    const auto page = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    if (page == state.scripts.pages.rend() || page->id != id ||
        page->kind != simulation::rules::WorldScriptPageKind::raw_page)
        return false;
    const auto raw = page->legacy_page;
    return (raw >= 60 && raw <= 66) || raw == 68 || raw == 70 || raw == 73;
}
bool valid_award_action(simulation::rules::WorldAwardAction action) {
    using Action = simulation::rules::WorldAwardAction;
    switch (action) {
    case Action::update:
    case Action::request_termination:
    case Action::confirm_termination:
    case Action::reject_termination:
    case Action::request_award:
    case Action::confirm_award:
    case Action::reject_award:
        return true;
    }
    return false;
}
// The source reports invalid prompt choices as missing_source. Reject stale UI choices
// before invoking it, while retaining missing-source errors for actual runtime corruption.
bool stale_award_choice(const WorldState &state, const WorldCommand &command) {
    using Action = simulation::rules::WorldAwardAction;
    const auto ranking = state.award_rankings.find(command.page);
    if (ranking == state.award_rankings.end())
        return false; // An uninitialized live page is still initialized by the source.
    const auto termination = state.award_termination_pending.find(command.page);
    const bool terminating =
        termination != state.award_termination_pending.end() && termination->second;
    const bool awarding = state.award_pending_humans.count(command.page) != 0;
    switch (command.award_action) {
    case Action::confirm_termination:
    case Action::reject_termination:
        return !terminating;
    case Action::confirm_award:
    case Action::reject_award:
        return !awarding;
    case Action::request_termination:
        return terminating || awarding;
    case Action::request_award:
        return terminating || awarding || command.selection < 0 ||
               static_cast<std::size_t>(command.selection) >= ranking->second.size();
    default:
        return false;
    }
}
} // namespace
bool is_world_decision(Kind kind) {
    return kind != Kind::set_paused && kind != Kind::set_speed && kind != Kind::set_view &&
           kind != Kind::acknowledge_page && kind != Kind::acknowledge_report;
}
bool is_decision_page(const simulation::rules::WorldScriptPage *page) {
    if (!page || page->kind != simulation::rules::WorldScriptPageKind::raw_page)
        return false;
    const auto raw = page->legacy_page;
    return raw == 4 || (raw >= 21 && raw <= 28) || raw == 33 || (raw >= 41 && raw <= 48) ||
           (raw >= 51 && raw <= 54) || (raw >= 60 && raw <= 66) || raw == 68 || raw == 70 ||
           raw == 69 || raw == 72 || raw == 73 || (raw >= 74 && raw <= 77) || raw == 79 ||
           raw == 80 || raw == 82 || (raw >= 83 && raw <= 86) || raw == 90 || raw == 93 ||
           raw == 98;
}
void apply_world_decision(WorldState &state, const WorldCommand &command,
                          WorldCommandResult &result) {
    // Missing definitions are integration errors even where a source entry conflates them
    // with an ineligible page. Gameplay refusal is never upgraded to missing-source failure.
    if (!state.rules) {
        result.runtime_error = Error::missing_source;
        return;
    }
    switch (command.kind) {
    case Kind::open_menu_magic_pot:
    case Kind::open_magic_pot:
        result.runtime_error = simulation::open_startup_world_magic_pot(
            state, simulation::StartupMagicPotEntry::main_menu);
        break;
    case Kind::magic_pot_action:
        result.runtime_error =
            state.magic_pot_pages_initialized.count(command.page)
                ? simulation::act_startup_world_magic_pot_page(
                      state, command.page, command.magic_pot_action, command.selection)
                : Error::invalid_page;
        break;
    case Kind::open_menu_tasks:
    case Kind::open_task_menu:
        result.runtime_error = simulation::open_startup_world_runtime_task_menu(state);
        break;
    case Kind::open_task_control_menu:
        result.runtime_error = simulation::open_startup_world_runtime_task_control_menu(state);
        break;
    case Kind::open_menu_village_activities:
    case Kind::open_village_activities:
        result.runtime_error = simulation::open_startup_world_village_activities(state);
        break;
    case Kind::open_menu_commerce:
    case Kind::open_commerce:
        result.runtime_error = simulation::open_startup_world_commerce(state);
        break;
    case Kind::commerce_action: {
        if (!valid_commerce_action(command.commerce_action) ||
            !state.commerce_pages_initialized.count(command.page)) {
            result.runtime_error = Error::invalid_page;
            break;
        }
        const auto view = simulation::inspect_startup_world_commerce_page(state, command.page);
        if (view && view->raw == 86) {
            result.runtime_error = Error::invalid_page; // Only its automatic consumer retires86.
            break;
        }
        std::optional<int> item;
        int inventory{}, stock{};
        const auto funds = state.scene.world.world.ai.accounting.funds();
        if (view && view->raw == 84 &&
            command.commerce_action == simulation::StartupCommerceAction::confirm &&
            view->selection >= 0 && view->selection < static_cast<int>(view->entries.size())) {
            item = view->entries[view->selection];
            inventory = state.items.at(*item).inventory;
            stock = state.shop_item_stock.at(*item).quantity;
        }
        result.runtime_error = simulation::act_startup_world_commerce_page(
            state, command.page, command.commerce_action, command.selection);
        if (result.runtime_error == Error::none && item &&
            (state.items.at(*item).inventory != inventory ||
             state.shop_item_stock.at(*item).quantity != stock)) {
            const auto delta = state.scene.world.world.ai.accounting.funds() - funds;
            result.commerce_amount = delta < 0 ? -delta : delta;
        }
        break;
    }
    case Kind::facility_catalog_action:
        // Reject input from a pending snapshot; a corrupt initialized payload still fails
        // through the source validator. Generic acknowledgements cannot bypass this path.
        result.runtime_error =
            state.facility_catalog_pages_initialized.count(command.page)
                ? simulation::act_startup_world_facility_catalog_page(
                      state, command.page, command.facility_catalog_action, command.selection)
                : Error::invalid_page;
        break;
    case Kind::facility_item_action:
        result.runtime_error =
            valid_facility_item_action(command.facility_item_action) &&
                    state.facility_item_pages_initialized.count(command.page)
                ? simulation::act_startup_world_facility_item_page(
                      state, command.page, command.facility_item_action, command.selection)
                : Error::invalid_page;
        break;
    case Kind::village_activity_action:
        result.runtime_error =
            valid_village_action(command.village_activity_action) &&
                    state.activity_pages_initialized.count(command.page) &&
                    !unsupported_village_choice(state, command)
                ? simulation::act_startup_world_village_activity_page(
                      state, command.page, command.village_activity_action, command.selection)
                : Error::invalid_page;
        break;
    case Kind::open_menu_build:
    case Kind::open_build_menu:
        result.runtime_error = simulation::open_startup_world_build_menu(state);
        break;
    case Kind::select_build_menu:
        if (command.definition == -1 || command.definition == -2 ||
            std::any_of(state.rules->facilities.begin(), state.rules->facilities.end(),
                        [&](const auto &d) { return d.id == command.definition && d.kind == 6; })) {
            // Source denials can carry valid feedback candidates, so the worker commits them.
            // This bridge has no such feedback: close and begin must succeed together.
            auto next = state;
            result.runtime_error = simulation::cancel_startup_world_build_menu(next, command.page);
            if (result.runtime_error == Error::none) {
                build_result(
                    command.definition < 0
                        ? simulation::begin_startup_world_edit(next, command.definition == -2)
                        : simulation::begin_startup_world_road(next, command.definition),
                    result);
                if (result.runtime_error == Error::none &&
                    result.build_denial == simulation::StartupBuildDenial::none)
                    state = std::move(next);
            }
        } else {
            build_result(simulation::select_startup_world_build_menu(state, command.page,
                                                                     command.definition),
                         result);
        }
        break;
    case Kind::cancel_build_menu:
        result.runtime_error = simulation::cancel_startup_world_build_menu(state, command.page);
        break;
    case Kind::confirm_build:
    case Kind::cancel_build:
        if (state.build_definition != command.definition) {
            result.runtime_error = Error::invalid_page;
        } else if (command.kind == Kind::confirm_build) {
            build_result(
                simulation::confirm_startup_world_build(state, command.anchor, command.orientation),
                result);
        } else {
            result.runtime_error = simulation::cancel_startup_world_build(state);
        }
        break;
    case Kind::confirm_edit:
    case Kind::cancel_edit:
        if (state.scene.scene_state != 1 || state.build_mode != command.selection ||
            state.build_mode < 1 || state.build_mode > 7 || state.build_mode == 4 ||
            (command.kind == Kind::confirm_edit &&
             command.orientation != simulation::rules::FacilityOrientation::first &&
             command.orientation != simulation::rules::FacilityOrientation::second) ||
            state.build_definition.value_or(-1) != command.definition ||
            !(state.build_anchor == command.edit_anchor) ||
            state.build_moving_facility.value_or(0) != command.facility) {
            result.runtime_error = Error::invalid_page;
        } else if (command.kind == Kind::cancel_edit) {
            result.runtime_error = simulation::cancel_startup_world_edit(state);
        } else {
            // The maintained single-binding map cannot represent a road over a special
            // facility while retaining its old pointer. Reject that known unsupported input
            // before the source missing-data failure; never partly commit a road segment.
            bool unsupported{};
            if (state.build_mode == 2 && state.rules && state.fence_level >= 0 &&
                static_cast<std::size_t>(state.fence_level) < state.rules->fences.size()) {
                const auto segment = simulation::startup_world_edit_segment(state, command.anchor);
                const auto &f = state.rules->fences.at(state.fence_level);
                if (segment)
                    for (const auto p : *segment) {
                        const auto &map = state.scene.world.world.map;
                        const auto &tile = map.cells.at(p.y * map.width + p.x);
                        if (p.x > f[0].x && p.x < f[1].x && p.y > f[1].y && p.y < f[0].y &&
                            tile.legacy_state != 1 && tile.legacy_state != 2 &&
                            tile.legacy_state != 3 && tile.facility)
                            unsupported = true;
                    }
            }
            if (unsupported)
                result.build_denial = simulation::StartupBuildDenial::unavailable;
            else
                build_result(simulation::confirm_startup_world_edit(state, command.anchor,
                                                                    command.orientation),
                             result);
        }
        break;
    case Kind::open_facility: {
        // Source prototype input admits only non-construction instances. Recheck against
        // the current Owner: a stale render snapshot can outlive construction/replacement.
        // Missing data for an eligible live instance still propagates as a source failure.
        const auto facility = state.scene.world.world.facilities.find(command.facility);
        if (facility == state.scene.world.world.facilities.end() || facility->second.status == 0)
            result.runtime_error = Error::invalid_page;
        else
            result.runtime_error =
                simulation::open_startup_world_facility_page(state, command.facility);
        break;
    }
    case Kind::open_human: {
        const auto &ai = state.scene.world.world.ai;
        const auto actor = ai.battle.actors.find(command.actor);
        const auto presence = state.human_presence.find(command.definition);
        if (actor == ai.battle.actors.end() ||
            actor->second.kind != simulation::rules::ActorKind::human ||
            actor->second.control.state == 4 || actor->second.definition != command.definition ||
            std::find(ai.human_order.begin(), ai.human_order.end(), command.actor) ==
                ai.human_order.end() ||
            presence == state.human_presence.end() || presence->second == 0)
            result.runtime_error = Error::invalid_page;
        else
            result.runtime_error =
                simulation::open_startup_world_human_page(state, command.definition);
        break;
    }
    case Kind::human_action:
        result.runtime_error =
            valid_human_action(command.human_action) &&
                    simulation::startup_world_human_page_ready(state, command.page)
                ? simulation::act_startup_world_human_page(state, command.page,
                                                           command.human_action, command.selection)
                : Error::invalid_page;
        break;
    case Kind::tax_action:
        // Only the framework may initialize a tax list. A stale click cannot create its
        // payload, advance automatic98, or consume the next report/page instead.
        result.runtime_error =
            valid_tax_action(command.tax_action) && state.tax_page_residents.count(command.page)
                ? simulation::act_startup_world_tax_page(state, command.page, command.tax_action,
                                                         command.selection)
                : Error::invalid_page;
        break;
    case Kind::facility_action: {
        using Action = simulation::StartupFacilityPageAction;
        if (command.facility_action != Action::previous &&
            command.facility_action != Action::next && command.facility_action != Action::confirm &&
            command.facility_action != Action::cancel)
            result.runtime_error = Error::invalid_page;
        else
            result.runtime_error = simulation::act_startup_world_facility_page(
                state, command.page, command.facility_action);
        break;
    }
    case Kind::residence_action:
        build_result(simulation::act_startup_world_residence_page(
                         state, command.page, command.selection, command.cancel),
                     result);
        break;
    case Kind::rank_action:
        result.runtime_error = command.selection < 0 || command.selection > 4
                                   ? Error::invalid_page
                                   : simulation::act_startup_world_runtime_rank_page(
                                         state, command.page, command.selection, command.cancel);
        break;
    case Kind::award_action:
        result.runtime_error =
            valid_award_action(command.award_action) && !stale_award_choice(state, command)
                ? simulation::act_startup_world_runtime_award_page(
                      state, command.page, command.award_action, command.selection)
                : Error::invalid_page;
        break;
    case Kind::cancel_page:
        result.runtime_error =
            human_page(state, command.page) &&
                    !simulation::startup_world_human_page_ready(state, command.page)
                ? Error::invalid_page
                : simulation::cancel_startup_world_runtime_page(state, command.page);
        break;
    case Kind::task_action: {
        if (!valid_task_action(command.task_action) ||
            (human_page(state, command.page) &&
             !simulation::startup_world_human_page_ready(state, command.page))) {
            result.runtime_error = Error::invalid_page;
            break;
        }
        const auto source = simulation::act_startup_world_runtime_task_page(
            state, command.page, command.task_action, command.selection);
        result.runtime_error = source.error;
        result.denial = source.denial;
        result.task_accepted = source.accepted;
        result.departed = source.departed;
        break;
    }
    default:
        result.runtime_error = Error::invalid_page;
        break;
    }
}
} // namespace ark::app::detail

namespace ark::app {
std::uint64_t WorldSession::open_menu_build() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_menu_build;
    return submit(command);
}
std::uint64_t WorldSession::open_build_menu() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_build_menu;
    return submit(command);
}
std::uint64_t WorldSession::open_task_control_menu() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_task_control_menu;
    return submit(command);
}
std::uint64_t WorldSession::select_build_menu(std::uint64_t page, int definition) {
    WorldCommand command;
    command.kind = WorldCommandKind::select_build_menu;
    command.page = page;
    command.definition = definition;
    return submit(command);
}
std::uint64_t WorldSession::cancel_build_menu(std::uint64_t page) {
    WorldCommand command;
    command.kind = WorldCommandKind::cancel_build_menu;
    command.page = page;
    return submit(command);
}
std::uint64_t WorldSession::confirm_build(int expected_definition,
                                          simulation::rules::Position anchor,
                                          simulation::rules::FacilityOrientation orientation) {
    WorldCommand command;
    command.kind = WorldCommandKind::confirm_build;
    command.definition = expected_definition;
    command.anchor = anchor;
    command.orientation = orientation;
    return submit(command);
}
std::uint64_t WorldSession::cancel_build(int expected_definition) {
    WorldCommand command;
    command.kind = WorldCommandKind::cancel_build;
    command.definition = expected_definition;
    return submit(command);
}
std::uint64_t WorldSession::open_facility(std::uint64_t facility) {
    WorldCommand command;
    command.kind = WorldCommandKind::open_facility;
    command.facility = facility;
    return submit(command);
}
std::uint64_t WorldSession::open_human(simulation::rules::CharacterId actor, int definition) {
    WorldCommand command;
    command.kind = WorldCommandKind::open_human;
    command.actor = actor;
    command.definition = definition;
    return submit(command);
}
std::uint64_t WorldSession::act_human(std::uint64_t page, simulation::StartupHumanPageAction action,
                                      int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::human_action;
    command.page = page;
    command.human_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::act_tax(std::uint64_t page, simulation::StartupWorldTaxAction action,
                                    int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::tax_action;
    command.page = page;
    command.tax_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::open_menu_village_activities() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_menu_village_activities;
    return submit(command);
}
std::uint64_t WorldSession::open_village_activities() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_village_activities;
    return submit(command);
}
std::uint64_t WorldSession::act_village_activity(std::uint64_t page,
                                                 simulation::StartupVillageActivityAction action,
                                                 int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::village_activity_action;
    command.page = page;
    command.village_activity_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::confirm_edit(const WorldState &observed,
                                         simulation::rules::Position target,
                                         simulation::rules::FacilityOrientation orientation) {
    WorldCommand command;
    command.kind = WorldCommandKind::confirm_edit;
    command.selection = observed.build_mode;
    command.definition = observed.build_definition.value_or(-1);
    command.edit_anchor = observed.build_anchor;
    command.facility = observed.build_moving_facility.value_or(0);
    command.anchor = target;
    command.orientation = orientation;
    return submit(command);
}
std::uint64_t WorldSession::cancel_edit(const WorldState &observed) {
    WorldCommand command;
    command.kind = WorldCommandKind::cancel_edit;
    command.selection = observed.build_mode;
    command.definition = observed.build_definition.value_or(-1);
    command.edit_anchor = observed.build_anchor;
    command.facility = observed.build_moving_facility.value_or(0);
    return submit(command);
}
std::uint64_t WorldSession::open_menu_commerce() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_menu_commerce;
    return submit(command);
}
std::uint64_t WorldSession::open_commerce() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_commerce;
    return submit(command);
}
std::uint64_t WorldSession::act_commerce(std::uint64_t page,
                                         simulation::StartupCommerceAction action, int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::commerce_action;
    command.page = page;
    command.commerce_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::act_facility_item(std::uint64_t page,
                                              simulation::StartupFacilityItemAction action,
                                              int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::facility_item_action;
    command.page = page;
    command.facility_item_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::act_facility(std::uint64_t page,
                                         simulation::StartupFacilityPageAction action) {
    WorldCommand command;
    command.kind = WorldCommandKind::facility_action;
    command.page = page;
    command.facility_action = action;
    return submit(command);
}
std::uint64_t WorldSession::act_facility_catalog(std::uint64_t page,
                                                 simulation::StartupFacilityCatalogAction action,
                                                 int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::facility_catalog_action;
    command.page = page;
    command.facility_catalog_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::open_menu_magic_pot() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_menu_magic_pot;
    return submit(command);
}
std::uint64_t WorldSession::open_magic_pot() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_magic_pot;
    return submit(command);
}
std::uint64_t WorldSession::act_magic_pot(std::uint64_t page,
                                          simulation::StartupMagicPotAction action, int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::magic_pot_action;
    command.page = page;
    command.magic_pot_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::act_residence(std::uint64_t page, int human, bool cancel) {
    WorldCommand command;
    command.kind = WorldCommandKind::residence_action;
    command.page = page;
    command.selection = human;
    command.cancel = cancel;
    return submit(command);
}
std::uint64_t WorldSession::act_rank(std::uint64_t page, int selection, bool cancel) {
    WorldCommand command;
    command.kind = WorldCommandKind::rank_action;
    command.page = page;
    command.selection = selection;
    command.cancel = cancel;
    return submit(command);
}
} // namespace ark::app
