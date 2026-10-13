#include "world_session_test_support.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ark::test::world_session {
void facility_catalog_commands() {
    using Action = sim::StartupFacilityCatalogAction;
    using Outcome = app::WorldCommandOutcome;
    for (const int raw : {79, 82}) {
        auto state = initial();
        rules::WorldScriptPage page;
        page.kind = rules::WorldScriptPageKind::raw_page;
        page.legacy_page = raw;
        page.legacy_f = raw == 79 ? 1 : 0;
        if (raw == 82)
            page.facility_definition = 33;
        const auto opened =
            rules::prepare_world_script_page(sim::startup_world_runtime_scripts(state), page);
        check(opened.candidate &&
                  sim::write_startup_world_runtime_scripts(state, opened.candidate->state) &&
                  sim::initialize_startup_world_facility_catalog_pages(state),
              "Source initializes the explicit commodity/publicity callsite fixture");
        const auto id = task_top(state).id;
        auto expected = state;
        app::WorldSession session(state);
        // A generic acknowledgement must not bypass NEW clearing or the two publicity gates.
        const auto generic = session.ack_page(id);
        const auto invalid = session.act_facility_catalog(id, static_cast<Action>(999));
        sim::StartupFacilityCatalogInput malformed;
        malformed.selection = std::numeric_limits<int>::max();
        const auto malformed_serial = session.input_facility_catalog(id, malformed);
        if (raw == 79) {
            const auto source = sim::inspect_startup_world_facility_catalog_page(expected, id);
            check(source && !source->entries.empty(),
                  "Real new-game equipment catalogue is nonempty");
            sim::StartupFacilityCatalogInput combined;
            combined.up = combined.down = combined.left = combined.right = true;
            combined.selection = static_cast<int>(source->entries.size()) - 1;
            combined.scroll = sim::StartupFacilityCatalogInput::Scroll{true, 0};
            check(sim::input_startup_world_facility_catalog_page(expected, id, combined) ==
                      sim::StartupWorldRuntimeError::none,
                  "Sequential reference composes one full commodity input frame");
            session.input_facility_catalog(id, combined);
            check(sim::act_startup_world_facility_catalog_page(expected, id, Action::cancel) ==
                      sim::StartupWorldRuntimeError::none,
                  "Sequential commodity close reference");
            session.act_facility_catalog(id, Action::cancel);
        } else {
            for (int n = 0; n < 4; ++n) {
                sim::StartupFacilityCatalogInput confirm;
                confirm.confirm = true;
                check(sim::input_startup_world_facility_catalog_page(expected, id, confirm) ==
                          sim::StartupWorldRuntimeError::none,
                      "Sequential two40 publicity reference");
                session.input_facility_catalog(id, confirm);
            }
        }
        const auto stale = session.act_facility_catalog(id, Action::confirm);
        const auto frame = input_frame(session, session.set_paused(true));
        check(!frame->failed && input_result(*frame, generic).outcome == Outcome::rejected &&
                  input_result(*frame, invalid).outcome == Outcome::rejected &&
                  input_result(*frame, malformed_serial).outcome == Outcome::rejected &&
                  input_result(*frame, stale).outcome == Outcome::rejected,
              "Generic, invalid-enum and retired catalogue inputs reject without failing worker");
        check(frame->state->scene.world.world.ai.accounting.funds() ==
                      expected.scene.world.world.ai.accounting.funds() &&
                  frame->state->scene.random.draws() == expected.scene.random.draws() &&
                  frame->state->popularity == expected.popularity &&
                  frame->state->scene.world.popularity_queue ==
                      expected.scene.world.popularity_queue,
              "FIFO close preserves cash/random and only queues the source delayed popularity");
        for (const auto &entry : expected.catalog)
            check(frame->state->catalog.at(entry.first).newly_unlocked ==
                          entry.second.newly_unlocked &&
                      frame->state->catalog.at(entry.first).inventory == entry.second.inventory,
                  "Commodity closing clears exact category NEW without buying any equipment");
        session.stop();
    }
}
// These are transport cases. The frozen building suite owns geometry/pricing combinations;
// here source calls are the sequential oracle for one real placement and page transaction.
rules::Position one_cell_site(const app::WorldState &state) {
    const auto bounds = state.rules->fences.at(state.fence_level);
    const auto &map = state.scene.world.world.map;
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x)
            if (!map.cells.at(y * map.width + x).facility)
                return {x, y};
    throw std::runtime_error("Initial fixture has no free one-cell building site");
}
void building_command_transactions() {
    using Outcome = app::WorldCommandOutcome;
    auto state = initial(true);
    app::WorldSession session(state);
    const auto blocked_open = input_frame(session, session.open_main_menu());
    check(!blocked_open->failed && !blocked_open->main_menu_open &&
              input_result(*blocked_open, blocked_open->last_command_serial).outcome ==
                  Outcome::rejected,
          "Paused framework refuses the real main menu without resuming or creating raw3");
    same_world(state, *blocked_open->state);
    const auto legacy_menu = input_frame(session, session.open_menu_build());
    check(!legacy_menu->failed && !legacy_menu->main_menu_open &&
              input_result(*legacy_menu, legacy_menu->last_command_serial).outcome ==
                  Outcome::rejected,
          "Retired Construction shortcut cannot bypass paused source navigation");
    same_world(state, *legacy_menu->state);
    session.set_paused(false);
    input_frame(session, session.open_main_menu());
    const auto main = menu_ready(session, 3);
    check(!main->failed && task_top(*main->state).legacy_page == 3,
          "Construction input waits for the actual initialized main menu");
    const auto paused_menu = input_frame(session, session.set_paused(true));
    sim::StartupWorldMenuInput confirm;
    confirm.confirm = true; // The actual initial raw3 selection is Construction/tag0.
    const auto blocked = input_frame(session, session.act_menu(task_top(*main->state).id, confirm));
    check(!blocked->failed && blocked->main_menu_open &&
              input_result(*blocked, blocked->last_command_serial).outcome == Outcome::rejected,
          "Paused Construction confirmation preserves the real menu and explicit pause");
    same_world(*paused_menu->state, *blocked->state);
    session.set_paused(false);
    const auto first_directory = menu_tag(session, 3, 0);
    const auto first_page = std::find_if(
        first_directory->state->scripts.pages.begin(), first_directory->state->scripts.pages.end(),
        [](const auto &p) { return p.lifecycle != 4 && p.legacy_page == 21; });
    const auto &tutorial = task_top(*first_directory->state);
    check(!first_directory->failed && first_page != first_directory->state->scripts.pages.end() &&
              tutorial.kind == rules::WorldScriptPageKind::dialogue &&
              tutorial.source_record == 91 && tutorial.id < first_page->id &&
              event_count(*first_directory->state, 114) == 1,
          "First Construction creates tutorial91 before21; retiring menus exposes the dialogue "
          "without deleting21");
    const auto tutorial_closed = session.ack_page(tutorial.id);
    const auto after_tutorial = input_frame(session, tutorial_closed);
    check(!after_tutorial->failed && task_top(*after_tutorial->state).legacy_page == 21 &&
              task_top(*after_tutorial->state).id == first_page->id,
          "Confirming actual first-build dialogue reveals the original construction directory");
    const auto cancelled_directory =
        input_frame(session, session.cancel_build_menu(first_page->id));
    check(
        !cancelled_directory->failed &&
            task_top(*cancelled_directory->state).kind == rules::WorldScriptPageKind::scene &&
            input_result(*cancelled_directory, cancelled_directory->last_command_serial).outcome ==
                Outcome::applied,
        "Closing21 after its tutorial returns to the existing source scene");
    const auto reopened = session.open_main_menu();
    const auto read_main = menu_ready(session, 3);
    check(!read_main->failed && input_result(*read_main, reopened).outcome == Outcome::applied &&
              event_count(*read_main->state, 114) == 1,
          "Tutorial and directory retirement precede reopening raw3 without repeating114");
    const auto chosen = menu_tag(session, 3, 0);
    const auto opened = chosen->last_command_serial;
    const auto page_frame = input_frame(session, session.set_paused(true));
    const auto page = task_top(*page_frame->state).id;
    check(
        !page_frame->failed && !page_frame->main_menu_open &&
            task_top(*page_frame->state).legacy_page == 21 &&
            input_result(*page_frame, opened).outcome == Outcome::applied &&
            event_count(*page_frame->state, 114) == 1 &&
            page_frame->state->scene.world.world.ai.accounting.funds() ==
                state.scene.world.world.ai.accounting.funds(),
        "Construction source directory retires real menus atomically without charge or world work");
    session.set_paused(false);
    const auto selected = session.select_build_menu(page, 28);
    const auto stale_page = session.select_build_menu(page, 30);
    const auto stale_placement =
        session.confirm_build(30, {0, 0}, rules::FacilityOrientation::first);
    const auto rejected = session.confirm_build(28, {-1, 0}, rules::FacilityOrientation::first);
    const auto anchor = one_cell_site(state);
    const auto placed = session.confirm_build(28, anchor, rules::FacilityOrientation::first);
    const auto duplicate = session.confirm_build(28, anchor, rules::FacilityOrientation::first);
    const auto final = input_frame(session, session.set_paused(true));
    check(!final->failed && final->state->build_definition == 28 &&
              input_result(*final, selected).outcome == Outcome::applied &&
              input_result(*final, stale_page).outcome == Outcome::rejected &&
              input_result(*final, stale_placement).outcome == Outcome::rejected &&
              input_result(*final, rejected).build_denial == sim::StartupBuildDenial::outside_map &&
              input_result(*final, duplicate).build_denial == sim::StartupBuildDenial::occupied,
          "Page identity and expected definition reject old clicks; source geometry denials remain "
          "recoverable");
    auto reference = state;
    reference.scene.framework_paused = false;
    check(sim::begin_startup_world_build(reference, 28).error ==
              sim::StartupWorldRuntimeError::none,
          "Source placement reference enters the same initial definition");
    const auto expected =
        sim::confirm_startup_world_build(reference, anchor, rules::FacilityOrientation::first);
    const auto created = input_result(*final, placed).created;
    check(expected.created && created == expected.created &&
              final->state->scene.world.facility_order == reference.scene.world.facility_order &&
              final->state->scene.world.world.ai.accounting.funds() ==
                  reference.scene.world.world.ai.accounting.funds() &&
              final->state->monthly_cash == reference.monthly_cash &&
              final->state->scene.random.draws() == state.scene.random.draws() &&
              final->state->scene.calendar.units == state.scene.calendar.units &&
              final->state->scene.world.world.map.cells.at(anchor.y * 24 + anchor.x)
                      .facility->instance_id.value == *created,
          "Successful build returns canonical stable identity, map and one shared ledger charge");
    session.set_paused(false);
    const auto wrong_cancel = session.cancel_build(30);
    const auto cancel = session.cancel_build(28);
    const auto closed = input_frame(session, session.set_paused(true));
    check(!closed->failed && !closed->state->build_definition &&
              closed->state->scene.scene_state == 0 &&
              closed->state->scene.world.world.facilities.count(*created) &&
              input_result(*closed, wrong_cancel).outcome == Outcome::rejected &&
              input_result(*closed, cancel).outcome == Outcome::applied,
          "Cancel binds selection, preserves the completed transaction and restores main mode");
    session.set_paused(false);
    const auto construction_click = session.open_facility(*created);
    const auto construction = input_frame(session, session.set_paused(true));
    check(
        !construction->failed &&
            construction->state->scene.world.world.facilities.at(*created).status == 0 &&
            input_result(*construction, construction_click).runtime_error ==
                sim::StartupWorldRuntimeError::invalid_page &&
            input_result(*construction, construction_click).outcome == Outcome::rejected &&
            task_top(*construction->state).kind == rules::WorldScriptPageKind::scene &&
            construction->state->scene.world.world.ai.accounting.funds() ==
                closed->state->scene.world.world.ai.accounting.funds(),
        "Construction click is an ineligible source input, not missing source or a world failure");
    session.stop();

    auto broken = initial();
    check(sim::begin_startup_world_build(broken, 28).error == sim::StartupWorldRuntimeError::none,
          "Failure fixture retains real source construction mode");
    broken.base_variants.pop_back();
    app::WorldSession failing(broken);
    const auto failure =
        input_frame(failing, failing.confirm_build(28, anchor, rules::FacilityOrientation::first));
    check(failure->failed &&
              failure->state->next_facility_identity == broken.next_facility_identity &&
              failure->state->scene.world.facility_order == broken.scene.world.facility_order &&
              failure->state->scene.world.world.ai.accounting.funds() ==
                  broken.scene.world.world.ai.accounting.funds() &&
              input_result(*failure, failure->last_command_serial).runtime_error ==
                  sim::StartupWorldRuntimeError::missing_source,
          "Late source refresh failure leaves canonical IDs, map and ledger uncommitted");
    failing.stop();
}
void facility_and_rank_commands() {
    using Action = sim::StartupFacilityPageAction;
    using Outcome = app::WorldCommandOutcome;
    const auto state = initial();
    const auto facility = state.scene.world.facility_order.front();
    app::WorldSession session(state);
    const auto opened = session.open_facility(facility);
    const auto detail = input_frame(session, session.set_paused(true));
    const auto page = task_top(*detail->state).id;
    check(!detail->failed && task_top(*detail->state).legacy_page == 74 &&
              detail->state->facility_page_bindings.at(page) == facility &&
              input_result(*detail, opened).outcome == Outcome::applied,
          "Clicking a stable current facility opens its real bound source detail page");
    session.set_paused(false);
    const auto next = session.act_facility(page, Action::next);
    const auto back = session.act_facility(page, Action::cancel);
    const auto stale = session.act_facility(page, Action::cancel);
    const auto vanished = session.open_facility(999999);
    const auto result = input_frame(session, session.set_paused(true));
    check(!result->failed && input_result(*result, next).outcome == Outcome::applied &&
              input_result(*result, back).outcome == Outcome::applied &&
              input_result(*result, stale).outcome == Outcome::rejected &&
              input_result(*result, vanished).outcome == Outcome::rejected &&
              result->state->scene.world.world.ai.accounting.funds() ==
                  state.scene.world.world.ai.accounting.funds(),
          "Detail navigation and Back use one FIFO; stale page/disappeared facility never fail or "
          "charge");
    // Exercise the common bounded acknowledgement store using recoverable stale inputs,
    // not a second event queue or a synthetic runtime tick.
    for (int n = 0; n < 70; ++n)
        session.act_residence(page, 1);
    const auto latest = session.act_rank(page, 0);
    const auto bounded = input_frame(session, latest);
    check(!bounded->failed && bounded->command_results.size() == 64 &&
              bounded->command_results.back().serial == latest &&
              bounded->command_results.front().serial == latest - 63 &&
              input_result(*bounded, latest).outcome == Outcome::rejected,
          "Residence/rank stale decisions share a bounded ordered 64-result history");
    session.stop();
    auto rank = initial();
    rank.scripts.pages.front().lifecycle = 3;
    rules::WorldScriptPage promotion;
    promotion.id = rank.scripts.next_page_id++;
    promotion.kind = rules::WorldScriptPageKind::raw_page;
    promotion.legacy_page = 48;
    rank.scripts.pages.push_back(promotion);
    auto expected = rank;
    check(sim::act_startup_world_runtime_rank_page(expected, promotion.id, 1) ==
              sim::StartupWorldRuntimeError::none,
          "Source promotion fixture accepts explicit condition explanation");
    app::WorldSession promoting(rank);
    const auto action = promoting.act_rank(promotion.id, 1);
    const auto shown = input_frame(promoting, promoting.set_paused(true));
    check(!shown->failed && input_result(*shown, action).outcome == Outcome::applied &&
              shown->state->scripts.event_calls == expected.scripts.event_calls &&
              shown->state->rank == expected.rank &&
              shown->state->scene.random.draws() == expected.scene.random.draws(),
          "Rank selection forwards exact source choice without granting an unearned promotion");
    promoting.stop();

    auto missing = state;
    missing.neighbourhood_details.erase(facility);
    app::WorldSession broken(missing);
    const auto failed = input_frame(broken, broken.open_facility(facility));
    check(failed->failed &&
              input_result(*failed, failed->last_command_serial).runtime_error ==
                  sim::StartupWorldRuntimeError::missing_source &&
              failed->state->scene.world.world.facilities.at(facility).status != 0,
          "Eligible live facility with missing source data still fails explicitly; guard is not "
          "error swallowing");
    broken.stop();
}

