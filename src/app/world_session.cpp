#include "ark/app/world_session.hpp"
#include "ark/app/original_loop.hpp"
#include "ark/app/world_report.hpp"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace ark::app {
namespace {
using Clock = std::chrono::steady_clock;
std::string update_error(const simulation::StartupWorldRuntimeResult &result) {
    return "World update rejected: runtime=" + std::to_string(static_cast<int>(result.error)) +
           " scene=" + std::to_string(static_cast<int>(result.scene_error)) +
           " world=" + std::to_string(static_cast<int>(result.world_error));
}
} // namespace

class WorldSession::Impl {
  public:
    explicit Impl(WorldState initial) {
        const auto gate = query_original_loop_wait({}, 0);
        if (!gate || gate->minimum_period_ms <= 0)
            throw std::logic_error("Invalid source world update period");
        period = std::chrono::milliseconds(gate->minimum_period_ms);
        auto initial_frame = std::make_shared<WorldFrame>();
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
    std::uint64_t submit(WorldCommand command) {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopping || published->failed)
            return 0;
        const auto serial = next_serial++;
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

    std::shared_ptr<const WorldFrame> publish(WorldFrame next) {
        next.published = Clock::now();
        auto result = std::make_shared<const WorldFrame>(std::move(next));
        {
            std::lock_guard<std::mutex> lock(mutex);
            published = result;
        }
        publications.notify_all();
        return result;
    }
    std::shared_ptr<const WorldFrame> fail(const WorldFrame &current, const std::string &error,
                                           std::uint64_t serial = 0) {
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
        const auto &command = input.value;
        const auto &old = *current.state;
        bool changed = true;
        switch (command.kind) {
        case WorldCommandKind::set_paused:
            changed = old.scene.framework_paused != command.paused;
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
        case WorldCommandKind::award_action:
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
            case WorldCommandKind::award_action: {
                const auto error = simulation::act_startup_world_runtime_award_page(
                    *candidate, command.page, command.award_action);
                if (error != simulation::StartupWorldRuntimeError::none)
                    return fail(
                        current,
                        "World award action rejected: page=" + std::to_string(command.page) +
                            " action=" + std::to_string(static_cast<int>(command.award_action)) +
                            " error=" + std::to_string(static_cast<int>(error)),
                        input.serial);
                break;
            }
            }
            next.state = std::move(candidate);
        }
        // View/pause/input publications must not restart interpolation of an earlier movement.
        next.previous = next.state;
        next.last_command_serial = input.serial;
        ++next.revision;
        return publish(std::move(next));
    }
    std::shared_ptr<const WorldFrame> update(const WorldFrame &current, double interval) {
        const auto started = Clock::now();
        auto result = simulation::prepare_startup_world_runtime(*current.state);
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
        next.state = std::make_shared<const WorldState>(std::move(*result.candidate));
        next.interval_seconds = interval;
        ++next.revision;
        ++next.outer_updates;
        return publish(std::move(next));
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
                if (!current->state->scene.framework_paused &&
                    !world_report_waiting(*current->state))
                    current = update(*current, interval);
            }
        } catch (const std::exception &error) {
            fail(*current, error.what());
        } catch (...) {
            fail(*current, "Unknown world worker exception");
        }
    }
};

WorldSession::WorldSession(WorldState initial)
    : impl_(std::make_unique<Impl>(std::move(initial))) {}
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
                                      simulation::rules::WorldAwardAction action) {
    WorldCommand command;
    command.kind = WorldCommandKind::award_action;
    command.page = page;
    command.award_action = action;
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
} // namespace ark::app
