// Adapted from research af85eb1 example/src/ai_perception.cpp; independent product build.
#include "ark/people/ai_perception.hpp"

#include <cmath>
#include <set>

namespace ark::people {
namespace {
bool within(world::Cell a, world::Cell b, int radius) {
    return std::abs(static_cast<std::int64_t>(a.x) - b.x) <= radius &&
           std::abs(static_cast<std::int64_t>(a.y) - b.y) <= radius;
}
} // namespace
EventGateResult prepare_event_gate(const EventGateInput &i) {
    if ((i.actor.kind != ActorKind::human && i.actor.kind != ActorKind::monster) ||
        i.actor.state_counter < 0 || i.task_kind < 0)
        return {ActorAiError::invalid_input, std::nullopt};
    std::set<std::uint64_t> ids;
    for (const auto &e : i.encounters)
        if (!ids.insert(e.id).second)
            return {ActorAiError::invalid_input, std::nullopt};
    EventGateCandidate c;
    if (i.actor.state_counter < 5 || (i.actor.flags & 512U) || !i.actor.in_move_area)
        return {ActorAiError::none, c};
    if (i.actor.kind == ActorKind::monster) {
        c.ready = i.actor.has_enemy && i.actor.inside_town == i.actor.enemy_inside_town;
        return {ActorAiError::none, c};
    }
    if (i.actor.has_object || !i.cell_in_map)
        return {ActorAiError::none, c};
    const bool task_actor = i.active_task && i.definition_task_flag;
    if (i.cell_event_flag) {
        for (const auto &e : i.encounters)
            if ((!task_actor || (i.task_encounter && *i.task_encounter == e.id)) &&
                within(i.cell, e.center, 1)) {
                c.bind_encounter = e.id;
                break;
            }
        c.ready = c.bind_encounter.has_value() || i.previous_encounter.has_value();
    } else if (task_actor && i.task_kind == 1 && within(i.cell, i.task_center, 1)) {
        c.ready = true;
        if (i.task_encounter)
            c.bind_encounter = i.task_encounter;
        else
            c.request_task_encounter = true;
    }
    return {ActorAiError::none, c};
}
std::optional<MoveAreaCandidate> prepare_move_area(int state, bool has_encounter,
                                                   const std::array<MoveAreaProbe, 4> &probes) {
    if (state < 0 || state > 20)
        return std::nullopt;
    for (const auto &p : probes) {
        if (!p.cell_in_map)
            return MoveAreaCandidate{false, 1};
        if (p.legacy_surface == 3 || p.logical_state == 0)
            return MoveAreaCandidate{false, 2};
        if ((state == 1 || state == 4) && has_encounter && !p.in_encounter_square)
            return MoveAreaCandidate{false, 3};
    }
    return MoveAreaCandidate{true, 0};
}
std::optional<int> select_idle_preemption(const PreemptionInput &i) {
    if (i.kind != ActorKind::human || (i.state != 0 && i.state != 5) || i.has_object)
        return std::nullopt;
    if (i.rescue_enabled && !i.definition_task_flag)
        for (const auto &p : i.people)
            if (p.state == 2 && p.inside_town == i.inside_town && !p.on_event_cell &&
                within(i.cell, p.cell, 2))
                return 13;
    if (!i.inside_town)
        for (const auto &o : i.objects)
            if (o.state == 3 && within(i.cell, o.cell, 2))
                return 11;
    return std::nullopt;
}
EnemySelectionResult select_rescue_target(world::WorldPosition position,
                                          const std::vector<RescueTargetSnapshot> &people) {
    // Share the proven strict reverse-scan tie algorithm, not combat's exclusions/region filters.
    EnemySelectionInput i;
    i.position = position;
    i.active_battle_group = true;
    std::set<ActorId> ids;
    for (const auto &p : people) {
        if (p.id.value == 0 || !ids.insert(p.id).second || !world_cell(p.position) || p.state < 0 ||
            p.state > 20)
            return {ActorAiError::invalid_input, std::nullopt};
        if (p.state == 2 && !p.on_event_cell)
            i.opposite_roster.push_back({p.id, 0, true, p.position, std::nullopt});
    }
    return select_combat_enemy(i);
}
std::optional<ActorId> select_healing_target(world::Cell half_cell,
                                             const std::vector<RescueTargetSnapshot> &people) {
    for (const auto &p : people) {
        const auto distance = std::abs(static_cast<std::int64_t>(half_cell.x) - p.half_cell.x) +
                              std::abs(static_cast<std::int64_t>(half_cell.y) - p.half_cell.y);
        if ((p.low_hp || p.state == 2) && distance <= 4)
            return p.id;
    }
    return std::nullopt;
}
SpawnProbeResult prepare_spawn_probe(const SpawnProbeInput &i) {
    if ((i.kind != ActorKind::human && i.kind != ActorKind::monster) || i.state < 0 ||
        i.state > 20 || i.monster_count < 0 || i.monster_limit < 0)
        return {ActorAiError::invalid_input, std::nullopt};
    SpawnProbeCandidate c;
    if ((i.inside_town && i.kind == ActorKind::monster) || i.destination_inside_town ||
        i.cell_y < i.minimum_y)
        return {ActorAiError::none, c};
    if (!i.ticket || *i.ticket < 0 || *i.ticket >= 1000)
        return {ActorAiError::invalid_input, std::nullopt};
    c.consumes_ticket = true;
    c.request_event_probe =
        *i.ticket < (i.state == 5 ? 13 : 0) && i.monster_count < i.monster_limit;
    return {ActorAiError::none, c};
}
std::optional<DepartureOverrideCandidate>
prepare_departure_override(const DepartureOverrideInput &i) {
    if (i.self.value == 0 || (i.kind != ActorKind::human && i.kind != ActorKind::monster) ||
        i.activity < 0 || i.activity > 8)
        return std::nullopt;
    DepartureOverrideCandidate c;
    if (i.reachable.empty() || i.kind == ActorKind::monster || (i.flags & (512U | 1024U)))
        return c;
    const auto find = [&](world::Cell target) -> std::optional<world::Cell> {
        for (world::Cell cell : i.reachable)
            if (cell == target)
                return cell;
        return std::nullopt;
    };
    if (i.active_task && !i.has_object && i.definition_task_flag)
        if (auto cell = find(i.task_center))
            return DepartureOverrideCandidate{DepartureOverrideKind::task, cell, true};
    if (i.definition_task_flag)
        return c;
    if (i.nearest_down) {
        bool reserved = false;
        for (const auto &p : i.people)
            if (!(p.id == i.self) && p.destination == *i.nearest_down)
                reserved = true;
        if (!reserved)
            if (auto cell = find(*i.nearest_down))
                return DepartureOverrideCandidate{DepartureOverrideKind::rescue, cell, false};
    }
    if (i.nearest_object)
        if (auto cell = find(*i.nearest_object))
            return DepartureOverrideCandidate{DepartureOverrideKind::object, cell, false};
    if (i.activity == 6 && i.nearest_encounter)
        for (world::Cell cell : i.reachable)
            if (within(cell, *i.nearest_encounter, 1))
                return DepartureOverrideCandidate{DepartureOverrideKind::encounter, cell, false};
    return c;
}
} // namespace ark::people