void residence_replacement_command() {
    auto state = initial();
    const auto anchor = one_cell_site(state);
    check(sim::begin_startup_world_build(state, 24).error == sim::StartupWorldRuntimeError::none,
          "Residence transport fixture selects actual recruitment facility");
    const auto built =
        sim::confirm_startup_world_build(state, anchor, rules::FacilityOrientation::first);
    check(built.created &&
              sim::cancel_startup_world_build(state) == sim::StartupWorldRuntimeError::none,
          "Residence fixture installs source recruitment site");
    const auto adapter = sim::startup_world_runtime_adapter();
    const auto completion = rules::prepare_world_facility_update(adapter.facilities.read(state),
                                                                 *built.created, adapter.catalog);
    check(completion.candidate && adapter.facilities.write(state, completion.candidate->state),
          "Source one-tick construction creates a usable recruitment site");
    state.shop_humans.at(1).satisfaction = state.rules->humans.at(1).residence_threshold;
    for (auto &page : state.scripts.pages)
        if (page.kind != rules::WorldScriptPageKind::scene)
            page.lifecycle = 4;
    check(sim::open_startup_world_facility_page(state, *built.created) ==
              sim::StartupWorldRuntimeError::none,
          "Residence fixture opens source recruitment details");
    check(sim::act_startup_world_facility_page(state, task_top(state).id,
                                               sim::StartupFacilityPageAction::confirm) ==
                  sim::StartupWorldRuntimeError::none &&
              task_top(state).legacy_page == 80,
          "Source residence candidate list is bound to the recruitment instance");
    const auto page = task_top(state).id;
    auto expected = state;
    const auto replacement = sim::act_startup_world_residence_page(expected, page, 1);
    check(replacement.created.has_value(), "Source residence oracle replaces the recruitment site");
    app::WorldSession session(state);
    const auto chosen = session.act_residence(page, 1);
    const auto stale = session.act_residence(page, 1);
    const auto done = input_frame(session, session.set_paused(true));
    check(
        !done->failed && input_result(*done, chosen).created == replacement.created &&
            input_result(*done, stale).outcome == app::WorldCommandOutcome::rejected &&
            done->state->scene.world.facility_order == expected.scene.world.facility_order &&
            done->state->human_homes == expected.human_homes &&
            done->state->facility_residents == expected.facility_residents &&
            done->state->scene.world.world.ai.accounting.funds() ==
                expected.scene.world.world.ai.accounting.funds() &&
            done->state->scene.random.draws() == state.scene.random.draws(),
        "Residence selection replaces one actual map instance, binds human and pays exactly once");
    session.set_paused(false);
    const auto old_snapshot_click = session.open_facility(*built.created);
    const auto rejected = input_frame(session, session.set_paused(true));
    check(
        !rejected->failed &&
            input_result(*rejected, old_snapshot_click).runtime_error ==
                sim::StartupWorldRuntimeError::invalid_page &&
            input_result(*rejected, old_snapshot_click).outcome ==
                app::WorldCommandOutcome::rejected &&
            !rejected->state->scene.world.world.facilities.count(*built.created) &&
            rejected->state->scene.world.world.facilities.count(*replacement.created) &&
            rejected->state->scene.world.world.ai.accounting.funds() ==
                done->state->scene.world.world.ai.accounting.funds(),
        "A stale render click on the replaced recruitment identity cannot retarget its new house");
    session.stop();
}

} // namespace ark::test::world_session
