#include "support/world_fixture.hpp"
#include "world_session_test_support.hpp"
#include <filesystem>
#include <fstream>

namespace ark::test::world_session {
void system_command_transactions() {
    const auto directory = std::filesystem::current_path() / "world-system-session-test";
    std::filesystem::remove_all(directory);
    auto state = initial(false);
    app::WorldSystemState system;
    app::WorldNewGameDraft draft;
    draft.human = {"UI测试甲", 1, true};
    draft.village = "UI测试村";
    draft.slot = 1;
    const auto draws = state.scene.random.draws();
    check(app::start_world_draft(state, system, draft, directory).empty(),
          "New draft commits system and profile");
    check(!std::filesystem::exists(app::world_save_slot_path(directory, 1)) &&
              state.scene.random.draws() == draws && system.records.last_slot == 1,
          "Start never overwrites a manual slot or consumes random");
    auto rejected = initial(false);
    const auto target = app::world_system_path(directory);
    std::filesystem::remove(target);
    std::filesystem::create_directory(target);
    check(!app::start_world_draft(rejected, system, draft, directory).empty() &&
              rejected.human_profiles.empty(),
          "Failed new-game system write leaves cold owner intact");
    std::filesystem::remove(target);
    check(app::write_world_system(directory, system.records).empty(),
          "Restore writable system target");
    // Explicit source raw95 reward fixture tests worker rollback, not a natural reward claim.
    state = initial(false);
    state.scripts.pages.front().lifecycle = 3;
    rules::WorldScriptPage page;
    page.id = state.scripts.next_page_id++;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 95;
    page.legacy_s = 500;
    state.scripts.pages.push_back(page);
    state.page_counters[page.id] = 40;
    app::WorldSession session(state, directory);
    const auto old = session.frame();
    std::filesystem::remove(target);
    std::filesystem::create_directory(target);
    session.ack_page(page.id);
    auto failed = await(session, [](const auto &f) { return f.failed || !f.system_error.empty(); });
    check(!failed->failed && failed->state == old->state && failed->system.records.cash_peak == 0,
          "System write failure preserves entire published world, random, reward and records");
    std::filesystem::remove(target);
    app::WorldCommand retry;
    retry.kind = app::WorldCommandKind::retry_system_write;
    const auto done = input_frame(session, session.submit(retry));
    check(done->system_error.empty() &&
              done->state->scene.world.world.ai.accounting.funds() ==
                  old->state->scene.world.world.ai.accounting.funds() + 500 &&
              app::read_world_system(directory).records->cash_peak ==
                  done->system.records.cash_peak,
          "Retry installs exactly one prepared award and durable cash peak");
    session.stop();
    // Complete six source stages directly without changing the world's natural clock.
    state = initial(false);
    state.scripts.pages.front().lifecycle = 3;
    page = {};
    page.id = state.scripts.next_page_id++;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 17;
    state.scripts.pages.push_back(page);
    system = {};
    check(app::advance_world_clear(state, system, false).empty(),
          "Initialize source six-category projection");
    std::int64_t expected{};
    for (const auto &row : system.clear->rows)
        expected += row.score;
    for (int n = 0; n < 3000 && system.clear &&
                    !(system.clear->score.stage == 6 && system.clear->score.counter == 64);
         ++n)
        check(app::advance_world_clear(state, system, true).empty(),
              "Advance one maintained scoring update");
    check(system.clear && system.clear->score.stage == 6 && system.clear->score.counter == 64 &&
              system.clear->score.sum == expected,
          "Every category has accumulated once before retirement");
    const auto before = state;
    const auto before_system = system;
    auto candidate = state;
    auto next = system;
    check(app::advance_world_clear(candidate, next, false).empty() && !next.clear,
          "Finish through source events");
    std::filesystem::remove(target);
    std::filesystem::create_directory(target);
    check(!app::commit_world_system(directory, before, candidate, next, true).empty() &&
              system.clear->score.counter == before_system.clear->score.counter &&
              state.scripts.event_calls == before.scripts.event_calls,
          "Failed tail cannot install score or events");
    std::filesystem::remove(target);
    check(app::commit_world_system(directory, before, candidate, next, true).empty() &&
              event_count(candidate, 6) == event_count(before, 6) + 1 &&
              app::read_world_system(directory).records->high_score == expected,
          "Retry tail writes one six-category record with one event6");
    std::filesystem::remove_all(directory);
}
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
    auto records = app::read_world_system(directory).records.value();
    records.cash_peak = 43210;
    records.cash_village = "跨局纪录";
    records.high_score = 12345;
    check(app::write_world_system(directory, records).empty(),
          "Persist newer cross-game records before loading old slot");
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
    check(frame->state->cash_peak == 43210 && frame->state->cash_peak_village == "跨局纪录" &&
              frame->system.records.high_score == 12345 &&
              app::read_world_system(directory).records->cash_peak == 43210,
          "Older player slot cannot roll back cross-game records or publish a new peak event");
    restarted.stop();
    std::filesystem::remove_all(directory);
}
} // namespace ark::test::world_session
