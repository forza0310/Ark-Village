// Management transport shares the real page stack, worker gates and private Owner commits.
// Frozen suites own activity/trade arithmetic; these cases cover UI identities and FIFO wiring.
#include "ark/app/world_save.hpp"
#include "ark/simulation/startup_world_editing.hpp"
#include "world_session_test_support.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ark::test::world_session {
namespace {
using Action = sim::StartupVillageActivityAction;
using Outcome = app::WorldCommandOutcome;
app::WorldState village_fixture() {
    auto state = initial();
    // Explicit management callsite fixture: published activity16 costs40 points, while25
    // has an unsupported activity consumer. This is not a natural unlock/window claim.
    state.scripts.event_calls[100] = 1;
    for (auto &entry : state.scripts.activities)
        entry.second.status = 0;
    state.scripts.activities.at(16).status = 1;
    state.scripts.activities.at(25).status = 1;
    state.village_points = 100;
    state.quarter_counter = 3;
    return state;
}
std::shared_ptr<const app::WorldFrame> ready(app::WorldSession &session, int raw) {
    return await(session, [raw](const auto &frame) {
        return frame.failed ||
               (task_top(*frame.state).legacy_page == raw &&
                frame.state->activity_pages_initialized.count(task_top(*frame.state).id) &&
                !frame.state->activity_page_answers.count(task_top(*frame.state).id));
    });
}
app::WorldState commerce_fixture(int choice = 0) {
    auto state = initial();
    state.scripts.user_flags |= 16U;
    state.scripts.event_calls[97] = 1;
    // Actual potato price400 and starting inventory2; only replenishment is a fixture.
    state.shop_item_stock.at(0).quantity = 2;
    check(sim::open_startup_world_commerce(state) == sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_commerce_pages(state),
          "Commerce fixture opens and initializes the real83");
    const auto menu = task_top(state).id;
    check(sim::act_startup_world_commerce_page(state, menu, sim::StartupCommerceAction::select,
                                               choice) == sim::StartupWorldRuntimeError::none &&
              sim::act_startup_world_commerce_page(state, menu,
                                                   sim::StartupCommerceAction::confirm) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_commerce_pages(state),
          "Commerce fixture follows83 into a real initialized purchase directory");
    return state;
}
std::uint64_t bun_shop(const app::WorldState &state) {
    const auto &facilities = state.scene.world.world.facilities;
    const auto found = std::find_if(facilities.begin(), facilities.end(), [](const auto &entry) {
        return entry.second.placement.definition_id == 33;
    });
    check(found != facilities.end(), "Actual initial bun-shop instance supplies item target");
    return found->first;
}
app::WorldState facility_items_fixture(bool initialize = true) {
    auto state = initial();
    for (auto &[id, value] : state.items) {
        value.inventory = id == 1 ? 2 : 0;
        state.catalog.at({0, id}) = value;
    }
    check(sim::open_startup_world_facility_page(state, bun_shop(state)) ==
              sim::StartupWorldRuntimeError::none,
          "Facility item transport opens actual bound74");
    check(sim::act_startup_world_facility_page(state, task_top(state).id,
                                               sim::StartupFacilityPageAction::confirm) ==
                  sim::StartupWorldRuntimeError::none &&
              task_top(state).legacy_page == 75,
          "Actual74 confirmation opens75 without spending inventory");
    if (initialize)
        check(sim::initialize_startup_world_facility_item_pages(state),
              "Source initializes actual75 catalogue");
    return state;
}
std::array<rules::Position, 2> road_sites(const app::WorldState &state) {
    const auto &map = state.scene.world.world.map;
    const auto &fence = state.rules->fences.at(state.fence_level);
    for (int y = fence[1].y + 1; y < fence[0].y; ++y)
        for (int x = fence[0].x + 1; x + 2 < fence[1].x; ++x) {
            bool vacant = true;
            for (int dx = 0; dx < 3; ++dx) {
                const auto &cell = map.cells.at(y * map.width + x + dx);
                vacant = vacant && cell.legacy_state == 4 && !cell.facility;
            }
            if (vacant)
                return {{{x, y}, {x + 2, y}}};
        }
    throw std::runtime_error("Editing transport fixture lacks three actual vacant road cells");
}
rules::Position move_site(const app::WorldState &state, std::uint64_t id) {
    const auto &map = state.scene.world.world.map;
    const auto &fence = state.rules->fences.at(state.fence_level);
    const auto &placement = state.scene.world.world.facilities.at(id).placement;
    for (int y = fence[1].y + 1; y < fence[0].y; ++y)
        for (int x = fence[0].x + 1; x < fence[1].x; ++x) {
            const auto footprint = rules::facility_footprint(placement.shape, placement.orientation,
                                                             {x, y}, map.width, map.height);
            if (footprint.error == rules::GeometryError::none &&
                std::all_of(footprint.cells.begin(), footprint.cells.end(), [&](const auto &cell) {
                    const auto p = cell.position;
                    const auto &tile = map.cells.at(p.y * map.width + p.x);
                    return p.x > fence[0].x && p.x < fence[1].x && p.y > fence[1].y &&
                           p.y < fence[0].y && tile.legacy_state == 4 && !tile.facility;
                }))
                return {x, y};
        }
    throw std::runtime_error("Editing transport fixture lacks an actual vacant full footprint");
}
void edit_source_clock(const app::WorldFrame &actual, app::WorldState expected) {
    expected.scene.framework_paused = false;
    for (std::uint64_t n = 0; n < actual.outer_updates; ++n)
        expected = advance(std::move(expected));
    expected.scene.framework_paused = true;
    same_world(*actual.state, expected);
    check(actual.state->monthly_cash == expected.monthly_cash &&
              actual.state->build_mode == expected.build_mode &&
              actual.state->build_anchor == expected.build_anchor &&
              actual.state->build_moving_facility == expected.build_moving_facility,
          "Edit transport preserves source mode, anchor, calendar, random and ledger order");
}
void edit_accepted(const sim::StartupBuildResult &result, const char *label) {
    check(result.error == sim::StartupWorldRuntimeError::none &&
              result.denial == sim::StartupBuildDenial::none,
          label);
}
} // namespace
void editing_command_transactions() {
    const auto orientation = rules::FacilityOrientation::first;
    const auto sites = road_sites(initial());
    // Published initial state has no moving entitlement. This is a gate/transport case,
    // not a natural first-star progression or window unlock claim.
    auto locked = initial();
    check((locked.scripts.user_flags & 32U) == 0,
          "Source new world has not naturally received the moving reward");
    check(sim::open_startup_world_build_menu(locked) == sim::StartupWorldRuntimeError::none,
          "Editing gate fixture opens actual21");
    const auto locked_page = task_top(locked).id;
    app::WorldSession locked_session(locked);
    const auto denied = locked_session.select_build_menu(locked_page, -2);
    const auto refused = input_frame(locked_session, locked_session.set_paused(true));
    check(!refused->failed && task_top(*refused->state).id == locked_page &&
              input_result(*refused, denied).outcome == Outcome::rejected &&
              input_result(*refused, denied).build_denial == sim::StartupBuildDenial::unavailable,
          "Moving entitlement refusal keeps21 and is a usable recoverable transport result");
    edit_source_clock(*refused, locked);
    locked_session.stop();

    auto directory = initial();
    check(sim::open_startup_world_build_menu(directory) == sim::StartupWorldRuntimeError::none,
          "Road transport opens the actual source construction page");
    const auto page = task_top(directory).id;
    app::WorldSession road_menu(directory);
    const auto selected = road_menu.select_build_menu(page, 18);
    const auto stale_page = road_menu.select_build_menu(page, -1);
    const auto road = input_frame(road_menu, road_menu.set_paused(true));
    check(!road->failed && road->state->build_definition == 18 && road->state->build_mode == 1 &&
              road->state->scene.scene_state == 1 &&
              input_result(*road, selected).outcome == Outcome::applied &&
              input_result(*road, stale_page).outcome == Outcome::rejected,
          "Road18 directory action enters mode1 once; a retired21 click cannot change the mode");
    auto reference = directory;
    check(sim::cancel_startup_world_build_menu(reference, page) ==
              sim::StartupWorldRuntimeError::none,
          "Road source oracle retires its real21");
    edit_accepted(sim::begin_startup_world_road(reference, 18), "Source road oracle enters mode1");
    edit_source_clock(*road, reference);
    road_menu.stop();

    auto start_state = *road->state;
    start_state.scene.framework_paused = false;
    app::WorldSession starting(start_state);
    const auto start = starting.confirm_edit(start_state, sites[0], orientation);
    const auto stale_start = starting.confirm_edit(start_state, sites[1], orientation);
    const auto endpoint = input_frame(starting, starting.set_paused(true));
    check(!endpoint->failed && endpoint->state->build_mode == 2 &&
              endpoint->state->build_anchor == sites[0] &&
              input_result(*endpoint, start).outcome == Outcome::applied &&
              input_result(*endpoint, stale_start).outcome == Outcome::rejected,
          "Road start binds mode/anchor; repeated mode1 input cannot become a mode2 commit");
    reference = start_state;
    edit_accepted(sim::confirm_startup_world_edit(reference, sites[0], orientation),
                  "Source oracle selects real road start");
    edit_source_clock(*endpoint, reference);
    starting.stop();

    auto end_state = *endpoint->state;
    end_state.scene.framework_paused = false;
    app::WorldSession cancelling(end_state);
    const auto back = cancelling.cancel_edit(end_state);
    const auto stale_back = cancelling.cancel_edit(end_state);
    const auto returned = input_frame(cancelling, cancelling.set_paused(true));
    check(!returned->failed && returned->state->build_mode == 1 && !returned->state->build_anchor &&
              input_result(*returned, back).outcome == Outcome::applied &&
              input_result(*returned, stale_back).outcome == Outcome::rejected &&
              returned->state->scene.world.world.ai.accounting.funds() ==
                  end_state.scene.world.world.ai.accounting.funds(),
          "Cancel2 returns to1 without payment; stale stage2 Back cannot leave placement");
    reference = end_state;
    check(sim::cancel_startup_world_edit(reference) == sim::StartupWorldRuntimeError::none,
          "Source oracle cancels road endpoint");
    edit_source_clock(*returned, reference);
    cancelling.stop();
    auto first_stage = *returned->state;
    first_stage.scene.framework_paused = false;
    app::WorldSession closing(first_stage);
    const auto exit = closing.cancel_edit(first_stage);
    const auto main = input_frame(closing, closing.set_paused(true));
    check(!main->failed && main->state->scene.scene_state == 0 && !main->state->build_definition &&
              input_result(*main, exit).outcome == Outcome::applied,
          "Cancel1 restores actual main scene without undoing or charging a preview");
    reference = first_stage;
    check(sim::cancel_startup_world_edit(reference) == sim::StartupWorldRuntimeError::none,
          "Source oracle exits continuous road placement");
    edit_source_clock(*main, reference);
    closing.stop();

    app::WorldSession building(end_state);
    auto stale_anchor = end_state;
    stale_anchor.build_anchor = sites[1]; // A stale render observation, not a mutable Owner edit.
    const auto wrong_anchor = building.confirm_edit(stale_anchor, sites[1], orientation);
    const auto bad_orientation =
        building.confirm_edit(end_state, sites[1], static_cast<rules::FacilityOrientation>(999));
    const auto placed = building.confirm_edit(end_state, sites[1], orientation);
    const auto duplicate = building.confirm_edit(end_state, sites[1], orientation);
    const auto built = input_frame(building, building.set_paused(true));
    const auto quote = sim::startup_world_build_quote(end_state, 18);
    check(quote.has_value() && !built->failed &&
              input_result(*built, wrong_anchor).outcome == Outcome::rejected &&
              input_result(*built, bad_orientation).outcome == Outcome::rejected &&
              input_result(*built, placed).outcome == Outcome::applied &&
              input_result(*built, duplicate).outcome == Outcome::rejected &&
              built->state->scene.world.world.ai.accounting.funds() ==
                  end_state.scene.world.world.ai.accounting.funds() - 3 * quote->construction_cost,
          "Bound road endpoint commits three input cells once; stale anchor/stage do not charge");
    for (int x = sites[0].x; x <= sites[1].x; ++x) {
        const auto n = sites[0].y * built->state->scene.world.world.map.width + x;
        check(built->state->scene.world.world.map.cells.at(n).legacy_state == 3 &&
                  built->state->surface.at(n).definition == 18,
              "Paid FIFO road cells use the actual road definition in the sole map");
    }
    reference = end_state;
    edit_accepted(sim::confirm_startup_world_edit(reference, sites[1], orientation),
                  "Source oracle commits actual three-cell road");
    edit_source_clock(*built, reference);
    building.stop();

    auto removal = *built->state;
    removal.scene.framework_paused = false;
    check(sim::cancel_startup_world_edit(removal) == sim::StartupWorldRuntimeError::none &&
              sim::open_startup_world_build_menu(removal) == sim::StartupWorldRuntimeError::none,
          "Removal fixture leaves road and opens real21");
    app::WorldSession remove_menu(removal);
    const auto select_remove = remove_menu.select_build_menu(task_top(removal).id, -1);
    const auto remove_mode = input_frame(remove_menu, remove_menu.set_paused(true));
    check(!remove_mode->failed && remove_mode->state->build_mode == 3 &&
              input_result(*remove_mode, select_remove).outcome == Outcome::applied,
          "Directory -1 uses the same Owner to enter actual removal mode3");
    remove_menu.stop();
    removal = *remove_mode->state;
    removal.scene.framework_paused = false;
    edit_accepted(sim::confirm_startup_world_edit(removal, sites[0], orientation),
                  "Removal fixture selects actual paid road start");
    app::WorldSession removing(removal);
    const auto remove = removing.confirm_edit(removal, sites[1], orientation);
    const auto removed = input_frame(removing, removing.set_paused(true));
    check(!removed->failed && removed->state->build_mode == 3 &&
              input_result(*removed, remove).outcome == Outcome::applied &&
              removed->state->scene.world.world.ai.accounting.funds() ==
                  removal.scene.world.world.ai.accounting.funds(),
          "Road segment removal commits without refunding its previously paid quote");
    for (int x = sites[0].x; x <= sites[1].x; ++x) {
        const auto n = sites[0].y * removed->state->scene.world.world.map.width + x;
        check(removed->state->scene.world.world.map.cells.at(n).legacy_state == 4 &&
                  removed->state->surface.at(n).definition == removed->state->ground_definition,
              "Road removal restores all actual paid cells in the sole map");
    }
    reference = removal;
    edit_accepted(sim::confirm_startup_world_edit(reference, sites[1], orientation),
                  "Source oracle removes the same actual road segment");
    edit_source_clock(*removed, reference);
    removing.stop();

    auto moving = initial();
    // Explicit entitlement callsite fixture only. Natural unlocks belong to progression95.
    moving.scripts.user_flags |= 32U;
    const auto old_id = bun_shop(moving);
    const auto old_placement = moving.scene.world.world.facilities.at(old_id).placement;
    const auto target = move_site(moving, old_id);
    check(sim::open_startup_world_build_menu(moving) == sim::StartupWorldRuntimeError::none,
          "Entitled movement fixture opens actual21");
    app::WorldSession move_menu(moving);
    const auto enter_move = move_menu.select_build_menu(task_top(moving).id, -2);
    const auto selecting = input_frame(move_menu, move_menu.set_paused(true));
    check(!selecting->failed && selecting->state->build_mode == 6 &&
              input_result(*selecting, enter_move).outcome == Outcome::applied,
          "Directory -2 admits the explicit entitlement fixture into actual mode6");
    move_menu.stop();
    moving = *selecting->state;
    moving.scene.framework_paused = false;
    app::WorldSession selecting_old(moving);
    const auto chosen =
        selecting_old.confirm_edit(moving, old_placement.anchor, old_placement.orientation);
    const auto stale_selection =
        selecting_old.confirm_edit(moving, old_placement.anchor, old_placement.orientation);
    const auto chosen_frame = input_frame(selecting_old, selecting_old.set_paused(true));
    check(!chosen_frame->failed && chosen_frame->state->build_mode == 7 &&
              chosen_frame->state->build_moving_facility == old_id &&
              chosen_frame->state->build_anchor == old_placement.anchor &&
              input_result(*chosen_frame, chosen).outcome == Outcome::applied &&
              input_result(*chosen_frame, stale_selection).outcome == Outcome::rejected,
          "Movement target binds the actual old stable ID and original anchor");
    reference = moving;
    edit_accepted(
        sim::confirm_startup_world_edit(reference, old_placement.anchor, old_placement.orientation),
        "Source oracle selects actual initial facility to move");
    edit_source_clock(*chosen_frame, reference);
    selecting_old.stop();
    moving = *chosen_frame->state;
    moving.scene.framework_paused = false;
    app::WorldSession overlap(moving);
    const auto overlap_click =
        overlap.confirm_edit(moving, old_placement.anchor, old_placement.orientation);
    const auto cancel_move = overlap.cancel_edit(moving);
    const auto cancelled = input_frame(overlap, overlap.set_paused(true));
    check(!cancelled->failed && cancelled->state->build_mode == 6 &&
              !cancelled->state->build_moving_facility &&
              cancelled->state->scene.world.world.facilities.count(old_id) &&
              cancelled->state->scene.world.world.ai.accounting.funds() ==
                  moving.scene.world.world.ai.accounting.funds() &&
              input_result(*cancelled, overlap_click).build_denial ==
                  sim::StartupBuildDenial::occupied &&
              input_result(*cancelled, cancel_move).outcome == Outcome::applied,
          "Overlapping movement is refused; Cancel7 preserves old facility and cash in mode6");
    reference = moving;
    check(sim::cancel_startup_world_edit(reference) == sim::StartupWorldRuntimeError::none,
          "Source oracle cancels selected move without retiring old instance");
    edit_source_clock(*cancelled, reference);
    overlap.stop();

    app::WorldSession mover(moving);
    auto stale_id = moving;
    stale_id.build_moving_facility = old_id + 1000;
    const auto wrong_id = mover.confirm_edit(stale_id, target, old_placement.orientation);
    const auto committed_move = mover.confirm_edit(moving, target, old_placement.orientation);
    const auto stale_move = mover.confirm_edit(moving, target, old_placement.orientation);
    const auto moved = input_frame(mover, mover.set_paused(true));
    const auto new_id = input_result(*moved, committed_move).created;
    check(!moved->failed && new_id && *new_id != old_id &&
              input_result(*moved, wrong_id).outcome == Outcome::rejected &&
              input_result(*moved, stale_move).outcome == Outcome::rejected &&
              !moved->state->scene.world.world.facilities.count(old_id) &&
              moved->state->scene.world.world.facilities.at(*new_id).placement.anchor == target &&
              moved->state->facility_original_ids.at(*new_id) ==
                  moving.facility_original_ids.at(old_id) &&
              moved->state->facility_ordinals.at(*new_id) == moving.facility_ordinals.at(old_id) &&
              moved->state->scene.world.world.ai.accounting.funds() ==
                  moving.scene.world.world.ai.accounting.funds() - 300,
          "Movement charges300 once, replaces stable ID and preserves raw identity/ordinal");
    const auto &moved_map = moved->state->scene.world.world.map;
    const auto old_cell = old_placement.anchor.y * moved_map.width + old_placement.anchor.x;
    const auto new_cell = target.y * moved_map.width + target.x;
    check(!moved_map.cells.at(old_cell).facility && moved_map.cells.at(new_cell).facility &&
              moved_map.cells.at(new_cell).facility->instance_id.value == *new_id &&
              std::none_of(moved_map.cells.begin(), moved_map.cells.end(),
                           [&](const auto &cell) {
                               return cell.facility && cell.facility->instance_id.value == old_id;
                           }),
          "Successful movement frees the old anchor and binds the target to the new sole instance");
    reference = moving;
    const auto source_move =
        sim::confirm_startup_world_edit(reference, target, old_placement.orientation);
    edit_accepted(source_move, "Source oracle moves the same actual facility");
    check(source_move.created == new_id, "Worker returns the sole source-created replacement ID");
    edit_source_clock(*moved, reference);
    mover.stop();

    auto demolition = *moved->state;
    demolition.scene.framework_paused = false;
    check(sim::cancel_startup_world_edit(demolition) == sim::StartupWorldRuntimeError::none,
          "Demolition fixture exits movement");
    edit_accepted(sim::begin_startup_world_edit(demolition, false), "Source enters demolition");
    app::WorldSession demolisher(demolition);
    const auto demolished_click =
        demolisher.confirm_edit(demolition, target, old_placement.orientation);
    const auto demolished = input_frame(demolisher, demolisher.set_paused(true));
    check(!demolished->failed &&
              input_result(*demolished, demolished_click).outcome == Outcome::applied &&
              !demolished->state->scene.world.world.facilities.count(*new_id) &&
              demolished->state->scene.world.world.ai.accounting.funds() ==
                  demolition.scene.world.world.ai.accounting.funds() &&
              std::none_of(demolished->state->scene.world.world.map.cells.begin(),
                           demolished->state->scene.world.world.map.cells.end(),
                           [&](const auto &cell) {
                               return cell.facility &&
                                      (cell.facility->instance_id.value == old_id ||
                                       cell.facility->instance_id.value == *new_id);
                           }),
          "Demolition refunds nothing and leaves neither old nor replacement map binding");
    reference = demolition;
    edit_accepted(sim::confirm_startup_world_edit(reference, target, old_placement.orientation),
                  "Source oracle demolishes the actual moved facility");
    edit_source_clock(*demolished, reference);
    demolisher.stop();
    auto stable = *demolished->state;
    stable.scene.framework_paused = false;
    check(sim::cancel_startup_world_edit(stable) == sim::StartupWorldRuntimeError::none,
          "Post-edit durable roundtrip returns to stable source main scene");
    // Closed raw21 pages are retired by the normal outer consumer, not by edit cancel.
    for (int n = 0; n < 20 && !app::world_save_eligible(stable); ++n)
        stable = advance(std::move(stable));
    const auto captured = app::capture_world_save(stable);
    if (!captured.image)
        throw std::runtime_error("Edited Owner capture rejected: " + captured.message);
    check(captured.image.has_value(), "Stable edited Owner remains eligible for durable capture");
    const auto decoded = app::decode_world_save(captured.image->bytes);
    check(decoded.state &&
              decoded.state->scene.world.world.facilities.size() ==
                  stable.scene.world.world.facilities.size() &&
              !decoded.state->scene.world.world.facilities.count(old_id) &&
              !decoded.state->scene.world.world.facilities.count(*new_id) &&
              decoded.state->scene.world.world.ai.accounting.funds() ==
                  stable.scene.world.world.ai.accounting.funds() &&
              decoded.state->facility_original_ids == stable.facility_original_ids &&
              decoded.state->facility_ordinals == stable.facility_ordinals,
          "Durable edited snapshot retains retired IDs, original mirrors and actual shared funds");
}
void village_command_transactions() {
    auto state = village_fixture();
    state.scene.framework_paused = true;
    app::WorldSession menu(state);
    input_frame(menu, menu.open_main_menu());
    const auto denied = input_frame(menu, menu.open_menu_village_activities());
    check(!denied->failed && denied->main_menu_open &&
              input_result(*denied, denied->last_command_serial).outcome == Outcome::rejected,
          "Paused village menu preserves the overlay and explicit pause on source refusal");
    menu.set_paused(false);
    const auto opened = menu.open_menu_village_activities();
    const auto listing = input_frame(menu, menu.set_paused(true));
    check(!listing->failed && !listing->main_menu_open &&
              task_top(*listing->state).legacy_page == 51 &&
              input_result(*listing, opened).outcome == Outcome::applied &&
              listing->state->village_points == 100,
          "Village menu and real51 page transition commit together without charging points");
    menu.stop();

    state = village_fixture();
    check(sim::open_startup_world_village_activities(state) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_village_activity_pages(state),
          "Management transport fixture uses the real source page initializer");
    const auto page = task_top(state).id;
    check(state.activity_page_lists.at(page) == std::vector<int>{16, 25},
          "Published source identities provide supported and unsupported directory entries");
    state.scene.framework_paused = true;
    app::WorldSession session(state);
    const auto blocked = input_frame(session, session.act_village_activity(page, Action::confirm));
    check(!blocked->failed &&
              input_result(*blocked, blocked->last_command_serial).outcome == Outcome::rejected,
          "Paused activity input is a recoverable refusal");
    session.set_paused(false);
    session.act_village_activity(page, Action::select, 1);
    const auto unsupported = session.act_village_activity(page, Action::confirm);
    const auto malformed = session.act_village_activity(page, static_cast<Action>(999));
    session.act_village_activity(page, Action::select, 0);
    const auto preview = session.act_village_activity(page, Action::confirm);
    const auto child = ready(session, 52);
    check(!child->failed && child->state->village_points == 100 &&
              input_result(*child, unsupported).outcome == Outcome::rejected &&
              input_result(*child, malformed).outcome == Outcome::rejected &&
              input_result(*child, preview).outcome == Outcome::applied,
          "Unsupported/stale enum inputs cannot fail the world;52 preview does not pay");
    const auto child_page = task_top(*child->state).id;
    session.act_village_activity(child_page, Action::cancel);
    const auto parent = ready(session, 51);
    check(!parent->failed && parent->state->village_points == 100 &&
              parent->state->events_held == state.events_held,
          "52 cancellation resumes the parent consumer without points or held-count changes");
    session.act_village_activity(page, Action::confirm);
    const auto starting = ready(session, 52);
    const auto start_page = task_top(*starting->state).id;
    const auto held = session.act_village_activity(start_page, Action::confirm);
    const auto duplicate = session.act_village_activity(start_page, Action::confirm);
    const auto paid = input_frame(session, session.set_paused(true));
    check(!paid->failed && task_top(*paid->state).legacy_page == 53 &&
              input_result(*paid, held).outcome == Outcome::applied &&
              input_result(*paid, duplicate).outcome == Outcome::rejected &&
              paid->state->village_points == 60 && paid->state->activity_counts.at(16) == 1 &&
              paid->state->events_held == state.events_held + 1 &&
              paid->state->quarter_counter == 3 &&
              paid->state->scene.world.world.ai.accounting.funds() ==
                  state.scene.world.world.ai.accounting.funds(),
          "52 start pays points once; stale input cannot repay or consume53's later effects");
    session.stop();
}
void commerce_command_transactions() {
    using A = sim::StartupCommerceAction;
    // Both independent menu gates retain the overlay and never create a source page.
    for (const bool paused : {false, true}) {
        auto state = initial(paused);
        if (paused)
            state.scripts.user_flags |= 16U;
        else
            state.scripts.user_flags &= ~16U;
        app::WorldSession session(state);
        const auto overlay = input_frame(session, session.open_main_menu());
        const auto refusal = input_frame(session, session.open_menu_commerce());
        check(!refusal->failed && refusal->main_menu_open &&
                  input_result(*refusal, refusal->last_command_serial).outcome == Outcome::rejected,
              "Commerce flag16/pause refusal preserves the menu instead of opening83");
        same_world(*overlay->state, *refusal->state);
        session.stop();
    }
    {
        auto state = initial();
        state.scripts.user_flags |= 16U;
        state.scripts.event_calls[97] = 1;
        check(sim::open_startup_world_commerce(state) == sim::StartupWorldRuntimeError::none,
              "Uninitialized commerce case opens83 through the real source entry");
        const auto page = task_top(state).id;
        app::WorldSession session(state);
        const auto early = session.act_commerce(page, A::confirm);
        const auto stopped = input_frame(session, session.set_paused(true));
        check(!stopped->failed && input_result(*stopped, early).outcome == Outcome::rejected &&
                  task_top(*stopped->state).id == page &&
                  stopped->state->scene.random.draws() == state.scene.random.draws(),
              "Input cannot initialize83 or create84 before the framework admits the page");
        session.stop();
    }
    for (const int owned : {2, 999}) {
        auto state = commerce_fixture();
        const auto page = task_top(state).id;
        check(state.commerce_page_lists.at(page) == std::vector<int>{0} &&
                  state.rules->items.at(0).commerce_price == 400,
              "Receipt fixture binds actual item0, not a displayed row or inferred price");
        state.items.at(0).inventory = owned;
        state.catalog.at({0, 0}) = state.items.at(0);
        state.scene.framework_paused = true;
        app::WorldSession session(state);
        const auto blocked = input_frame(session, session.act_commerce(page, A::confirm));
        check(!blocked->failed &&
                  input_result(*blocked, blocked->last_command_serial).outcome ==
                      Outcome::rejected &&
                  !input_result(*blocked, blocked->last_command_serial).commerce_amount,
              "Paused purchase is recoverable and has no receipt");
        same_world(state, *blocked->state);
        session.set_paused(false);
        const auto invalid = session.act_commerce(page, static_cast<A>(999));
        const auto stale = session.act_commerce(page + 1000, A::confirm);
        const auto bought = session.act_commerce(page, A::confirm);
        const auto duplicate = session.act_commerce(page, A::confirm);
        const auto paid = input_frame(session, session.set_paused(true));
        check(
            !paid->failed && task_top(*paid->state).legacy_page == 86 &&
                task_top(*paid->state).legacy_s == 0 &&
                input_result(*paid, invalid).outcome == Outcome::rejected &&
                input_result(*paid, stale).outcome == Outcome::rejected &&
                input_result(*paid, duplicate).outcome == Outcome::rejected &&
                !input_result(*paid, duplicate).commerce_amount &&
                input_result(*paid, bought).page == page &&
                input_result(*paid, bought).commerce_amount == 400 &&
                paid->state->items.at(0).inventory == (owned == 999 ? 999 : 3) &&
                paid->state->shop_item_stock.at(0).quantity == 1 &&
                paid->state->scene.world.world.ai.accounting.funds() ==
                    state.scene.world.world.ai.accounting.funds() - 400 &&
                paid->state->scene.random.draws() == state.scene.random.draws(),
            "FIFO payment/receipt commits once, including saturated999 inventory with stock loss");
        auto feedback_state = *paid->state;
        session.stop();
        feedback_state.scene.framework_paused = false;
        check(sim::initialize_startup_world_commerce_pages(feedback_state),
              "Actual86 initializes without inventing a player confirmation");
        const auto receipt = task_top(feedback_state).id;
        app::WorldSession feedback(feedback_state);
        const auto ack = feedback.ack_page(receipt);
        const auto confirm = feedback.act_commerce(receipt, A::confirm);
        const auto resumed = await(feedback, [page, confirm](const auto &frame) {
            return frame.failed ||
                   (frame.last_command_serial >= confirm && task_top(*frame.state).id == page);
        });
        check(!resumed->failed && input_result(*resumed, ack).outcome == Outcome::rejected &&
                  input_result(*resumed, confirm).outcome == Outcome::rejected &&
                  resumed->state->shop_item_stock.at(0).quantity == 1 &&
                  resumed->state->scene.world.world.ai.accounting.funds() ==
                      paid->state->scene.world.world.ai.accounting.funds(),
              "Real86 feedback closes through its automatic update, never another confirmation");
        feedback.stop();
    }
    // Late page insertion fails after candidate charging. The worker must retain the Owner.
    auto broken = commerce_fixture();
    broken.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
    app::WorldSession rollback(broken);
    const auto rejected =
        input_frame(rollback, rollback.act_commerce(task_top(broken).id, A::confirm));
    check(rejected->failed && rejected->state->items.at(0).inventory == 2 &&
              rejected->state->shop_item_stock.at(0).quantity == 2 &&
              !input_result(*rejected, rejected->last_command_serial).commerce_amount,
          "Late86 insertion failure publishes no inventory mutation or successful receipt");
    same_world(broken, *rejected->state);
    rollback.stop();

    auto state = commerce_fixture(2);
    const auto shop = task_top(state).id;
    const auto &list = state.commerce_page_lists.at(shop);
    const auto found = std::find(list.begin(), list.end(), 34);
    check(found != list.end() && state.rules->facility_initial.at(34).capacity == 30,
          "Facility redemption uses actual definition34 and30-point quote");
    state.village_points = 30;
    check(sim::act_startup_world_commerce_page(state, shop, A::select,
                                               static_cast<int>(found - list.begin())) ==
              sim::StartupWorldRuntimeError::none,
          "Source selection binds34 before transport payment");
    state.scene.framework_paused = true;
    app::WorldSession redemption(state);
    redemption.set_paused(false);
    const auto purchase = redemption.act_commerce(shop, A::confirm);
    auto frame = input_frame(redemption, redemption.set_paused(true));
    check(!frame->failed && input_result(*frame, purchase).outcome == Outcome::applied &&
              task_top(*frame->state).legacy_page == 93 && frame->state->village_points == 0 &&
              frame->state->facility_free_builds.at(34) == 0,
          "85 spends points while bound93 still owns the pending build entitlement");
    auto claim_state = *frame->state;
    redemption.stop();
    claim_state.scene.framework_paused = false;
    check(sim::initialize_startup_world_commerce_pages(claim_state),
          "Real pending93 initializes before receiving input");
    const auto reward = task_top(claim_state).id;
    claim_state.scene.framework_paused = true;
    app::WorldSession claim(claim_state);
    claim.set_paused(false);
    const auto early = claim.act_commerce(reward, A::confirm);
    frame = input_frame(claim, claim.set_paused(true));
    check(!frame->failed && input_result(*frame, early).outcome == Outcome::applied &&
              frame->state->page_counters.at(reward) >= 40 &&
              frame->state->facility_free_builds.at(34) == 0,
          "Early93 input only advances the confirmation threshold");
    claim.set_paused(false);
    const auto grant = claim.act_commerce(reward, A::confirm);
    const auto duplicate = claim.act_commerce(reward, A::confirm);
    frame = input_frame(claim, claim.set_paused(true));
    check(!frame->failed && input_result(*frame, grant).outcome == Outcome::applied &&
              input_result(*frame, duplicate).outcome == Outcome::rejected &&
              frame->state->facility_free_builds.at(34) == 1 &&
              frame->state->facility_presence.at(34) == 2 && frame->state->village_points == 0,
          "Qualified93 grants once; duplicate stale page cannot grant or charge again");
    claim.stop();
}

