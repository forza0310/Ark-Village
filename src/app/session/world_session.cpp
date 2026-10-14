#include "ark/app/session/world_session.hpp"
#include "ark/app/session/world_report.hpp"
#include "ark/app/timing/original_loop.hpp"
#include "ark/simulation/world/startup_world_save.hpp"
#include "world_commands.hpp"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <list>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace ark::app {
namespace {
using Clock = std::chrono::steady_clock;
using RuntimeError = simulation::StartupWorldRuntimeError;
const simulation::rules::WorldScriptPage *top_page(const WorldState &state) {
    const auto found = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                    [](const auto &page) { return page.lifecycle != 4; });
    return found == state.scripts.pages.rend() ? nullptr : &*found;
}
std::string update_error(const simulation::StartupWorldRuntimeResult &result) {
    auto message =
        "World update rejected: runtime=" + std::to_string(static_cast<int>(result.error)) +
        " scene=" + std::to_string(static_cast<int>(result.scene_error)) +
        " world=" + std::to_string(static_cast<int>(result.world_error));
    if (result.failure) {
        const auto &f = *result.failure;
        message += " stage=" + std::to_string(static_cast<int>(f.stage)) +
                   " id=" + (f.id ? std::to_string(*f.id) : "none") + " layer=" + f.layer +
                   " detail=" + std::to_string(f.error);
    }
    return message;
}
} // namespace

class WorldSession::Impl {
  public:
    explicit Impl(WorldState initial, std::filesystem::path directory)
        : save_directory(std::move(directory)) {
        initial.page_confirm_held = false; // A new desktop session has no physical press owner.
        const auto gate = query_original_loop_wait({}, 0);
        if (!gate || gate->minimum_period_ms <= 0)
            throw std::logic_error("Invalid source world update period");
        period = std::chrono::milliseconds(gate->minimum_period_ms);
        auto initial_frame = std::make_shared<WorldFrame>();
        const auto system = read_world_system(save_directory);
        if (!system.records)
            throw std::runtime_error("System records: " + system.error);
        initial_frame->system.records = *system.records;
        if (!save_directory.empty()) {
            initial.cash_peak = system.records->cash_peak;
            initial.cash_peak_village = system.records->cash_village;
        }
        take_sound_outputs(initial, *initial_frame);
        audio_outputs.splice(audio_outputs.end(), prepared_audio);
        initial_frame->state = std::make_shared<const WorldState>(std::move(initial));
        initial_frame->previous = initial_frame->state;
        initial_frame->published = Clock::now();
        initial_frame->interval_seconds = std::chrono::duration<double>(period).count();
        published = std::move(initial_frame);
        worker = std::thread([this] { run(); });
    }
    ~Impl() { stop(); }

