// Platform integration: original gate deadlines remain independent of render deadlines.
#include "ark/app/game.hpp"
#include "ark/app/simulation_clock.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace ark::app;
void check(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
template <class F> void rejects(F action) {
    try {
        action();
    } catch (const std::invalid_argument &) {
        return;
    }
    throw std::runtime_error("Invalid clock input accepted");
}
void original_gate() {
    SimulationClock clock;
    check(clock.original_pacing() && clock.remaining_seconds(0) == 0,
          "default uses source gate and first observation seeds it");
    check(clock.advance(0, true) == 0 && clock.advance(0.046, true) == 0 &&
              clock.advance(0.047, true) == 1,
          "original integer gate admits exactly at47ms");
    check(clock.advance(10, true) == 1 && clock.advance(10, true) == 0 &&
              clock.advance(10.046, true) == 0 && clock.advance(10.0471, true) == 1,
          "stall adopts observed start with no catch-up or residual debt");
    check(clock.advance(20, false) == 0 && clock.advance(20.01, true) == 0 &&
              clock.advance(20.0471, true) == 1,
          "blocked scene still advances gate, resume does not replay blocked work");
    clock.reset();
    check(clock.advance(30, false) == 0 && clock.advance(30.0471, false) == 0 &&
              clock.advance(30.048, true) == 0 && clock.advance(30.0941, true) == 1,
          "new game seeds gate and repeated modals preserve fresh deadlines");
    check(clock.advance(29, true) == 0 && clock.remaining_seconds(29) > 1 &&
              clock.advance(30.1411, true) == 1,
          "source gate tolerates clock rollback by extending wait");
}
void fixed_override() {
    for (int rate : {20, 60}) {
        SimulationClock clock(rate);
        const auto interval = 1.0 / rate;
        check(!clock.original_pacing() && clock.advance(0, true) == 0,
              "explicit frequency seeds fixed accumulator");
        check(std::abs(clock.remaining_seconds(interval / 2) - interval / 2) < 1e-12,
              "deadline includes fresh time since last observation");
        check(clock.advance(interval, true) == 1 && clock.advance(100, true) == 8 &&
                  clock.advance(100, true) == 0,
              "explicit experiments retain bounded catch-up and discard excess debt");
        check(clock.advance(101, false) == 0 && std::isinf(clock.remaining_seconds(102)) &&
                  clock.advance(110, true) == 0 && clock.advance(110 + interval, true) == 1,
              "fixed pause clears debt, resume needs one fresh interval");
        rejects([&] { clock.advance(109, false); });
        check(!std::isinf(clock.remaining_seconds(110 + interval)),
              "rejected backward fixed observation preserves eligibility");
        clock.reset();
        check(clock.remaining_seconds(200) == 0 && clock.advance(200, true) == 0,
              "fixed reset clears observation and accumulated debt");
    }
}
Game initial() {
    Game game(20261004, PlayMode::ai_preview);
    for (int n = 0; n < 420; ++n)
        game.update();
    game.acknowledge_talk();
    game.acknowledge_talk();
    game.finish_camera();
    return game;
}
void equal(const Game &a, const Game &b) {
    const auto &x = *a.ai_state();
    const auto &y = *b.ai_state();
    check(x.rounds == y.rounds && x.position.x == y.position.x && x.position.z == y.position.z &&
              x.waypoint == y.waypoint && x.control.state == y.control.state &&
              x.control.queue == y.control.queue && x.counters.state == y.counters.state &&
              x.arrivals == y.arrivals && x.completions == y.completions &&
              x.accounting.entries() == y.accounting.entries() &&
              a.state().money == b.state().money &&
              a.state().adventurer->hp == b.state().adventurer->hp &&
              a.state().adventurer->equipment == b.state().adventurer->equipment,
          "equal admitted rounds preserve real AI position, service, funds and equipment");
}
void schedules() {
    for (int speed : {1, 2}) {
        auto expected = initial();
        for (int n = 0; n < 42; ++n)
            expected.update(speed);
        for (int fps : {30, 60, 144}) {
            auto game = initial();
            SimulationClock clock;
            clock.advance(0, true);
            double now{}, next_render = 1.0 / fps;
            int updates{}, renders{};
            // Wake on the earliest deadline, with a small scheduling delay like an OS sleep.
            // This verifies logical wakeups between renders, not polling only once per frame.
            while (true) {
                now += std::min(next_render - now, clock.remaining_seconds(now)) + 0.00001;
                if (now > 2)
                    break;
                const auto due = clock.advance(now, true);
                for (int n = 0; n < due; ++n) {
                    game.update(speed);
                    ++updates;
                }
                if (now >= next_render) {
                    ++renders;
                    next_render = now + 1.0 / fps;
                }
            }
            check(updates == 42 && renders >= fps * 2 - 1,
                  "47ms logic not quantized to three60FPS frames or other render rates");
            equal(game, expected);
        }
    }
    Game startup;
    for (int n = 0; n < 419; ++n)
        startup.update();
    SimulationClock clock;
    clock.advance(0, true);
    const auto due = clock.advance(0.047, true);
    for (int n = 0; n < due; ++n)
        startup.update(2);
    check(startup.state().simulation_steps == 420 && startup.state().mode == Mode::tutorial &&
              startup.state().event89_count == 1,
          "two-times outer update rechecks qualification after first iteration opens tutorial");
    check(clock.advance(10, false) == 0 && clock.advance(10.01, true) == 0,
          "new modal continues pacing without accumulated domain work");
}
} // namespace
int main() {
    try {
        rejects([] { SimulationClock clock(-1); });
        rejects([] { SimulationClock clock(241); });
        for (int rate : {0, 20}) {
            SimulationClock clock(rate);
            for (double bad :
                 {-1.0, std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::max()}) {
                rejects([&] { clock.advance(bad, true); });
                rejects([&] { clock.remaining_seconds(bad); });
            }
        }
        original_gate();
        fixed_override();
        schedules();
        std::cout << "PASS original pacing, independent rendering, real AI, pause and overrides\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
