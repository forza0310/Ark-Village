#include "dungeon_village_reference/world_actor_routes.hpp"

#include <algorithm>
#include <cmath>

namespace dungeon_village_reference {
namespace {
bool same_map(const LegacyMap &a, const LegacyMap &b) {
    if (!valid_legacy_map(a) || !valid_legacy_map(b) || a.width != b.width ||
        a.height != b.height || a.cells.size() != b.cells.size())
        return false;
    for (std::size_t n = 0; n < a.cells.size(); ++n) {
        const auto &x = a.cells[n];
        const auto &y = b.cells[n];
        if (x.legacy_state != y.legacy_state || x.category != y.category ||
            x.facility.has_value() != y.facility.has_value())
            return false;
        if (x.facility && (!(x.facility->instance_id == y.facility->instance_id) ||
                           x.facility->definition_id != y.facility->definition_id ||
                           x.facility->fragment_index != y.facility->fragment_index))
            return false;
    }
    return true;
}
bool live(const WorldActorRoutesState &s, CharacterId id) {
    const auto a = s.world.ai.battle.actors.find(id);
    if (!id.value || a == s.world.ai.battle.actors.end() || !(a->second.id == id) ||
        !s.world.actors.count(id) || !s.world.ai.contexts.count(id))
        return false;
    const auto &order =
        a->second.kind == ActorKind::human ? s.world.ai.human_order : s.world.ai.monster_order;
    return std::count(order.begin(), order.end(), id) == 1;
}
DungeonWorldState dungeon(const WorldActorRoutesState &s) {
    return {s.world, s.dungeon_facilities, s.dungeon_actors, s.catalog,
            s.shops, s.shop_order,         s.item_rewards};
}
bool matching_item_definitions(const std::map<int, ObjectCatalogRecord> &items,
                               const std::map<std::pair<int, int>, ObjectCatalogRecord> &catalog) {
    for (const auto &item : items)
        if (item.first < 0 || !catalog.count({0, item.first}))
            return false;
    for (const auto &record : catalog)
        if (record.first.first == 0 && !items.count(record.first.second))
            return false;
    return true;
}
bool write_dungeon(WorldActorRoutesState &s, DungeonWorldState d) {
    if (!matching_item_definitions(s.items, d.catalog))
        return false;
    // 探索/地下城按catalog授予道具；先回写完整原p/q/r/z，再保留该次奖励目录。
    for (auto &item : s.items)
        item.second = d.catalog.at({0, item.first});
    s.world = std::move(d.world);
    s.dungeon_facilities = std::move(d.facilities);
    s.dungeon_actors = std::move(d.actors);
    s.catalog = std::move(d.catalog);
    s.shops = std::move(d.shops);
    s.shop_order = std::move(d.shop_order);
    s.item_rewards = d.item_rewards;
    return true;
}
ShopWorldState shop(const WorldActorRoutesState &s) {
    return {s.world, s.shop_humans, s.shop_actors, s.items, s.popularity_queue};
}
bool write_shop(WorldActorRoutesState &s, ShopWorldState d) {
    if (!matching_item_definitions(d.items, s.catalog))
        return false;
    // 真实商店到达通过items执行携物交付；不能反向取旧catalog盖掉刚加的z和解锁。
    for (const auto &item : d.items)
        s.catalog.at({0, item.first}) = item.second;
    s.world = std::move(d.world);
    s.shop_humans = std::move(d.humans);
    s.shop_actors = std::move(d.actors);
    s.items = std::move(d.items);
    s.popularity_queue = std::move(d.popularity_queue);
    return true;
}
const RescueFacility *current_facility(const WorldActorRoutesState &s, CharacterId id,
                                       bool occupation) {
    const auto &p = s.world.actors.at(id);
    const auto goal = p.destination ? p.destination
                      : p.binding   ? std::optional<Position>(p.binding->goal)
                                    : std::optional<Position>{};
    if (occupation) {
        if (!goal || goal->x < 0 || goal->y < 0 || goal->x >= s.world.map.width ||
            goal->y >= s.world.map.height)
            return nullptr;
        const auto &cell =
            s.world.map.cells[static_cast<std::size_t>(goal->y) * s.world.map.width + goal->x];
        if (!cell.facility)
            return nullptr;
        const auto f = s.world.facilities.find(cell.facility->instance_id.value);
        return f == s.world.facilities.end() ? nullptr : &f->second;
    }
    if (!p.binding ||
        !arrival_binding_matches(s.world.map, *p.binding, s.world.ai.contexts.at(id).cell))
        return nullptr;
    const auto f = s.world.facilities.find(p.binding->instance_id.value);
    return f == s.world.facilities.end() ? nullptr : &f->second;
}
bool expression(WorldActorRoutesState &s, CharacterId id, int type,
                const WorldExpressionTicket &t) {
    const auto r = prepare_actor_expression(
        {s.world.ai.contexts.at(id).effects, type, 0, t.probability, t.variant_count, t.variant});
    if (!r.candidate)
        return false;
    s.world.ai.contexts.at(id).effects = r.candidate->state;
    return true;
}
bool event(WorldActorRoutesState &s, int id, const WorldActorEventConsumer &consumer,
           std::vector<int> &audit) {
    if (!consumer)
        return false;
    try {
        auto next = consumer(s, id);
        if (!next)
            return false;
        s = std::move(*next);
        audit.push_back(id);
        return true;
    } catch (...) {
        return false;
    }
}
float death_arc(int counter) {
    // bi[0]=12，d.b(20,12,B)，与逻辑n高度分离。
    constexpr float v = 40.0F / 5.0F;
    constexpr float acceleration = -40.0F / 30.0F;
    const float t = static_cast<float>(counter);
    return std::max(0.0F, v * t + acceleration * t * (t + 1.0F) / 2.0F);
}
} // namespace
WorldActorDecisionResult prepare_world_actor_decision(const WorldActorRoutesState &s,
                                                      const WorldActorDecisionInput &i) {
    const auto fail = [](WorldActorRouteError e) { return WorldActorDecisionResult{e, {}}; };
    if (!live(s, i.actor))
        return fail(WorldActorRouteError::stale_actor);
    const auto &old = s.world.ai.battle.actors.at(i.actor);
    if (!world_control_detail::valid_control(old.control) || old.state_counter < 0 ||
        !valid_world_map_facts(s.facts) || !same_map(s.world.map, s.facts.map))
        return fail(WorldActorRouteError::invalid_input);
    WorldActorDecisionCandidate c;
    c.state = s;
    const auto draw = [&](int bound) -> std::optional<int> {
        const auto r = c.state.random.draw(bound);
        return r.error == WorldRandomError::none ? std::optional<int>(r.ticket) : std::nullopt;
    };
    const auto expression_draw = [&](const ActorEffectState &effects,
                                     int type) -> std::optional<WorldExpressionTicket> {
        if (!i.primary_expression_table)
            return {};
        return prepare_world_random_expression(c.state.random, effects, type, 0,
                                               *i.primary_expression_table)
            .ticket;
    };
    const int state = old.control.state;
    if (state == 0 || state == 5 || state == 8 || state == 9 || state == 11) {
        auto input = i.daily;
        input.actor = i.actor;
        if (i.encounter)
            input.encounter =
                [&](const AiRewardState &ai,
                    const EncounterCreationRequest &request) -> std::optional<AiRewardState> {
                c.state.world.ai = ai;
                const auto result = i.encounter(c.state, request);
                if (!result)
                    return {};
                c.state = *result;
                return c.state.world.ai;
            };
        if (input.path && old.object_slot == -2 && !input.path->facility_consumer) {
            // P先清路线再递归交付被救者；两人的使用计划与原救援领域共同提交。
            input.path->facility_consumer = [&](const RescueWorldState &world,
                                                const WorldPathFacilityRequest &request)
                -> std::optional<WorldPathFacilityCandidate> {
                if (!request.human_arrival_and_use || !(request.actor == i.actor))
                    return {};
                const auto f = world.facilities.find(request.binding.instance_id.value);
                if (f == world.facilities.end())
                    return {};
                RescueDeliveryProjection projection;
                if (f->second.category == 8 && f->second.detail == 2) {
                    const auto rescued = world.ai.battle.actors.at(i.actor).rescue;
                    if (!i.rescue_direction_target || !i.use_shared_random || !rescued)
                        return {};
                    projection.rescued_direction = draw(4);
                    if (!projection.rescued_direction)
                        return {};
                    projection.rescued_target =
                        i.rescue_direction_target(*rescued, *projection.rescued_direction);
                    projection.carrier_direction = draw(4);
                    if (!projection.carrier_direction)
                        return {};
                    projection.carrier_target =
                        i.rescue_direction_target(i.actor, *projection.carrier_direction);
                }
                const auto r = prepare_world_rescue_delivery(world, i.actor, projection);
                if (!r.candidate)
                    return {};
                c.state.world = r.candidate->state;
                return WorldPathFacilityCandidate{c.state.world};
            };
        } else if (input.path && i.shop_arrival && !input.path->facility_consumer) {
            input.path->facility_consumer = [&](const RescueWorldState &world,
                                                const WorldPathFacilityRequest &request)
                -> std::optional<WorldPathFacilityCandidate> {
                if (!request.human_arrival_and_use || !(request.actor == i.actor) ||
                    !(i.shop_arrival->actor == i.actor))
                    return {};
                auto projected = shop(c.state);
                projected.world = world;
                auto arrival = *i.shop_arrival;
                if (i.use_shared_random)
                    arrival.draw = draw;
                const auto r = prepare_world_shop_arrival(projected, arrival);
                if (!r.candidate)
                    return {};
                if (!write_shop(c.state, r.candidate->state))
                    return {};
                c.shop_requests.insert(c.shop_requests.end(), r.candidate->requests.begin(),
                                       r.candidate->requests.end());
                for (const auto &presentation : r.candidate->requests)
                    if (i.presentation) {
                        const auto p =
                            i.presentation(c.state, {i.actor, {}, {}, {}, {}, presentation});
                        if (!p)
                            return {};
                        c.state = *p;
                    }
                return WorldPathFacilityCandidate{c.state.world};
            };
        }
        if (i.use_shared_random) {
            input.draw = draw;
            input.expression_draw = expression_draw;
            if (input.path) {
                input.path->draw = draw;
                input.path->expression_draw = expression_draw;
                if (input.task_creation && !input.path->task_attempt)
                    input.path->task_attempt =
                        [&, source = *input.task_creation](
                            const AiRewardState &ai, const WorldMapFacts &facts, CharacterId actor,
                            const WorldEventTask &task) -> std::optional<WorldEventEntryCandidate> {
                        auto entry = source;
                        entry.actor = actor;
                        entry.task = task;
                        entry.draw = draw;
                        return prepare_world_event_entry(ai, facts, entry).candidate;
                    };
            }
        }
        input.event = [&](const RescueWorldState &world,
                          int id) -> std::optional<RescueWorldState> {
            c.state.world = world;
            if (!event(c.state, id, i.event, c.consumed_events))
                return {};
            return c.state.world;
        };
        const auto r = prepare_world_daily_c({s.world, s.facts, s.task}, input);
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world = r.candidate->state.world;
        c.state.facts = r.candidate->state.facts;
        c.state.task = r.candidate->state.task;
        // 同一次daily按旧状态分派，直接F和路径P不能同时给出创建载荷。
        if (r.candidate->task_entry && r.candidate->path)
            return fail(WorldActorRouteError::invalid_input);
        const auto publish_task_start = [&](std::optional<std::uint64_t> created,
                                            bool music2, bool notice24) {
            if (music2 != created.has_value() || notice24 != created.has_value() ||
                (created && c.state.task.encounter != created))
                return false;
            if (!created || !i.presentation)
                return true; // 无消费者的纯规则调用仍保留完整daily审计产物。
            WorldActorPresentationRequest request;
            request.actor = i.actor;
            request.task_encounter_start = created;
            const auto presented = i.presentation(c.state, request);
            if (!presented)
                return false;
            c.state = *presented;
            return true;
        };
        if (r.candidate->task_entry &&
            !publish_task_start(r.candidate->task_entry->created,
                                r.candidate->task_entry->music2,
                                r.candidate->task_entry->notice24))
            return fail(WorldActorRouteError::consumer_failed);
        if (r.candidate->path &&
            !publish_task_start(r.candidate->path->created_task_encounter,
                                r.candidate->path->music2, r.candidate->path->notice24))
            return fail(WorldActorRouteError::consumer_failed);
        for (const int id : r.candidate->event_requests)
            if (!event(c.state, id, i.event, c.consumed_events))
                return fail(WorldActorRouteError::missing_consumer);
        c.delete_requested = r.candidate->path && r.candidate->path->delete_instance;
        // P的true只是删除请求，尚未实际从bm移除；由外层调度决定删除时点。
        c.daily = r.candidate;
        if (r.candidate->ground_effect21)
            c.lifecycle_requests.push_back({LifecycleRequestKind::ground_effect, 21});
        if (r.candidate->path && r.candidate->path->ground_effect20)
            c.lifecycle_requests.push_back({LifecycleRequestKind::ground_effect, 20});
    } else if (state == 1) {
        if (!i.combat || !(i.combat->actor == i.actor))
            return fail(WorldActorRouteError::missing_fact);
        auto input = *i.combat;
        if (i.use_shared_random) {
            input.draw = draw;
            input.expression = [&](CharacterId, const ActorEffectState &effects, int type,
                                   int delay) -> std::optional<ActorEffectState> {
                if (!i.primary_expression_table)
                    return {};
                const auto result = prepare_world_random_expression(
                    c.state.random, effects, type, delay, *i.primary_expression_table);
                return result.candidate ? std::optional<ActorEffectState>(result.candidate->state)
                                        : std::nullopt;
            };
        }
        const auto r = prepare_world_combat_policy(s.world.ai, input, s.facts);
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world.ai = r.candidate->state;
        c.attack_requests = r.candidate->requests;
        for (const auto &request : r.candidate->requests)
            if (i.presentation) {
                const auto p =
                    i.presentation(c.state, {request.actor, request.target, request, {}, {}, {}});
                if (!p)
                    return fail(WorldActorRouteError::consumer_failed);
                c.state = *p;
            }
        if (!s.world.ai.battle.events.count(116) && c.state.world.ai.battle.events.count(116) &&
            !event(c.state, 116, i.event, c.consumed_events))
            return fail(WorldActorRouteError::missing_consumer);
    } else if (state == 3) {
        if (old.kind != ActorKind::monster)
            return fail(WorldActorRouteError::invalid_input);
        c.state.world.ai.battle.actors.at(i.actor).attack_position.height =
            death_arc(old.state_counter);
        const auto r = prepare_monster_death_commit(c.state.world.ai, i.actor);
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world.ai = r.candidate->state;
        c.removed = r.candidate->removed;
        c.lifecycle_requests = r.candidate->death_requests;
    } else if (state == 13) {
        if (!i.actor_box || !i.rescue_box)
            return fail(WorldActorRouteError::missing_fact);
        const auto r = prepare_world_rescue_seek(s.world, i.actor, *i.actor_box, *i.rescue_box);
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world = r.candidate->state;
        c.lifecycle_requests = r.candidate->requests;
        if (r.candidate->binding_action == RescueBindingAction::bind) {
            auto ticket = i.rescue_expression;
            if (!ticket && i.use_shared_random)
                ticket = expression_draw(c.state.world.ai.contexts.at(i.actor).effects, 13);
            if (!ticket || !expression(c.state, i.actor, 13, *ticket))
                return fail(WorldActorRouteError::missing_fact);
            if (i.presentation) {
                WorldActorPresentationRequest request;
                request.actor = i.actor;
                request.cached_sound = 7;
                const auto p = i.presentation(c.state, request);
                if (!p)
                    return fail(WorldActorRouteError::consumer_failed);
                c.state = *p;
            }
        }
    } else if (state == 15) {
        auto ticket = i.special_expression;
        if (old.state_counter == 15 && !ticket && i.use_shared_random)
            ticket = expression_draw(s.world.ai.contexts.at(i.actor).effects, 16);
        const auto r = prepare_world_special_entry_c(s.world, i.actor, s.facts.town, ticket);
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world = r.candidate->state;
        if (r.candidate->ground_effect)
            c.lifecycle_requests.push_back({LifecycleRequestKind::landing_effect, 18});
    } else if (state == 17) {
        auto path = i.monster_path;
        if (path) {
            path->actor = i.actor;
            path->facts = s.facts;
            path->task = s.task;
            if (i.use_shared_random) {
                path->draw = draw;
                path->expression_draw = expression_draw;
            }
        }
        const auto r = prepare_world_monster_act_c(s.world, {i.actor, path});
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world = r.candidate->state;
        if (r.candidate->path) {
            c.state.facts = r.candidate->path->facts;
            c.state.task = r.candidate->path->task;
            if (r.candidate->path->event116 && !event(c.state, 116, i.event, c.consumed_events))
                return fail(WorldActorRouteError::missing_consumer);
        }
        c.monster = r.candidate;
    } else if (state == 18) {
        const auto r = prepare_world_battle_preparation(s.world.ai, i.actor, s.facts,
                                                        s.world.actors.at(i.actor).monster_mode);
        if (!r.state)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world.ai = *r.state;
    } else if (state == 20) {
        WorldActorRoutesState scratch = c.state;
        const auto r = prepare_world_dungeon_landing(
            dungeon(s), i.actor, i.cached_view,
            [&](const DungeonWorldState &d, CharacterId id) -> std::optional<DungeonWorldState> {
                if (!i.landing_departure || !(i.landing_departure->actor == id))
                    return {};
                if (!write_dungeon(scratch, d))
                    return {};
                auto input = *i.landing_departure;
                if (i.use_shared_random && !input.draw)
                    input.draw = [&](int bound) -> std::optional<std::int64_t> {
                        const auto r = scratch.random.draw(bound);
                        return r.error == WorldRandomError::none
                                   ? std::optional<std::int64_t>(r.ticket)
                                   : std::nullopt;
                    };
                const auto depart = prepare_world_departure(scratch.world, input);
                if (!depart.candidate)
                    return {};
                scratch.world = depart.candidate->state;
                if (depart.candidate->event116 && !event(scratch, 116, i.event, c.consumed_events))
                    return {};
                return dungeon(scratch);
            });
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state = std::move(scratch);
        if (!write_dungeon(c.state, r.candidate->state))
            return fail(WorldActorRouteError::missing_fact);
    } else {
        auto input = i.lifecycle;
        input.actor = i.actor;
        if (i.use_shared_random)
            input.expression_draw = expression_draw;
        const auto r = prepare_world_lifecycle_c(s.world, input);
        if (!r.candidate)
            return fail(WorldActorRouteError::preparation_failed);
        c.state.world = r.candidate->state;
        c.lifecycle = r.candidate;
    }
    for (const auto &request : c.lifecycle_requests)
        if (i.presentation && (request.kind == LifecycleRequestKind::ground_effect ||
                               request.kind == LifecycleRequestKind::normal_death_rewards ||
                               request.kind == LifecycleRequestKind::cancelled_death_effect ||
                               request.kind == LifecycleRequestKind::landing_effect)) {
            const auto p =
                i.presentation(c.state, {i.actor, {}, {}, {}, {}, {}, request, old.definition});
            if (!p)
                return fail(WorldActorRouteError::consumer_failed);
            c.state = *p;
        }
    return {WorldActorRouteError::none, std::move(c)};
}

WorldActorControlResult prepare_world_actor_control(const WorldActorRoutesState &s, CharacterId id,
                                                    const WorldActorCommandProvider &provider,
                                                    std::size_t budget) {
    if (!live(s, id))
        return {WorldActorRouteError::stale_actor, {}, WorldControlError::stale_actor};
    if (!valid_world_map_facts(s.facts) || !same_map(s.world.map, s.facts.map))
        return {WorldActorRouteError::invalid_input, {}, WorldControlError::invalid_adapter};
    WorldActorControlCandidate audit;
    WorldActorRouteError error{WorldActorRouteError::none};
    WorldControlAdapter<WorldActorRoutesState> adapter;
    adapter.read = [](const auto &owner, CharacterId actor) -> const ActorControlState * {
        const auto a = owner.world.ai.battle.actors.find(actor);
        return a == owner.world.ai.battle.actors.end() ? nullptr : &a->second.control;
    };
    adapter.write = [](auto &owner, CharacterId actor, const auto &control) {
        const auto a = owner.world.ai.battle.actors.find(actor);
        if (a == owner.world.ai.battle.actors.end())
            return false;
        a->second.control = control;
        return true;
    };
    adapter.domain =
        [&](const WorldActorRoutesState &owner,
            CharacterId actor) -> std::optional<WorldControlStep<WorldActorRoutesState>> {
        if (!provider) {
            error = WorldActorRouteError::missing_consumer;
            return {};
        }
        const auto &command = owner.world.ai.battle.actors.at(actor).control.queue.front();
        std::optional<WorldActorCommandInput> provided;
        try {
            provided = provider(owner, actor, command);
        } catch (...) {
            error = WorldActorRouteError::consumer_failed;
            return {};
        }
        if (!provided) {
            error = WorldActorRouteError::missing_fact;
            return {};
        }
        auto i = *provided;
        WorldControlStep<WorldActorRoutesState> next{owner};
        const auto fail = [&]() -> std::optional<WorldControlStep<WorldActorRoutesState>> {
            error = WorldActorRouteError::preparation_failed;
            return {};
        };
        const int op = command[0];
        const auto draw = [&](int bound) -> std::optional<int> {
            const auto r = next.state.random.draw(bound);
            return r.error == WorldRandomError::none ? std::optional<int>(r.ticket) : std::nullopt;
        };
        if (i.use_shared_random && op == 2 && command[1] == 18 &&
            !(owner.world.ai.battle.actors.at(actor).control.flags & 2048U) && !i.boost_ticket) {
            i.boost_ticket = draw(100);
            if (!i.boost_ticket)
                return fail();
        }
        if (op == 2) {
            const auto r = prepare_world_state_command(owner.world, {actor, i.boost_ticket});
            if (!r.candidate)
                return fail();
            next.state.world = r.candidate->state;
            for (const int e : r.candidate->event_requests)
                if (!event(next.state, e, i.event, audit.consumed_events))
                    return fail();
        } else if (op == 8) {
            if (!i.departure || !(i.departure->departure.actor == actor))
                return fail();
            if (i.use_shared_random && !i.departure->departure.draw)
                i.departure->departure.draw = [&](int bound) -> std::optional<std::int64_t> {
                    const auto ticket = draw(bound);
                    return ticket ? std::optional<std::int64_t>(*ticket) : std::nullopt;
                };
            if (i.use_shared_random && !i.departure->failure_expression)
                i.departure->expression_draw =
                    [&](const ActorEffectState &effects,
                        int type) -> std::optional<WorldExpressionTicket> {
                    if (!i.primary_expression_table)
                        return {};
                    return prepare_world_random_expression(next.state.random, effects, type, 0,
                                                           *i.primary_expression_table)
                        .ticket;
                };
            const auto r = prepare_world_departure_control(owner.world, *i.departure);
            if (!r.candidate)
                return fail();
            next.state.world = r.candidate->state;
            next.action = r.candidate->delete_instance ? WorldControlAction::delete_true
                          : r.candidate->departure_succeeded
                              ? WorldControlAction::hold_false
                              : WorldControlAction::continue_same_call;
            // o的116已记录在battle.events，实际解释器仍必须同步消费一次。
            if (!owner.world.ai.battle.events.count(116) &&
                next.state.world.ai.battle.events.count(116) &&
                !event(next.state, 116, i.event, audit.consumed_events))
                return fail();
        } else if (op == 10 || op == 12 || op == 13) {
            const auto r = prepare_world_wander(
                owner.world, {actor, owner.facts, i.wander_tickets,
                              i.use_shared_random ? std::function<std::optional<int>(int)>(draw)
                                                  : std::function<std::optional<int>(int)>{}});
            if (!r.candidate)
                return fail();
            next.state.world = r.candidate->state;
        } else if (op >= 14 && op <= 17) {
            if (!i.attack || !(i.attack->actor == actor))
                return fail();
            auto input = *i.attack;
            if (i.use_shared_random) {
                input.draw = draw;
                input.expression = [&](CharacterId, const ActorEffectState &effects, int type,
                                       int delay) -> std::optional<ActorEffectState> {
                    if (!i.primary_expression_table)
                        return {};
                    const auto result = prepare_world_random_expression(
                        next.state.random, effects, type, delay, *i.primary_expression_table);
                    return result.candidate
                               ? std::optional<ActorEffectState>(result.candidate->state)
                               : std::nullopt;
                };
            }
            if (i.event) {
                input.event = [&](const AiRewardState &current,
                                  int code) -> std::optional<WorldCombatExternalWriteback> {
                    next.state.world.ai = current;
                    if (!event(next.state, code, i.event, audit.consumed_events))
                        return {};
                    return WorldCombatExternalWriteback{
                        encounter_external_writeback(next.state.world.ai),
                        next.state.popularity_queue};
                };
            }
            const auto r = prepare_world_attack_control(owner.world.ai, input);
            if (!r.candidate)
                return fail();
            next.state.world.ai = r.candidate->state;
            // 原hit创建时已追加bp；人物分支也必须在本轮后续物体阶段前发布同序名单。
            for (const auto object : r.candidate->objects)
                next.state.world.object_order.push_back(object);
            if (r.candidate->popularity_queue)
                next.state.popularity_queue = *r.candidate->popularity_queue;
            audit.attack_requests.insert(audit.attack_requests.end(), r.candidate->requests.begin(),
                                         r.candidate->requests.end());
            if (i.presentation) {
                if (r.candidate->hit)
                    for (const auto &request : r.candidate->hit->requests)
                        if (request.kind == HitRequestKind::face_attacker ||
                            request.kind == HitRequestKind::attack_sound) {
                            const auto p = i.presentation(
                                next.state, {actor, r.candidate->target, {}, request, {}, {}});
                            if (!p)
                                return fail();
                            next.state = *p;
                        }
                for (const auto &request : r.candidate->requests) {
                    const auto p = i.presentation(
                        next.state, {request.actor, request.target, request, {}, {}, {}});
                    if (!p)
                        return fail();
                    next.state = *p;
                }
            }
            next.action = r.candidate->completed ? WorldControlAction::continue_same_call
                                                 : WorldControlAction::hold_false;
        } else if (op == 19 || op == 27 || op == 28 || op == 29 || op == 30) {
            const auto r = prepare_world_shop_command(shop(owner), actor, i.equipment);
            if (!r.candidate)
                return fail();
            if (!write_shop(next.state, r.candidate->state))
                return fail();
            for (const auto &request : r.candidate->requests) {
                if (request.kind == ShopWorldRequestKind::equipment_display) {
                    const auto display = prepare_world_equipment_display(
                        next.state.world, {actor, request, i.cached_view});
                    if (!display.candidate)
                        return fail();
                    next.state.world = display.candidate->state;
                } else
                    audit.shop_requests.push_back(request);
                if (request.kind != ShopWorldRequestKind::equipment_display && i.presentation) {
                    const auto p = i.presentation(next.state, {actor, {}, {}, {}, {}, request});
                    if (!p)
                        return fail();
                    next.state = *p;
                }
            }
        } else if (op == 25 || op == 26 || op == 32 || op == 33) {
            const auto r = prepare_world_misc_control({owner.world, owner.human_definition_state},
                                                      {actor, i.cached_view, i.sound_projection});
            if (!r.candidate)
                return fail();
            next.state.world = r.candidate->state.world;
            next.state.human_definition_state = r.candidate->state.human_definition_state;
            next.action = r.candidate->action;
            audit.sounds.insert(audit.sounds.end(), r.candidate->sounds.begin(),
                                r.candidate->sounds.end());
            for (const auto &request : r.candidate->sounds)
                if (i.presentation) {
                    const auto p = i.presentation(next.state, {actor, {}, {}, {}, request, {}});
                    if (!p)
                        return fail();
                    next.state = *p;
                }
        } else if (op == 21 && current_facility(owner, actor, true) &&
                   current_facility(owner, actor, true)->category == 5) {
            const auto r = prepare_world_dungeon_entry(
                dungeon(owner), actor, i.dungeon_notice_ticket,
                i.use_shared_random ? std::function<std::optional<int>(int)>(draw)
                                    : std::function<std::optional<int>(int)>{});
            if (!r.candidate)
                return fail();
            if (!write_dungeon(next.state, r.candidate->state))
                return fail();
            if (r.candidate->entry_event &&
                !event(next.state, *r.candidate->entry_event, i.event, audit.consumed_events))
                return fail();
        } else if (op == 24 && current_facility(owner, actor, false) &&
                   current_facility(owner, actor, false)->category == 1) {
            if (!i.shop_exit || !(i.shop_exit->actor == actor))
                return fail();
            auto input = *i.shop_exit;
            if (i.use_shared_random)
                input.draw = draw;
            const auto r = prepare_world_shop_exit(shop(owner), input);
            if (!r.candidate)
                return fail();
            if (!write_shop(next.state, r.candidate->state))
                return fail();
            audit.shop_requests.insert(audit.shop_requests.end(), r.candidate->requests.begin(),
                                       r.candidate->requests.end());
            for (const auto &request : r.candidate->requests)
                if (i.presentation) {
                    const auto p = i.presentation(next.state, {actor, {}, {}, {}, {}, request});
                    if (!p)
                        return fail();
                    next.state = *p;
                }
        } else {
            auto input = i.facility;
            input.actor = actor;
            input.domain_limit = 1;
            if (i.use_shared_random && op == 18 && input.expressions.empty()) {
                if (!i.primary_expression_table)
                    return fail();
                const auto r = prepare_world_random_expression(
                    next.state.random, owner.world.ai.contexts.at(actor).effects, command[1],
                    command[2], *i.primary_expression_table);
                if (!r.ticket)
                    return fail();
                input.expressions.push_back(*r.ticket);
            }
            if (i.use_shared_random && op == 23 && input.launch_tickets.empty()) {
                const auto ticket = draw(4);
                if (!ticket)
                    return fail();
                input.launch_tickets.push_back(*ticket);
            }
            const auto r = prepare_world_facility_control(owner.world, input);
            if (!r.candidate)
                return fail();
            next.state.world = r.candidate->state;
            if (r.candidate->flow == ActorControlFlow::moving ||
                r.candidate->flow == ActorControlFlow::waiting)
                next.action = WorldControlAction::hold_false;
        }
        return next;
    };
    const auto r = prepare_world_control(s, id, adapter, budget);
    if (!r.candidate)
        return {error == WorldActorRouteError::none ? WorldActorRouteError::control_failed : error,
                {},
                r.error};
    audit.state = r.candidate->state;
    audit.flow = r.candidate->flow;
    audit.local_commands = r.candidate->local_commands;
    audit.domain_segments = r.candidate->domain_segments;
    return {WorldActorRouteError::none, std::move(audit), WorldControlError::none};
}
} // namespace dungeon_village_reference
