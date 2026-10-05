#pragma once

// The desktop reads immutable publications; only the worker commits the canonical world.
#include "ark/simulation/startup_world_runtime.hpp"

#include <chrono>
#include <memory>
#include <string>

namespace ark::app {
using WorldState = simulation::StartupWorldRuntimeState;
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
};

enum class WorldCommandKind {
    acknowledge_page,
    acknowledge_report,
    award_action,
    set_paused,
    set_speed,
    set_view
};
struct WorldCommand {
    WorldCommandKind kind{WorldCommandKind::set_paused};
    std::uint64_t page{};
    int report_phase{};
    simulation::rules::WorldAwardAction award_action{simulation::rules::WorldAwardAction::update};
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