    std::shared_ptr<const WorldFrame> frame() const {
        std::lock_guard<std::mutex> lock(mutex);
        return published;
    }
    std::vector<simulation::StartupAudioRequest> take_audio_requests() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<simulation::StartupAudioRequest> out;
        for (const auto &batch : audio_outputs)
            out.insert(out.end(), batch.begin(), batch.end());
        audio_outputs.clear(); // Allocation failure leaves the complete FIFO available.
        return out;
    }
    std::uint64_t submit(WorldCommand command) {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopping || published->failed)
            return 0;
        const auto serial = next_serial++;
        if (!command.generation)
            command.generation = published->generation;
        // Only adjacent, still-queued views can supersede one another. A pause, speed or
        // confirmation remains an ordering barrier; the latest serial also acknowledges the
        // superseded view requests when that final view reaches the worker's safe boundary.
        if (command.kind == WorldCommandKind::set_view && !commands.empty() &&
            commands.back().value.kind == WorldCommandKind::set_view)
            commands.back() = {serial, std::move(command)};
        else
            commands.push_back({serial, std::move(command)});
        wake.notify_one();
        return serial;
    }
    std::shared_ptr<const WorldFrame> wait_after(std::uint64_t revision,
                                                 std::chrono::milliseconds timeout) const {
        std::unique_lock<std::mutex> lock(mutex);
        publications.wait_for(lock, timeout,
                              [&] { return stopping || published->revision > revision; });
        return published;
    }
    void stop() {
        // Joining is serialized separately: no queue/snapshot lock spans world work or join.
        std::lock_guard<std::mutex> joining(join_mutex);
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
        }
        wake.notify_all();
        publications.notify_all();
        if (worker.joinable())
            worker.join();
    }

  private:
    struct QueuedCommand {
        std::uint64_t serial{};
        WorldCommand value;
    };
    mutable std::mutex mutex;
    std::mutex join_mutex;
    std::condition_variable wake;
    mutable std::condition_variable publications;
    std::deque<QueuedCommand> commands;
    std::shared_ptr<const WorldFrame> published;
    std::thread worker;
    std::chrono::milliseconds period{};
    std::uint64_t next_serial{1};
    bool stopping{};
    std::filesystem::path save_directory;
    // Fully prepared but unpublished transaction; retry never recomputes awards or randomness.
    std::shared_ptr<WorldFrame> pending_system;
    using AudioBatches = std::list<std::vector<simulation::StartupAudioRequest>>;
    AudioBatches audio_outputs;                 // Mutex-protected, independent of immutable frames.
    AudioBatches prepared_audio, pending_audio; // Worker-only transaction staging.
    std::optional<std::uint64_t> held_page; // Worker-only physical input binding, not source state.
    struct SaveRequest {
        int slot{};
        std::uint64_t page{}, generation{};
        bool await_menu{};
    };
    std::optional<SaveRequest> save_request;
    // Worker-only deduplication for completed GPU facts. It holds no business world state.
    struct BuildDrawSeen {
        std::uint64_t generation{}, epoch{};
        std::vector<std::uint64_t> pages;
    } build_draw_seen;

    void take_sound_outputs(WorldState &candidate, WorldFrame &frame) {
        if (candidate.sound_requests.empty())
            return;
        // Allocate before any durable write; final publication only splices list nodes.
        prepared_audio.emplace_back(candidate.sound_requests);
        frame.consumed_sound_requests += candidate.sound_requests.size();
        candidate.sound_requests.clear();
    }

    // Called after every successful input/update. A press cannot survive a new modal page,
    // or pause, even if a release event arrives late or the UI has lost focus.
    void synchronize_held(WorldState &state) {
        const auto *page = top_page(state);
        if (!page || !held_page || page->id != *held_page ||
            page->kind != simulation::rules::WorldScriptPageKind::raw_page ||
            page->legacy_page != 24 || state.scene.framework_paused)
            held_page.reset();
        state.page_confirm_held = held_page.has_value();
    }

    std::shared_ptr<const WorldFrame> apply_decision(const WorldFrame &current,
                                                     const QueuedCommand &input) {
        const auto &command = input.value;
        auto candidate = std::make_shared<WorldState>(*current.state);
        WorldCommandResult result;
        result.serial = input.serial;
        result.page = command.page;
        result.kind = command.kind;
        if (!current.state->rules) {
            result.runtime_error = RuntimeError::missing_source;
        } else if (command.kind == WorldCommandKind::open_main_menu) {
            result.runtime_error = simulation::open_startup_world_main_menu(*candidate);
        } else if (command.kind == WorldCommandKind::close_main_menu) {
            const auto *page = top_page(*candidate);
            if (!page || page->kind != simulation::rules::WorldScriptPageKind::raw_page ||
                page->legacy_page != 3)
                result.runtime_error = RuntimeError::invalid_page;
            else {
                simulation::StartupWorldMenuInput close;
                close.cancel = true;
                result.runtime_error =
                    simulation::input_startup_world_menu_page(*candidate, page->id, close);
            }
        } else if (command.kind == WorldCommandKind::open_menu_tasks ||
                   command.kind == WorldCommandKind::open_menu_build ||
                   command.kind == WorldCommandKind::open_menu_village_activities ||
                   command.kind == WorldCommandKind::open_menu_commerce ||
                   command.kind == WorldCommandKind::open_menu_information ||
                   command.kind == WorldCommandKind::open_menu_magic_pot) {
            // Retired desktop menu shortcuts cannot bypass the real source navigation.
            // A stable-world information shortcut remains useful to existing tooling.
            const auto *page = top_page(*candidate);
            if (command.kind == WorldCommandKind::open_menu_information && page &&
                page->kind == simulation::rules::WorldScriptPageKind::scene)
                detail::apply_world_decision(*candidate, command, result);
            else
                result.runtime_error = RuntimeError::invalid_page;
        } else if (save_request && command.kind != WorldCommandKind::acknowledge_page &&
                   command.kind != WorldCommandKind::cancel_page) {
            result.runtime_error = RuntimeError::invalid_page;
        } else if (command.kind == WorldCommandKind::build_menu_draw) {
            const bool duplicate =
                build_draw_seen.generation == current.generation &&
                build_draw_seen.epoch == command.presentation_epoch &&
                std::find(build_draw_seen.pages.begin(), build_draw_seen.pages.end(),
                          command.page) != build_draw_seen.pages.end();
            // The caller reports an actual completed paint, not a logical Update or a query.
            // Parent21 may be covered, but must still be a live, drawable source page.
            if (!command.presentation_epoch ||
                command.presentation_epoch != current.presentation_epoch || duplicate ||
                current.save_menu_open || current.state->scene.framework_paused)
                result.runtime_error = RuntimeError::invalid_page;
            else
                result.runtime_error =
                    simulation::draw_startup_world_build_menu(*candidate, command.page);
        } else if (command.kind == WorldCommandKind::page_confirm_held) {
            if (!command.held) {
                // A late release may clear only its own old press, never a newer page's press.
                if (held_page == command.page)
                    held_page.reset();
            } else {
                const auto *page = top_page(*candidate);
                if (candidate->scene.framework_paused || !page || page->id != command.page ||
                    page->kind != simulation::rules::WorldScriptPageKind::raw_page ||
                    page->legacy_page != 24)
                    result.runtime_error = RuntimeError::invalid_page;
                else
                    held_page = command.page;
            }
        } else {
            detail::apply_world_decision(*candidate, command, result);
        }
        result.outcome = result.runtime_error == RuntimeError::none &&
                                 result.denial == simulation::rules::TaskCommandDenial::none &&
                                 result.build_denial == simulation::StartupBuildDenial::none
                             ? WorldCommandOutcome::applied
                             : WorldCommandOutcome::rejected;
        auto next = current;
        next.command_results.push_back(result);
        if (next.command_results.size() > 64)
            next.command_results.erase(next.command_results.begin());
        if (result.runtime_error != RuntimeError::none &&
            result.runtime_error != RuntimeError::invalid_page)
            return fail(next,
                        "World decision input failed: error=" +
                            std::to_string(static_cast<int>(result.runtime_error)),
                        input.serial);
        if (result.runtime_error == RuntimeError::none) {
            // A source denial can legitimately open message11/63. Keep that complete source
            // candidate; do not charge anyway or roll back its valid feedback page.
            if (candidate) {
                synchronize_held(*candidate);
                if (command.kind != WorldCommandKind::page_confirm_held)
                    take_sound_outputs(*candidate, next);
                next.state = std::move(candidate);
            }
        }
        next.previous = next.state;
        next.last_command_serial = input.serial;
        ++next.revision;
        const bool draw = command.kind == WorldCommandKind::build_menu_draw;
        // Prepare bookkeeping before publication too: allocation failure cannot leave
        // a committed Draw whose token was never recorded for duplicate rejection.
        auto seen = build_draw_seen;
        if (draw && result.outcome == WorldCommandOutcome::applied) {
            if (seen.generation != current.generation || seen.epoch != command.presentation_epoch)
                seen = {current.generation, command.presentation_epoch, {}};
            seen.pages.push_back(command.page);
        }
        auto published_frame = publish(std::move(next), false, draw);
        if (draw && result.outcome == WorldCommandOutcome::applied &&
            published_frame->system_error.empty() && !published_frame->failed)
            build_draw_seen = std::move(seen);
        return published_frame;
    }

    std::shared_ptr<const WorldFrame> publish(WorldFrame next, bool force_system = false,
                                              bool source_draw = false) {
        const auto *visible = top_page(*next.state);
        next.main_menu_open = visible &&
                              visible->kind == simulation::rules::WorldScriptPageKind::raw_page &&
                              visible->legacy_page == 3;
        // Prepare every allocating publication before the final file replacement. Installation
        // afterward only swaps shared pointers; a failed write keeps the full old publication.
        next.published = Clock::now();
        auto result = std::make_shared<WorldFrame>(std::move(next));
        const auto before = frame();
        // A Draw acknowledgement must not create the next presentation opportunity.
        // Otherwise 60FPS painting and snapshot publication would drive one another forever.
        result->presentation_epoch = before->presentation_epoch;
        if (!source_draw &&
            (result->state != before->state || result->save_menu_open != before->save_menu_open))
            ++result->presentation_epoch;
        if ((result->state != before->state && result->generation == before->generation) ||
            force_system) {
            const auto error = commit_world_system(
                save_directory, *before->state, *result->state, result->system,
                force_system || (before->system.clear && !result->system.clear));
            if (!error.empty()) {
                pending_system = std::move(result);
                pending_audio.splice(pending_audio.end(), prepared_audio);
                result = std::make_shared<WorldFrame>(*before);
                result->previous = result->state;
                result->system_error = error;
                ++result->revision;
            } else {
                pending_system.reset();
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            audio_outputs.splice(audio_outputs.end(), prepared_audio);
            published = result;
        }
        publications.notify_all();
        return result;
    }
    std::shared_ptr<const WorldFrame> fail(const WorldFrame &current, const std::string &error,
                                           std::uint64_t serial = 0) {
        prepared_audio.clear();
        auto next = current;
        next.previous = next.state;
        next.failed = true;
        next.error = error;
        if (serial)
            next.last_command_serial = serial;
        ++next.revision;
        return publish(std::move(next));
    }
    // Each command is a separate transaction: earlier successes remain committed if a later
    // command fails. No stale-page retry or replacement confirmation is synthesized here.
    std::shared_ptr<const WorldFrame> apply(const WorldFrame &current, const QueuedCommand &input) {
        if (input.value.generation != current.generation) {
            auto next = current;
            WorldCommandResult result;
            result.serial = input.serial;
            result.kind = input.value.kind;
            result.outcome = WorldCommandOutcome::rejected;
            result.runtime_error = RuntimeError::invalid_page;
            next.command_results.push_back(result);
            if (next.command_results.size() > 64)
                next.command_results.erase(next.command_results.begin());
            next.last_command_serial = input.serial;
            next.previous = next.state;
            ++next.revision;
            return publish(std::move(next));
        }
        const auto kind = input.value.kind;
        if (save_request && save_request->await_menu && kind != WorldCommandKind::set_paused &&
            kind != WorldCommandKind::set_view && kind != WorldCommandKind::retry_system_write) {
            auto next = current;
            WorldCommandResult result;
            result.serial = input.serial;
            result.kind = kind;
            result.page = input.value.page;
            result.outcome = WorldCommandOutcome::rejected;
            result.runtime_error = RuntimeError::invalid_page;
            next.command_results.push_back(result);
            if (next.command_results.size() > 64)
                next.command_results.erase(next.command_results.begin());
            next.last_command_serial = input.serial;
            ++next.revision;
            return publish(std::move(next));
        }
        if (!current.system_error.empty()) {
            if (kind != WorldCommandKind::retry_system_write || !pending_system)
                return frame();
            auto retry = *pending_system;
            prepared_audio.splice(prepared_audio.end(), pending_audio);
            retry.system_error.clear();
            retry.last_command_serial = input.serial;
            retry.revision = current.revision + 1;
            return publish(std::move(retry), true);
        }
        if (kind == WorldCommandKind::retry_system_write)
            return frame();
        if (kind == WorldCommandKind::acknowledge_page && world_clear_page(*current.state)) {
            const auto *page = top_page(*current.state);
            auto next = current;
            next.last_command_serial = input.serial;
            ++next.revision;
            if (page->id != input.value.page || current.state->scene.framework_paused)
                return publish(std::move(next));
            auto candidate = std::make_shared<WorldState>(*current.state);
            const auto error = advance_world_clear(*candidate, next.system, true);
            if (!error.empty())
                return fail(current, error, input.serial);
            take_sound_outputs(*candidate, next);
            next.state = std::move(candidate);
            next.previous = next.state;
            return publish(std::move(next));
        }
        if (kind == WorldCommandKind::open_save_menu ||
            kind == WorldCommandKind::open_source_save_menu ||
            kind == WorldCommandKind::close_save_menu || kind == WorldCommandKind::save_slot ||
            kind == WorldCommandKind::load_slot)
            return apply_save(current, input);
        if (kind == WorldCommandKind::build_menu_draw)
            return apply_decision(current, input); // Overlay hiding is an explicit rejection.
        if (current.save_menu_open) {
            auto next = current;
            next.last_command_serial = input.serial;
            next.previous = next.state;
            ++next.revision;
            return publish(std::move(next));
        }
        const auto *current_page = top_page(*current.state);
        if (kind == WorldCommandKind::acknowledge_page && current_page &&
            (current_page->legacy_page == 3 || current_page->legacy_page == 4 ||
             current_page->legacy_page == 7 || current_page->legacy_page == 10 ||
             current_page->legacy_page == 13 || current_page->legacy_page == 14 ||
             current_page->legacy_page == 76 || current_page->legacy_page == 86 ||
             current_page->legacy_page == 72 || current_page->legacy_page == 79 ||
             current_page->legacy_page == 82 || current_page->legacy_page == 9 ||
             (current_page->legacy_page >= 34 && current_page->legacy_page <= 40) ||
             (current_page->legacy_page >= 41 && current_page->legacy_page <= 47)))
            return apply_decision(current,
                                  input); // Dedicated/automatic pages reject generic late clicks.
        if (detail::is_world_decision(input.value.kind))
            return apply_decision(current, input);
        const auto &command = input.value;
        const auto &old = *current.state;
        bool changed = true;
        switch (command.kind) {
        case WorldCommandKind::set_paused:
            changed = old.scene.framework_paused != command.paused ||
                      (command.paused && old.page_confirm_held);
            break;
        case WorldCommandKind::set_speed:
            if (command.speed != 0 && command.speed != 1)
                return fail(current, "World speed setting must be 0 or 1", input.serial);
            changed = old.scene.speed_setting != command.speed;
            break;
        case WorldCommandKind::set_view:
            if (!std::isfinite(command.camera[0]) || !std::isfinite(command.camera[1]) ||
                command.viewport[2] <= 0 || command.viewport[3] <= 0)
                return fail(current, "World view requires finite camera and positive extent",
                            input.serial);
            changed = old.camera != command.camera || old.reference_viewport != command.viewport;
            break;
        case WorldCommandKind::acknowledge_page:
        case WorldCommandKind::acknowledge_report:
            break;
        default:
            return fail(current, "Unknown world command", input.serial);
        }
        auto next = current;
        if (changed) {
            auto candidate = std::make_shared<WorldState>(old);
            switch (command.kind) {
            case WorldCommandKind::set_paused:
                candidate->scene.framework_paused = command.paused;
                break;
            case WorldCommandKind::set_speed:
                candidate->scene.speed_setting = command.speed;
                break;
            case WorldCommandKind::set_view:
                candidate->camera = command.camera;
                candidate->reference_viewport = command.viewport;
                break;
            case WorldCommandKind::acknowledge_page: {
                // Task choices require an explicit action/selection. In particular raw33's
                // source generic ack would select renewal; the desktop must not infer that.
                if (detail::is_decision_page(top_page(*candidate)) ||
                    candidate->task_abort_questions.count(command.page))
                    return fail(current, "Page decision requires an explicit action", input.serial);
                const auto error =
                    simulation::acknowledge_startup_world_runtime_page(*candidate, command.page);
                if (error != simulation::StartupWorldRuntimeError::none)
                    return fail(
                        current,
                        "World page confirmation rejected: page=" + std::to_string(command.page) +
                            " error=" + std::to_string(static_cast<int>(error)),
                        input.serial);
                break;
            }
            case WorldCommandKind::acknowledge_report:
                if (!acknowledge_world_report(*candidate, command.report_phase))
                    return fail(current,
                                "World report confirmation rejected: phase=" +
                                    std::to_string(command.report_phase),
                                input.serial);
                break;
            default:
                return fail(current, "Unexpected world input dispatch", input.serial);
            }
            synchronize_held(*candidate);
            if (command.kind == WorldCommandKind::acknowledge_page ||
                command.kind == WorldCommandKind::acknowledge_report)
                take_sound_outputs(*candidate, next);
            next.state = std::move(candidate);
        }
        // View/pause/input publications must not restart interpolation of an earlier movement.
        next.previous = next.state;
        next.last_command_serial = input.serial;
        ++next.revision;
        return publish(std::move(next));
    }
    std::shared_ptr<const WorldFrame> apply_save(const WorldFrame &current,
                                                 const QueuedCommand &input) {
        auto next = current;
        next.last_command_serial = input.serial;
        next.previous = next.state;
        next.save_message.clear();
        ++next.revision;
        const auto kind = input.value.kind;
        bool applied = false;
        const auto complete = [&]() {
            WorldCommandResult result;
            result.serial = input.serial;
            result.kind = kind;
            result.outcome = applied ? WorldCommandOutcome::applied : WorldCommandOutcome::rejected;
            result.runtime_error = applied ? RuntimeError::none : RuntimeError::invalid_page;
            next.command_results.push_back(result);
            if (next.command_results.size() > 64)
                next.command_results.erase(next.command_results.begin());
            return publish(std::move(next));
        };
        if (kind == WorldCommandKind::close_save_menu) {
            next.save_menu_open = false;
            next.save_source_page.reset();
            applied = current.save_menu_open;
            return complete();
        }
        std::string reason;
        const auto source_valid = [&](std::uint64_t page) {
            const auto *top = top_page(*current.state);
            const auto view = simulation::inspect_startup_world_menu_page(*current.state, page);
            return top && top->id == page && top->lifecycle == 2 && view && view->raw == 10 &&
                   !current.state->scene.framework_paused && view->selection >= 0 &&
                   static_cast<std::size_t>(view->selection) < view->tags.size() &&
                   view->tags[view->selection] == 20;
        };
        const auto source = kind == WorldCommandKind::open_source_save_menu
                                ? std::optional<std::uint64_t>{input.value.page}
                                : current.save_source_page;
        if (save_request ||
            (source ? !source_valid(*source) : !world_save_eligible(*current.state, &reason))) {
            next.save_message = "当前世界尚不能存读档：" + reason;
            return complete();
        }
        try {
            if (save_directory.empty())
                save_directory = default_world_save_directory();
            if (kind == WorldCommandKind::open_save_menu ||
                kind == WorldCommandKind::open_source_save_menu) {
                next.save_slots = inspect_world_save_slots(save_directory);
                next.main_menu_open = false;
                next.save_menu_open = true;
                next.save_source_page = source;
                held_page.reset();
                applied = true;
                return complete();
            }
            const int slot = input.value.selection;
            if (!current.save_menu_open || slot < 0 || slot >= 2) {
                next.save_message = "无效的存档操作";
                return complete();
            }
            next.save_busy = true;
            auto busy = next;
            busy.last_command_serial = current.last_command_serial;
            publish(std::move(busy)); // Completion is acknowledged only after file I/O finishes.
            if (kind == WorldCommandKind::save_slot) {
                if (current.state->scene.framework_paused) {
                    next.save_busy = false;
                    next.save_message = "请恢复游戏后保存";
                    return complete();
                }
                if (current.state->build_mode != 0) {
                    next.save_busy = false;
                    next.save_message = "当前建设工具状态不满足保存页导出条件";
                    return complete();
                }
                auto candidate = std::make_shared<WorldState>(*current.state);
                RuntimeError error;
                if (source) {
                    simulation::StartupWorldMenuInput confirm;
                    confirm.confirm = true;
                    error = simulation::input_startup_world_menu_page(*candidate, *source, confirm);
                } else {
                    error = simulation::open_startup_world_main_menu(*candidate);
                }
                const auto *opened = top_page(*candidate);
                if (error != RuntimeError::none || !opened ||
                    opened->legacy_page != (source ? 14 : 3)) {
                    next.save_message = "保存入口已失效";
                } else {
                    take_sound_outputs(*candidate, next);
                    save_request = SaveRequest{slot, opened->id, current.generation, !source};
                    next.state = std::move(candidate);
                    next.previous = next.state;
                    next.save_menu_open = false;
                    next.save_source_page.reset();
                    applied = true;
                }
            } else {
                if (source) {
                    next.save_busy = false;
                    next.save_message = "当前入口只允许保存";
                    return complete();
                }
                auto loaded = read_world_save_slot(save_directory, slot);
                if (!loaded.state)
                    next.save_message =
                        std::string("读取失败：") + world_save_error_text(loaded.error);
                else if (prepare_world_save_candidate(*loaded.state, *current.state, reason) !=
                         WorldSaveError::none)
                    next.save_message = "读取失败：存档中的世界数据无效";
                else {
                    loaded.state->cash_peak = next.system.records.cash_peak;
                    loaded.state->cash_peak_village = next.system.records.cash_village;
                    // Loading an older world is not a new cross-game record or inheritance event.
                    next.system.clear.reset();
                    loaded.state->sound_requests.push_back(
                        {simulation::StartupAudioOperation::replace_bgm,
                         loaded.state->active_task && loaded.state->task.encounter ? 2 : 1});
                    take_sound_outputs(*loaded.state, next);
                    auto replacement = std::make_shared<const WorldState>(std::move(*loaded.state));
                    std::string success = "读取完毕";
                    next.save_message = std::move(success);
                    // All allocating preparation precedes this candidate commit.
                    next.state = std::move(replacement);
                    next.previous = next.state;
                    ++next.generation;
                    next.command_results.clear();
                    held_page.reset();
                    next.save_menu_open = false;
                    next.save_source_page.reset();
                    next.main_menu_open = false;
                    applied = true;
                }
            }
        } catch (const std::exception &) {
            if (applied && kind == WorldCommandKind::save_slot)
                next.save_message = "保存完毕，目录暂无法刷新";
            else {
                prepared_audio.clear();
                next.consumed_sound_requests = current.consumed_sound_requests;
                next.state = current.state;
                next.previous = current.state;
                next.generation = current.generation;
                next.save_message = "存档文件操作失败，当前世界已保留";
                applied = false;
            }
        }
        next.save_busy = false;
        ++next.revision;
        return complete();
    }
    std::shared_ptr<const WorldFrame> update(const WorldFrame &current, double interval) {
        const auto started = Clock::now();
        if (save_request && save_request->generation == current.generation) {
            const auto *page = top_page(*current.state);
            if (!page || page->id != save_request->page)
                return fail(current, "Save request lost its source page");
            if (save_request->await_menu && page->lifecycle == 2 &&
                simulation::inspect_startup_world_menu_page(*current.state, page->id)) {
                auto candidate = std::make_shared<WorldState>(*current.state);
                // F5 is an explicitly approved application entry, transported through
                // the same ready raw3 callback that owns the source save request.
                candidate->scripts.executing_page = page->id;
                const auto error = simulation::open_startup_world_save_page(*candidate);
                candidate->scripts.executing_page.reset();
                const auto *opened = top_page(*candidate);
                if (error != RuntimeError::none || !opened || opened->legacy_page != 14)
                    return fail(current, "Stable save entry rejected by source callback");
                save_request->page = opened->id;
                save_request->await_menu = false;
                auto next = current;
                take_sound_outputs(*candidate, next);
                next.state = std::move(candidate);
                next.previous = next.state;
                ++next.revision;
                return publish(std::move(next));
            }
            if (!save_request->await_menu) {
                const auto view =
                    simulation::inspect_startup_world_save_page(*current.state, page->id);
                if (view && view->stage == 1)
                    return finish_save(current);
            }
        }
        if (world_clear_page(*current.state)) {
            auto next = current;
            auto candidate = std::make_shared<WorldState>(*current.state);
            const auto error = advance_world_clear(*candidate, next.system, false);
            if (!error.empty())
                return fail(current, error);
            take_sound_outputs(*candidate, next);
            next.state = std::move(candidate);
            next.previous = next.state;
            next.interval_seconds = interval;
            ++next.revision;
            ++next.outer_updates;
            return publish(std::move(next));
        }
        auto result = simulation::prepare_startup_world_runtime(*current.state);
        if (result.candidate)
            synchronize_held(*result.candidate);
        const bool valid =
            result.candidate && simulation::update_startup_world_render_cache(*result.candidate);
        const auto elapsed =
            std::chrono::duration<double, std::milli>(Clock::now() - started).count();
        auto next = current;
        next.max_update_ms = std::max(next.max_update_ms, elapsed);
        if (!valid)
            return fail(next, result.candidate ? "World render-cache update rejected"
                                               : update_error(result));
        next.previous = current.state;
        take_sound_outputs(*result.candidate, next);
        next.state = std::make_shared<const WorldState>(std::move(*result.candidate));
        next.interval_seconds = interval;
        ++next.revision;
        ++next.outer_updates;
        return publish(std::move(next));
    }
    std::shared_ptr<const WorldFrame> finish_save(const WorldFrame &current) {
        const auto request = *save_request;
        auto exported = simulation::prepare_startup_world_save_export(*current.state, request.page);
        auto success =
            simulation::complete_startup_world_save_page(*current.state, request.page, true);
        auto failure = simulation::complete_startup_world_save_page(
            *current.state, request.page, false, "玩家存档写入失败，原档已保留");
        if (!exported || !success || !failure ||
            !simulation::update_startup_world_render_cache(*success) ||
            !simulation::update_startup_world_render_cache(*failure) ||
            !success->sound_requests.empty() || !failure->sound_requests.empty())
            return fail(current, "Source save export or completion candidate rejected");
        auto capture = capture_world_save(*exported);
        // Allocate both immutable result publications before the irreversible file replace.
        // raw14 changes no business, random, cross-game record or audio output; publication
        // therefore only installs the prebuilt result, without a second durable transaction.
        auto make_result = [&](WorldState state, bool saved) {
            auto result = std::make_shared<WorldFrame>(current);
            result->state = std::make_shared<const WorldState>(std::move(state));
            result->previous = result->state;
            result->main_menu_open = false;
            result->save_busy = false;
            result->save_message = saved ? "保存完毕" : "保存失败，原存档已保留";
            result->published = Clock::now();
            ++result->revision;
            ++result->outer_updates;
            ++result->presentation_epoch;
            return result;
        };
        auto good = make_result(std::move(*success), true);
        auto bad = make_result(std::move(*failure), false);
        if (capture.image) {
            auto &slot = good->save_slots[static_cast<std::size_t>(request.slot)];
            slot.exists = true;
            slot.metadata = capture.image->metadata;
            slot.error = WorldSaveError::none;
            slot.message.clear();
        }
        const bool saved =
            capture.image &&
            write_world_save_slot(save_directory, request.slot, *capture.image).error ==
                WorldSaveError::none;
        const auto result = saved ? std::move(good) : std::move(bad);
        save_request.reset();
        {
            std::lock_guard<std::mutex> lock(mutex);
            published = result;
        }
        publications.notify_all();
        return result;
    }
    void run() {
        auto current = frame();
        auto last_start = Clock::now();
        auto deadline = last_start + period;
        try {
            for (;;) {
                std::deque<QueuedCommand> pending;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    if (current->failed)
                        wake.wait(lock, [&] { return stopping; });
                    else
                        wake.wait_until(lock, deadline,
                                        [&] { return stopping || !commands.empty(); });
                    if (stopping)
                        return;
                    pending.swap(commands);
                }
                if (!pending.empty()) {
                    for (const auto &command : pending) {
                        {
                            std::lock_guard<std::mutex> lock(mutex);
                            if (stopping)
                                return;
                        }
                        try {
                            current = apply(*current, command);
                        } catch (const std::exception &error) {
                            current = fail(*current, error.what(), command.serial);
                        } catch (...) {
                            current =
                                fail(*current, "Unknown world command exception", command.serial);
                        }
                        if (current->failed)
                            break;
                    }
                    if (current->failed)
                        continue;
                }
                // A finite batch is the current input boundary. New input waits for the next
                // boundary, so continuous dragging cannot starve an already-due runtime call.
                // Commands in the batch (especially pause) still commit before this gate.
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (stopping)
                        return;
                }
                const auto now = Clock::now();
                if (now < deadline)
                    continue;
                const auto interval = std::chrono::duration<double>(now - last_start).count();
                last_start = now;
                deadline = now + period; // Adopt actual start; a slow call creates no tick debt.
                if (current->system_error.empty() && !current->save_menu_open &&
                    !current->state->scene.framework_paused)
                    current = update(*current, interval);
            }
        } catch (const std::exception &error) {
            fail(*current, error.what());
        } catch (...) {
            fail(*current, "Unknown world worker exception");
        }
    }
};

