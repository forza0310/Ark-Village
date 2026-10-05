#pragma once

// The desktop reads immutable publications; only the worker commits the canonical world.
#include "ark/simulation/startup_world_runtime.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace ark::app {
using WorldState = simulation::StartupWorldRuntimeState;
enum class WorldCommandKind {
    acknowledge_page,
    acknowledge_report,
    award_action,
    open_task_menu,
    task_action,
    page_confirm_held,
    cancel_page,
    set_paused,
    set_speed,
    set_view
};
enum class WorldCommandOutcome { applied, rejected };
struct WorldCommandResult {
    std::uint64_t serial{};
    std::uint64_t page{};
    WorldCommandKind kind{WorldCommandKind::open_task_menu};
    WorldCommandOutcome outcome{WorldCommandOutcome::applied};
    simulation::StartupWorldRuntimeError runtime_error{simulation::StartupWorldRuntimeError::none};
    simulation::rules::TaskCommandDenial denial{simulation::rules::TaskCommandDenial::none};
    bool task_accepted{};
    bool departed{};
};
struct WorldFrame {
    std::shared_ptr<const WorldState> state;
    std::shared_ptr<const WorldState> previous;
    std::chrono::steady_clock::time_point published;
    double interval_seconds{.047};
    std::uint64_t revision{};
    std::uint64_t last_command_serial{};
    bool failed{};
    std::string error;
    std::uint64_t outer_updates{};
    double max_update_ms{};
    // Last 64 task/cancel/held input results, retained across ticks and camera publications.
    // This bounded FIFO acknowledgement history is not a gameplay event log.
    std::vector<WorldCommandResult> command_results;
};

struct WorldCommand {
    WorldCommandKind kind{WorldCommandKind::set_paused};
    std::uint64_t page{};
    int report_phase{};
    simulation::rules::WorldAwardAction award_action{simulation::rules::WorldAwardAction::update};
    simulation::StartupWorldTaskAction task_action{simulation::StartupWorldTaskAction::confirm};
    int selection{};
    bool held{};
    bool paused{};
    int speed{};
    std::array<float, 2> camera{};
    std::array<int, 4> viewport{};
};

// Commands enter a FIFO and commit only between complete source-runtime updates. The worker
// uses the source minimum start interval, never render FPS, catch-up debt or a second speed
// multiplier. Destruction wakes and joins it; an in-flight atomic update finishes first.
class WorldSession {
  public:
    explicit WorldSession(WorldState initial);
    ~WorldSession();
    WorldSession(const WorldSession &) = delete;
    WorldSession &operator=(const WorldSession &) = delete;

    std::shared_ptr<const WorldFrame> frame() const;
    // Zero means the session has stopped/failed and did not accept the command. Accepted
    // serials are strictly increasing; a failed command is acknowledged by the failure frame.
    std::uint64_t submit(WorldCommand command);
    std::uint64_t ack_page(std::uint64_t page);
    std::uint64_t ack_report(int expected_phase);
    // Annual-page input is explicit: ordinary page confirmation never chooses termination.
    std::uint64_t act_award(std::uint64_t page, simulation::rules::WorldAwardAction action);
    std::uint64_t open_task_menu();
    std::uint64_t act_task_page(std::uint64_t page, simulation::StartupWorldTaskAction action,
                                int selection = 0);
    // A held edge belongs only to this raw24 identity. Send false on release/focus loss;
    // page transitions and pause also clear it. Source page updates, never FPS, consume it.
    std::uint64_t set_page_confirm_held(std::uint64_t page, bool held);
    std::uint64_t cancel_page(std::uint64_t page); // Explicit raw83 Back, not ordinary confirm.
    std::uint64_t set_paused(bool paused);
    std::uint64_t set_speed(int setting);
    std::uint64_t set_view(std::array<float, 2> camera, std::array<int, 4> viewport);
    // Useful to headless observers/tests: wait for a newer publication without polling/sleeping.
    // Timeout or stop returns the latest publication, which may retain the supplied revision.
    std::shared_ptr<const WorldFrame> wait_for_frame_after(std::uint64_t revision,
                                                           std::chrono::milliseconds timeout) const;
    void stop();

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace ark::app