void facility_item_command_transactions() {
    using A = sim::StartupFacilityItemAction;
    {
        auto state = facility_items_fixture(false);
        const auto page = task_top(state).id;
        app::WorldSession session(state);
        const auto early = session.act_facility_item(page, A::confirm);
        const auto stopped = input_frame(session, session.set_paused(true));
        check(!stopped->failed && input_result(*stopped, early).outcome == Outcome::rejected &&
                  task_top(*stopped->state).id == page &&
                  stopped->state->items.at(1).inventory == 2 &&
                  stopped->state->scene.random.draws() == state.scene.random.draws(),
              "Uninitialized75 input cannot consume inventory or initialize the source catalogue");
        session.stop();
    }
    auto state = facility_items_fixture();
    const auto page = task_top(state).id;
    const auto facility = bun_shop(state);
    check(state.facility_item_page_lists.at(page) == std::vector<int>{1} &&
              state.facility_page_bindings.at(page) == facility,
          "75 catalogue and live74 binding use actual item and instance identities");
    state.scene.framework_paused = true;
    app::WorldSession session(state);
    const auto blocked = input_frame(session, session.act_facility_item(page, A::confirm));
    check(!blocked->failed &&
              input_result(*blocked, blocked->last_command_serial).outcome == Outcome::rejected,
          "Paused75 cannot consume an item");
    same_world(state, *blocked->state);
    session.set_paused(false);
    const auto invalid = session.act_facility_item(page, static_cast<A>(999));
    const auto bad_row = session.act_facility_item(page, A::confirm, 1);
    const auto use = session.act_facility_item(page, A::confirm, 0);
    const auto duplicate = session.act_facility_item(page, A::confirm, 0);
    const auto paid = input_frame(session, session.set_paused(true));
    check(!paid->failed && input_result(*paid, invalid).outcome == Outcome::rejected &&
              input_result(*paid, bad_row).outcome == Outcome::rejected &&
              input_result(*paid, use).outcome == Outcome::applied &&
              input_result(*paid, duplicate).outcome == Outcome::rejected &&
              task_top(*paid->state).legacy_page == 76 &&
              paid->state->facility_page_bindings.at(task_top(*paid->state).id) == facility &&
              paid->state->items.at(1).inventory == 1 &&
              paid->state->catalog.at({0, 1}).inventory == 1 &&
              paid->state->facility_item_confirmations.at(facility) == 1 &&
              paid->state->scene.random.draws() == state.scene.random.draws(),
          "75 consumes one true item and instance count; invalid/stale commands cannot reuse it");
    auto animation = *paid->state;
    session.stop();
    animation.scene.framework_paused = false;
    check(sim::initialize_startup_world_facility_item_pages(animation),
          "Real76 source initialization applies improvement before animation input test");
    const auto animation_page = task_top(animation).id;
    check(task_top(animation).legacy_page == 76, "Item1 keeps its actual76 animation topmost");
    app::WorldSession playing(animation);
    const auto ack = playing.ack_page(animation_page);
    const auto skip = playing.act_facility_item(animation_page, A::confirm);
    const auto stopped = input_frame(playing, playing.set_paused(true));
    check(
        !stopped->failed && input_result(*stopped, ack).outcome == Outcome::rejected &&
            input_result(*stopped, skip).outcome == Outcome::rejected &&
            task_top(*stopped->state).id == animation_page &&
            stopped->state->items.at(1).inventory == 1 &&
            stopped->state->scripts.facilities.at(33).improvements ==
                animation.scripts.facilities.at(33).improvements,
        "Initialized76 rejects generic and specialized confirmation without a second improvement");
    playing.stop();
}
} // namespace ark::test::world_session