WorldSession::WorldSession(WorldState initial, std::filesystem::path save_directory)
    : impl_(std::make_unique<Impl>(std::move(initial), std::move(save_directory))) {}
WorldSession::~WorldSession() = default;
std::shared_ptr<const WorldFrame> WorldSession::frame() const { return impl_->frame(); }
std::uint64_t WorldSession::submit(WorldCommand command) {
    return impl_->submit(std::move(command));
}
std::uint64_t WorldSession::ack_page(std::uint64_t page) {
    WorldCommand command;
    command.kind = WorldCommandKind::acknowledge_page;
    command.page = page;
    return submit(command);
}
std::uint64_t WorldSession::ack_report(int expected_phase) {
    WorldCommand command;
    command.kind = WorldCommandKind::acknowledge_report;
    command.report_phase = expected_phase;
    return submit(command);
}
std::uint64_t WorldSession::act_award(std::uint64_t page,
                                      simulation::rules::WorldAwardAction action, int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::award_action;
    command.page = page;
    command.award_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::open_main_menu() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_main_menu;
    return submit(command);
}
std::uint64_t WorldSession::close_main_menu() {
    WorldCommand command;
    command.kind = WorldCommandKind::close_main_menu;
    return submit(command);
}
std::uint64_t WorldSession::act_menu(std::uint64_t page,
                                     const simulation::StartupWorldMenuInput &input) {
    WorldCommand command;
    command.kind = WorldCommandKind::menu_input;
    command.page = page;
    command.menu_input = input;
    return submit(command);
}
std::uint64_t WorldSession::act_manual(std::uint64_t page,
                                       const simulation::StartupManualInput &input) {
    WorldCommand command;
    command.kind = WorldCommandKind::manual_input;
    command.page = page;
    command.manual_input = input;
    return submit(command);
}
std::uint64_t WorldSession::open_source_save_menu(std::uint64_t page) {
    WorldCommand command;
    command.kind = WorldCommandKind::open_source_save_menu;
    command.page = page;
    return submit(command);
}
std::uint64_t WorldSession::open_save_menu() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_save_menu;
    return submit(command);
}
std::uint64_t WorldSession::close_save_menu() {
    WorldCommand command;
    command.kind = WorldCommandKind::close_save_menu;
    return submit(command);
}
std::uint64_t WorldSession::save_slot(int slot) {
    WorldCommand command;
    command.kind = WorldCommandKind::save_slot;
    command.selection = slot;
    return submit(command);
}
std::uint64_t WorldSession::load_slot(int slot) {
    WorldCommand command;
    command.kind = WorldCommandKind::load_slot;
    command.selection = slot;
    return submit(command);
}
std::uint64_t WorldSession::open_menu_tasks() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_menu_tasks;
    return submit(command);
}
std::uint64_t WorldSession::open_task_menu() {
    WorldCommand command;
    command.kind = WorldCommandKind::open_task_menu;
    return submit(command);
}
std::uint64_t WorldSession::act_task_page(std::uint64_t page,
                                          simulation::StartupWorldTaskAction action,
                                          int selection) {
    WorldCommand command;
    command.kind = WorldCommandKind::task_action;
    command.page = page;
    command.task_action = action;
    command.selection = selection;
    return submit(command);
}
std::uint64_t WorldSession::set_page_confirm_held(std::uint64_t page, bool held) {
    WorldCommand command;
    command.kind = WorldCommandKind::page_confirm_held;
    command.page = page;
    command.held = held;
    return submit(command);
}
std::uint64_t WorldSession::cancel_page(std::uint64_t page) {
    WorldCommand command;
    command.kind = WorldCommandKind::cancel_page;
    command.page = page;
    return submit(command);
}
std::uint64_t WorldSession::set_paused(bool paused) {
    WorldCommand command;
    command.kind = WorldCommandKind::set_paused;
    command.paused = paused;
    return submit(command);
}
std::uint64_t WorldSession::set_speed(int setting) {
    WorldCommand command;
    command.kind = WorldCommandKind::set_speed;
    command.speed = setting;
    return submit(command);
}
std::uint64_t WorldSession::set_view(std::array<float, 2> camera, std::array<int, 4> viewport) {
    WorldCommand command;
    command.kind = WorldCommandKind::set_view;
    command.camera = camera;
    command.viewport = viewport;
    return submit(command);
}
std::shared_ptr<const WorldFrame>
WorldSession::wait_for_frame_after(std::uint64_t revision,
                                   std::chrono::milliseconds timeout) const {
    return impl_->wait_after(revision, timeout);
}
void WorldSession::stop() { impl_->stop(); }
std::vector<simulation::StartupAudioRequest> WorldSession::take_audio_requests() {
    return impl_->take_audio_requests();
}
} // namespace ark::app
