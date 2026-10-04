#include "ark/simulation/rules/world_dungeon.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ark::simulation::rules {
namespace {
bool live(const DungeonWorldState &s, CharacterId id) {
    const auto a = s.world.ai.battle.actors.find(id);
    return id.value && a != s.world.ai.battle.actors.end() && a->second.id == id &&
           a->second.kind == ActorKind::human && s.world.actors.count(id) &&
           s.world.ai.contexts.count(id) && s.actors.count(id) &&
           std::count(s.world.ai.human_order.begin(), s.world.ai.human_order.end(), id) == 1;
}
bool town_valid(const TownBounds &t) { return t.left < t.right && t.top < t.bottom; }
std::vector<Position> destinations(const LegacyMap &map, Position origin, TownBounds t) {
    static constexpr Position offsets[]{{-1, 1}, {0, 1},   {1, 1},  {-1, 0},
                                        {1, 0},  {-1, -1}, {0, -1}, {1, -1}};
    std::vector<Position> cells;
    for (const auto d : offsets) {
        const Position p{origin.x + d.x, origin.y + d.y};
        if (p.x < 0 || p.x >= map.width || p.y < 0 || p.y >= map.height)
            continue;
        const bool inside = p.x > t.left && p.x < t.right && p.y > t.top && p.y < t.bottom;
        if (!inside && map.cells[p.y * map.width + p.x].legacy_state == 4)
            cells.push_back(p);
    }
    return cells;
}
std::optional<CombatPoint> destination(const std::vector<Position> &cells, DungeonLaunchTickets t) {
    if (t.destination < 0 || static_cast<std::size_t>(t.destination) >= cells.size() ||
        t.x_offset < 0 || t.x_offset >= 80 || t.z_offset < 0 || t.z_offset >= 80)
        return {};
    const auto p = cells[t.destination];
    return CombatPoint{p.x * 100.0F + (t.x_offset + 10) * 100.0F / 100.0F, 0,
                       p.y * 100.0F + (t.z_offset + 10) * 100.0F / 100.0F};
}
std::optional<DungeonLaunchTickets>
resolve_launch(const std::vector<Position> &cells, std::optional<DungeonLaunchTickets> prefix,
               const std::function<std::optional<int>(int)> &draw, DungeonWorldError &error) {
    if (prefix)
        return prefix;
    if (!draw) {
        error = DungeonWorldError::missing_ticket;
        return {};
    }
    std::array<int, 3> values{};
    const std::array<int, 3> bounds{static_cast<int>(cells.size()), 80, 80};
    for (std::size_t n = 0; n < values.size(); ++n) {
        const auto value = draw(bounds[n]);
        if (!value) {
            error = DungeonWorldError::missing_ticket;
            return {};
        }
        if (*value < 0 || *value >= bounds[n]) {
            error = DungeonWorldError::invalid_input;
            return {};
        }
        values[n] = *value;
    }
    return DungeonLaunchTickets{values[0], values[1], values[2]};
}
bool transition(DungeonWorldState &s, CharacterId id, int state) {
    auto &a = s.world.ai.battle.actors.at(id);
    ActorStateTransitionInput i;
    i.control = a.control;
    i.baseline = a.baseline;
    i.next_state = state;
    const auto r = prepare_actor_state_transition(i);
    if (!r)
        return false;
    a.control = r->control;
    a.baseline = r->baseline;
    a.state_counter = a.state_parameter = 0;
    if (r->clear_encounter)
        a.encounter.reset();
    return true;
}
const RescueFacility *facility_at(const DungeonWorldState &s, Position cell) {
    if (cell.x < 0 || cell.x >= s.world.map.width || cell.y < 0 || cell.y >= s.world.map.height)
        return nullptr;
    const auto &binding = s.world.map.cells[cell.y * s.world.map.width + cell.x].facility;
    if (!binding)
        return nullptr;
    const auto f = s.world.facilities.find(binding->instance_id.value);
    if (f == s.world.facilities.end() ||
        !(f->second.placement.instance_id == binding->instance_id) ||
        f->second.placement.definition_id != binding->definition_id)
        return nullptr;
    return &f->second;
}
} // namespace
std::optional<DungeonTaskSuccessCandidate>
prepare_dungeon_task_success(const DungeonTaskSuccessState &s, int definition, int year,
                             int month) {
    const auto old = s.definitions.find(definition);
    if (definition < 0 || old == s.definitions.end() || old->second.kind < 0 ||
        old->second.kind > 1 || old->second.completed < 0 || s.successes < 0 ||
        s.ordinary_explorations < 0 || s.exploration_stage < 0 || s.exploration_stage > 5 ||
        s.task_pool_progress < 0 || year < 0 || month < 0 || month >= 12)
        return {};
    const auto increment = [](int &value) {
        if (value == std::numeric_limits<int>::max())
            return false;
        ++value;
        return true;
    };
    DungeonTaskSuccessCandidate c{s, {}};
    auto &task = c.state.definitions.at(definition);
    if (!increment(c.state.successes) || !increment(task.completed) ||
        (task.kind == 0 && !(task.flags & 8U) && !increment(c.state.ordinary_explorations)))
        return {};
    if (task.flags & 2U) {
        c.state.exploration_dates[c.state.exploration_stage] = {year, month};
        c.state.exploration_stage = std::min(c.state.exploration_stage + 1, 5);
    }
    if (task.flags & 4U) {
        const auto monster = c.state.monsters.find(task.monster_definition);
        if (task.monster_definition < 0 || monster == c.state.monsters.end() ||
            monster->second.status < 0)
            return {};
        if (monster->second.status == 0)
            monster->second.pending_notice = true;
        monster->second.status = 1;
    }
    // 成功统计先于clear_active_task，当前刚完成的特殊任务本身也能抑制G增长。
    for (const int current : s.remaining_task_definitions) {
        const auto t = s.definitions.find(current);
        if (t == s.definitions.end())
            return {};
        if (t->second.flags & 2U)
            return c;
    }
    const int amount = task.kind == 0 ? 50 : 100;
    if (s.task_pool_progress > std::numeric_limits<int>::max() - amount)
        return {};
    c.state.task_pool_progress += amount;
    constexpr int thresholds[]{300, 400, 500};
    for (int n = 0; n < 3; ++n)
        if (s.task_pool_progress < thresholds[n] && c.state.task_pool_progress >= thresholds[n])
            c.threshold_notice_ids.push_back(29 + n);
    return c;
}
DungeonWorldResult prepare_world_dungeon_entry(const DungeonWorldState &s, CharacterId id,
                                               std::optional<int> ticket,
                                               const std::function<std::optional<int>(int)> &draw) {
    if (!live(s, id))
        return {DungeonWorldError::stale_actor, {}};
    const auto &a = s.world.ai.battle.actors.at(id);
    const auto &binding = s.world.actors.at(id).binding;
    if (!valid_legacy_map(s.world.map) || !binding || a.control.queue.empty() ||
        !valid_actor_control(a.control.queue.front()) || a.control.queue.front()[0] != 21)
        return {DungeonWorldError::invalid_input, {}};
    const auto *f = facility_at(s, binding->goal);
    if (!f || f->category != 5 || !s.facilities.count(f->placement.instance_id.value))
        return {DungeonWorldError::stale_facility, {}};
    const auto g = s.world.ai.growth.find(a.definition);
    if (g == s.world.ai.growth.end())
        return {DungeonWorldError::invalid_input, {}};
    const auto entry = prepare_dungeon_actor_entry(g->second.definition.profession_levels,
                                                   s.actors.at(id).constrained);
    if (!entry)
        return {DungeonWorldError::preparation_failed, {}};
    DungeonWorldCandidate c;
    c.state = s;
    auto &progress = c.state.facilities.at(f->placement.instance_id.value);
    if (f->occupants.empty() && progress.progress == 0 && progress.challenges.size() >= 6) {
        const int first_type = progress.challenges.front()[1];
        const bool uniform = std::all_of(progress.challenges.begin(), progress.challenges.end() - 1,
                                         [&](const auto &r) { return r[1] == first_type; });
        if (uniform && (first_type == 0 || first_type == 1)) {
            if (!ticket && draw)
                ticket = draw(2);
            if (!ticket)
                return {DungeonWorldError::missing_ticket, {}};
            if (*ticket < 0 || *ticket >= 2)
                return {DungeonWorldError::invalid_input, {}};
            c.entry_event = (first_type == 0 ? 168 : 170) + *ticket;
        }
    }
    c.state.actors.at(id) = *entry;
    c.state.world.ai.battle.actors.at(id).control.queue.erase(
        c.state.world.ai.battle.actors.at(id).control.queue.begin());
    c.state.world.facilities.at(f->placement.instance_id.value).occupants.push_back(id);
    return {DungeonWorldError::none, c};
}
DungeonWorldResult
prepare_world_dungeon_retreat(const DungeonWorldState &s, CharacterId id, Position origin,
                              TownBounds town, std::optional<DungeonLaunchTickets> tickets,
                              const std::function<std::optional<int>(int)> &draw) {
    if (!live(s, id))
        return {DungeonWorldError::stale_actor, {}};
    if (!valid_legacy_map(s.world.map) || !town_valid(town) || origin.x < 0 ||
        origin.x >= s.world.map.width || origin.y < 0 || origin.y >= s.world.map.height)
        return {DungeonWorldError::invalid_input, {}};
    DungeonWorldCandidate c;
    c.state = s;
    auto &a = c.state.world.ai.battle.actors.at(id);
    a.position.x = origin.x * 100.0F + 50.0F;
    a.position.z = origin.y * 100.0F + 50.0F;
    c.state.world.ai.contexts.at(id).cell = origin; // t/u/ax remain OLD until d projection.
    const auto cells = destinations(s.world.map, origin, town);
    if (cells.empty())
        return {DungeonWorldError::none, c};
    DungeonWorldError error{DungeonWorldError::none};
    tickets = resolve_launch(cells, tickets, draw, error);
    if (!tickets)
        return {error, {}};
    const auto target = destination(cells, *tickets);
    if (!target)
        return {DungeonWorldError::invalid_input, {}};
    a.attack_destination = *target;
    c.state.world.actors.at(id).horizontal_velocity = {(target->x - a.position.x) / 36.0F,
                                                       (target->z - a.position.z) / 36.0F};
    a.vertical_velocity = 80.0F / 17.0F;
    a.control.flags &= ~97U;
    const auto &binding = c.state.world.actors.at(id).binding;
    const auto *f = facility_at(c.state, origin);
    if (binding && f && arrival_binding_matches(c.state.world.map, *binding, origin)) {
        auto &crew = c.state.world.facilities.at(f->placement.instance_id.value).occupants;
        const auto first = std::find(crew.begin(), crew.end(), id);
        if (first != crew.end()) {
            crew.erase(first);
            c.removed_occupation = true;
        }
    }
    if (!transition(c.state, id, 20))
        return {DungeonWorldError::preparation_failed, {}};
    const auto hp = prepare_hp_assignment(a.hp, 0);
    if (!hp.candidate)
        return {DungeonWorldError::preparation_failed, {}};
    a.hp = *hp.candidate;
    a.control.action = 7;
    a.control.action_counter = 0;
    c.consumed_launches = 1;
    return {DungeonWorldError::none, c};
}
DungeonWorldResult prepare_world_dungeon_crew(const DungeonWorldState &s,
                                              const DungeonWorldCrewInput &i,
                                              const DungeonWorldRequestConsumer &consumer) {
    const auto old = s.world.facilities.find(i.facility);
    const auto p = s.facilities.find(i.facility);
    if (old == s.world.facilities.end() || p == s.facilities.end() || old->second.category != 5 ||
        old->second.status != 1)
        return {DungeonWorldError::stale_facility, {}};
    if (!town_valid(i.town) || !valid_legacy_map(s.world.map))
        return {DungeonWorldError::invalid_input, {}};
    DungeonWorldCandidate c;
    c.state = s;
    if (old->second.occupants.empty())
        return {DungeonWorldError::none, c}; // Original early return does not refresh i/j.
    DungeonCrewInput crew;
    crew.state.facility_updates = p->second.updates;
    crew.state.extent = p->second.extent;
    crew.state.progress = p->second.progress;
    crew.state.percent = p->second.percent;
    crew.state.previous_percent = p->second.previous_percent;
    crew.state.challenges = p->second.challenges;
    crew.state.occupants = old->second.occupants;
    crew.state.actors = s.actors;
    crew.active_task_extent = i.active_task_extent;
    // Resolve actual side effects in the same live-index order; the pure crew pass below owns
    // the progress arithmetic and verifies every matching resolution again.
    auto ages = s.actors;
    for (std::size_t n = old->second.occupants.size(); n > 0; --n) {
        const auto &live_crew = c.state.world.facilities.at(i.facility).occupants;
        if (n > live_crew.size() || !live(s, live_crew[n - 1]))
            return {DungeonWorldError::stale_actor, {}};
        const auto id = live_crew[n - 1];
        auto &a = ages.at(id);
        if (a.retreat_updates < 0 || a.retreat_updates == std::numeric_limits<int>::max())
            return {DungeonWorldError::invalid_input, {}};
        ++a.retreat_updates;
        if (a.retreat_state != 1 || a.retreat_updates < 20)
            continue;
        std::optional<DungeonLaunchTickets> launch;
        if (c.consumed_launches < i.launches.size())
            launch = i.launches[c.consumed_launches];
        const auto retreat = prepare_world_dungeon_retreat(
            c.state, id, old->second.placement.anchor, i.town, launch, i.draw);
        if (!retreat.candidate)
            return retreat;
        c.state = retreat.candidate->state;
        c.consumed_launches += retreat.candidate->consumed_launches;
        crew.retreats.push_back({id, retreat.candidate->removed_occupation});
    }
    const auto planned = prepare_dungeon_crew(crew);
    if (!planned.candidate)
        return {DungeonWorldError::preparation_failed, {}};
    const auto &next = planned.candidate->state;
    c.state.actors = next.actors;
    c.state.world.facilities.at(i.facility).occupants = next.occupants;
    auto &f = c.state.facilities.at(i.facility);
    f = {next.facility_updates, next.progress,         next.extent,
         next.percent,          next.previous_percent, next.challenges};
    c.state.world.facilities.at(i.facility).status = next.phase;
    c.completed = planned.candidate->completed;
    for (const auto &request : planned.candidate->requests) {
        DungeonWorldRequest record{request, {}, {}};
        if (request.kind == DungeonCrewRequestKind::grant_catalog_reward) {
            ObjectCommitState catalog;
            catalog.catalog = c.state.catalog;
            catalog.shops = c.state.shops;
            catalog.shop_order = c.state.shop_order;
            catalog.item_rewards = c.state.item_rewards;
            catalog.events = c.state.world.ai.battle.events;
            const auto r = prepare_object_grant(catalog, request.first, request.second,
                                                ObjectGrantOrigin::direct);
            if (!r.candidate)
                return {DungeonWorldError::preparation_failed, {}};
            c.state.catalog = r.candidate->state.catalog;
            c.state.shops = r.candidate->state.shops;
            c.state.item_rewards = r.candidate->state.item_rewards;
            c.state.world.ai.battle.events = r.candidate->state.events;
            record.catalog_requests = r.candidate->requests;
        } else if (request.kind == DungeonCrewRequestKind::spawn_catalog_reward) {
            const auto cells = destinations(s.world.map, old->second.placement.anchor, i.town);
            if (!cells.empty()) {
                std::optional<DungeonLaunchTickets> prefix;
                if (c.consumed_launches < i.launches.size())
                    prefix = i.launches[c.consumed_launches];
                DungeonWorldError error{DungeonWorldError::none};
                const auto launch = resolve_launch(cells, prefix, i.draw, error);
                if (!launch)
                    return {error, {}};
                const auto target = destination(cells, *launch);
                ++c.consumed_launches;
                const auto id = c.state.world.ai.battle.next_object_id;
                if (!target || !id || id == std::numeric_limits<std::uint64_t>::max() ||
                    c.state.world.ai.battle.objects.count(id))
                    return {DungeonWorldError::invalid_input, {}};
                const auto origin = old->second.placement.anchor;
                const auto object = prepare_ground_throw(
                    {id}, {origin.x * 100.0F + 50.0F, 0, origin.y * 100.0F + 50.0F}, *target,
                    request.first, request.second);
                if (!object)
                    return {DungeonWorldError::preparation_failed, {}};
                c.state.world.ai.battle.objects.emplace(id, *object);
                c.state.world.object_order.push_back(id);
                ++c.state.world.ai.battle.next_object_id;
                record.object = ObjectId{id};
            }
        } else if (request.kind == DungeonCrewRequestKind::exit_crew) {
            if (!request.actor || !live(c.state, *request.actor))
                return {DungeonWorldError::stale_actor, {}};
            auto &a = c.state.world.ai.battle.actors.at(*request.actor);
            a.control.flags &= ~1U;
            if (!transition(c.state, *request.actor, 0))
                return {DungeonWorldError::preparation_failed, {}};
            a.control.queue = {{1, request.first, 0}, {8, 0}};
        }
        if (consumer) {
            const auto consumed = consumer(c.state, record);
            if (!consumed)
                return {DungeonWorldError::preparation_failed, {}};
            c.state = *consumed;
        }
        c.requests.push_back(std::move(record));
    }
    return {DungeonWorldError::none, c};
}
DungeonWorldResult prepare_world_dungeon_landing(const DungeonWorldState &s, CharacterId id,
                                                 std::optional<Position> old_view,
                                                 const DungeonLandingDeparture &departure) {
    if (!live(s, id))
        return {DungeonWorldError::stale_actor, {}};
    const auto &old = s.world.ai.battle.actors.at(id);
    if (old.control.state != 20 || old.state_counter < 0)
        return {DungeonWorldError::invalid_input, {}};
    DungeonWorldCandidate c;
    c.state = s;
    auto &a = c.state.world.ai.battle.actors.at(id);
    const auto velocity = s.world.actors.at(id).horizontal_velocity;
    a.position.x += velocity.x;
    a.position.z += velocity.z;
    if (!character_world_cell({a.position.x, a.position.z}))
        return {DungeonWorldError::invalid_input, {}};
    if (old.state_counter >= 36) {
        if (!old_view || !departure)
            return {DungeonWorldError::invalid_input, {}};
        a.position.height = a.vertical_velocity = 0;
        c.state.world.actors.at(id).horizontal_velocity = {};
        c.state.world.ai.contexts.at(id).effects.display.push_back(
            {18, 0, old_view->x, old_view->y});
        if (!transition(c.state, id, 0))
            return {DungeonWorldError::preparation_failed, {}};
        const auto next = departure(c.state, id);
        if (!next || !live(*next, id))
            return {DungeonWorldError::preparation_failed, {}};
        c.state = *next;
        if (!transition(c.state, id, 2))
            return {DungeonWorldError::preparation_failed, {}};
        c.landed = true;
    }
    return {DungeonWorldError::none, c};
}
} // namespace ark::simulation::rules
