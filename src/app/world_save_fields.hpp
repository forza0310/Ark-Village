#pragma once

// Explicit schema order. Adding a durable field requires a schema/version review.
// No rules pointer, random engine, UI page, output queue or ledger history is visited.
// world_save_policy.json separately classifies every Owner-reachable declaration; the AST
// audit checks additions and visitor inclusion without generating or changing this wire order.
#include "ark/app/world_save.hpp"

#include <limits>
#include <stdexcept>

namespace ark::app::save_detail {
namespace r = simulation::rules;
namespace s = simulation;

template <class Archive> void index(Archive &io, std::size_t &value) {
    std::uint64_t encoded = value;
    io(encoded);
    if constexpr (Archive::reading) {
        if (encoded > std::numeric_limits<std::size_t>::max())
            throw std::overflow_error("Save index exceeds host range");
        value = static_cast<std::size_t>(encoded);
    }
}

#define ARK_SAVE_FIELDS(Type, ...)                                                                 \
    template <class Archive> void fields(Archive &io, Type &x) { io(__VA_ARGS__); }

ARK_SAVE_FIELDS(r::BuildingId, x.value)
ARK_SAVE_FIELDS(r::CharacterId, x.value)
ARK_SAVE_FIELDS(r::ObjectId, x.value)
ARK_SAVE_FIELDS(r::Position, x.x, x.y)
ARK_SAVE_FIELDS(r::WorldPosition, x.x, x.z)
ARK_SAVE_FIELDS(r::CombatPoint, x.x, x.height, x.z)
ARK_SAVE_FIELDS(r::TownBounds, x.left, x.right, x.top, x.bottom)
ARK_SAVE_FIELDS(r::LevelEndpoints, x.first, x.fifth)
ARK_SAVE_FIELDS(r::FacilityEconomyDefinition, x.attributes, x.upgrade_uses, x.construction_cost,
                x.construction_ticks, x.legacy_flags, x.decoration)
ARK_SAVE_FIELDS(r::FacilityPlacement, x.instance_id, x.definition_id, x.shape, x.orientation,
                x.anchor)
ARK_SAVE_FIELDS(r::FacilityTileBinding, x.instance_id, x.definition_id, x.fragment_index)
ARK_SAVE_FIELDS(r::LegacyMapCell, x.legacy_state, x.category, x.facility)
ARK_SAVE_FIELDS(r::LegacyMap, x.width, x.height, x.cells)
ARK_SAVE_FIELDS(r::ArrivalBinding, x.goal, x.instance_id, x.definition_id)
ARK_SAVE_FIELDS(r::LegacyPathResult, x.error, x.steps, x.cost)
ARK_SAVE_FIELDS(r::CandidateDefinition, x.definition_id, x.legacy_category, x.definition_charm)
ARK_SAVE_FIELDS(r::CandidateInstance, x.instance_id, x.definition_id, x.legacy_phase)
template <class Archive> void fields(Archive &io, r::ActivityCandidateCell &x) {
    io(x.position, x.definition, x.instance, x.cost, x.origin);
    index(io, x.source_index);
}
template <class Archive> void fields(Archive &io, r::SnapshotFacilityTarget &x) {
    index(io, x.drawn_active_index);
    index(io, x.drawn_snapshot_index);
    index(io, x.goal_snapshot_index);
    io(x.goal);
}
ARK_SAVE_FIELDS(r::FacilityDeparture, x.category, x.selection, x.binding, x.route,
                x.legacy_direction)
ARK_SAVE_FIELDS(r::FacilityArrivalState, x.legacy_visit_counts, x.legacy_category_one_count,
                x.legacy_category_six_counter, x.last_visited_instance, x.legacy_actor_total,
                x.current_month_facility_sales)
template <class Archive> void fields(Archive &io, r::RescueActorContext &x) {
    io(x.visits, x.binding, x.path_pending, x.on_event_cell, x.definition_task_flag, x.journey);
    index(io, x.waypoint);
    io(x.horizontal_velocity, x.town_updates, x.outside_updates, x.blocked_updates, x.spawn_updates,
       x.no_path_updates, x.short_exit_updates, x.bad_area_updates, x.monster_mode, x.destination,
       x.unbound_route);
}
ARK_SAVE_FIELDS(r::RescueFacility, x.placement, x.kind, x.category, x.detail, x.price, x.status,
                x.upgrade_uses, x.sales, x.occupants, x.definition_wait)
ARK_SAVE_FIELDS(r::FacilityUseProgress, x.level, x.completed_uses, x.upgrade_pending)
ARK_SAVE_FIELDS(r::ActorControlState, x.flags, x.state, x.action, x.action_counter,
                x.alternate_counter, x.facing, x.queue)
ARK_SAVE_FIELDS(r::CharacterHpState, x.requested_delta, x.displayed, x.origin, x.target,
                x.animating, x.legacy_tick)
ARK_SAVE_FIELDS(r::BattleActorRecord, x.id, x.kind, x.definition, x.control, x.hp, x.capacity,
                x.baseline, x.state_counter, x.state_parameter, x.attack_count, x.down_timer,
                x.object_slot, x.rescue, x.encounter, x.group, x.attack_slot, x.monster_posture,
                x.legacy_id, x.body, x.sprite, x.position, x.attack_position, x.miss,
                x.damage_total, x.hit_count, x.hit_flash, x.label_timer, x.miss_label, x.follow,
                x.attack_armed, x.combo_index, x.combo_count, x.attack_idle, x.attack_destination,
                x.perceived_enemy, x.perceived_distance, x.blocked_battle_steps, x.attack_cooldown,
                x.decision_start, x.physics_pause, x.vertical_velocity, x.area_after)
ARK_SAVE_FIELDS(r::HumanBattleRecord, x.kills, x.killed_stat1, x.battle_reward_stat, x.task_kills,
                x.participant_downs, x.recent_reward, x.recent_kills, x.luck)
ARK_SAVE_FIELDS(r::MonsterBattleRecord, x.human_kills, x.stat1, x.statF, x.death_reward, x.rank,
                x.flags)
ARK_SAVE_FIELDS(r::GroundObjectState, x.id, x.state, x.counter, x.duration, x.delay, x.kind,
                x.definition, x.position, x.velocity, x.acceleration, x.cached_cell, x.inside_town)
ARK_SAVE_FIELDS(r::BattleCommitState, x.actors, x.humans, x.monsters, x.participants,
                x.quest_encounters, x.global_downs, x.defeated_definitions, x.drop_progress,
                x.objects, x.next_object_id)
ARK_SAVE_FIELDS(r::HumanDefinitionStatsInput, x.current_profession, x.legacy_u, x.base, x.extra,
                x.profession_levels, x.equipment, x.learned_spells, x.spell_professions)
ARK_SAVE_FIELDS(r::HumanDerivedStats, x.growth, x.attributes, x.combat, x.available_spells)
ARK_SAVE_FIELDS(r::DelayedRewardState, x.amount, x.counter)
ARK_SAVE_FIELDS(r::RewardHumanDefinition, x.definition, x.derived, x.experience, x.pending,
                x.notice_pending, x.notice_attributes)
// cd/ce are deliberately omitted; cached c() facts retain their original update timing.
ARK_SAVE_FIELDS(r::RewardActorContext, x.cell, x.inside_town, x.facility_category, x.move_area,
                x.half_cell, x.low_hp)
ARK_SAVE_FIELDS(r::HumanProfessionRule, x.maximum_growth, x.attribute_percent, x.difficulty,
                x.unlocked)
ARK_SAVE_FIELDS(r::RewardMonsterDefinition, x.defeats, x.growth, x.base_death_reward,
                x.base_cash_reward, x.base_hp, x.body, x.sprite_variant, x.required_progress,
                x.status, x.newly_unlocked, x.introduced, x.has_introduction_script, x.base_attack,
                x.base_defense)
ARK_SAVE_FIELDS(r::EncounterRuntimeState, x.id, x.center, x.state, x.counter, x.idle, x.spawned,
                x.quota, x.reward)
ARK_SAVE_FIELDS(r::BattleGroupMember, x.id, x.flags)
ARK_SAVE_FIELDS(r::BattleGroupState, x.tick, x.duration, x.cycle, x.alternating_side, x.humans,
                x.monsters)
ARK_SAVE_FIELDS(r::CombatInfluenceCandidate, x.width, x.height, x.human_field, x.monster_field)
ARK_SAVE_FIELDS(r::RewardEncounter, x.runtime, x.members, x.group_exists, x.group, x.legacy_id,
                x.influence, x.human_scratch, x.monster_scratch, x.linked_monsters)
ARK_SAVE_FIELDS(r::ProjectileState, x.kind, x.caster, x.original_target, x.position, x.velocity,
                x.acceleration, x.facing, x.counter, x.delay, x.effect, x.damage)

template <class Archive> void fields(Archive &io, r::PeriodAccounting &x) {
    std::int64_t funds = x.funds();
    std::uint16_t points = x.village_points();
    io(funds, points);
    if constexpr (Archive::reading)
        x = r::PeriodAccounting(funds, points);
}
ARK_SAVE_FIELDS(r::AiRewardState, x.battle, x.retired_actors, x.human_order, x.monster_order,
                x.contexts, x.growth, x.professions, x.monster_growth, x.monster_definition_order,
                x.monster_progress, x.monster_limit, x.encounters, x.encounter_order,
                x.retired_encounters, x.projectiles, x.projectile_order, x.next_projectile_id,
                x.accounting, x.next_cash_id, x.next_actor_id, x.next_encounter_id,
                x.legacy_encounter_counter, x.period, x.pending_completion, x.task_active,
                x.task_completed, x.feature16)
ARK_SAVE_FIELDS(r::RescueWorldState, x.ai, x.map, x.actors, x.facilities, x.human_spending,
                x.facility_uses, x.month_index, x.object_order)
ARK_SAVE_FIELDS(r::WorldScheduleState, x.world, x.surface, x.map_flags, x.town, x.spawn_cells,
                x.facility_order, x.updates, x.popularity_queue, x.rescue_available)
ARK_SAVE_FIELDS(r::WorldCalendarState, x.year, x.month, x.subperiod, x.units, x.previous_units,
                x.month_ticks)
// Only durable clock/speed counters are retained; loading enters the ordinary main scene.
ARK_SAVE_FIELDS(r::WorldSceneState, x.world, x.calendar, x.speed_setting, x.frame_counter,
                x.scene_counter, x.first_normal_refresh)
ARK_SAVE_FIELDS(r::WorldEventTask, x.definition_task_flag, x.kind, x.center, x.encounter)
ARK_SAVE_FIELDS(r::ShopHumanRecord, x.equipment, x.reselect, x.satisfaction)
ARK_SAVE_FIELDS(r::ShopActorRecord, x.weapon, x.selected_weapon, x.selected_armor,
                x.selected_accessory)
ARK_SAVE_FIELDS(r::ObjectCatalogRecord, x.flags, x.status, x.unlock_counter, x.newly_unlocked,
                x.inventory, x.free_purchases)
ARK_SAVE_FIELDS(r::ObjectShopRecord, x.category)
ARK_SAVE_FIELDS(r::DungeonFacilityProgress, x.updates, x.progress, x.extent, x.percent,
                x.previous_percent, x.challenges)
ARK_SAVE_FIELDS(r::DungeonActorProgress, x.progress, x.previous, x.percent, x.endurance,
                x.retreat_state, x.retreat_updates, x.constrained)
ARK_SAVE_FIELDS(s::StartupWorldHumanCalendar, x.absent_months, x.yearly_totals, x.legacy_F,
                x.legacy_G, x.continuation_cost, x.contribution, x.celebrations)
ARK_SAVE_FIELDS(s::StartupWorldActorMetadata, x.sex, x.profession, x.weapon, x.cached_view,
                x.render_position, x.cached_screen_position)
ARK_SAVE_FIELDS(r::NeighbourSource, x.instance_id, x.definition_id)
ARK_SAVE_FIELDS(r::WorldMapNeighbourCache, x.current, x.previous, x.sources, x.visited)
ARK_SAVE_FIELDS(r::DungeonFinishSurface, x.definition, x.updates, x.instance, x.fragment,
                x.display_definition, x.variant, x.road_mask)
template <class Archive> void fields(Archive &io, r::WorldScriptContinuation &x) {
    io(x.event);
    index(io, x.next_instruction);
    io(x.remaining_updates, x.legacy_tag, x.saved_context, x.replacement, x.parameters);
}
ARK_SAVE_FIELDS(r::WorldScriptUnlockDefinition, x.status, x.pending_notice, x.name, x.extra,
                x.satisfaction)
ARK_SAVE_FIELDS(r::WorldScriptFacilityDefinition, x.category, x.level, x.economy, x.improvements,
                x.attributes)
ARK_SAVE_FIELDS(r::WorldScriptState, x.event_calls, x.continuations, x.context, x.village_name,
                x.activities, x.humans, x.professions, x.facilities, x.human_catalog_complete,
                x.facility_catalog_complete, x.job_counts, x.user_flags)
ARK_SAVE_FIELDS(r::CalendarMaintenanceShopItem, x.definition, x.minimum_rank, x.maximum_quantity,
                x.quantity, x.presence, x.legacy_q, x.newly_available)
ARK_SAVE_FIELDS(r::WorldPopularityReward, x.definition, x.legacy_tag, x.threshold, x.human,
                x.facility, x.program, x.flags, x.status, x.pending_notice)
ARK_SAVE_FIELDS(r::WorldFacilityUpdateDefinition, x.flags, x.popularity_reward)
ARK_SAVE_FIELDS(r::WorldFacilityUpdateDetails, x.construction_limit, x.condition,
                x.completion_popularity, x.residence_mode, x.resident_definition, x.notices)
ARK_SAVE_FIELDS(r::DungeonTaskDefinitionProgress, x.kind, x.flags, x.completed,
                x.monster_definition)
ARK_SAVE_FIELDS(r::DungeonMonsterAvailability, x.status, x.pending_notice)
ARK_SAVE_FIELDS(r::DungeonTaskSuccessState, x.successes, x.ordinary_explorations,
                x.exploration_stage, x.exploration_dates, x.task_pool_progress, x.definitions,
                x.monsters, x.remaining_task_definitions)
ARK_SAVE_FIELDS(r::DungeonFinishTask, x.identity, x.definition, x.difficulty,
                x.pending_completion_value, x.facility, x.site)
ARK_SAVE_FIELDS(r::DungeonFinishSite, x.occupied_cells)
ARK_SAVE_FIELDS(r::WorldMagicPotRecipeProgress, x.identity, x.status, x.pending_notice)
ARK_SAVE_FIELDS(s::StartupWorldHumanProfile, x.name, x.sex, x.custom_name)

ARK_SAVE_FIELDS(
    s::StartupWorldRuntimeState, x.scene, x.task, x.shop_humans, x.shop_actors, x.items,
    x.dungeon_facilities, x.dungeon_actors, x.catalog, x.shops, x.shop_order, x.item_rewards,
    x.human_definition_state, x.human_homes, x.human_presence, x.human_flags, x.human_calendar,
    x.actor_metadata, x.facility_original_ids, x.facility_ordinals, x.facility_residents,
    x.facility_difficulties, x.facility_flags, x.neighbourhood, x.neighbourhood_details, x.surface,
    x.road_patches, x.scripts, x.cash_peak, x.cash_peak_village, x.monthly_cash,
    x.facility_monthly_cash, x.facility_month_age, x.shop_item_stock, x.report_state,
    x.report_counter, x.report_snapshot, x.report_records, x.report_new_records, x.report_portraits,
    x.report_kills, x.maximum_income, x.village_points, x.popularity, x.maximum_popularity,
    x.popularity_display, x.popularity_rewards, x.facility_definitions, x.facility_presence,
    x.residence_catalog_available, x.human_profession_changes, x.residence_hint_counter,
    x.activity_flags, x.activity_counts, x.human_activity_previous, x.facility_details,
    x.task_progress, x.tasks, x.task_order, x.task_original_ids, x.task_sequence,
    x.next_facility_identity, x.next_task_identity, x.task_replay_order, x.task_special_selection,
    x.task_special_selection_index, x.task_special_selection_list, x.fence_level, x.base_variants,
    x.active_task, x.participants, x.sites, x.ground_definition, x.special_ground_definition,
    x.arrival_counter, x.event89_count, x.entry_updates, x.global_updates, x.camera,
    x.previous_camera, x.camera_velocity, x.facility_item_confirmations, x.item_commerce_read,
    x.facility_commerce_read, x.medal_count, x.facility_free_builds, x.facility_unlock_counters,
    x.facility_unlock_notices, x.rank, x.quarter_counter, x.legacy_D, x.legacy_n,
    x.yearly_statistics, x.events_held, x.task_subperiods, x.generation_retry, x.completion_mode,
    x.system_completion_mode, x.save_marker, x.system_unlock_data, x.rank_met, x.rank_values,
    x.rank_history, x.simulation_steps, x.clock_parameter, x.calendar_advance, x.magic_pot_recipes,
    x.human_profiles)

#undef ARK_SAVE_FIELDS
} // namespace ark::app::save_detail
