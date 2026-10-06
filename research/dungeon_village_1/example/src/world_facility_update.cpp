#include "dungeon_village_reference/world_facility_update.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
struct Failure {
    WorldFacilityUpdateError error;
};
[[noreturn]] void fail(WorldFacilityUpdateError error) { throw Failure{error}; }
int checked(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        fail(WorldFacilityUpdateError::overflow);
    return static_cast<int>(value);
}
void validate(const WorldFacilityUpdateState &state, std::uint64_t identity,
              const WorldScriptCatalog &catalog) {
    if (!identity || !state.finish.dungeon.world.facilities.count(identity) ||
        !state.finish.dungeon.facilities.count(identity) || !state.details.count(identity) ||
        state.cycle_length <= 0 || state.finish.event_calls != state.scripts.event_calls ||
        state.finish.dungeon.world.ai.pending_completion != state.scripts.pending_completion ||
        validate_world_script_state(catalog, state.scripts) != WorldScriptError::none)
        fail(WorldFacilityUpdateError::invalid_owner);
    const auto definition =
        state.finish.dungeon.world.facilities.at(identity).placement.definition_id;
    if (!state.definitions.count(definition))
        fail(WorldFacilityUpdateError::invalid_owner);
}
} // namespace
WorldFacilityUpdateResult
prepare_world_facility_update(const WorldFacilityUpdateState &state, std::uint64_t identity,
                              const WorldScriptCatalog &catalog,
                              const WorldFacilityUpdateConsumer &consumer) {
    try {
        validate(state, identity, catalog);
        WorldFacilityUpdateCandidate candidate{state, {}, {}, {}, false};
        auto invoke = [&](int event) {
            const auto result =
                prepare_world_script(catalog, candidate.state.scripts, {event, {}, {}});
            if (!result.candidate)
                fail(WorldFacilityUpdateError::script_failed);
            candidate.state.scripts = result.candidate->state;
            candidate.state.finish.event_calls = candidate.state.scripts.event_calls;
            candidate.state.finish.dungeon.world.ai.pending_completion =
                candidate.state.scripts.pending_completion;
            candidate.scripts.insert(candidate.scripts.end(), result.candidate->executed.begin(),
                                     result.candidate->executed.end());
            candidate.pages.insert(candidate.pages.end(), result.candidate->inserted_pages.begin(),
                                   result.candidate->inserted_pages.end());
        };
        auto consume = [&](WorldFacilityUpdateConsumerKind kind) {
            if (!consumer)
                fail(WorldFacilityUpdateError::missing_consumer);
            const WorldFacilityUpdateRequest request{kind, identity};
            const auto result = consumer(candidate.state, request);
            if (!result)
                fail(WorldFacilityUpdateError::consumer_failed);
            // finish可实际移除该设施；这里只检验共享投影，不强制其继续存在。
            if (result->finish.event_calls != result->scripts.event_calls ||
                result->finish.dungeon.world.ai.pending_completion !=
                    result->scripts.pending_completion ||
                result->random.draws() < candidate.state.random.draws() ||
                validate_world_script_state(catalog, result->scripts) != WorldScriptError::none)
                fail(WorldFacilityUpdateError::consumer_failed);
            if (kind == WorldFacilityUpdateConsumerKind::residence_join) {
                const auto current = result->finish.dungeon.world.facilities.find(identity);
                if (current == result->finish.dungeon.world.facilities.end() ||
                    !result->definitions.count(current->second.placement.definition_id))
                    fail(WorldFacilityUpdateError::consumer_failed);
            }
            candidate.state = *result;
            candidate.consumer_calls.push_back(request);
        };
        auto &next = candidate.state;
        auto &facility = next.finish.dungeon.world.facilities.at(identity);
        auto &progress = next.finish.dungeon.facilities.at(identity);
        auto &detail = next.details.at(identity);
        auto &definition = next.definitions.at(facility.placement.definition_id);
        progress.updates = checked(static_cast<std::int64_t>(progress.updates) + 1);
        if (!detail.notices.empty()) {
            auto &front = detail.notices.front();
            if (front[0] < 0 || front[0] >= 8)
                fail(WorldFacilityUpdateError::invalid_owner);
            front[1] = checked(static_cast<std::int64_t>(front[1]) + 1);
            const int duration = front[0] == 0 ? 44 : front[0] == 7 ? 60 : 43;
            if (front[1] >= duration)
                detail.notices.erase(detail.notices.begin());
        }
        if (facility.status == 0) {
            if (progress.updates < detail.construction_limit)
                return {WorldFacilityUpdateError::none, std::move(candidate)};
            if (facility.kind != 13 &&
                std::none_of(detail.notices.begin(), detail.notices.end(),
                             [](const auto &value) { return value[0] == 0; }))
                detail.notices.insert(detail.notices.begin(), {0, 0});
            facility.status = 1;
            progress.updates = 0;
            progress.progress = 0;
            // 原c.d.a(...,true)先夹1..9，再整数线性插值；b1未运行地图重绑定。
            const auto first = checked(static_cast<std::int64_t>(next.cycle_length) * 120);
            const auto last = checked(static_cast<std::int64_t>(next.cycle_length) * 24);
            const auto product = checked(
                static_cast<std::int64_t>(std::clamp(detail.condition, 1, 9) - 1) * (last - first));
            progress.extent = checked(static_cast<std::int64_t>(first) + product / 8);
            if (facility.kind == 13 || facility.kind == 12)
                detail.completion_popularity = 0;
            else {
                detail.completion_popularity = definition.popularity_reward;
                next.scripts.popularity_queue.insert(next.scripts.popularity_queue.begin(),
                                                     {25, definition.popularity_reward, 1});
                definition.popularity_reward = std::max(detail.completion_popularity / 2, 1);
            }
            candidate.construction_completed = true;
            if (facility.kind != 13 && !world_script_seen(next.scripts, 88))
                invoke(88);
            if (detail.residence_mode == 1 && facility.kind == 12 &&
                detail.resident_definition != -1)
                consume(WorldFacilityUpdateConsumerKind::residence_join);
            // 消费者可重建目录；按当前原共享定义逐条重新检查aM，而非预先收集一组事件。
            const std::array<std::pair<std::uint32_t, int>, 9> flags{{{256, 73},
                                                                      {512, 74},
                                                                      {1024, 75},
                                                                      {2048, 76},
                                                                      {4096, 211},
                                                                      {512, 212},
                                                                      {16384, 213},
                                                                      {32768, 214},
                                                                      {1024, 215}}};
            for (const auto &entry : flags)
                if ((next.definitions
                         .at(next.finish.dungeon.world.facilities.at(identity)
                                 .placement.definition_id)
                         .flags &
                     entry.first) != 0 &&
                    !world_script_seen(next.scripts, entry.second))
                    invoke(entry.second);
            if (next.finish.dungeon.world.facilities.at(identity).kind == 13 &&
                !world_script_seen(next.scripts, 117))
                invoke(117);
            if (next.finish.dungeon.world.facilities.at(identity).kind == 12 &&
                !world_script_seen(next.scripts, 118))
                invoke(118);
        } else if (facility.status == 1) {
            // 源此处是o.f83f活动类别，不是o.f82e设施种类。
            if (facility.category == 5 && !facility.occupants.empty())
                consume(WorldFacilityUpdateConsumerKind::dungeon_crew);
        } else if (facility.status == 2 && progress.updates >= 10)
            consume(WorldFacilityUpdateConsumerKind::dungeon_finish);
        return {WorldFacilityUpdateError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
} // namespace dungeon_village_reference
