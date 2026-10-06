// Management transport shares the real page stack, worker gates and private Owner commits.
// Frozen suites own activity/trade arithmetic; these cases cover UI identities and FIFO wiring.
#include "world_session_test_support.hpp"

#include <algorithm>
#include <limits>

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
} // namespace
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
