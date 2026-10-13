#include "../support/world_fixture.hpp"
#include "ark/assets/sha256.hpp"
#include "ark/simulation/world/startup_world_save.hpp"
#include "world_session_test_support.hpp"
#include <filesystem>
#include <fstream>
#include <limits>

namespace ark::test::world_session {
namespace {
std::string file_bytes(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void same_clear_state(const app::WorldState &left, const app::WorldState &right) {
    same_world(left, right);
    check(left.page_phases == right.page_phases &&
              left.page_secondary_counters == right.page_secondary_counters &&
              left.sound_requests == right.sound_requests,
          "Rejected raw17 transaction changed stage, independent main clock or audio");
}
void source_save_bridge() {
    const auto directory = std::filesystem::current_path() / "world-source-save-session-test";
    std::filesystem::remove_all(directory);
    app::WorldSession session(initial(false), directory);
    input_frame(session, session.open_main_menu());
    menu_tag(session, 3, 6);
    auto frame = menu_ready(session, 10);
    const auto page = task_top(*frame->state).id;
    const auto old = frame->state;
    frame = input_frame(session, session.open_source_save_menu(page));
    check(frame->save_menu_open && frame->save_source_page == page && frame->state == old,
          "source system save opens player slots without creating raw14 or advancing Owner");
    const auto revision = frame->revision;
    check(session.wait_for_frame_after(revision, std::chrono::milliseconds(100)) == frame,
          "slot selection freezes source menu lifecycle and world");
    frame = input_frame(session, session.load_slot(0));
    check(frame->save_menu_open && frame->state == old && frame->generation == 1 &&
              input_result(*frame, frame->last_command_serial).outcome ==
                  app::WorldCommandOutcome::rejected,
          "source-only save overlay cannot silently load another world");
    frame = input_frame(session, session.close_save_menu());
    check(!frame->save_menu_open && task_top(*frame->state).id == page,
          "cancel returns to the same system parent and selection");
    input_frame(session, session.open_source_save_menu(page));
    frame = input_frame(session, session.save_slot(0));
    const auto save_page = task_top(*frame->state).id;
    check(!frame->failed && !frame->save_menu_open && task_top(*frame->state).legacy_page == 14,
          "selected player slot opens actual source raw14 once");
    input_frame(session,
                session.ack_page(save_page)); // Early input must neither close nor write twice.
    frame = await(session, [save_page](const auto &f) {
        const auto view = sim::inspect_startup_world_save_page(*f.state, save_page);
        return f.failed || (view && view->stage == 2);
    });
    const auto view = sim::inspect_startup_world_save_page(*frame->state, save_page);
    check(!frame->failed && view && view->saved == true && frame->save_slots[0].metadata &&
              frame->state->scene.random.draws() == old->scene.random.draws(),
          "stage1 publishes one player file and stage2 result with original random stream");
    const auto saved = file_bytes(app::world_save_slot_path(directory, 0));
    auto loaded = app::read_world_save_slot(directory, 0);
    check(loaded.error == app::WorldSaveError::none && loaded.state &&
              loaded.state->scripts.pages.empty(),
          "raw14 player decode contains durable data and no persisted result/modal page");
    std::string restore_reason;
    const auto restored =
        app::prepare_world_save_candidate(*loaded.state, *frame->state, restore_reason);
    check(restored == app::WorldSaveError::none && restore_reason.empty() &&
              app::world_save_eligible(*loaded.state) && loaded.state->scripts.pages.size() == 1 &&
              loaded.state->scene.world.world.ai.accounting.funds() ==
                  old->scene.world.world.ai.accounting.funds(),
          "raw14 export stores a stable player scene rather than its modal result page");
    input_frame(session, session.ack_page(save_page));
    frame = await(session,
                  [](const auto &f) { return f.failed || app::world_save_eligible(*f.state); });
    input_frame(session, session.set_paused(true));
    check(!frame->failed && file_bytes(app::world_save_slot_path(directory, 0)) == saved,
          "closing result never writes the file a second time");
    session.stop();
    // Replacement failure keeps the existing target and all business/random data.
    const auto blocked = directory / "blocked";
    std::filesystem::create_directories(app::world_save_slot_path(blocked, 0));
    app::WorldSession denied(initial(false), blocked);
    input_frame(denied, denied.open_save_menu());
    frame = input_frame(denied, denied.save_slot(0));
    frame = await(denied, [](const auto &f) {
        const auto &p = task_top(*f.state);
        const auto v = sim::inspect_startup_world_save_page(*f.state, p.id);
        return f.failed || (v && v->stage == 2);
    });
    const auto failed_view =
        sim::inspect_startup_world_save_page(*frame->state, task_top(*frame->state).id);
    check(!frame->failed && failed_view && failed_view->saved == false &&
              std::filesystem::is_directory(app::world_save_slot_path(blocked, 0)) &&
              frame->state->scene.world.world.ai.accounting.funds() == 5000 &&
              frame->state->scene.random.draws() == initial(false).scene.random.draws(),
          "failed F5 source save preserves target, initial business and random with usable failure "
          "result");
    denied.stop();
    auto dormant = initial(false);
    dormant.build_mode = 7;
    app::WorldSession tools(dormant, directory / "dormant");
    frame = input_frame(tools, tools.open_save_menu());
    const auto unchanged = frame->state;
    frame = input_frame(tools, tools.save_slot(0));
    check(!frame->failed && frame->state == unchanged && frame->save_menu_open &&
              input_result(*frame, frame->last_command_serial).outcome ==
                  app::WorldCommandOutcome::rejected &&
              !std::filesystem::exists(app::world_save_slot_path(directory / "dormant", 0)),
          "dormant build tool refuses strict raw14 export before creating a save page or file");
    tools.stop();
    std::filesystem::remove_all(directory);
}
} // namespace
void system_command_transactions() {
    const auto directory = std::filesystem::current_path() / "world-system-session-test";
    std::filesystem::remove_all(directory);
    auto state = initial(false);
    // Product identity is an explicit system2 contract. Maintenance files still use their
    // own dataset and never gain a second automatic fallback in the player loader.
    sim::StartupSystemRecords retained;
    retained.last_slot = 1;
    retained.high_score = 76543;
    retained.cash_peak = 912345;
    retained.trophy = 3;
    retained.revision = 8;
    retained.facility_levels = {0, 1, 0, 2};
    retained.profession_status = {0, 1};
    retained.opaque_sections.push_back({1024, 2, {1, 2, 3}});
    std::filesystem::create_directories(directory);
    const auto system_path = app::world_system_path(directory);
    check(sim::save_startup_system_file(system_path, retained, app::world_system_data_identity())
              .empty(),
          "Canonical system2 fixture is written with its existing independent identity");
    const auto original_system = file_bytes(system_path);
    const auto compatible = app::read_world_system(directory);
    check(compatible.records && !compatible.missing && compatible.error.empty() &&
              compatible.records->high_score == retained.high_score &&
              compatible.records->cash_peak == retained.cash_peak &&
              compatible.records->trophy == 3 &&
              compatible.records->facility_levels == retained.facility_levels &&
              compatible.records->profession_status == retained.profession_status &&
              compatible.records->opaque_sections.front().bytes ==
                  retained.opaque_sections.front().bytes &&
              compatible.records->revision == retained.revision &&
              file_bytes(system_path) == original_system,
          "Same-semantic pre-manual system2 file loads read-only with records and inheritance "
          "intact");
    check(
        !sim::load_startup_system_file(system_path).records,
        "Default maintenance loader rejects product identity instead of silently changing policy");
    check(sim::save_startup_system_file(system_path, retained).empty() &&
              !app::read_world_system(directory).records,
          "Product loader rejects maintenance identity instead of falling back");
    check(sim::save_startup_system_file(system_path, retained, std::string(64, '0')).empty() &&
              !app::read_world_system(directory).records,
          "Unknown well-formed system identity remains rejected");
    const auto rewrite = [&](const std::string &bytes) {
        std::ofstream out(system_path, std::ios::binary | std::ios::trunc);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        check(bool(out), "Isolated adversarial system fixture writes its exact bytes");
    };
    auto corrupted = original_system;
    corrupted.at(100) ^= 1;
    rewrite(corrupted);
    check(!app::read_world_system(directory).records, "Corrupt system payload remains rejected");
    auto old_version = original_system;
    old_version.at(8) = 1;
    old_version.resize(old_version.size() - 64);
    old_version +=
        ark::assets::sha256_hex(std::vector<std::uint8_t>(old_version.begin(), old_version.end()));
    rewrite(old_version);
    check(!app::read_world_system(directory).records,
          "Old system version with a valid checksum remains rejected");
    rewrite(original_system);
    auto invalid_records = retained;
    invalid_records.trophy = 5;
    check(!app::write_world_system(directory, invalid_records).empty() &&
              file_bytes(system_path) == original_system,
          "Failed system encoding preserves the previously valid file");
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
    check(session.take_audio_requests().empty(),
          "Uncommitted system candidate cannot leak a playback request");
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
    page.lifecycle = 0; // Natural clear producer publishes a pending page without clocks.
    state.scripts.pages.push_back(page);
    system = {};
    // Reject a half-initialized natural entry before it can capture rows or emit Init audio.
    for (int fault = 0; fault < 4; ++fault) {
        auto bad = state;
        auto bad_system = system;
        if (fault == 0)
            bad.page_counters.emplace(page.id, 0);
        if (fault == 1)
            bad.page_phases.emplace(page.id, 0);
        if (fault == 2)
            bad.page_secondary_counters.emplace(page.id, 0);
        if (fault == 3)
            bad.scene.framework_paused = true;
        const auto unchanged = bad;
        check(!app::advance_world_clear(bad, bad_system, true).empty() && !bad_system.clear,
              "Pending or paused raw17 rejects extra clocks without capturing rows or audio");
        same_clear_state(bad, unchanged);
    }
    const auto clear_random = state.scene.random.draws();
    const auto clear_steps = state.simulation_steps;
    check(app::advance_world_clear(state, system, false).empty(),
          "Initialize source six-category projection");
    check(
        system.clear && system.clear->page == page.id && task_top(state).lifecycle == 2 &&
            state.page_phases.at(page.id) == 0 && state.page_counters.at(page.id) == 1 &&
            state.page_secondary_counters.at(page.id) == 2 &&
            state.sound_requests ==
                std::vector<sim::StartupAudioRequest>{{sim::StartupAudioOperation::replace_bgm, 0}},
        "First raw17 update installs three clocks and emits exactly one typed BGM0 request");
    // Consumption belongs to the application output bridge, never to querying or scoring.
    state.sound_requests.clear();
    for (int fault = 0; fault < 11; ++fault) {
        auto bad = state;
        auto bad_system = system;
        if (fault == 0)
            bad.page_counters.erase(page.id);
        if (fault == 1)
            bad.page_phases.erase(page.id);
        if (fault == 2)
            bad.page_secondary_counters.erase(page.id);
        if (fault == 3)
            bad.page_secondary_counters.at(page.id) = -1;
        if (fault == 4)
            bad.page_secondary_counters.at(page.id) = std::numeric_limits<int>::max();
        if (fault == 5)
            ++bad.page_counters.at(page.id);
        if (fault == 6)
            ++bad.page_phases.at(page.id);
        if (fault == 7)
            bad.scripts.pages.back().lifecycle = 0;
        if (fault == 8)
            bad_system.clear.reset();
        if (fault == 9)
            ++bad_system.clear->page;
        if (fault == 10)
            bad.scene.framework_paused = true;
        const auto unchanged = bad;
        const auto control_page = bad_system.clear ? bad_system.clear->page : 0;
        check(
            !app::advance_world_clear(bad, bad_system, true).empty() &&
                (bad_system.clear ? bad_system.clear->page : 0) == control_page &&
                (!bad_system.clear ||
                 (bad_system.clear->score.stage == 0 && bad_system.clear->score.counter == 1 &&
                  bad_system.clear->score.sum == 0)),
            "Live raw17 rejects missing or mismatched clocks, controller and pause before writes");
        same_clear_state(bad, unchanged);
    }
    // These are main-clock conditions, not changes to the source stage thresholds or date.
    for (int distance : {1, 2}) {
        auto wrap = state;
        auto wrap_system = system;
        wrap.page_secondary_counters.at(page.id) = std::numeric_limits<int>::max() - distance;
        wrap.page_phases.at(page.id) = wrap_system.clear->score.stage = 1;
        wrap.page_counters.at(page.id) = wrap_system.clear->score.counter = 74;
        check(app::advance_world_clear(wrap, wrap_system, false).empty() &&
                  wrap.page_secondary_counters.at(page.id) == 2 - distance &&
                  wrap.page_phases.at(page.id) == 2 && wrap.page_counters.at(page.id) == 0 &&
                  wrap_system.clear->score.stage == 2 && wrap_system.clear->score.counter == 0 &&
                  wrap.sound_requests.empty(),
              "Two sequential main-clock modulo advances survive stage reset without Init replay");
    }
    std::int64_t expected{};
    for (const auto &row : system.clear->rows)
        expected += row.score;
    for (int n = 0; n < 3000 && system.clear &&
                    !(system.clear->score.stage == 6 && system.clear->score.counter == 64);
         ++n) {
        const auto main = state.page_secondary_counters.at(page.id);
        check(app::advance_world_clear(state, system, true).empty(),
              "Advance one maintained scoring update");
        check(system.clear &&
                  state.page_secondary_counters.at(page.id) ==
                      (std::int64_t(main) + 2) % std::numeric_limits<int>::max() &&
                  state.page_counters.at(page.id) == system.clear->score.counter &&
                  state.page_phases.at(page.id) == system.clear->score.stage &&
                  state.sound_requests.empty() && state.scene.random.draws() == clear_random &&
                  state.simulation_steps == clear_steps,
              "Scoring updates preserve stage mirrors, random and world while not replaying BGM0");
    }
    check(system.clear && system.clear->score.stage == 6 && system.clear->score.counter == 64 &&
              system.clear->score.sum == expected,
          "Every category has accumulated once before retirement");
    const auto before = state;
    const auto before_system = system;
    const auto before_main = before.page_secondary_counters.at(page.id);
    auto candidate = state;
    auto next = system;
    check(app::advance_world_clear(candidate, next, false).empty() && !next.clear,
          "Finish through source events");
    const std::vector<sim::StartupAudioRequest> finish_audio{
        {sim::StartupAudioOperation::replace_bgm, 1}};
    check(candidate.page_phases.at(page.id) == 7 && candidate.page_counters.at(page.id) == 0 &&
              candidate.page_secondary_counters.at(page.id) ==
                  (std::int64_t(before_main) + 2) % std::numeric_limits<int>::max() &&
              candidate.sound_requests == finish_audio &&
              candidate.scene.random.draws() == clear_random &&
              candidate.simulation_steps == clear_steps,
          "Prepared finish keeps both clocks until retirement and queues one typed BGM1 request");
    const auto prepared = candidate;
    std::filesystem::remove(target);
    std::filesystem::create_directory(target);
    check(!app::commit_world_system(directory, before, candidate, next, true).empty() &&
              system.clear->score.counter == before_system.clear->score.counter &&
              state.scripts.event_calls == before.scripts.event_calls &&
              state.sound_requests.empty() && candidate.sound_requests == finish_audio &&
              std::filesystem::is_directory(target),
          "Failed tail cannot install score or events");
    same_clear_state(state, before);
    same_clear_state(candidate, prepared);
    std::filesystem::remove(target);
    check(app::commit_world_system(directory, before, candidate, next, true).empty() &&
              event_count(candidate, 6) == event_count(before, 6) + 1 &&
              app::read_world_system(directory).records->high_score == expected &&
              candidate.sound_requests == finish_audio && state.sound_requests.empty(),
          "Retry tail writes one six-category record with one event6");
    same_clear_state(candidate, prepared);
    check(!app::advance_world_clear(candidate, next, true).empty() &&
              candidate.sound_requests == finish_audio && !next.clear,
          "Retired raw17 cannot repeat scoring, events or the pending finish audio on retry");
    same_clear_state(candidate, prepared);
    std::filesystem::remove_all(directory);
}
void save_command_transactions() {
    source_save_bridge();
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
    check(!frame->failed && frame->state == original && frame->save_menu_open &&
              frame->state->scene.framework_paused &&
              !std::filesystem::exists(app::world_save_slot_path(directory, 0)) &&
              input_result(*frame, frame->last_command_serial).outcome ==
                  app::WorldCommandOutcome::rejected,
          "Paused save refuses source page creation without silently resuming or writing");
    const auto saved = app::capture_world_save(*frame->state);
    check(saved.image && app::write_world_save_slot(directory, 0, *saved.image).error ==
                             app::WorldSaveError::none,
          "Prepare independent stable file for load transaction checks");
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
    check(session.take_audio_requests() ==
                  std::vector<sim::StartupAudioRequest>{
                      {sim::StartupAudioOperation::replace_bgm, 1}} &&
              session.take_audio_requests().empty(),
          "Committed player load activates its BGM once outside immutable snapshots");
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
    check(session.take_audio_requests().empty(), "Rejected load cannot restart BGM");
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
