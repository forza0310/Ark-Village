#include "../support/world_fixture.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "world_campaign_clear.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
void require(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error("Campaign lifecycle: " + message);
}
void good(const std::string &error) { require(error.empty(), error); }
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
std::array<std::int64_t, 3> events(const State &s) {
    std::array<std::int64_t, 3> result{};
    for (int n = 0; n < 3; ++n) {
        const auto found = s.scripts.event_calls.find(n + 4);
        if (found != s.scripts.event_calls.end())
            result[n] = found->second;
    }
    return result;
}
std::string file_bytes(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    require(bool(in), "cannot read " + path.u8string());
    return {std::istreambuf_iterator<char>(in), {}};
}
} // namespace

// Fast source-boundary and durable-bridge qualifications; no natural campaign claim.
void campaign_clear_contract() {
    const char *contract_phase = "empty-sidecar";
    try {
        CampaignClearEvidence empty;
        std::ostringstream encoded;
        empty.encode(encoded);
        std::istringstream in(encoded.str());
        auto again = CampaignClearEvidence::decode(in);
        std::ostringstream recaptured;
        again.encode(recaptured);
        auto state = initial_world();
        again.verify(state, {});
        require(recaptured.str() == encoded.str() && !again.finished() && !again.inherited(),
                "empty lifecycle evidence must roundtrip without claiming a score");
        for (const auto &bad : {encoded.str() + "extra", encoded.str().substr(0, 30),
                                std::string("ARK_CAMPAIGN_LIFECYCLE_1\n1 0 0 0")}) {
            bool rejected{};
            try {
                std::istringstream invalid(bad);
                (void)CampaignClearEvidence::decode(invalid);
            } catch (const std::exception &) {
                rejected = true;
            }
            require(rejected, "bad or in-flight evidence cannot qualify a player prefix");
        }

        // Short boundary qualification reused from startup_application_replay_checks:
        // the date alone is placed just before the source transition. Nothing sets raw17,
        // score phases/counters, rewards or completion. This is not a natural 16-year route.
        contract_phase = "owned-directory";
        const auto root = std::filesystem::weakly_canonical(
            std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "build");
        require(root.filename() == "build" && std::filesystem::is_directory(root),
                "clear contract must stay in the real product build root");
        struct Owned {
            std::filesystem::path directory;
            bool created{};
            ~Owned() {
                if (!created)
                    return;
                std::error_code ignored;
                std::filesystem::remove_all(directory, ignored);
            }
        } owned{root /
                ("campaign-clear-contract-" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
        const auto relative = owned.directory.lexically_relative(root);
        require(!relative.empty() && !relative.is_absolute() &&
                    relative.begin()->string().find("..") != 0,
                "contract directory must stay inside product build root");
        owned.created = std::filesystem::create_directory(owned.directory);
        require(owned.created, "exclusive bounded contract directory");
        for (const bool new_high : {true, false}) {
            contract_phase = new_high ? "new-high-setup" : "existing-high-setup";
            const auto directory = owned.directory / (new_high ? "new-high" : "existing-high");
            require(std::filesystem::create_directory(directory), "exclusive scoring scenario");
            auto world = initial_world();
            app::WorldSystemState system;
            if (!new_high) {
                system.records.high_score = std::numeric_limits<std::int64_t>::max();
                system.records.score_village = "既有纪录";
                system.records.trophy = 4;
            }
            contract_phase = "initial-system-write";
            good(app::write_world_system(directory, system.records));
            contract_phase = "scene-and-score-qualification";
            CampaignClearEvidence evidence;
            const char *phase = "initial-scene";
            const auto diagnostic = [&] {
                const auto top =
                    std::find_if(world.scripts.pages.rbegin(), world.scripts.pages.rend(),
                                 [](const auto &page) { return page.lifecycle != 4; });
                std::ostringstream out;
                out << " phase=" << phase << " new_high=" << new_high << " month=" << month(world)
                    << " completion=" << world.completion_mode << '/'
                    << world.system_completion_mode
                    << " top=" << (top == world.scripts.pages.rend() ? -1 : top->legacy_page)
                    << " kind=" << (top == world.scripts.pages.rend() ? -1 : int(top->kind))
                    << " lifecycle=" << (top == world.scripts.pages.rend() ? -1 : top->lifecycle)
                    << " main=" << world.scene.top_is_main << " scene=" << world.scene.scene_state
                    << " processing=" << world.scene.processing_phase
                    << " paused=" << world.scene.framework_paused
                    << " frame_counter=" << world.scene.frame_counter
                    << " scene_counter=" << world.scene.scene_counter
                    << " first_refresh=" << world.scene.first_normal_refresh
                    << " calendar_advance=" << world.calendar_advance
                    << " units=" << world.scene.calendar.units
                    << " world_updates=" << world.scene.world.updates
                    << " score_stage=" << (system.clear ? system.clear->score.stage : -1)
                    << " score_counter=" << (system.clear ? system.clear->score.counter : -1);
                return out.str();
            };
            try {
                // Source-only cleanup serves real initial-host preparation, covering pages and
                // score follow-ups; none of these paths fabricates lifecycle or a stable scene.
                const auto consume_source_page = [&] {
                    const auto top =
                        std::find_if(world.scripts.pages.rbegin(), world.scripts.pages.rend(),
                                     [](const auto &page) { return page.lifecycle != 4; });
                    require(top != world.scripts.pages.rend(),
                            "source follow-up has a real parent page");
                    if (top->lifecycle != 2 || top->kind == ref::WorldScriptPageKind::scene)
                        return;
                    sim::StartupWorldRuntimeError error{sim::StartupWorldRuntimeError::none};
                    if (top->kind != ref::WorldScriptPageKind::raw_page) {
                        error = sim::acknowledge_startup_world_runtime_page(world, top->id);
                    } else if (top->legacy_page == 83) {
                        error = sim::cancel_startup_world_runtime_page(world, top->id);
                    } else if (top->legacy_page == 48) {
                        error = sim::act_startup_world_runtime_rank_page(world, top->id, 0, true);
                    } else if (top->legacy_page == 87) {
                        if (!world.award_termination_pending.count(top->id))
                            return; // Source Init must install the award payload before input.
                        error = sim::act_startup_world_runtime_award_page(
                            world, top->id,
                            world.award_termination_pending.at(top->id)
                                ? ref::WorldAwardAction::confirm_termination
                                : ref::WorldAwardAction::request_termination);
                    } else {
                        const std::initializer_list<int> automatic{16, 24, 56, 57, 97, 98};
                        if (std::find(automatic.begin(), automatic.end(), top->legacy_page) !=
                            automatic.end())
                            return;
                        const std::initializer_list<int> confirmable{0,  1,  11, 15, 30, 31, 32,
                                                                     49, 50, 59, 67, 81, 88, 89,
                                                                     94, 95, 96, 99, 100};
                        require(std::find(confirmable.begin(), confirmable.end(),
                                          top->legacy_page) != confirmable.end(),
                                "uncovered source follow-up" + diagnostic());
                        error = sim::acknowledge_startup_world_runtime_page(world, top->id);
                    }
                    require(error == sim::StartupWorldRuntimeError::none ||
                                error == sim::StartupWorldRuntimeError::invalid_page,
                            "source covering/follow-up consumer failed" + diagnostic());
                    world.sound_requests.clear();
                };
                const auto source_update = [&] {
                    auto update = sim::prepare_startup_world_runtime(world);
                    require(update.candidate &&
                                sim::update_startup_world_render_cache(*update.candidate),
                            "source covering/follow-up update rejected" + diagnostic() +
                                " error=" + std::to_string(int(update.error)));
                    good(app::commit_world_system(directory, world, *update.candidate, system));
                    evidence.observe_world(world, *update.candidate, system);
                    update.candidate->sound_requests.clear();
                    world = std::move(*update.candidate);
                };
                const auto host_ready = [&] {
                    const auto top =
                        std::find_if(world.scripts.pages.rbegin(), world.scripts.pages.rend(),
                                     [](const auto &page) { return page.lifecycle != 4; });
                    return top != world.scripts.pages.rend() &&
                           top->kind == ref::WorldScriptPageKind::scene && top->lifecycle == 2 &&
                           world.scene.top_is_main && world.scene.scene_state == 0 &&
                           world.scene.processing_phase == -1 && world.scene.world.updates > 0;
                };
                for (int n = 0; n < 500 && !host_ready(); ++n) {
                    consume_source_page();
                    source_update();
                }
                require(host_ready(), "real source must initialize the scene host" + diagnostic());
                // The maintenance fixture already owned an initialized host. This product contract
                // earns that qualification above, then supplies only the same near-date boundary.
                world.scene.calendar = {15, 2, 3, 10799, 10799, 0};
                phase = "date-producer";
                // The cold fixture has not seen late-year normal conditions. Their real
                // continuations may skip a round before calendar; keep the one date qualification
                // and consume those actual pages until the true month/completion transition,
                // without injecting seen flags.
                for (int n = 0; n < 500 && world.completion_mode == 0; ++n) {
                    consume_source_page();
                    source_update();
                }
                require(world.completion_mode == 1 && world.system_completion_mode == 1 &&
                            month(world) == 183,
                        "qualified host must reach the actual calendar/completion transition" +
                            diagnostic());
                const auto entry_events = events(world);
                const auto pending = std::find_if(
                    world.scripts.pages.begin(), world.scripts.pages.end(), [](const auto &page) {
                        return page.kind == ref::WorldScriptPageKind::raw_page &&
                               page.legacy_page == 17 && page.lifecycle == 0;
                    });
                require(pending != world.scripts.pages.end() &&
                            sim::valid_startup_world_clear_page(world, *pending) &&
                            !evidence.finished() && !system.clear,
                        "actual producer must own pending raw17 before application Init" +
                            diagnostic());
                const auto trigger_file = app::read_world_system(directory);
                require(trigger_file.records &&
                            trigger_file.records->facility_levels == world.system_unlock_data[0] &&
                            trigger_file.records->profession_status == world.system_unlock_data[1],
                        "trigger G/p must be durable before score Init" + diagnostic());
                // A month change can also stack other pages above the published pending17.
                for (int n = 0; n < 500 && !app::world_clear_page(world); ++n) {
                    consume_source_page();
                    if (!app::world_clear_page(world))
                        source_update();
                }
                require(app::world_clear_page(world) && !system.clear,
                        "normal covering-page consumers must reveal pending17 before score Init" +
                            diagnostic());
                const auto entry_steps = world.simulation_steps;

                phase = "score";
                std::ostringstream score_log;
                bool failed_tail_checked{};
                int confirmations{};
                for (int n = 0; n < 3000 && !evidence.finished(); ++n) {
                    require(!app::world_save_eligible(world) &&
                                app::capture_world_save(world).error ==
                                    app::WorldSaveError::ineligible,
                            "raw17 cannot leak an ordinary player checkpoint");
                    const bool confirm = evidence.confirm_due(system);
                    if (confirm) {
                        require(system.clear && ((system.clear->score.stage == 0 &&
                                                  system.clear->score.counter == 75) ||
                                                 (system.clear->score.stage == 3 &&
                                                  system.clear->score.counter == 45)),
                                "player confirms only at the real 75/45 score thresholds");
                        ++confirmations;
                    }
                    if (system.clear && system.clear->score.stage == 6 &&
                        system.clear->score.counter == 64) {
                        // Even the unchanged non-new-high record must attempt a final durable
                        // write. Failure cannot install the prepared retirement/events/sidecar
                        // qualification.
                        phase = "tail-rollback-digest";
                        const auto digest = sim::startup_world_state_digest(world);
                        const auto observed = evidence.diagnose();
                        const auto old_record = system.records.high_score;
                        const auto target = app::world_system_path(directory);
                        require(std::filesystem::remove(target) &&
                                    std::filesystem::create_directory(target),
                                "prepare explicit unavailable system target in the owned fixture");
                        bool rejected{};
                        try {
                            advance_campaign_clear(world, system, evidence, false, directory,
                                                   score_log);
                        } catch (const std::exception &) {
                            rejected = true;
                        }
                        require(
                            rejected && sim::startup_world_state_digest(world) == digest &&
                                system.clear && system.clear->score.stage == 6 &&
                                system.clear->score.counter == 64 &&
                                system.records.high_score == old_record &&
                                evidence.diagnose() == observed && !evidence.finished() &&
                                std::filesystem::is_directory(target),
                            "failed score tail must retain the entire old Owner and qualification");
                        require(std::filesystem::remove(target),
                                "restore the owned writable target");
                        failed_tail_checked = true;
                        phase = "score-tail";
                    }
                    advance_campaign_clear(world, system, evidence, confirm, directory, score_log);
                }
                require(evidence.finished() && !system.clear && failed_tail_checked &&
                            confirmations == 7 && world.simulation_steps == entry_steps &&
                            events(world)[2] == entry_events[2] + 1 &&
                            events(world)[new_high ? 0 : 1] == entry_events[new_high ? 0 : 1] + 1 &&
                            events(world)[new_high ? 1 : 0] == entry_events[new_high ? 1 : 0] &&
                            (new_high ? system.records.high_score > 0
                                      : system.records.high_score ==
                                            std::numeric_limits<std::int64_t>::max()),
                        "real score thresholds must retire once with the proper strict-high tail");
                const auto finished_file = file_bytes(app::world_system_path(directory));
                const auto finished_digest = sim::startup_world_state_digest(world);
                bool repeat_rejected{};
                try {
                    advance_campaign_clear(world, system, evidence, false, directory, score_log);
                } catch (const std::exception &) {
                    repeat_rejected = true;
                }
                require(repeat_rejected &&
                            sim::startup_world_state_digest(world) == finished_digest &&
                            file_bytes(app::world_system_path(directory)) == finished_file,
                        "retired score cannot repeat its effects or durable transaction");

                // Drain only source follow-up pages/updates to the existing stable player-save
                // gate. No page erasure, phase assignment or hand-created stable scene qualifies
                // this file.
                phase = "follow-up";
                for (int n = 0; n < 500 && !app::world_save_eligible(world); ++n) {
                    consume_source_page();
                    source_update();
                }
                require(app::world_save_eligible(world),
                        "score follow-up did not reach a real stable scene" + diagnostic());
                phase = "player-save";
                evidence.verify(world, system);
                const auto captured = app::capture_world_save(world);
                require(captured.image.has_value(), captured.message);
                require(app::write_world_save_slot(directory, 0, *captured.image).error ==
                            app::WorldSaveError::none,
                        "qualified fixture writes an actual stable player file");
                write_campaign_clear_evidence(directory, 0, evidence);
                phase = "cold-read";
                auto cold = app::read_world_save_slot(directory, 0);
                const auto durable = app::read_world_system(directory);
                require(cold.state && durable.records,
                        "qualified fixture player/system files cold-read");
                std::string reason;
                require(app::prepare_world_save_candidate(*cold.state, initial_world(9), reason) ==
                            app::WorldSaveError::none,
                        reason);
                app::WorldSystemState cold_system{*durable.records, {}};
                const auto bound =
                    read_campaign_clear_evidence(directory, 0, *cold.state, cold_system);
                require(bound.finished() && !bound.inherited() && !bound.continued() &&
                            bound.diagnose() == evidence.diagnose(),
                        "positive bound sidecar cold-read retains only observed lifecycle "
                        "qualification");
                auto wrong_system = cold_system;
                wrong_system.records.facility_levels.front() ^= 1;
                bool wrong_rejected{};
                try {
                    (void)read_campaign_clear_evidence(directory, 0, *cold.state, wrong_system);
                } catch (const std::exception &) {
                    wrong_rejected = true;
                }
                require(wrong_rejected, "wrong system G/p cannot certify a valid player file");
                std::ofstream invalid_binding(directory / "lifecycle0.txt",
                                              std::ios::binary | std::ios::trunc);
                invalid_binding << std::string(64, '0') << '\n';
                evidence.encode(invalid_binding);
                invalid_binding.close();
                bool binding_rejected{};
                try {
                    (void)read_campaign_clear_evidence(directory, 0, *cold.state, cold_system);
                } catch (const std::exception &) {
                    binding_rejected = true;
                }
                require(binding_rejected,
                        "wrong player-file binding cannot certify lifecycle evidence");
            } catch (const std::exception &error) {
                throw std::runtime_error(std::string(error.what()) + diagnostic());
            }
        }
    } catch (const std::exception &error) {
        throw std::runtime_error(std::string("Clear qualification ") + contract_phase + ": " +
                                 error.what());
    }
}
} // namespace ark::test
