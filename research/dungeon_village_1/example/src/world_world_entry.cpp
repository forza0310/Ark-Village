#include "dungeon_village_reference/world_world_entry.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace dungeon_village_reference {
namespace {
struct Failure {
    WorldWorldEntryError error;
};
[[noreturn]] void fail(WorldWorldEntryError error) { throw Failure{error}; }
int checked(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        fail(WorldWorldEntryError::overflow);
    return static_cast<int>(value);
}
bool inside(Position p, const TownBounds &town, bool inclusive) {
    return inclusive
               ? p.x >= town.left && p.x <= town.right && p.y >= town.top && p.y <= town.bottom
               : p.x > town.left && p.x < town.right && p.y > town.top && p.y < town.bottom;
}
bool near(Position first, Position second) {
    return std::abs(static_cast<std::int64_t>(first.x) - second.x) <= 3 &&
           std::abs(static_cast<std::int64_t>(first.y) - second.y) <= 3;
}
void validate(const WorldWorldEntryState &state, const WorldCalendarState &date,
              const WorldScriptCatalog &catalog) {
    if (!valid_world_calendar_state(date) || state.updates < 0 ||
        state.updates >= std::numeric_limits<int>::max() ||
        state.finish.event_calls != state.scripts.event_calls ||
        state.finish.dungeon.world.ai.pending_completion != state.scripts.pending_completion ||
        !prepare_world_script_continuations(catalog, state.scripts, false).candidate)
        fail(WorldWorldEntryError::invalid_owner);
}
} // namespace
WorldWorldEntryResult prepare_world_world_entry(const WorldWorldEntryState &state,
                                                const WorldCalendarState &date,
                                                const WorldScriptCatalog &catalog,
                                                const WorldWorldEntryCreationConsumer &consumer) {
    try {
        validate(state, date, catalog);
        WorldWorldEntryCandidate candidate{state, {}, {}, {}, EncounterCreationDenial::none, false};
        auto &next = candidate.state;
        next.updates = (next.updates + 1) % std::numeric_limits<int>::max();
        next.global_updates = checked(static_cast<std::int64_t>(next.global_updates) + 1);
        auto draw = [&](int bound) {
            candidate.random_bounds.push_back(bound);
            const auto result = next.random.draw(bound);
            if (result.error != WorldRandomError::none)
                fail(WorldWorldEntryError::random_failed);
            return result.ticket;
        };
        if (date.year == 0 && date.month + 1 <= 6)
            return {WorldWorldEntryError::none, std::move(candidate)};
        // 源在怪物数量/地图/全部门槛之前必抽1000，只有<10才进入c/k.c。
        if (draw(1000) >= 10)
            return {WorldWorldEntryError::none, std::move(candidate)};
        const auto &map = next.finish.dungeon.world.map;
        if (map.width <= 0 || map.height <= 0 ||
            map.cells.size() != static_cast<std::size_t>(map.width) * map.height ||
            next.town.left > next.town.right || next.town.top > next.town.bottom)
            fail(WorldWorldEntryError::invalid_owner);
        const int width = checked(static_cast<std::int64_t>(next.generation_bounds[1].x) -
                                  next.generation_bounds[0].x);
        const int height = checked(static_cast<std::int64_t>(next.generation_bounds[0].y) -
                                   next.generation_bounds[1].y);
        if (width < 0 || height < 0)
            fail(WorldWorldEntryError::invalid_owner);
        for (int attempt = 0; attempt < 20; ++attempt) {
            const Position position{
                checked(static_cast<std::int64_t>(next.generation_bounds[0].x) + draw(width)),
                checked(static_cast<std::int64_t>(next.generation_bounds[1].y) + draw(height))};
            if (position.x < 0 || position.y < 0 || position.x >= map.width ||
                position.y >= map.height || inside(position, next.town, false) ||
                map.cells[static_cast<std::size_t>(position.y) * map.width + position.x]
                        .legacy_state != 4)
                continue;
            bool admitted = true;
            for (auto id : next.finish.task_order) {
                const auto task = next.finish.tasks.find(id);
                if (task == next.finish.tasks.end() || !task->second.site)
                    fail(WorldWorldEntryError::invalid_owner);
                if (near(position, *task->second.site)) {
                    admitted = false;
                    break;
                }
            }
            if (admitted)
                for (auto id : next.finish.dungeon.world.ai.encounter_order) {
                    const auto encounter = next.finish.dungeon.world.ai.encounters.find(id);
                    if (encounter == next.finish.dungeon.world.ai.encounters.end())
                        fail(WorldWorldEntryError::invalid_owner);
                    if (near(position, encounter->second.runtime.center)) {
                        admitted = false;
                        break;
                    }
                }
            if (admitted) {
                candidate.selected_site = position;
                break;
            }
        }
        if (!candidate.selected_site)
            return {WorldWorldEntryError::none, std::move(candidate)};
        if (!consumer)
            fail(WorldWorldEntryError::missing_consumer);
        EncounterCreationInput creation;
        creation.kind = 0;
        creation.center = *candidate.selected_site;
        creation.year_index = date.year;
        creation.month_index = date.month;
        for (int offset = -1; offset <= 1; ++offset)
            creation.upper_band_town[static_cast<std::size_t>(offset + 1)] =
                inside({creation.center.x + offset, creation.center.y - 1}, next.town, true);
        // 不捕获外部Owner引用；消费者须用自己的候选random供f.a后续统一抽取。
        const auto result = consumer(next, creation);
        if (!result)
            fail(WorldWorldEntryError::consumer_failed);
        validate(result->state, date, catalog);
        if (result->state.random.draws() < next.random.draws())
            fail(WorldWorldEntryError::consumer_failed);
        if (result->created) {
            const auto &old_events = next.finish.dungeon.world.ai;
            const auto &new_events = result->state.finish.dungeon.world.ai;
            if (result->denial != EncounterCreationDenial::none ||
                next.finish.dungeon.world.ai.encounters.count(*result->created) ||
                !result->state.finish.dungeon.world.ai.encounters.count(*result->created) ||
                result->state.finish.dungeon.world.ai.encounters.at(*result->created)
                        .runtime.state != 0)
                fail(WorldWorldEntryError::consumer_failed);
            const auto &event = new_events.encounters.at(*result->created);
            if (new_events.encounters.size() != old_events.encounters.size() + 1 ||
                new_events.encounter_order.size() != old_events.encounter_order.size() + 1 ||
                !std::equal(old_events.encounter_order.begin(), old_events.encounter_order.end(),
                            new_events.encounter_order.begin()) ||
                new_events.encounter_order.back() != *result->created || event.members.empty() ||
                event.runtime.spawned <= 0)
                fail(WorldWorldEntryError::consumer_failed);
        } else if (result->denial == EncounterCreationDenial::none)
            fail(WorldWorldEntryError::consumer_failed);
        next = result->state;
        candidate.created = result->created;
        candidate.denial = result->denial;
        candidate.attempted_creation = true;
        return {WorldWorldEntryError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
} // namespace dungeon_village_reference
