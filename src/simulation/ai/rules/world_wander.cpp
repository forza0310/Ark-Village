#include "ark/simulation/ai/rules/world_wander.hpp"

#include <algorithm>
#include <utility>

namespace ark::simulation::rules {
namespace {
bool live(const RescueWorldState &s, CharacterId id) {
    const auto actor = s.ai.battle.actors.find(id);
    if (!id.value || actor == s.ai.battle.actors.end() || !(actor->second.id == id) ||
        !s.ai.contexts.count(id) || !s.actors.count(id) ||
        (actor->second.kind != ActorKind::human && actor->second.kind != ActorKind::monster))
        return false;
    const auto &roster =
        actor->second.kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order;
    return std::count(roster.begin(), roster.end(), id) == 1;
}
const RewardEncounter *event(const AiRewardState &s, std::uint64_t id) {
    const auto e = s.encounters.find(id);
    if (e != s.encounters.end())
        return &e->second;
    const auto retired = s.retired_encounters.find(id);
    return retired == s.retired_encounters.end() ? nullptr : &retired->second;
}
const BattleActorRecord *followed(const AiRewardState &s, CharacterId id) {
    const auto a = s.battle.actors.find(id);
    if (a != s.battle.actors.end())
        return &a->second;
    const auto retired = s.retired_actors.find(id);
    return retired == s.retired_actors.end() ? nullptr : &retired->second;
}
bool same_logical_map(const LegacyMap &a, const LegacyMap &b) {
    if (!valid_legacy_map(a) || a.width != b.width || a.height != b.height ||
        a.cells.size() != b.cells.size())
        return false;
    for (std::size_t n = 0; n < a.cells.size(); ++n)
        if (a.cells[n].legacy_state != b.cells[n].legacy_state)
            return false;
    return true;
}
} // namespace
WorldWanderResult prepare_world_wander(const RescueWorldState &s, const WorldWanderInput &i) {
    const auto fail = [](RescueWorldError e) -> WorldWanderResult { return {e, {}}; };
    if (!live(s, i.actor))
        return fail(RescueWorldError::stale_actor);
    const auto &a = s.ai.battle.actors.at(i.actor);
    if (a.control.queue.empty() ||
        !std::all_of(a.control.queue.begin(), a.control.queue.end(), valid_actor_control))
        return fail(RescueWorldError::invalid_input);
    const auto command = a.control.queue.front();
    if (command[0] != 10 && command[0] != 12 && command[0] != 13)
        return fail(RescueWorldError::invalid_input);
    WorldWanderCandidate c;
    c.state = s;
    c.opcode = command[0];
    c.state.ai.battle.actors.at(i.actor).control.queue.erase(
        c.state.ai.battle.actors.at(i.actor).control.queue.begin());
    if (command[0] == 10) {
        c.center = s.ai.contexts.at(i.actor).cell;
    } else if (command[0] == 12) {
        if (!a.encounter)
            return {RescueWorldError::none, std::move(c)};
        const auto *e = event(s.ai, *a.encounter);
        if (!e)
            return fail(RescueWorldError::invalid_input);
        c.center = e->runtime.center;
    } else {
        if (!a.follow)
            return {RescueWorldError::none, std::move(c)};
        const auto *other = followed(s.ai, *a.follow);
        if (!other || !(other->id == *a.follow) || other->kind != ActorKind::monster ||
            !s.ai.contexts.count(*a.follow))
            return fail(RescueWorldError::invalid_input);
        c.center = s.ai.contexts.at(*a.follow).cell;
    }
    if (!valid_world_map_facts(i.facts) || !same_logical_map(s.map, i.facts.map))
        return fail(RescueWorldError::invalid_input);
    ActorWanderInput pure;
    pure.opcode = command[0];
    pure.parameter = command[0] == 10 ? command[1] : 0;
    pure.actor = *c.center;
    pure.center = c.center;
    pure.width = s.map.width;
    pure.height = s.map.height;
    pure.tickets = i.tickets;
    pure.draw = i.draw;
    for (int y = 0; y < s.map.height; ++y)
        for (int x = 0; x < s.map.width; ++x) {
            const auto index = static_cast<std::size_t>(y) * s.map.width + x;
            const bool town = x > i.facts.town.left && x < i.facts.town.right &&
                              y > i.facts.town.top && y < i.facts.town.bottom;
            pure.cells.push_back({s.map.cells[index].legacy_state, town, i.facts.flags[index]});
        }
    const auto plan = prepare_actor_wander(pure);
    if (!plan)
        return fail(RescueWorldError::preparation_failed);
    auto &queue = c.state.ai.battle.actors.at(i.actor).control.queue;
    queue.insert(queue.end(), plan->append.begin(), plan->append.end());
    c.cells = plan->cells;
    c.append = plan->append;
    c.consumed_tickets = plan->consumed_tickets;
    return {RescueWorldError::none, std::move(c)};
}
} // namespace ark::simulation::rules
