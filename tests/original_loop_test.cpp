// Adapted from published research 1e50a60/c4ce4b2; independent product build.
#include "ark/app/original_loop.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::app;
namespace {
int checks{};
void check(bool v, const char *message) {
    ++checks;
    if (!v)
        throw std::runtime_error(message);
}
void intervals() {
    for (int parameter = 0; parameter < 1000; ++parameter)
        for (const int elapsed : {0, 1, 46, 47, 48, 1000, 5000}) {
            const OriginalLoopPacing c{10000, parameter};
            const auto wait = query_original_loop_wait(c, 10000 + elapsed);
            const int period = 1000 / (parameter + 1);
            check(wait && wait->minimum_period_ms == period &&
                      wait->remaining_ms == (elapsed < period ? period - elapsed : 0),
                  "integer millisecond minimum gate, not1000/parameter");
            const auto start = start_original_loop(c, 10000 + elapsed);
            check(start.has_value() == (elapsed >= period), "observed start must satisfy gate");
            if (start)
                check(start->last_start_ms == 10000 + elapsed,
                      "slow iteration commits observed time rather than old deadline");
        }
    OriginalLoopPacing c{};
    const auto first = start_original_loop(c, 47);
    check(first && query_original_loop_wait(*first, 94)->remaining_ms == 0,
          "fixed APK v21 has47ms nominal minimum interval");
    const auto slow = start_original_loop(*first, 600);
    check(slow && query_original_loop_wait(*slow, 601)->remaining_ms == 46 &&
              !start_original_loop(*slow, 601),
          "long stall makes one update, next deadline647, no accumulated catch-up loops");
    const auto backward = query_original_loop_wait({600, 20}, 550);
    check(backward && backward->remaining_ms == 97,
          "source wall clock backwards waits until previous start plus47, not clamp tozero");
    check(!query_original_loop_wait({std::numeric_limits<std::int64_t>::max(), 20}, 0) &&
              !query_original_loop_wait({0, -1}, 0) && !query_original_loop_wait({0, 1000}, 0) &&
              !query_original_loop_wait({0, 20}, -1),
          "reject unsupported parameters/overflow as maintenance safety contract");
    for (int mode = 0; mode <= 6; ++mode)
        for (int speed = -1; speed <= 3; ++speed)
            check(original_scene_iterations(mode, speed) == (mode == 0 && speed == 1 ? 2 : 1),
                  "scene snapshots two iterations only normal mode and raw speed1");
}
} // namespace
int main() {
    try {
        intervals();
        std::cout << checks << " loop pacing checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
