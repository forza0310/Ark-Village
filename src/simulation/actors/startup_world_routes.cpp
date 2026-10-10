#include "ark/simulation/actors/startup_world_routes.hpp"

#include "ark/simulation/facilities/facility_projection.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
const StartupDefinition *definition(const StartupWorldRouteFacts &f, int id) {
    if (!f.rules)
        return nullptr;
    const auto found = std::find_if(f.rules->facilities.begin(), f.rules->facilities.end(),
                                    [id](const auto &d) { return d.id == id; });
    return found == f.rules->facilities.end() ? nullptr : &*found;
}
std::optional<ref::FacilityEconomyValues> economy(const ref::WorldActorRoutesState &r,
                                                  const StartupWorldRouteFacts &f,
                                                  const StartupDefinition &d,
                                                  std::optional<std::uint64_t> instance = {}) {
    const auto use = r.world.facility_uses.find(d.id);
    const auto improvement = f.facility_improvements.find(d.id);
    if (use == r.world.facility_uses.end() || improvement == f.facility_improvements.end())
        return {};
    ref::FacilityEconomyInput input;
    input.level = use->second.level;
    input.completed_definition_uses = use->second.completed_uses;
    input.definition_improvements = improvement->second;
    input.legacy_job_counts = f.job_counts;
    if (instance) {
        const auto neighbour = f.neighbourhood.find(*instance);
        if (neighbour == f.neighbourhood.end())
            return {};
        std::copy(neighbour->second.begin(), neighbour->second.end(),
                  input.instance_modifiers.begin());
    }
    return ref::derive_facility_economy(d.economy, input).values;
}
std::optional<std::vector<ref::ShopEquipmentDefinition>>
equipment(const ref::WorldActorRoutesState &r, const StartupWorldRouteFacts &f,
          bool materialize = true) {
    if (!f.rules)
        return {};
    std::vector<ref::ShopEquipmentDefinition> result;
    for (const auto &d : f.rules->equipment) {
        const auto current = r.catalog.find({d.shop.kind, d.shop.id});
        if (current == r.catalog.end())
            return {};
        if (materialize) {
            auto copy = d.shop;
            copy.unlocked = current->second.status != 0;
            result.push_back(copy);
        }
    }
    return result;
}
std::optional<ref::WorldDepartureInput> departure(const ref::WorldActorRoutesState &r,
                                                  ref::CharacterId id,
                                                  const StartupWorldRouteFacts &f,
                                                  bool materialize = true) {
    if (!f.rules || f.surface.size() != r.world.map.cells.size())
        return {};
    ref::WorldDepartureInput input;
    input.actor = id;
    input.catalogue.town = r.facts.town;
    const auto ground = std::find_if(f.rules->facilities.begin(), f.rules->facilities.end(),
                                     [](const auto &d) { return d.kind == 7; });
    if (ground == f.rules->facilities.end())
        return {};
    input.catalogue.ground_definition = ground->id; // c/n初始化o.S取首个kind7。
    if (materialize)
        for (const auto &cell : f.surface)
            input.catalogue.cell_definition_ids.push_back(cell.definition);
    // 实际出发复用明细索引；未消费的出发输入不建节点，但仍按原序验证每项
    // 经济值并使用重复ID的首项。没有跨调用缓存等级／职业相关结果。
    for (const auto &d : f.rules->facilities) {
        const StartupDefinition *first;
        if (!materialize)
            first = definition(f, d.id);
        else
            first =
                input.definition_details.emplace(d.id, d.detail).second ? &d : definition(f, d.id);
        const auto values = economy(r, f, *first);
        if (!values)
            return {};
        if (materialize)
            input.catalogue.definitions.push_back(
                {d.id, d.category, values->definition_attributes[2], d.kind});
    }
    // Even an unused departure validates every economy and duplicate-ID lookup.
    // Only storage with no rejection or random behavior is omitted.
    if (!materialize)
        return input;
    for (const auto &entry : r.world.facilities)
        input.catalogue.instances.push_back(
            {{entry.first}, entry.second.placement.definition_id, entry.second.status});
    input.catalogue.events = f.tasks;
    input.catalogue.last_visited_instance = r.world.actors.at(id).visits.last_visited_instance;
    input.exits = f.exits;
    input.task_center = r.task.center;
    const auto home = f.human_homes.find(r.world.ai.battle.actors.at(id).definition);
    if (home != f.human_homes.end())
        input.home = ref::WorldDepartureHome{{home->second[0], home->second[1]}, home->second[2]};
    return input;
}
std::optional<ref::CombatWeaponRule> weapon(const ref::WorldActorRoutesState &r,
                                            ref::CharacterId id, const StartupWorldRouteFacts &f) {
    const auto actor = r.world.ai.battle.actors.find(id);
    if (actor == r.world.ai.battle.actors.end() || !f.rules)
        return {};
    // 怪物攻击领域不读取人类武器属性；原新对象ae=0仍使N()引用bt[0]。
    if (actor->second.kind == ref::ActorKind::monster) {
        for (const auto &d : f.rules->equipment)
            if (d.shop.kind == 1 && d.shop.id == 0)
                return d.battle;
        return {};
    }
    const auto current = r.shop_actors.find(id);
    if (current == r.shop_actors.end())
        return {};
    for (const auto &d : f.rules->equipment)
        if (d.shop.kind == 1 && d.shop.id == current->second.weapon)
            return d.battle;
    return {};
}
std::optional<ref::WorldPathInput> path(const ref::WorldActorRoutesState &r, ref::CharacterId id,
                                        const StartupWorldRouteFacts &f, bool materialize = true) {
    if (!f.rules)
        return {};
    ref::WorldPathInput p;
    p.actor = id;
    if (materialize) {
        p.facts = r.facts;
        p.task = r.task;
        p.exits = f.exits;
        for (const auto &d : f.rules->facilities)
            p.definition_directions.emplace(d.id, d.direction);
    }
    const auto &binding = r.world.actors.at(id).binding;
    // 探索恢复后O/旧路线仍可保留原身份；P先核对当前地图，不把退休目标当缺输入。
    if (binding &&
        ref::arrival_binding_matches(r.world.map, *binding, r.world.ai.contexts.at(id).cell)) {
        const auto instance = r.world.facilities.find(binding->instance_id.value);
        if (instance == r.world.facilities.end())
            return {};
        const auto &facility = instance->second;
        if (facility.category == 6 && facility.detail == 3) {
            const auto target = project_facility_use_target(r.world.ai.contexts.at(id).cell, 6, 3);
            if (!target)
                return {};
            if (materialize)
                p.use_world_target = target->world_target;
        } else if (materialize && facility.category == 8 && facility.detail == 2) {
            const auto cell = r.world.ai.contexts.at(id).cell;
            p.use_direction_target = [cell](int direction) -> std::optional<ref::Position> {
                const auto target = project_facility_use_target(cell, 8, 2, direction);
                return target ? std::optional<ref::Position>{target->world_target} : std::nullopt;
            };
        }
    }
    if (materialize)
        p.task_attempt = f.task_attempt;
    return p;
}
bool live(const ref::WorldActorRoutesState &r, ref::CharacterId id) {
    return r.world.ai.battle.actors.count(id) && r.world.actors.count(id) &&
           r.world.ai.contexts.count(id);
}
std::optional<ref::WorldActorDecisionInput> decision_input(const ref::WorldActorRoutesState &r,
                                                           ref::CharacterId id,
                                                           const StartupWorldRouteFacts &f,
                                                           bool selective) {
    if (!live(r, id) || !f.rules)
        return {};
    const auto &actor = r.world.ai.battle.actors.at(id);
    const int state = actor.control.state;
    const bool daily = state == 0 || state == 5 || state == 8 || state == 9 || state == 11;
    auto p = path(r, id, f, !selective || daily || state == 17);
    bool shop_arrival{};
    const auto &binding = r.world.actors.at(id).binding;
    if (daily && actor.kind == ref::ActorKind::human && binding &&
        ref::arrival_binding_matches(r.world.map, *binding, r.world.ai.contexts.at(id).cell)) {
        const auto instance = r.world.facilities.find(binding->instance_id.value);
        shop_arrival = instance != r.world.facilities.end() &&
                       (instance->second.category == 1 || instance->second.category == 7);
    }
    auto gear = equipment(r, f, !selective || shop_arrival);
    if (!p || !gear)
        return {};
    ref::WorldActorDecisionInput input;
    input.actor = id;
    input.use_shared_random = true;
    input.primary_expression_table = f.primary_expression_table;
    if (!selective || daily) {
        input.daily.actor = id;
        // Full input retains both independent paths; the selected input transfers
        // its sole path to the branch that actually reads it.
        input.daily.path = selective ? std::move(*p) : *p;
        input.daily.task_creation = f.task_entry;
        if (input.daily.task_creation)
            input.daily.task_creation->actor = id;
        input.daily.actor_box = f.actor_box;
        input.daily.object_box = f.object_box;
        for (const auto &task : f.tasks)
            input.daily.task_centers.push_back(task.position);
        ref::EncounterCreationInput spawn;
        spawn.kind = 0;
        spawn.year_index = f.calendar[0];
        spawn.month_index = f.calendar[1];
        // L根据当前s/O/状态和计数重建probe，minimum_y源h.a()为2。
        spawn.probe = ref::EncounterCreationProbe{id, false, 2, {}, 0, input.daily.task_centers};
        input.daily.spawn_creation = spawn;
    }
    input.lifecycle.actor = id;
    if (!selective || state == 17)
        input.monster_path = std::move(*p);
    input.actor_box = f.actor_box;
    input.rescue_box = f.rescue_box;
    if (!selective || (daily && actor.object_slot == -2)) {
        std::map<ref::CharacterId, ref::Position> rescue_cells;
        for (const auto &[actor_id, context] : r.world.ai.contexts)
            rescue_cells.emplace(actor_id, context.cell);
        input.rescue_direction_target =
            [cells = std::move(rescue_cells)](ref::CharacterId actor_id,
                                              int direction) -> std::optional<ref::Position> {
            const auto cell = cells.find(actor_id);
            if (cell == cells.end())
                return {};
            const auto target = project_facility_use_target(cell->second, 8, 2, direction);
            return target ? std::optional<ref::Position>{target->world_target} : std::nullopt;
        };
    }
    const auto metadata = f.actor_metadata.find(id);
    if (metadata != f.actor_metadata.end())
        input.cached_view = metadata->second.cached_view;
    auto d = departure(r, id, f, !selective || state == 20);
    if (!d)
        return {};
    if (!selective || state == 20)
        input.landing_departure = std::move(*d);
    if (actor.kind == ref::ActorKind::human) {
        const auto &binding = r.world.actors.at(id).binding;
        if (binding &&
            ref::arrival_binding_matches(r.world.map, *binding, r.world.ai.contexts.at(id).cell)) {
            const auto instance = r.world.facilities.find(binding->instance_id.value);
            if (instance == r.world.facilities.end())
                return {};
            if ((!selective || daily) &&
                (instance->second.category == 1 || instance->second.category == 7))
                input.shop_arrival = ref::ShopArrivalInput{id, std::move(*gear), {}, {}, {}};
        }
        const auto w = weapon(r, id, f);
        if (!w || metadata == f.actor_metadata.end() || metadata->second.profession < 0 ||
            metadata->second.profession >= static_cast<int>(f.rules->jobs.size()))
            return {};
        ref::WorldCombatPolicyInput combat;
        combat.actor = id;
        combat.weapon = *w;
        combat.profession_role = f.rules->jobs.at(metadata->second.profession).role;
        const auto facing = f.facing.find(id);
        if (facing != f.facing.end())
            combat.facing = facing->second;
        if (!selective || state == 1)
            input.combat = combat;
    } else {
        if (actor.body < 0 || actor.body >= 4)
            return {};
        const auto w = weapon(r, id, f);
        if (!w)
            return {};
        ref::WorldCombatPolicyInput combat;
        combat.actor = id;
        combat.weapon = *w;                         // 怪物setup只要求合法载荷，不消费人类武器数值。
        constexpr int ranges[]{100, 130, 130, 130}; // 原a.k.a/E[g]，c/n把k.d写g。
        combat.monster_range = ranges[actor.body];
        combat.monster_mode = r.world.actors.at(id).monster_mode;
        const auto facing = f.facing.find(id);
        if (facing != f.facing.end())
            combat.facing = facing->second;
        if (!selective || state == 1)
            input.combat = combat;
    }
    return input;
}
} // namespace

