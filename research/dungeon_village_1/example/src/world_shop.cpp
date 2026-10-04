#include "dungeon_village_reference/world_shop.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
bool live(const ShopWorldState &s, CharacterId id) {
    const auto a = s.world.ai.battle.actors.find(id);
    return id.value && a != s.world.ai.battle.actors.end() && a->second.id == id &&
           a->second.kind == ActorKind::human && s.world.actors.count(id) &&
           s.world.ai.contexts.count(id) && s.world.ai.growth.count(a->second.definition) &&
           s.humans.count(a->second.definition) && s.actors.count(id) &&
           std::count(s.world.ai.human_order.begin(), s.world.ai.human_order.end(), id) == 1;
}
const RescueFacility *bound(const ShopWorldState &s, CharacterId id) {
    const auto &b = s.world.actors.at(id).binding;
    if (!b || !arrival_binding_matches(s.world.map, *b, s.world.ai.contexts.at(id).cell))
        return nullptr;
    const auto f = s.world.facilities.find(b->instance_id.value);
    if (f == s.world.facilities.end() || !(f->second.placement.instance_id == b->instance_id) ||
        f->second.placement.definition_id != b->definition_id)
        return nullptr;
    return &f->second;
}
bool valid_catalogue(const std::vector<ShopEquipmentDefinition> &definitions) {
    std::set<std::pair<int, int>> seen;
    for (const auto &d : definitions)
        if (d.kind < 1 || d.kind > 3 || d.id < 0 || d.rank < 0 || d.type < 0 || d.price < 0 ||
            !seen.insert({d.kind, d.id}).second)
            return false;
    return true;
}
const ShopEquipmentDefinition *definition(const std::vector<ShopEquipmentDefinition> &definitions,
                                          int kind, int id) {
    const auto d = std::find_if(definitions.begin(), definitions.end(),
                                [&](const auto &d) { return d.kind == kind && d.id == id; });
    return d == definitions.end() ? nullptr : &*d;
}
ShopWorldResult error(ShopWorldError e) { return {e, {}}; }
} // namespace
ShopWorldResult prepare_world_shop_arrival(const ShopWorldState &s, const ShopArrivalInput &i) {
    if (!live(s, i.actor))
        return error(ShopWorldError::stale_actor);
    const auto *f = bound(s, i.actor);
    if (!f || (f->category != 1 && f->category != 7))
        return error(ShopWorldError::stale_binding);
    const auto &old = s.world.ai.battle.actors.at(i.actor);
    if (old.control.state != 0 || old.object_slot < -1 || !valid_catalogue(i.catalogue) ||
        !s.world.human_spending.count(old.definition))
        return error(ShopWorldError::invalid_input);
    ShopWorldCandidate c;
    c.state = s;
    auto &a = c.state.world.ai.battle.actors.at(i.actor);
    ResolvedArrivalInput arrival;
    arrival.arrival = {i.actor,
                       f->placement.instance_id,
                       f->placement.definition_id,
                       f->kind,
                       f->category,
                       f->detail,
                       0,
                       a.control.flags,
                       a.object_slot,
                       s.world.month_index,
                       f->price};
    arrival.statistics = s.world.actors.at(i.actor).visits;
    arrival.statistics.legacy_actor_total = s.world.human_spending.at(a.definition);
    arrival.statistics.current_month_facility_sales = f->sales;
    if (a.object_slot >= 0) {
        const auto it = c.state.items.find(a.object_slot);
        if (it == c.state.items.end() || it->second.inventory < 0 || it->second.inventory > 999 ||
            it->second.status < 0 || it->second.unlock_counter < 0)
            return error(ShopWorldError::invalid_input);
        auto &item = it->second;
        item.inventory = std::min(item.inventory + 1, 999);
        if (item.status == 0) {
            item.newly_unlocked = true;
            item.status = 1;
            item.unlock_counter = 0;
        }
        c.requests.push_back({ShopWorldRequestKind::delivered_item_notice, a.object_slot, 2, {}});
        arrival.carried_object_exists = true;
    }
    const auto &human = s.humans.at(a.definition);
    int kind{};
    std::optional<int> selected;
    if (f->detail == 1) {
        if (!human.equipment[0])
            return error(ShopWorldError::invalid_input);
        std::vector<WeaponChoiceDefinition> definitions;
        for (const auto &d : i.catalogue)
            if (d.kind == 1)
                definitions.push_back({d.id, d.rank, d.unlocked});
        const auto choice = prepare_weapon_choice(definitions, *human.equipment[0],
                                                  human.reselect[0], i.selection_ticket, i.draw);
        if (!choice.candidate)
            return error(choice.error == WeaponChoiceError::missing_ticket
                             ? ShopWorldError::missing_ticket
                             : ShopWorldError::invalid_input);
        selected = choice.candidate->weapon_id;
        c.consumed_selection = choice.candidate->consumes_ticket;
        kind = 1;
        c.state.actors.at(i.actor).selected_weapon = selected;
    } else if (f->detail == 4 || f->detail == 5) {
        const bool armor = f->detail == 4;
        int slot = 3;
        if (armor) {
            auto ticket = i.armor_slot_ticket;
            if (!ticket && i.draw) {
                try {
                    ticket = i.draw(2);
                } catch (...) {
                    return error(ShopWorldError::preparation_failed);
                }
            }
            if (!ticket)
                return error(ShopWorldError::missing_ticket);
            if (*ticket < 0 || *ticket >= 2)
                return error(ShopWorldError::invalid_input);
            slot = *ticket + 1;
            c.consumed_armor_slot = true;
        }
        kind = armor ? 2 : 3;
        EquipmentChoiceInput choice;
        choice.kind = armor ? EquipmentChoiceKind::armor : EquipmentChoiceKind::accessory;
        choice.slot = slot;
        choice.current = human.equipment[slot];
        choice.reselect_counter = human.reselect[slot];
        choice.ticket = i.selection_ticket;
        choice.draw = i.draw;
        for (const auto &d : i.catalogue)
            if (d.kind == kind)
                choice.catalogue.push_back({d.id, d.rank, d.type, d.unlocked});
        const auto result = prepare_equipment_choice(choice);
        if (!result.candidate)
            return error(result.error == WeaponChoiceError::missing_ticket
                             ? ShopWorldError::missing_ticket
                             : ShopWorldError::invalid_input);
        selected = result.candidate->equipment_id;
        c.consumed_selection = result.candidate->consumes_ticket;
        if (armor)
            c.state.actors.at(i.actor).selected_armor = selected;
        else
            c.state.actors.at(i.actor).selected_accessory = selected;
    }
    if (selected) {
        const auto *d = definition(i.catalogue, kind, *selected);
        if (!d)
            return error(ShopWorldError::invalid_input);
        arrival.equipment_id = selected;
        arrival.equipment_price = d->price;
    }
    const auto resolved = prepare_resolved_arrival(arrival);
    if (!resolved.candidate)
        return error(ShopWorldError::preparation_failed);
    const auto use = prepare_facility_use_plan({a.control,
                                                f->category,
                                                f->detail,
                                                0,
                                                f->definition_wait,
                                                arrival.statistics.legacy_category_six_counter,
                                                {},
                                                {}});
    if (!use.candidate || use.candidate->cleanup)
        return error(ShopWorldError::preparation_failed);
    const auto income = resolved.candidate->arrival.cash_income;
    if (income > 0) {
        auto &owner = c.state.world.ai;
        if (!owner.next_cash_id ||
            owner.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
            owner.accounting.post_cash({owner.next_cash_id, owner.period, CashCategory::facilities,
                                        CashDirection::income, income}) != AccountingError::none)
            return error(ShopWorldError::preparation_failed);
        ++owner.next_cash_id;
        c.requests.push_back({ShopWorldRequestKind::cash_display, static_cast<int>(income), 0, {}});
    }
    auto visits = resolved.candidate->arrival.state;
    c.state.world.human_spending.at(a.definition) = visits.legacy_actor_total;
    c.state.world.facilities.at(f->placement.instance_id.value).sales =
        visits.current_month_facility_sales;
    visits.legacy_actor_total = visits.current_month_facility_sales = 0;
    auto &context = c.state.world.actors.at(i.actor);
    context.visits = visits;
    context.journey.reset();
    context.unbound_route.reset();
    context.path_pending = false;
    context.waypoint = 0;
    a.control = use.candidate->control;
    a.control.flags &= ~256U; // P先完成到达收费守卫，再清256并安排实际use。
    a.state_counter = a.state_parameter = 0;
    a.encounter.reset();
    a.object_slot = resolved.candidate->object_slot;
    return {ShopWorldError::none, c};
}
ShopWorldResult prepare_world_shop_exit(const ShopWorldState &s, const ShopExitInput &input) {
    if (!live(s, input.actor))
        return error(ShopWorldError::stale_actor);
    if (!valid_catalogue(input.catalogue))
        return error(ShopWorldError::invalid_input);
    const auto &old = s.world.ai.battle.actors.at(input.actor);
    if (old.control.queue.empty() || !valid_actor_control(old.control.queue.front()) ||
        old.control.queue.front()[0] != 24)
        return error(ShopWorldError::invalid_input);
    const auto *f = bound(s, input.actor);
    ShopWorldCandidate c;
    c.state = s;
    if (!f) {
        const auto cleanup = prepare_world_rescue_cleanup(s.world, input.actor);
        if (!cleanup.candidate)
            return error(ShopWorldError::preparation_failed);
        c.state.world = cleanup.candidate->state;
        c.cleaned_up = true;
        return {ShopWorldError::none, c};
    }
    if (f->category != 1 || !s.world.facility_uses.count(f->placement.definition_id))
        return error(ShopWorldError::invalid_input);
    FacilityServiceExitInput i;
    i.actor = input.actor;
    i.control = old.control;
    i.binding_valid = true;
    i.map = s.world.map;
    i.facility = f->placement;
    i.position = {old.position.x, old.position.z};
    i.category = f->category;
    i.detail = f->detail;
    i.progress = s.world.facility_uses.at(f->placement.definition_id);
    i.upgrade_uses = f->upgrade_uses;
    i.occupants = f->occupants;
    i.satisfaction = {input.actor,
                      f->placement.instance_id,
                      f->placement.definition_id,
                      s.humans.at(old.definition).satisfaction,
                      input.job_thresholds,
                      input.quality,
                      input.satisfaction_ticket};
    i.effects = input.effects;
    i.effect_ticket = input.effect_ticket;
    // 普通商店才先抽满意度10，再按真实效果数抽属性；装备退出完全不抽。
    if (input.draw && f->detail != 1 && f->detail != 4 && f->detail != 5) {
        try {
            const auto satisfaction = input.draw(10);
            if (!satisfaction)
                return error(ShopWorldError::missing_ticket);
            i.satisfaction.ticket = *satisfaction;
            if (!i.effects.empty() && !i.effect_ticket) {
                if (i.effects.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
                    return error(ShopWorldError::invalid_input);
                i.effect_ticket = input.draw(static_cast<int>(i.effects.size()));
                if (!i.effect_ticket)
                    return error(ShopWorldError::missing_ticket);
            }
        } catch (...) {
            return error(ShopWorldError::preparation_failed);
        }
    }
    const auto &selected = s.actors.at(input.actor);
    i.equipment.old_weapon = selected.weapon;
    if (f->detail == 1) {
        if (!selected.selected_weapon || !definition(input.catalogue, 1, *selected.selected_weapon))
            return error(ShopWorldError::invalid_input);
        i.equipment.new_weapon = *selected.selected_weapon;
    } else if (f->detail == 4) {
        const auto *d = selected.selected_armor
                            ? definition(input.catalogue, 2, *selected.selected_armor)
                            : nullptr;
        if (!d)
            return error(ShopWorldError::invalid_input);
        i.equipment.armor = d->id;
        i.equipment.armor_type = d->type;
    } else if (f->detail == 5) {
        if (!selected.selected_accessory ||
            !definition(input.catalogue, 3, *selected.selected_accessory))
            return error(ShopWorldError::invalid_input);
        i.equipment.accessory = *selected.selected_accessory;
    }
    const auto exit = prepare_facility_service_exit(i);
    if (!exit.candidate || !exit.candidate->position || !exit.candidate->shared_use)
        return error(ShopWorldError::preparation_failed);
    const auto &e = *exit.candidate;
    auto &a = c.state.world.ai.battle.actors.at(input.actor);
    a.control = e.control;
    a.position.x = e.position->position.x;
    a.position.z = e.position->position.z;
    a.state_counter = a.state_parameter = a.baseline = 0;
    a.encounter.reset();
    c.state.world.ai.contexts.at(input.actor).cell = e.position->logical_cell;
    c.state.world.facilities.at(f->placement.instance_id.value).occupants = e.occupants;
    c.state.world.facility_uses.at(f->placement.definition_id) = e.shared_use->progress;
    if (e.satisfaction) {
        c.state.humans.at(old.definition).satisfaction = e.satisfaction->satisfaction;
        const auto request = e.satisfaction->popularity;
        c.state.popularity_queue.insert(
            c.state.popularity_queue.begin(),
            {request.legacy_countdown, request.delta, request.show_notice ? 1 : 0});
    }
    return {ShopWorldError::none, c};
}
ShopWorldResult prepare_world_shop_command(const ShopWorldState &s, CharacterId id,
                                           const std::vector<ShopEquipmentDefinition> &catalogue) {
    if (!live(s, id))
        return error(ShopWorldError::stale_actor);
    const auto &old = s.world.ai.battle.actors.at(id);
    if (old.control.queue.empty() || !valid_actor_control(old.control.queue.front()) ||
        !valid_catalogue(catalogue))
        return error(ShopWorldError::invalid_input);
    const auto command = old.control.queue.front();
    ShopWorldCandidate c;
    c.state = s;
    auto &a = c.state.world.ai.battle.actors.at(id);
    if (command[0] == 19) {
        if (command[1] == std::numeric_limits<int>::min() || command[2] < 0 || command[2] >= 6)
            return error(ShopWorldError::invalid_input);
        auto &extra = c.state.world.ai.growth.at(a.definition).definition.extra[command[2]];
        const auto sum = static_cast<std::int64_t>(extra) + command[3];
        if (sum < std::numeric_limits<int>::min() || sum > std::numeric_limits<int>::max())
            return error(ShopWorldError::preparation_failed);
        extra = static_cast<int>(sum);
        c.state.world.ai.contexts.at(id).effects.display.insert(
            c.state.world.ai.contexts.at(id).effects.display.begin(),
            {13, -command[1], command[2], command[3]}); // z only; NO eager x/w recalculation.
    } else if (command[0] == 27 || command[0] == 29) {
        if (command[0] == 27 &&
            (!definition(catalogue, 1, command[1]) || !definition(catalogue, 1, command[2])))
            return error(ShopWorldError::invalid_input);
        if (command[0] == 29 && ((command[1] != 21 && command[1] != 22) ||
                                 !definition(catalogue, command[1] == 21 ? 2 : 3, command[2])))
            return error(ShopWorldError::invalid_input);
        c.requests.push_back({ShopWorldRequestKind::equipment_display, command[0], 0, command});
    } else {
        const auto commit = prepare_equipment_commit(command);
        if (!commit)
            return error(ShopWorldError::invalid_input);
        const int kind = commit->slot == 0 ? 1 : commit->slot == 3 ? 3 : 2;
        const auto *d = definition(catalogue, kind, commit->equipment);
        if (!d)
            return error(ShopWorldError::invalid_input);
        auto &growth = c.state.world.ai.growth.at(a.definition);
        growth.definition.equipment[commit->slot] = d->combat;
        const auto stats = derive_human_stats(growth.definition, c.state.world.ai.professions);
        if (!stats.candidate)
            return error(ShopWorldError::preparation_failed);
        growth.derived = *stats.candidate;
        auto &human = c.state.humans.at(a.definition);
        human.equipment[commit->slot] = d->id;
        human.reselect[commit->slot] = commit->reselect_counter;
        if (commit->update_actor_weapon)
            c.state.actors.at(id).weapon = d->id;
        // h() reads shared derived w0 for active and retained instances, but equip never heals.
        for (auto &[other_id, other] : c.state.world.ai.battle.actors) {
            (void)other_id;
            if (other.kind == ActorKind::human && other.definition == a.definition)
                other.capacity = growth.derived.combat[0];
        }
        for (auto &[other_id, other] : c.state.world.ai.retired_actors) {
            (void)other_id;
            if (other.kind == ActorKind::human && other.definition == a.definition)
                other.capacity = growth.derived.combat[0];
        }
    }
    a.control.queue.erase(a.control.queue.begin());
    return {ShopWorldError::none, c};
}
} // namespace dungeon_village_reference
