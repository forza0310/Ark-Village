#include "dungeon_village_reference/world_dungeon_finish.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
std::optional<std::size_t> cell_index(const LegacyMap &map, Position p) {
    if (p.x < 0 || p.y < 0 || p.x >= map.width || p.y >= map.height)
        return {};
    return static_cast<std::size_t>(p.y) * map.width + p.x;
}
bool add_int(int &value, int amount) {
    const auto sum = static_cast<std::int64_t>(value) + amount;
    if (sum < std::numeric_limits<int>::min() || sum > std::numeric_limits<int>::max())
        return false;
    value = static_cast<int>(sum);
    return true;
}
bool valid_task_reference(const DungeonFinishTask &task, std::uint64_t id) {
    return id && task.identity == id && (!task.facility || task.site.has_value()) &&
           (!task.facility || *task.facility != 0);
}
} // namespace
DungeonFinishResult prepare_world_dungeon_finish(const DungeonFinishState &s,
                                                 const DungeonFinishInput &input,
                                                 const DungeonFinishConsumer &consumer) {
    const auto fail = [](DungeonFinishError error) { return DungeonFinishResult{error, {}}; };
    const auto fi = s.dungeon.world.facilities.find(input.facility);
    const auto pi = s.dungeon.facilities.find(input.facility);
    const auto zi = s.sites.find(input.facility);
    if (fi == s.dungeon.world.facilities.end() || pi == s.dungeon.facilities.end() ||
        zi == s.sites.end() || fi->second.category != 5 || fi->second.status != 2 ||
        zi->second.occupied_cells.empty())
        return fail(DungeonFinishError::stale_facility);
    if (!valid_legacy_map(s.dungeon.world.map) ||
        s.surface.size() != s.dungeon.world.map.cells.size() || s.ground_definition < 0 ||
        pi->second.updates < 0)
        return fail(DungeonFinishError::invalid_input);
    DungeonFinishCandidate c{s, {}, {}, 0, false, false};
    if (pi->second.updates < 10)
        return {DungeonFinishError::none, std::move(c)};
    const Position source_site = zi->second.occupied_cells.front();
    if (!cell_index(s.dungeon.world.map, source_site))
        return fail(DungeonFinishError::invalid_input);
    DungeonCompletionInput rule;
    rule.updates = pi->second.updates;
    rule.source_site = source_site;
    rule.summary_tickets = input.summary_tickets;
    // 晚期201/92由实际候选检查；不沿用事件126之前的旧seen快照。
    rule.event201_seen = rule.event92_seen = true;
    for (const auto id : s.task_order) {
        const auto t = s.tasks.find(id);
        if (t == s.tasks.end() || !valid_task_reference(t->second, id))
            return fail(DungeonFinishError::stale_task);
        const auto d = s.task_progress.definitions.find(t->second.definition);
        if (d == s.task_progress.definitions.end())
            return fail(DungeonFinishError::stale_task);
        rule.site_tasks.push_back({d->second.kind, t->second.site});
    }
    for (const auto id : s.dungeon.world.ai.human_order) {
        const auto a = s.dungeon.world.ai.battle.actors.find(id);
        if (a == s.dungeon.world.ai.battle.actors.end() || a->second.kind != ActorKind::human)
            return fail(DungeonFinishError::invalid_input);
        rule.human_order.push_back({id, a->second.definition});
    }
    if (s.active_task) {
        const auto t = s.tasks.find(*s.active_task);
        if (t == s.tasks.end() || !valid_task_reference(t->second, *s.active_task))
            return fail(DungeonFinishError::stale_task);
        const auto d = s.task_progress.definitions.find(t->second.definition);
        if (d == s.task_progress.definitions.end())
            return fail(DungeonFinishError::stale_task);
        std::vector<DungeonChallenge> challenges;
        if (d->second.kind == 0) {
            if (!t->second.facility)
                return fail(DungeonFinishError::stale_facility);
            const auto f = s.dungeon.facilities.find(*t->second.facility);
            if (f == s.dungeon.facilities.end())
                return fail(DungeonFinishError::stale_facility);
            challenges = f->second.challenges;
            for (const int participant : s.participants)
                if (!s.human_definition_flags.count(participant))
                    return fail(DungeonFinishError::invalid_input);
        }
        rule.active_task = DungeonCompletionTask{
            d->second.kind, t->second.difficulty, s.participants,
            challenges,     t->second.definition, t->second.pending_completion_value};
        if (d->second.kind == 0 && !s.participants.empty() && s.participants.size() <= 100000 &&
            input.draw) {
            for (std::size_t n = 0; n < std::min<std::size_t>(2, rule.summary_tickets.size()); ++n)
                if (rule.summary_tickets[n] < 0 ||
                    static_cast<std::size_t>(rule.summary_tickets[n]) >= s.participants.size())
                    return fail(DungeonFinishError::invalid_input);
            while (rule.summary_tickets.size() < 2) {
                const auto ticket = input.draw(static_cast<int>(s.participants.size()));
                if (!ticket || *ticket < 0 ||
                    static_cast<std::size_t>(*ticket) >= s.participants.size())
                    return fail(DungeonFinishError::invalid_input);
                rule.summary_tickets.push_back(*ticket);
            }
        }
    }
    const auto plan = prepare_dungeon_completion(rule);
    if (!plan)
        return fail(DungeonFinishError::invalid_input);
    c.consumed_summary_tickets = plan->consumed_summary_tickets;
    DungeonFinishError effect_error{DungeonFinishError::none};
    const auto effect = [&](DungeonFinishEffect e) {
        if (!consumer) {
            effect_error = DungeonFinishError::missing_consumer;
            return false;
        }
        const auto next = consumer(c.state, e);
        if (!next || !valid_legacy_map(next->dungeon.world.map) ||
            next->surface.size() != next->dungeon.world.map.cells.size()) {
            effect_error = DungeonFinishError::consumer_failed;
            return false;
        }
        c.state = *next;
        c.effects.push_back(std::move(e));
        return true;
    };
    const auto event = [&](int id) {
        const auto before = c.state.event_calls.find(id);
        const int count = before == c.state.event_calls.end() ? 0 : before->second;
        if (count < 0 || count == std::numeric_limits<int>::max()) {
            effect_error = DungeonFinishError::numeric_overflow;
            return false;
        }
        if (!effect({DungeonFinishEffectKind::event, {}, {}, c.state.active_task, id, 0, {}}))
            return false;
        const auto after = c.state.event_calls.find(id);
        if (after == c.state.event_calls.end() || after->second <= count) {
            effect_error = DungeonFinishError::consumer_failed;
            return false;
        }
        return true;
    };
    for (const auto &r : plan->requests) {
        c.requests.push_back(r);
        switch (r.kind) {
        case DungeonCompletionRequestKind::actor_reward_display:
            if (!effect({DungeonFinishEffectKind::actor_reward_display,
                         r.actor,
                         {},
                         c.state.active_task,
                         r.first,
                         r.second,
                         {}}))
                return fail(effect_error);
            break;
        case DungeonCompletionRequestKind::definition_reward: {
            const auto a = c.state.dungeon.world.ai.battle.actors.find(*r.actor);
            if (a == c.state.dungeon.world.ai.battle.actors.end())
                return fail(DungeonFinishError::invalid_input);
            const auto g = c.state.dungeon.world.ai.growth.find(a->second.definition);
            if (g == c.state.dungeon.world.ai.growth.end())
                return fail(DungeonFinishError::invalid_input);
            const auto pending = prepare_delayed_reward(g->second.pending, r.second, r.first);
            if (!pending)
                return fail(DungeonFinishError::numeric_overflow);
            g->second.pending = *pending;
            break;
        }
        case DungeonCompletionRequestKind::clear_boost: {
            const auto a = c.state.dungeon.world.ai.battle.actors.find(*r.actor);
            if (a == c.state.dungeon.world.ai.battle.actors.end())
                return fail(DungeonFinishError::invalid_input);
            a->second.control.flags &= ~std::uint32_t{2048};
            break;
        }
        case DungeonCompletionRequestKind::summary30:
        case DungeonCompletionRequestKind::summary32:
            if (!effect({r.kind == DungeonCompletionRequestKind::summary30
                             ? DungeonFinishEffectKind::summary30
                             : DungeonFinishEffectKind::summary32,
                         {},
                         {},
                         c.state.active_task,
                         r.first,
                         r.second,
                         r.kind == DungeonCompletionRequestKind::summary32
                             ? plan->summary_rewards
                             : std::vector<std::array<int, 2>>{}}))
                return fail(effect_error);
            break;
        case DungeonCompletionRequestKind::event126:
            if (!event(126))
                return fail(effect_error);
            break;
        case DungeonCompletionRequestKind::record_complete:
            if (!add_int(c.state.dungeon.world.ai.pending_completion, r.first))
                return fail(DungeonFinishError::numeric_overflow);
            break;
        case DungeonCompletionRequestKind::restore_site: {
            const auto index = cell_index(c.state.dungeon.world.map, source_site);
            if (!index)
                return fail(DungeonFinishError::invalid_input);
            const auto binding = c.state.dungeon.world.map.cells[*index].facility;
            if (!binding)
                break; // a/o:无当前实例直接返回；后续任务消费者仍继续。
            const auto id = binding->instance_id.value;
            const auto site = c.state.sites.find(id);
            const auto facility = c.state.dungeon.world.facilities.find(id);
            if (site == c.state.sites.end() || site->second.occupied_cells.empty() ||
                facility == c.state.dungeon.world.facilities.end())
                return fail(DungeonFinishError::stale_facility);
            std::set<std::size_t> cells;
            for (const Position p : site->second.occupied_cells) {
                const auto n = cell_index(c.state.dungeon.world.map, p);
                if (!n || !cells.insert(*n).second)
                    return fail(DungeonFinishError::invalid_input);
                const auto &cell = c.state.dungeon.world.map.cells[*n];
                if (!cell.facility || cell.facility->instance_id.value != id ||
                    cell.facility->definition_id != facility->second.placement.definition_id)
                    return fail(DungeonFinishError::stale_facility);
            }
            for (std::size_t n = 0; n < c.state.dungeon.world.map.cells.size(); ++n) {
                const auto &b = c.state.dungeon.world.map.cells[n].facility;
                if (b && b->instance_id.value == id && !cells.count(n))
                    return fail(DungeonFinishError::stale_facility);
            }
            const auto positions = site->second.occupied_cells;
            for (const Position p : positions) {
                const auto n = *cell_index(c.state.dungeon.world.map, p);
                auto &cell = c.state.dungeon.world.map.cells[n];
                cell.facility.reset();
                cell.legacy_state = 4;
                cell.category = RouteCategory::ground;
                auto &surface = c.state.surface[n];
                surface.definition = c.state.ground_definition;
                surface.updates = 0;
                surface.instance = surface.fragment = -1;
                if (r.first &&
                    !effect({DungeonFinishEffectKind::site_success_display, {}, p, {}, 0, 0, {}}))
                    return fail(effect_error);
            }
            // 页面/任务可以继续持有被移除设施的身份；不清人物O、路线或旧o。
            c.state.dungeon.world.facilities.erase(id);
            c.state.dungeon.facilities.erase(id);
            c.state.sites.erase(id);
            for (const auto kind : {DungeonFinishEffectKind::rebuild_display,
                                    DungeonFinishEffectKind::rebuild_roads_fences,
                                    DungeonFinishEffectKind::refresh_scene,
                                    DungeonFinishEffectKind::rebuild_neighbours})
                if (!effect({kind, {}, {}, {}, r.first, 0, {}}))
                    return fail(effect_error);
            c.site_restored = true;
            break;
        }
        case DungeonCompletionRequestKind::record_task_success: {
            auto projection = c.state.task_progress;
            projection.remaining_task_definitions.clear();
            projection.monsters.clear();
            for (const auto id : c.state.task_order) {
                const auto t = c.state.tasks.find(id);
                if (t == c.state.tasks.end())
                    return fail(DungeonFinishError::stale_task);
                projection.remaining_task_definitions.push_back(t->second.definition);
            }
            for (const auto &m : c.state.dungeon.world.ai.monster_growth)
                projection.monsters.emplace(
                    m.first, DungeonMonsterAvailability{m.second.status, m.second.newly_unlocked});
            const auto success = prepare_dungeon_task_success(projection, r.first, c.state.raw_year,
                                                              c.state.raw_month);
            if (!success)
                return fail(DungeonFinishError::invalid_input);
            c.state.task_progress = success->state;
            for (const auto &m : success->state.monsters) {
                auto &original = c.state.dungeon.world.ai.monster_growth.at(m.first);
                original.status = m.second.status;
                original.newly_unlocked = m.second.pending_notice;
            }
            // 临时monster/任务名单投影不保存第二份长期权威。
            c.state.task_progress.monsters.clear();
            c.state.task_progress.remaining_task_definitions.clear();
            for (const int notice : success->threshold_notice_ids)
                if (!effect({DungeonFinishEffectKind::threshold_notice, {}, {}, {}, notice, 0, {}}))
                    return fail(effect_error);
            break;
        }
        case DungeonCompletionRequestKind::clear_active_task:
            for (auto &d : c.state.human_definition_flags)
                d.second &= ~std::uint32_t{2};
            if (c.state.active_task) {
                const auto i = std::find(c.state.task_order.begin(), c.state.task_order.end(),
                                         *c.state.active_task);
                if (i != c.state.task_order.end())
                    c.state.task_order.erase(i); // Vector.removeElement只移除首个引用。
                c.state.active_task.reset();
            }
            c.state.dungeon.world.ai.task_active = false;
            c.task_cleared = true;
            break;
        case DungeonCompletionRequestKind::remove_site_task:
            if (r.first < 0 || static_cast<std::size_t>(r.first) >= c.state.task_order.size())
                return fail(DungeonFinishError::stale_task);
            c.state.task_order.erase(c.state.task_order.begin() + r.first);
            break;
        case DungeonCompletionRequestKind::event201:
        case DungeonCompletionRequestKind::event92:
            return fail(DungeonFinishError::invalid_input); // 不消费旧快照的晚期seen计划。
        }
    }
    if (rule.active_task && rule.active_task->kind == 0)
        for (const auto pair : {std::pair<int, DungeonCompletionRequestKind>{
                                    201, DungeonCompletionRequestKind::event201},
                                {92, DungeonCompletionRequestKind::event92}}) {
            const auto seen = c.state.event_calls.find(pair.first);
            if (seen != c.state.event_calls.end() && seen->second > 0)
                continue;
            c.requests.push_back({pair.second, {}, 0, 0});
            if (!event(pair.first))
                return fail(effect_error);
        }
    return {DungeonFinishError::none, std::move(c)};
}
} // namespace dungeon_village_reference
