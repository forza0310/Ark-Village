#include "ark/simulation/combat/rules/object_commit.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
ObjectCommitResult prepare_object_grant(const ObjectCommitState &s, int kind, int definition,
                                        ObjectGrantOrigin origin) {
    if (kind < 0 || kind > 3 || definition < 0 || s.item_rewards < 0 ||
        s.item_rewards >= std::numeric_limits<int>::max() ||
        (origin != ObjectGrantOrigin::ground_pickup && origin != ObjectGrantOrigin::direct))
        return {ObjectCommitError::invalid_input, std::nullopt};
    const auto it = s.catalog.find({kind, definition});
    if (it == s.catalog.end())
        return {ObjectCommitError::missing_definition, std::nullopt};
    const auto &old = it->second;
    if (old.status < 0 || old.unlock_counter < 0 || old.inventory < 0 || old.inventory > 999 ||
        old.free_purchases < 0)
        return {ObjectCommitError::invalid_input, std::nullopt};
    ObjectCommitCandidate c{s, false, {}};
    auto req = [&](ObjectCommitRequestKind k, int p, std::optional<std::uint64_t> shop = {}) {
        c.requests.push_back({k, p, definition, shop});
    };
    auto event = [&](int id) {
        if (c.state.events.insert(id).second)
            req(ObjectCommitRequestKind::event, id);
    };
    const int notice = kind == 0 ? 2 : kind == 1 ? 10 : 11;
    if (kind == 0 && origin == ObjectGrantOrigin::ground_pickup)
        req(ObjectCommitRequestKind::notice, notice);
    req(ObjectCommitRequestKind::grant, kind);
    auto &d = c.state.catalog.at({kind, definition});
    if (kind == 0) {
        d.inventory = std::min(d.inventory + 1, 999);
        if (d.status == 0) {
            d.newly_unlocked = true;
            d.status = 1;
            d.unlock_counter = 0;
        }
    } else {
        if (d.status == 0) {
            d.newly_unlocked = true;
            d.free_purchases = 1;
            if (kind == 1 && (d.flags & 32U))
                event(216);
            const int category = kind == 1 ? 1 : kind == 2 ? 4 : 5;
            std::set<std::uint64_t> visited;
            if (c.state.shop_order.size() != c.state.shops.size())
                return {ObjectCommitError::invalid_input, std::nullopt};
            for (const auto id : c.state.shop_order) {
                if (!visited.insert(id).second || !c.state.shops.count(id))
                    return {ObjectCommitError::invalid_input, std::nullopt};
                auto &shop = c.state.shops.at(id);
                if (shop.category == category) {
                    if (std::any_of(shop.notices.begin(), shop.notices.end(),
                                    [](const auto &n) { return n[0] == 7; }))
                        continue;
                    shop.notices.push_back({7, 0});
                    req(ObjectCommitRequestKind::shop_notice, 7, id);
                }
            }
        }
        d.status = 1; // p!=0 still becomes1, without refilling free purchases.
    }
    if (kind != 0 || origin == ObjectGrantOrigin::direct)
        req(ObjectCommitRequestKind::notice, notice);
    if (kind == 1 && origin == ObjectGrantOrigin::direct)
        event(110);
    if (kind == 0) {
        c.state.item_rewards = (s.item_rewards + 1) % std::numeric_limits<int>::max();
        if (c.state.item_rewards >= 10)
            event(151);
    }
    return {ObjectCommitError::none, c};
}
ObjectCommitResult prepare_object_update(const ObjectCommitState &s, ObjectId id, bool town) {
    const auto it = s.objects.find(id.value);
    if (it == s.objects.end() || !(it->second.id == id))
        return {ObjectCommitError::stale_object, std::nullopt};
    const auto step = advance_ground_object(it->second, town, s.item_rewards, s.events.count(151));
    if (!step.candidate)
        return {ObjectCommitError::invalid_input, std::nullopt};
    ObjectCommitCandidate c{s, step.candidate->remove, {}};
    if (!step.candidate->requests.empty()) {
        const auto grant = prepare_object_grant(s, it->second.kind, it->second.definition,
                                                ObjectGrantOrigin::ground_pickup);
        if (!grant.candidate)
            return grant;
        c.state = grant.candidate->state;
        c.requests.push_back(
            {ObjectCommitRequestKind::ground_effect, 0, it->second.definition, {}});
        c.requests.insert(c.requests.end(), grant.candidate->requests.begin(),
                          grant.candidate->requests.end());
    }
    if (c.remove)
        c.state.objects.erase(id.value);
    else
        c.state.objects.at(id.value) = step.candidate->state;
    return {ObjectCommitError::none, c};
}
PickupCommitResult prepare_ground_pickup_commit(const BattleCommitState &s,
                                                const PickupCommitInput &i) {
    const auto a = s.actors.find(i.actor);
    if (a == s.actors.end() || a->second.kind != ActorKind::human || !(a->second.id == i.actor) ||
        a->second.control.state != 11)
        return {ObjectCommitError::invalid_input, std::nullopt};
    PickupInput input;
    input.battle_ready = i.battle_ready;
    input.actor_position = a->second.position;
    input.carried_slot = a->second.object_slot;
    input.touching = i.touching;
    // Source F returns before H, so an urgent battle must not require object catalog validity.
    if (!i.battle_ready) {
        if (i.object_order.size() != s.objects.size())
            return {ObjectCommitError::invalid_input, std::nullopt};
        std::set<std::uint64_t> seen;
        std::vector<ObjectProbe> probes;
        for (const auto id : i.object_order) {
            const auto object = s.objects.find(id);
            if (!seen.insert(id).second || object == s.objects.end() ||
                object->second.id.value != id)
                return {ObjectCommitError::stale_object, std::nullopt};
            probes.push_back({object->second.id, object->second.state, object->second.position});
        }
        const auto nearest = select_ground_object(input.actor_position, probes);
        if (nearest.error != ObjectError::none)
            return {ObjectCommitError::invalid_input, std::nullopt};
        if (nearest.selected)
            input.nearest = s.objects.at(nearest.selected->value);
    }
    const auto plan = prepare_ground_pickup(input);
    if (!plan.candidate)
        return {ObjectCommitError::invalid_input, std::nullopt};
    PickupCommitCandidate c{s, plan.candidate->action, {}, {}, plan.candidate->expression};
    auto &actor = c.state.actors.at(i.actor);
    if (c.action == PickupAction::baseline) {
        const auto restore = prepare_actor_baseline_restore(actor.control, actor.baseline, true, 0);
        if (!restore)
            return {ObjectCommitError::invalid_input, std::nullopt};
        actor.control = restore->control;
        actor.encounter.reset();
    } else if (c.action == PickupAction::chase) {
        c.object = input.nearest->id;
        c.chase_target = input.nearest->position;
    } else {
        ActorStateTransitionInput transition;
        transition.control = actor.control;
        transition.baseline = actor.baseline;
        transition.next_state = *plan.candidate->actor_state;
        transition.legacy_u = i.legacy_u;
        transition.boost_ticket = i.boost_ticket;
        transition.boost_event116_seen = s.events.count(116);
        const auto t = prepare_actor_state_transition(transition);
        if (!t)
            return {ObjectCommitError::invalid_input, std::nullopt};
        actor.control = t->control;
        actor.baseline = t->baseline;
        actor.state_counter = actor.state_parameter = 0;
        if (t->clear_encounter)
            actor.encounter.reset();
        if (t->reset_attack_count)
            actor.attack_count = 0;
        if (t->request_boost_event116)
            c.state.events.insert(116);
        if (c.action == PickupAction::pickup) {
            c.object = plan.candidate->object->id;
            c.state.objects.at(c.object->value) = *plan.candidate->object;
            actor.control.facing = *plan.candidate->facing;
            actor.control.queue = plan.candidate->queue;
        }
    }
    return {ObjectCommitError::none, c};
}
} // namespace ark::simulation::rules
