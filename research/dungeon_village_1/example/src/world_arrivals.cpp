#include "dungeon_village_reference/world_arrivals.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
struct Failure {
    WorldArrivalsError error;
};
[[noreturn]] void fail(WorldArrivalsError error) { throw Failure{error}; }
const WorldArrivalDefinition &definition(const WorldArrivalsState &s, int id) {
    const auto found = std::find_if(s.definitions.begin(), s.definitions.end(),
                                    [&](const auto &d) { return d.identity == id; });
    if (found == s.definitions.end())
        fail(WorldArrivalsError::invalid_owner);
    return *found;
}
void validate(const WorldArrivalsState &s, const WorldScriptCatalog &catalog) {
    const auto &ai = s.finish.dungeon.world.ai;
    if (s.soft_limit < 0 || s.hard_limit < 0 || s.finish.event_calls != s.scripts.event_calls ||
        ai.pending_completion != s.scripts.pending_completion ||
        validate_world_script_state(catalog, s.scripts) != WorldScriptError::none)
        fail(WorldArrivalsError::invalid_owner);
    std::set<int> definitions;
    for (const auto &d : s.definitions)
        if (d.identity < 0 || !definitions.insert(d.identity).second)
            fail(WorldArrivalsError::invalid_owner);
    std::set<CharacterId> actors;
    std::set<int> uids;
    if (ai.human_order.size() != s.scripts.human_order.size())
        fail(WorldArrivalsError::invalid_owner);
    for (std::size_t i = 0; i < ai.human_order.size(); ++i) {
        const auto id = ai.human_order[i];
        const auto actor = ai.battle.actors.find(id);
        if (!actors.insert(id).second || actor == ai.battle.actors.end() ||
            actor->second.kind != ActorKind::human || !(actor->second.id == id) ||
            actor->second.legacy_id < 0 || !uids.insert(actor->second.legacy_id).second ||
            s.scripts.human_order[i] != id.value)
            fail(WorldArrivalsError::invalid_owner);
        (void)definition(s, actor->second.definition);
    }
}
void execute(WorldArrivalsCandidate &c, const WorldScriptCatalog &catalog, int event) {
    auto result = prepare_world_script(catalog, c.state.scripts, {event, {}, {}});
    if (!result.candidate)
        fail(WorldArrivalsError::script_failed);
    c.state.scripts = std::move(result.candidate->state);
    c.state.finish.event_calls = c.state.scripts.event_calls;
    c.state.finish.dungeon.world.ai.pending_completion = c.state.scripts.pending_completion;
    c.executed_events.push_back(event);
}
} // namespace
WorldArrivalsResult prepare_world_arrivals(const WorldArrivalsState &state,
                                           const WorldScriptCatalog &catalog,
                                           const WorldArrivalCreationConsumer &consumer) {
    try {
        validate(state, catalog);
        WorldArrivalsCandidate c;
        c.state = state;
        auto &s = c.state;
        const auto count = s.finish.dungeon.world.ai.human_order.size();
        if (s.camera_delay > 0)
            --s.camera_delay;
        else if (count >= static_cast<std::size_t>(s.soft_limit))
            return {WorldArrivalsError::none, std::move(c)};
        if (count >= static_cast<std::size_t>(s.hard_limit))
            return {WorldArrivalsError::none, std::move(c)};
        if (s.arrival_counter > 0)
            --s.arrival_counter;
        if (s.arrival_counter > 0)
            return {WorldArrivalsError::none, std::move(c)};
        auto draw = [&](std::size_t size) {
            if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
                fail(WorldArrivalsError::numeric_overflow);
            const int bound = static_cast<int>(size);
            c.random_bounds.push_back(bound);
            const auto result = s.random.draw(bound);
            if (result.error != WorldRandomError::none)
                fail(WorldArrivalsError::random_failed);
            return result.ticket;
        };
        c.due = true;
        s.arrival_counter =
            s.camera_delay > 0 || s.camera_follow == 1 ? 5 + draw(15) : 100 + draw(150);
        c.first = !world_script_seen(s.scripts, 89);
        const auto &ai = s.finish.dungeon.world.ai;
        for (const auto &d : s.definitions) {
            if (!d.presence)
                continue;
            const bool exists =
                std::any_of(ai.human_order.begin(), ai.human_order.end(), [&](auto id) {
                    return ai.battle.actors.at(id).definition == d.identity;
                });
            if (!exists)
                c.sorted_candidates.push_back(d.identity);
        }
        for (std::size_t left = 0; left + 1 < c.sorted_candidates.size(); ++left)
            for (std::size_t right = c.sorted_candidates.size() - 1; right > left; --right)
                if (definition(s, c.sorted_candidates[right]).priority >
                    definition(s, c.sorted_candidates[left]).priority)
                    std::swap(c.sorted_candidates[left], c.sorted_candidates[right]);
        int selected = -1;
        if (!c.sorted_candidates.empty()) {
            const int top = definition(s, c.sorted_candidates.front()).priority;
            for (int id : c.sorted_candidates) {
                if (definition(s, id).priority != top)
                    break;
                c.top_candidates.push_back(id);
            }
            selected = c.top_candidates[static_cast<std::size_t>(draw(c.top_candidates.size()))];
        }
        if (s.debug_mode == 1 || s.debug_mode == 2) {
            const auto &pool = s.debug_definitions[static_cast<std::size_t>(s.debug_mode - 1)];
            const auto ticket = static_cast<std::size_t>(draw(pool.size()));
            if (ticket >= pool.size())
                fail(WorldArrivalsError::invalid_owner);
            selected = pool[ticket];
        }
        if (c.first)
            for (const auto &d : s.definitions)
                if (d.flags & 8U) {
                    selected = d.identity;
                    break;
                }
        if (selected == -1)
            return {WorldArrivalsError::none, std::move(c)};
        (void)definition(s, selected);
        c.selected_definition = selected;
        if (!consumer)
            fail(WorldArrivalsError::missing_consumer);
        int uid = 0;
        while (std::any_of(ai.human_order.begin(), ai.human_order.end(),
                           [&](auto id) { return ai.battle.actors.at(id).legacy_id == uid; })) {
            if (uid == std::numeric_limits<int>::max())
                fail(WorldArrivalsError::numeric_overflow);
            ++uid;
        }
        const auto spawn_ticket = static_cast<std::size_t>(draw(s.spawn_cells.size()));
        if (spawn_ticket >= s.spawn_cells.size())
            fail(WorldArrivalsError::invalid_owner);
        const WorldArrivalCreationInput input{selected, uid, s.spawn_cells[spawn_ticket]};
        const auto made = consumer(s, input);
        if (!made)
            fail(WorldArrivalsError::consumer_failed);
        validate(made->state, catalog);
        const auto &after = made->state.finish.dungeon.world;
        const auto actor = after.ai.battle.actors.find(made->actor);
        if (ai.battle.actors.count(made->actor) || actor == after.ai.battle.actors.end() ||
            actor->second.definition != selected || actor->second.legacy_id != uid ||
            actor->second.control.flags != 2U || actor->second.control.state != 0 ||
            actor->second.baseline != 0 ||
            actor->second.control.queue != std::vector<std::vector<int>>{{8, 0}} ||
            actor->second.hp.displayed != actor->second.hp.origin ||
            actor->second.hp.displayed != actor->second.hp.target ||
            after.ai.human_order.size() != ai.human_order.size() + 1 ||
            !(after.ai.human_order.back() == made->actor) ||
            !std::equal(ai.human_order.begin(), ai.human_order.end(),
                        after.ai.human_order.begin()) ||
            after.ai.battle.actors.size() != ai.battle.actors.size() + 1 ||
            made->state.random.draws() != s.random.draws() ||
            made->state.arrival_counter != s.arrival_counter ||
            made->state.camera_delay != s.camera_delay ||
            made->state.camera_follow != s.camera_follow ||
            made->state.scripts.event_calls != s.scripts.event_calls ||
            made->state.scripts.pending_completion != s.scripts.pending_completion ||
            !after.ai.contexts.count(made->actor) || !after.actors.count(made->actor) ||
            !(after.ai.contexts.at(made->actor).cell == input.spawn) ||
            actor->second.position.x != input.spawn.x * 100.0F + 50.0F ||
            actor->second.position.z != input.spawn.y * 100.0F + 50.0F ||
            actor->second.position.height != 0)
            fail(WorldArrivalsError::consumer_failed);
        s = made->state;
        c.created = made->actor;
        if (c.first) {
            s.finish.dungeon.world.ai.battle.actors.at(made->actor).control.flags |= 8192U;
            execute(c, catalog, 89);
        }
        const auto &history = definition(s, selected).job_history;
        if (!world_script_seen(s.scripts, 218) &&
            std::any_of(history.begin(), history.end(), [](bool value) { return value; }))
            execute(c, catalog, 218);
        return {WorldArrivalsError::none, std::move(c)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
} // namespace dungeon_village_reference