std::optional<ref::WorldActorDecisionInput>
prepare_startup_world_decision_input(const ref::WorldActorRoutesState &r, ref::CharacterId id,
                                     const StartupWorldRouteFacts &f) {
    return decision_input(r, id, f, false);
}

std::optional<ref::WorldActorDecisionInput> prepare_startup_world_decision_input_for_state(
    const ref::WorldActorRoutesState &r, ref::CharacterId id, const StartupWorldRouteFacts &f) {
    return decision_input(r, id, f, true);
}
namespace {
std::optional<ref::WorldActorCommandInput>
command_input(const ref::WorldActorRoutesState &r, ref::CharacterId id,
              const ref::LegacyActorControl &command, const StartupWorldRouteFacts &f,
              bool selective) {
    if (!live(r, id) || command.empty() || !f.rules)
        return {};
    // 19的成长提示及27..30的装备显示/提交共用商店消费者，均须完整目录。
    const bool shop_command = command[0] == 19 || command[0] == 27 || command[0] == 28 ||
                              command[0] == 29 || command[0] == 30;
    auto gear = equipment(r, f, !selective || shop_command);
    if (!gear)
        return {};
    ref::WorldActorCommandInput input;
    input.use_shared_random = true;
    input.primary_expression_table = f.primary_expression_table;
    input.facility.actor = id;
    if (!selective || shop_command)
        input.equipment = selective ? std::move(*gear) : *gear;
    const auto metadata = f.actor_metadata.find(id);
    if (metadata != f.actor_metadata.end())
        input.cached_view = metadata->second.cached_view;
    input.sound_projection = f.sound_projection;
    if (command[0] == 8) {
        const auto d = departure(r, id, f);
        if (!d)
            return {};
        input.departure = ref::WorldDepartureControlInput{*d, {}, {}};
    }
    if (command[0] == 24) {
        const auto &binding = r.world.actors.at(id).binding;
        if (binding &&
            ref::arrival_binding_matches(r.world.map, *binding, r.world.ai.contexts.at(id).cell)) {
            const auto instance = r.world.facilities.find(binding->instance_id.value);
            if (instance == r.world.facilities.end())
                return {};
            if (instance->second.category == 1) {
                if (metadata == f.actor_metadata.end() || metadata->second.profession < 0 ||
                    metadata->second.profession >= static_cast<int>(f.rules->jobs.size()))
                    return {};
                const auto *d = definition(f, instance->second.placement.definition_id);
                if (!d)
                    return {};
                const auto values = economy(r, f, *d, instance->first);
                if (!values || values->instance_attributes[1] < 0 ||
                    values->instance_attributes[1] > std::numeric_limits<int>::max())
                    return {};
                // 全键校验已在原入口执行；实际普通店退出才物化独立gear输入。
                if (selective) {
                    gear = equipment(r, f);
                    if (!gear)
                        return {};
                }
                input.shop_exit =
                    ref::ShopExitInput{id,
                                       std::move(*gear),
                                       f.rules->jobs.at(metadata->second.profession).satisfaction,
                                       static_cast<int>(values->instance_attributes[1]),
                                       0,
                                       d->exit_effects,
                                       {},
                                       {}};
            }
        }
    }
    if (command[0] >= 14 && command[0] <= 17) {
        const auto w = weapon(r, id, f);
        if (!w)
            return {};
        ref::WorldAttackInput attack;
        attack.actor = id;
        attack.weapon = *w;
        const auto visible = f.actor_visible.find(id);
        if (visible == f.actor_visible.end())
            return {};
        attack.actor_visible = visible->second;
        const auto facing = f.facing.find(id);
        if (facing != f.facing.end())
            attack.facing = facing->second;
        input.attack = attack;
    }
    return input;
}
} // namespace
std::optional<ref::WorldActorCommandInput>
prepare_startup_world_command_input(const ref::WorldActorRoutesState &r, ref::CharacterId id,
                                    const ref::LegacyActorControl &command,
                                    const StartupWorldRouteFacts &f) {
    return command_input(r, id, command, f, false);
}
std::optional<ref::WorldActorCommandInput>
prepare_startup_world_command_input_for_command(const ref::WorldActorRoutesState &r,
                                                ref::CharacterId id,
                                                const ref::LegacyActorControl &command,
                                                const StartupWorldRouteFacts &f) {
    return command_input(r, id, command, f, true);
}
} // namespace ark::simulation
