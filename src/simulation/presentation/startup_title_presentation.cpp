#include "ark/simulation/presentation/startup_title_presentation.hpp"
#include <algorithm>
#include <limits>
#include <numeric>
#include <utility>

namespace ark::simulation {
namespace {
constexpr int counter_max = 2147483646;
bool counter_valid(int value) { return value >= 0 && value <= counter_max; }
int next_counter(int value) { return value == counter_max ? 0 : value + 1; }
} // namespace

bool pristine_startup_title_presentation(const StartupTitlePresentation &s) {
    if (s.l || s.f132f || s.s || s.t) return false;
    for (const auto &slot : s.slots)
        if (slot.active || slot.definition || slot.x || slot.y || slot.direction || slot.age) return false;
    return true;
}

std::string validate_startup_title_presentation(const StartupTitlePresentation &state) {
    if (!counter_valid(state.l) || !counter_valid(state.f132f))
        return "title.counter";
    if (state.s != 0 && (state.s < 20 || state.s > 70))
        return "title.spawn_interval";
    if (state.t < 0 || (state.s == 0 ? state.t != 0 : state.t >= state.s))
        return "title.spawn_elapsed";
    for (std::size_t i = 0; i < state.slots.size(); ++i) {
        const auto &slot = state.slots[i];
        const auto prefix = "title.slot[" + std::to_string(i) + "].";
        if (slot.active != 0 && slot.active != 1)
            return prefix + "active";
        if (slot.definition < 0 || slot.definition > 10)
            return prefix + "definition";
        if (slot.direction != 0 && slot.direction != 1)
            return prefix + "direction";
        if (!counter_valid(slot.age))
            return prefix + "age";
        if (slot.x < -11 || slot.x > 251 ||
            (slot.active && (slot.x < -10 || slot.x > 250)))
            return prefix + "x";
        if ((slot.y < 210 || slot.y > 217) && (slot.active || slot.y != 0))
            return prefix + "y";
    }
    return {};
}

StartupTitleUpdateResult prepare_startup_title_update(
    const StartupTitlePresentation &state,
    const ark::simulation::rules::WorldRandomStream &random,
    StartupTitleUpdateRequest request) {
    if (request.admission != StartupTitleAdmission::top_lifecycle_ready)
        return {"title.admission", std::nullopt};
    if (const auto error = validate_startup_title_presentation(state); !error.empty())
        return {error, std::nullopt};
    StartupTitleUpdateCandidate candidate{state, random};
    auto &next = candidate.state;
    next.l = std::min(counter_max, next.l + 1);
    next.f132f = next_counter(next.f132f);
    if (next.l < 100) {
        if (request.confirm_pulse) {
            next.l = 100;
            candidate.confirm_consumed = true;
        }
        if (next.l < 100)
            return {{}, std::move(candidate)};
    }
    candidate.menu_confirm_ready = request.confirm_pulse && !candidate.confirm_consumed;
    // 原先移动全部旧人，再找最小空槽生新人；退休不清 age/y 等历史字段。
    for (auto &slot : next.slots) {
        if (!slot.active)
            continue;
        slot.age = next_counter(slot.age);
        slot.x += 2 * slot.direction - 1;
        if (slot.x < -10 || slot.x > 250)
            slot.active = 0;
    }
    ++next.t;
    if (next.t >= next.s) {
        next.t = 0;
        const auto interval = candidate.random.draw(100);
        if (interval.error != ark::simulation::rules::WorldRandomError::none)
            return {"title.random.interval", std::nullopt};
        next.s = 20 + interval.ticket * 50 / 99;
        const auto empty = std::find_if(next.slots.begin(), next.slots.end(),
                                       [](const auto &slot) { return slot.active == 0; });
        if (empty != next.slots.end()) {
            const auto definition = candidate.random.draw(11);
            if (definition.error != ark::simulation::rules::WorldRandomError::none)
                return {"title.random.definition", std::nullopt};
            const auto direction = candidate.random.draw(2);
            if (direction.error != ark::simulation::rules::WorldRandomError::none)
                return {"title.random.direction", std::nullopt};
            const auto height = candidate.random.draw(100);
            if (height.error != ark::simulation::rules::WorldRandomError::none)
                return {"title.random.height", std::nullopt};
            empty->active = 1;
            empty->definition = definition.ticket;
            empty->direction = direction.ticket;
            empty->x = direction.ticket == 1 ? 0 : 240;
            empty->y = 210 + height.ticket * 7 / 99;
        }
    }
    candidate.random_draws = candidate.random.draws() - random.draws();
    return {{}, std::move(candidate)};
}

StartupTitleProjection project_startup_title_presentation(
    const StartupTitlePresentation &state, int surface_height) {
    if (const auto error = validate_startup_title_presentation(state); !error.empty())
        return {error, {}};
    if (surface_height <= 0)
        return {"title.surface_height", {}};
    std::array<int, 20> order{};
    std::iota(order.begin(), order.end(), 0);
    // 原交换序不稳定；inactive 的 y 也参与，因此不能先过滤或改 stable_sort。
    for (int i = 0; i < 19; ++i)
        for (int j = 19; j > i; --j)
            if (state.slots[order[i]].y > state.slots[order[j]].y)
                std::swap(order[i], order[j]);
    StartupTitleProjection result;
    for (const int index : order) {
        const auto &slot = state.slots[index];
        if (!slot.active)
            continue;
        // 合法 y 最大217，先减240可保证任意正int表面高度不溢出。
        result.people.push_back({index, slot.definition, slot.age, (slot.age % 20) / 5,
                                 slot.direction == 1 ? 1 : 2,
                                 {slot.x, surface_height + (slot.y - 240)}});
    }
    return result;
}
} // namespace ark::simulation
