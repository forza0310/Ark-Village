// Management transport shares the real page stack, worker gates and private Owner commits.
// Frozen suites own activity/trade arithmetic; these cases cover UI identities and FIFO wiring.
#include "world_session_test_support.hpp"

#include <algorithm>

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
} // namespace ark::test::world_session
