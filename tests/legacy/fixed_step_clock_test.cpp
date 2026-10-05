// Compare real initial AI at equal elapsed time under different rendering schedules.
#include "ark/app/fixed_step_clock.hpp"
#include "ark/app/game.hpp"
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
Game initial() {
    Game game(20261004, PlayMode::ai_preview);
    for (int n = 0; n < 420; ++n)
        game.update();
    game.acknowledge_talk();
    game.acknowledge_talk();
    game.finish_camera();
    return game;
}
void equal(const Game &left, const Game &right) {
    const auto &a = *left.ai_state();
    const auto &b = *right.ai_state();
    check(a.rounds == b.rounds && a.position.x == b.position.x && a.position.z == b.position.z &&
              a.waypoint == b.waypoint && a.control.state == b.control.state &&
              a.control.queue == b.control.queue && a.counters.state == b.counters.state &&
              a.arrivals == b.arrivals && a.completions == b.completions &&
              a.accounting.entries() == b.accounting.entries() &&
              left.state().money == right.state().money &&
              left.state().adventurer->hp == right.state().adventurer->hp &&
              left.state().adventurer->equipment == right.state().adventurer->equipment,
          "equal elapsed time preserves real position, AI counters, service, cash and equipment");
}
void schedules() {
    for (const int rate : {20, 60})
        for (const int speed : {1, 2}) {
            auto expected = initial();
            for (int n = 0; n < rate * 2; ++n)
                expected.update(speed);
            for (const int fps : {30, 60, 144}) {
                auto game = initial();
                FixedStepClock clock(rate);
                check(clock.advance(0, true) == 0, "first interval is discarded");
                int updates{};
                for (int frame = 0; frame < fps * 2; ++frame) {
                    const auto due = clock.advance(1.0 / fps, true);
                    for (int n = 0; n < due; ++n) {
                        game.update(speed);
                        ++updates;
                    }
                }
                check(updates == rate * 2, "logic frequency is independent of 30/60/144 FPS");
                equal(game, expected);
            }
            auto irregular = initial();
            FixedStepClock clock(rate);
            clock.advance(0, true);
            for (int n = 0; n < 20; ++n)
                for (double elapsed : {0.005, 0.035, 0.06})
                    for (int tick = clock.advance(elapsed, true); tick > 0; --tick)
                        irregular.update(speed);
            equal(irregular, expected);
        }
}
void blocking_and_stalls() {
    FixedStepClock clock(20);
    clock.advance(0, true);
    check(clock.advance(0.025, true) == 0, "fraction accumulates without movement");
    check(clock.advance(10, false) == 0, "pause/modal do not accumulate time");
    check(clock.advance(100, true) == 0, "resume drops blocked interval");
    check(clock.advance(0.025, true) == 0 && clock.advance(0.025, true) == 1,
          "resume requires fresh full interval");
    check(clock.advance(60, true) == 8, "debugger stall has bounded catch-up");
    check(clock.advance(0, true) == 0 && clock.advance(0.05, true) == 1,
          "discarded stall debt cannot cause later catch-up");
    clock.advance(0.025, true);
    clock.reset();
    check(clock.advance(0.025, true) == 0 && clock.advance(0.025, true) == 0 &&
              clock.advance(0.025, true) == 1,
          "new game clears fractional debt");

    auto paused = initial();
    const auto before = paused;
    paused.set_paused(true);
    for (int n = 0; n < 60; ++n)
        check(clock.advance(1, !paused.state().paused) == 0, "paused Game blocks clock");
    equal(paused, before);
    paused.set_paused(false);
    check(clock.advance(30, true) == 0, "real Game resume cannot replay paused duration");

    // A catch-up batch can open a modal; its remaining steps must not advance the world.
    Game startup;
    for (int n = 0; n < 419; ++n)
        startup.update();
    clock.reset();
    clock.advance(0, true);
    const auto due = clock.advance(1, true);
    for (int n = 0; n < due && startup.state().mode == Mode::normal; ++n)
        startup.update();
    check(startup.state().simulation_steps == 420 && startup.state().event89_count == 1 &&
              startup.state().mode == Mode::tutorial,
          "first arrival opens tutorial and stops remaining catch-up work");
    check(clock.advance(1, false) == 0, "new modal clears time debt");
}
} // namespace
int main() {
    try {
        rejects([] { FixedStepClock clock(0); });
        rejects([] { FixedStepClock clock(241); });
        FixedStepClock clock(60);
        rejects([&] { clock.advance(-1, true); });
        rejects([&] { clock.advance(std::numeric_limits<double>::infinity(), true); });
        rejects([&] { clock.advance(std::numeric_limits<double>::quiet_NaN(), false); });
        schedules();
        blocking_and_stalls();
        std::cout
            << "PASS fixed-step FPS independence, actual AI, 2x, pause/modal/reset and stalls\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
