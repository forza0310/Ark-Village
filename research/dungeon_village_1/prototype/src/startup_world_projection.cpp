#include "dungeon_village_prototype/startup_world_projection.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace dungeon_village_prototype {
namespace {
const StartupDefinition &facility(const StartupWorldRules &rules, int identity) {
    const auto d = std::find_if(rules.facilities.begin(), rules.facilities.end(),
                                [identity](const auto &f) { return f.id == identity; });
    if (d == rules.facilities.end())
        throw StartupWorldProjectionError::missing_definition;
    return *d;
}
int narrow(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        throw StartupWorldProjectionError::projection_failed;
    return static_cast<int>(value);
}
void install_definitions(StartupWorldProjection &c) {
    auto &ai = c.routes.world.ai;
    const auto &rules = *c.rules;
    ai.professions = startup_ai_rules().professions;
    if (ai.professions.size() != rules.jobs.size())
        throw StartupWorldProjectionError::source_mismatch;
    for (const auto &human : rules.humans) {
        ref::RewardHumanDefinition definition;
        definition.definition = human.definition;
        const auto derived = ref::derive_human_stats(human.definition, ai.professions);
        if (!derived.candidate)
            throw StartupWorldProjectionError::projection_failed;
        definition.derived = *derived.candidate;
        definition.notice_attributes = {}; // 原new int[4][2]全部0，不沿用维护缺省-1。
        ai.growth.emplace(human.identity, definition);
        ref::HumanBattleRecord battle;
        battle.luck = derived.candidate->attributes[5];
        ai.battle.humans.emplace(human.identity, battle);
        ref::ShopHumanRecord shop;
        for (std::size_t slot = 0; slot < shop.equipment.size(); ++slot)
            if (human.equipment[slot] >= 0)
                shop.equipment[slot] = human.equipment[slot];
        c.routes.shop_humans.emplace(human.identity, shop);
        c.routes.human_definition_state.emplace(human.identity, 0);  // a/e.r m0。
        c.routes.world.human_spending.emplace(human.identity, 0);    // B2。
        c.human_homes.emplace(human.identity, std::array<int, 4>{}); // a/e.r D全部0。
        c.human_presence.emplace(human.identity, human.status);
    }
    for (const auto &monster : rules.monsters) {
        ai.monster_growth.emplace(monster.identity, monster.initial);
        ai.monster_definition_order.push_back(monster.identity);
        ai.battle.monsters.emplace(
            monster.identity,
            ref::MonsterBattleRecord{0, monster.points_per_defeat, monster.initial.base_cash_reward,
                                     monster.initial.base_death_reward,
                                     monster.initial.sprite_variant, monster.flags});
    }
    for (const auto &equipment : rules.equipment)
        c.routes.catalog.emplace(std::make_pair(equipment.shop.kind, equipment.shop.id),
                                 equipment.initial);
    for (const auto &item : rules.items) {
        c.routes.items.emplace(item.identity, item.initial);
        c.routes.catalog.emplace(std::make_pair(0, item.identity), item.initial);
    }
    for (const auto &d : rules.facilities)
        c.routes.world.facility_uses.emplace(d.id, ref::FacilityUseProgress{});
}
void install_map(StartupWorldProjection &c, const StartupState &s) {
    auto &world = c.routes.world;
    const auto &loaded = s.loaded_map;
    const auto expected = reconstruct_startup_map(startup_evidence());
    if (loaded.cells.size() != expected.cells.size() ||
        loaded.instances.size() != expected.instances.size())
        throw StartupWorldProjectionError::source_mismatch;
    for (std::size_t n = 0; n < loaded.cells.size(); ++n) {
        const auto &a = loaded.cells[n], &b = expected.cells[n];
        if (a.definition_id != b.definition_id || a.legacy_state != b.legacy_state ||
            a.category != b.category || a.display_id != b.display_id || a.variant != b.variant ||
            a.road_mask != b.road_mask || a.boundary_fragment != b.boundary_fragment ||
            a.external_direction != b.external_direction || a.road_quad != b.road_quad ||
            a.edge_road_pair != b.edge_road_pair || a.legacy_instance_id != b.legacy_instance_id)
            throw StartupWorldProjectionError::source_mismatch;
    }
    for (std::size_t n = 0; n < loaded.instances.size(); ++n) {
        const auto &a = loaded.instances[n], &b = expected.instances[n];
        if (a.legacy_id != b.legacy_id || a.definition_id != b.definition_id ||
            !(a.anchor == b.anchor))
            throw StartupWorldProjectionError::source_mismatch;
    }
    world.map = startup_route_map(loaded);
    c.routes.facts.map = world.map; // API临时投影契约，不作为额外持久地图。
    const auto bounds = c.rules->fences.at(0);
    c.routes.facts.town = {bounds[0].x, bounds[1].x, bounds[1].y, bounds[0].y};
    c.routes.facts.flags.assign(world.map.cells.size(), 0); // 无事件新局new i.o的明确0。
    for (const auto &cell : loaded.cells) {
        c.routes.facts.surface.push_back(static_cast<int>(cell.category)); // 原g，非h显示记录。
        c.surface.push_back({cell.definition_id, 0, cell.external_direction, cell.boundary_fragment,
                             cell.display_id, cell.variant, cell.road_mask});
        c.road_patches.push_back({cell.road_quad, cell.edge_road_pair});
    }
    std::vector<ref::FacilityPlacement> placements;
    std::vector<ref::NeighbourDefinition> definitions;
    std::vector<ref::Position> roads;
    for (const auto &d : c.rules->facilities)
        definitions.push_back(
            {d.id, static_cast<ref::FacilityShape>(d.shape), d.kind, d.neighbour_effects});
    for (const auto &instance : loaded.instances) {
        const auto identity = static_cast<std::uint64_t>(instance.legacy_id) + 1;
        const auto &d = facility(*c.rules, instance.definition_id);
        const auto current = s.facilities.find(identity);
        if (current == s.facilities.end() || current->second.definition_id != d.id ||
            !(current->second.cell == instance.anchor) || current->second.remaining_ticks != 0 ||
            !current->second.seed || current->second.legacy_id != instance.legacy_id)
            throw StartupWorldProjectionError::invalid_snapshot;
        ref::RescueFacility f;
        f.placement = {{identity},
                       d.id,
                       static_cast<ref::FacilityShape>(d.shape),
                       ref::FacilityOrientation::first,
                       instance.anchor};
        f.kind = d.kind;
        f.category = d.category;
        f.detail = d.detail;
        f.definition_wait = d.use_wait;
        f.upgrade_uses = d.economy.upgrade_uses;
        f.status = 1;
        world.facilities.emplace(identity, f);
        placements.push_back(f.placement);
        c.facility_order.push_back(identity);
        c.facility_original_ids.emplace(identity, instance.legacy_id);
        c.facility_residents.emplace(identity, -1);
        int ordinal = 0;
        for (const auto previous : c.facility_order)
            if (previous != identity &&
                world.facilities.at(previous).placement.definition_id == d.id)
                ordinal = std::max(ordinal, c.facility_ordinals.at(previous) + 1);
        c.facility_ordinals.emplace(identity, ordinal);
        if (d.category == 1) {
            c.routes.shops.emplace(identity, ref::ObjectShopRecord{d.detail, {}});
            c.routes.shop_order.push_back(identity);
        }
    }
    for (int y = 0; y < loaded.height; ++y)
        for (int x = 0; x < loaded.width; ++x)
            if (world.map.cells[static_cast<std::size_t>(y) * loaded.width + x].legacy_state == 3)
                roads.push_back({x, y});
    const auto neighbours = ref::derive_facility_neighbourhood(definitions, placements, roads,
                                                               loaded.width, loaded.height);
    if (neighbours.error != ref::NeighbourhoodError::none)
        throw StartupWorldProjectionError::projection_failed;
    std::array<int, 10> job_counts{};
    for (const auto &human : c.rules->humans)
        if (human.status != 0) {
            const int type = c.rules->jobs.at(human.definition.current_profession).type;
            if (type < 0 || type >= static_cast<int>(job_counts.size()))
                throw StartupWorldProjectionError::source_mismatch;
            ++job_counts[type];
        }
    for (const auto &neighbour : neighbours.facilities) {
        const auto id = neighbour.instance_id.value;
        auto input = ref::neighbourhood_economy_input(neighbour);
        if (!input)
            throw StartupWorldProjectionError::projection_failed;
        input->legacy_job_counts = job_counts;
        const auto &d = facility(*c.rules, world.facilities.at(id).placement.definition_id);
        const auto values = ref::derive_facility_economy(d.economy, *input);
        if (!values.values)
            throw StartupWorldProjectionError::projection_failed;
        world.facilities.at(id).price = narrow(values.values->instance_attributes[0]);
        c.neighbourhood[id] = {narrow(neighbour.modifiers[0]), narrow(neighbour.modifiers[1]),
                               narrow(neighbour.modifiers[2])};
    }
    if (!ref::valid_world_map_facts(c.routes.facts))
        throw StartupWorldProjectionError::projection_failed;
}
void install_first(StartupWorldProjection &c, const StartupCharacter &character) {
    const auto &expected = startup_evidence().first_character;
    if (character.definition_id != expected.definition_id || character.uid != expected.uid ||
        character.attributes != expected.attributes || character.combat != expected.combat ||
        character.hp != expected.hp || character.equipment != expected.equipment ||
        character.legacy_D != expected.legacy_D || character.legacy_m != expected.legacy_m ||
        character.flags != (expected.flags | 2U | 8192U) || character.pending_activity != 0 ||
        std::find(startup_evidence().spawn_points.begin(), startup_evidence().spawn_points.end(),
                  character.cell) == startup_evidence().spawn_points.end())
        throw StartupWorldProjectionError::invalid_snapshot;
    auto &world = c.routes.world;
    const ref::CharacterId identity{static_cast<std::uint64_t>(character.uid) + 1};
    ref::BattleActorRecord actor;
    actor.id = identity;
    actor.definition = character.definition_id;
    actor.legacy_id = character.uid;
    actor.control.flags = character.flags;
    actor.control.queue = {{8, *character.pending_activity}};
    actor.hp = {0, character.hp[0], character.hp[1], character.hp[2], false, 0};
    actor.capacity = character.combat[0];
    actor.position = {character.cell.x * 100.0F + 50.0F, 0, character.cell.y * 100.0F + 50.0F};
    // 原b构造au为零；n.a只写n/s/t/u/v。此首访快照尚未运行d，不提前复制au。
    actor.attack_armed = false;   // Java新对象ai=false；不是维护结构的缺省true。
    actor.combo_count = 0;        // Java新对象y=0，真正攻击setup才计算。
    actor.perceived_distance = 0; // aA尚未c，不提前改为MAX_VALUE。
    const auto &derived = world.ai.growth.at(character.definition_id).derived;
    if (derived.attributes != character.attributes || derived.combat != character.combat)
        throw StartupWorldProjectionError::source_mismatch;
    world.ai.battle.actors.emplace(identity, actor);
    c.routes.dungeon_actors.emplace(identity, ref::DungeonActorProgress{0, 0, 0, 0, 0, 0, false});
    world.ai.human_order.push_back(identity);
    ref::RewardActorContext cache;
    cache.cell = character.cell;
    cache.half_cell = {static_cast<int>(actor.position.x / 50.0F),
                       static_cast<int>(actor.position.z / 50.0F)};
    // ax/ak/aB在首次c前仍为false，即使位置在村内也不能提前刷新。
    world.ai.contexts.emplace(identity, cache);
    ref::RescueActorContext context;
    context.destination = ref::Position{}; // 原O new int[4]全0，不是不存在目标。
    world.actors.emplace(identity, context);
    c.routes.shop_actors.emplace(identity,
                                 ref::ShopActorRecord{character.equipment[0], {}, {}, {}});
    c.routes.shop_humans.at(character.definition_id).reselect[0] =
        character.weapon_reselect_counter;
    c.routes.shop_humans.at(character.definition_id).satisfaction = character.satisfaction;
    const float projected_x = actor.position.x * 30.0F / 100.0F + actor.position.z * 30.0F / 100.0F;
    const float projected_y =
        actor.position.x * -15.0F / 100.0F + actor.position.z * 15.0F / 100.0F;
    c.actor_metadata.emplace(identity, StartupWorldActorMetadata{character.sex,
                                                                 character.job_id,
                                                                 character.equipment[0],
                                                                 {static_cast<int>(projected_x),
                                                                  static_cast<int>(projected_y)}});
    world.ai.next_actor_id = identity.value + 1; // 维护allocator，原UID仍独立保留。
}
} // namespace
StartupWorldProjectionResult
prepare_startup_world_projection(const StartupState &s, const ref::WorldRandomStream &random) {
    try {
        const auto &evidence = startup_evidence();
        if (!s.terrain_edits.empty() || s.facilities.size() != s.loaded_map.instances.size() ||
            s.loaded_map.width != evidence.width || s.loaded_map.height != evidence.height ||
            s.simulation_steps > 420 || s.event89_count < 0 || s.event89_count > 1 ||
            s.character.has_value() != (s.event89_count == 1))
            throw StartupWorldProjectionError::invalid_snapshot;
        StartupWorldProjection c;
        c.rules = &startup_world_rules();
        c.routes.random = random;
        c.routes.world.ai.accounting = s.accounting;
        c.routes.world.ai.next_cash_id = s.next_cash_id;
        c.routes.world.month_index = s.calendar[1];
        c.calendar = s.calendar;
        c.popularity = s.popularity;
        c.arrival_counter = s.arrival_counter;
        c.event89_count = s.event89_count;
        c.simulation_steps = s.simulation_steps;
        const auto ground = std::find_if(c.rules->facilities.begin(), c.rules->facilities.end(),
                                         [](const auto &d) { return d.kind == 7; });
        const auto special = std::find_if(c.rules->facilities.begin(), c.rules->facilities.end(),
                                          [](const auto &d) { return d.kind == 10; });
        if (ground == c.rules->facilities.end() || special == c.rules->facilities.end())
            throw StartupWorldProjectionError::missing_definition;
        c.ground_definition = ground->id;          // c/n1876：o.S首个kind7。
        c.special_ground_definition = special->id; // c/n1883：o.T首个kind10，实际19非23。
        install_definitions(c);
        install_map(c, s);
        if (s.character)
            install_first(c, *s.character);
        return {StartupWorldProjectionError::none, std::move(c)};
    } catch (StartupWorldProjectionError error) {
        return {error, {}};
    } catch (const std::exception &) {
        return {StartupWorldProjectionError::projection_failed, {}};
    }
}
} // namespace dungeon_village_prototype
