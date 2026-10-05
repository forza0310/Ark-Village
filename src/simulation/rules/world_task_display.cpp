#include "ark/simulation/rules/world_task_display.hpp"

#include <limits>

namespace ark::simulation::rules {
namespace {
using Row = std::array<std::int32_t, 9>;
int map(int value, int start, int end) {
    // c/d.a的加法在整数商之后，不使用浮点插值/四舍五入。
    return start + value * (end - start) / 99;
}
bool valid(const WorldTaskDisplayState &s) {
    if (s.legacy_page != 99 && s.legacy_page != 100)
        return false;
    for (const auto &r : s.bd)
        if (r[2] < 0 || r[2] > 1 || r[3] < 0 || r[4] < 0 || r[7] < 0 || r[7] > 2 || r[8] < 0)
            return false;
    return true;
}
bool add(std::int32_t &value, std::int64_t amount) {
    const auto sum = static_cast<std::int64_t>(value) + amount;
    if (sum < std::numeric_limits<std::int32_t>::min() ||
        sum > std::numeric_limits<std::int32_t>::max())
        return false;
    value = static_cast<std::int32_t>(sum);
    return true;
}
struct Runner {
    WorldTaskDisplayCandidate &c;
    WorldTaskDisplayError error{WorldTaskDisplayError::none};
    std::optional<int> draw() {
        const auto ticket = c.random.draw(100);
        if (ticket.error != WorldRandomError::none) {
            error = WorldTaskDisplayError::random_failed;
            return {};
        }
        return ticket.ticket;
    }
    bool transition(Row &r, std::size_t index, int state) {
        r[7] = state;
        if (state == 0) {
            r[3] = 0;
            r[4] = static_cast<int>(index) * 10;
        } else if (state == 1) {
            const auto duration = draw();
            if (!duration)
                return false;
            r[3] = 0;
            r[4] = map(*duration, 5, 60);
        } else {
            const auto destination = draw();
            if (!destination)
                return false;
            const auto width = static_cast<std::int64_t>(r[6]) - r[5];
            const auto offset = static_cast<std::int64_t>(*destination) * width / 99;
            auto point = r[5];
            if (!add(point, offset)) {
                error = WorldTaskDisplayError::overflow;
                return false;
            }
            r[1] = point;
            r[2] = r[0] >= r[1] ? 1 : 0;
            const auto speed = draw();
            if (!speed)
                return false;
            r[8] = map(*speed, 30, 200);
            // f(i,2)不清3/4；不能把所有状态转移统一成计数归零。
        }
        return true;
    }
    bool initialize() {
        for (std::size_t index = 0; index < c.state.bd.size(); ++index) {
            auto &r = c.state.bd[index];
            const auto position = draw();
            if (!position)
                return false;
            r[0] = index < 3 ? map(*position, -3000, 3000) + static_cast<int>(index) * 7000 + 5000
                             : (*position < 50 ? 2000 : 22000);
            const auto lower = draw();
            if (!lower)
                return false;
            r[5] = map(*lower, 2000, 16000);
            r[6] = r[5] + 6000;
            if (!transition(r, index, index < 3 ? 1 : 0))
                return false;
        }
        return true;
    }
    bool update() {
        for (std::size_t index = 0; index < c.state.bd.size(); ++index) {
            auto &r = c.state.bd[index];
            if (!add(r[3], 1)) {
                error = WorldTaskDisplayError::overflow;
                return false;
            }
            if (r[7] == 0 || r[7] == 1) {
                if (r[3] >= r[4] && !transition(r, index, 2))
                    return false;
            } else if (r[2] == 0) {
                if (!add(r[0], r[8])) {
                    error = WorldTaskDisplayError::overflow;
                    return false;
                }
                if (r[0] >= r[1]) {
                    if (!transition(r, index, 1))
                        return false;
                } else if (r[0] >= 22000)
                    r[2] = 1;
            } else {
                if (!add(r[0], -static_cast<std::int64_t>(r[8]))) {
                    error = WorldTaskDisplayError::overflow;
                    return false;
                }
                if (r[0] <= r[1]) {
                    if (!transition(r, index, 1))
                        return false;
                } else if (r[0] <= 2000)
                    r[2] = 0;
            }
        }
        return true;
    }
};
} // namespace
WorldTaskDisplayResult prepare_world_task_display_page(const WorldTaskDisplayState &state,
                                                       const WorldTaskDisplayInput &input,
                                                       const WorldRandomStream &random) {
    if (!valid(state) || input.counter < 0 ||
        (input.action != WorldTaskDisplayAction::initialize &&
         input.action != WorldTaskDisplayAction::update &&
         input.action != WorldTaskDisplayAction::confirm))
        return {WorldTaskDisplayError::invalid_input, {}};
    WorldTaskDisplayCandidate candidate{state, random, false};
    Runner run{candidate};
    if (!candidate.state.initialized) {
        if (state.legacy_page == 100 && !run.initialize())
            return {run.error, {}};
        candidate.state.initialized = true;
    }
    if (input.action == WorldTaskDisplayAction::update && state.legacy_page == 100 && !run.update())
        return {run.error, {}};
    candidate.closed = input.action == WorldTaskDisplayAction::confirm && input.counter >= 40;
    return {WorldTaskDisplayError::none, std::move(candidate)};
}
} // namespace ark::simulation::rules
