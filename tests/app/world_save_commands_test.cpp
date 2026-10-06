#include "support/world_fixture.hpp"
#include "world_session_test_support.hpp"
#include <filesystem>
#include <fstream>

namespace ark::test::world_session {
void save_command_transactions() {
    const auto directory = std::filesystem::current_path() / "world-save-session-test";
    std::filesystem::remove_all(directory);
    auto state = initial(true);
    app::WorldSession session(state, directory);
    auto frame = input_frame(session, session.open_save_menu());
    check(frame->save_menu_open && !frame->failed && frame->state == frame->previous &&
              frame->state->scene.framework_paused,
          "Opening save menu preserves explicit pause and complete immutable Owner");
    const auto original = frame->state;
    frame = input_frame(session, session.save_slot(0));
    // Busy publication acknowledges serial for observers; wait for file completion too.
    frame = await(session, [](const auto &f) { return !f.save_busy && f.save_slots[0].metadata; });
    check(!frame->failed && frame->state == original && frame->save_menu_open &&
              frame->save_slots[0].metadata->funds == state.scene.world.world.ai.accounting.funds(),
          "Save captures at FIFO boundary without mutating world or unpausing");
    const auto saved = app::capture_world_save(*frame->state);
    const auto generation = frame->generation;
    app::WorldCommand stale;
    stale.kind = app::WorldCommandKind::set_speed;
    stale.speed = 1;
    stale.generation = generation;
    const auto load = session.load_slot(0);
    const auto old_input = session.submit(stale);
    frame = input_frame(session, old_input);
    check(!frame->failed && frame->generation == generation + 1 && !frame->save_menu_open &&
              frame->state->scene.speed_setting == state.scene.speed_setting &&
              frame->state->scene.framework_paused && frame->previous == frame->state,
          "Load replaces Owner atomically, clears interpolation and preserves player pause");
    check(load < old_input &&
              input_result(*frame, old_input).outcome == app::WorldCommandOutcome::rejected,
          "Queued old-world input is rejected by generation after successful load");
    const auto reloaded = app::capture_world_save(*frame->state);
    check(saved.image && reloaded.image && saved.image->bytes == reloaded.image->bytes,
          "Worker load restores durable snapshot without advancing source update");
    frame = input_frame(session, session.open_save_menu());
    const auto before_failure = frame->state;
    std::ofstream(app::world_save_slot_path(directory, 1), std::ios::binary) << "corrupt";
    frame = input_frame(session, session.load_slot(1));
    frame = await(session, [](const auto &f) { return !f.save_busy && !f.save_message.empty(); });
    check(!frame->failed && frame->state == before_failure && frame->save_menu_open &&
              frame->generation == generation + 1 && !frame->save_message.empty(),
          "Corrupt load keeps current Owner, generation and modal usable");
    frame = input_frame(session, session.close_save_menu());
    check(!frame->save_menu_open && frame->state->scene.framework_paused,
          "Closing file overlay never resumes a manually paused world");
    session.stop();

    // A fresh session has a new independent stream, and loading uses that stream.
    auto fresh = ark::test::initial_world(20261006);
    fresh.scene.framework_paused = true;
    fresh.scene.random.draw(103);
    auto random = fresh.scene.random;
    app::WorldSession restarted(fresh, directory);
    input_frame(restarted, restarted.open_save_menu());
    frame = input_frame(restarted, restarted.load_slot(0));
    frame = await(restarted, [](const auto &f) { return f.generation == 2 && !f.save_busy; });
    auto loaded_random = frame->state->scene.random;
    check(loaded_random.draws() == random.draws() &&
              loaded_random.draw(197).ticket == random.draw(197).ticket,
          "New session loading preserves its fresh stream rather than save seed1");
    restarted.stop();
    std::filesystem::remove_all(directory);
}
} // namespace ark::test::world_session
