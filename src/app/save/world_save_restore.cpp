#include "ark/app/save/world_save.hpp"
#include "ark/simulation/facilities/rules/world_magic_pot.hpp"
#include "ark/simulation/map/rules/world_map_refresh.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace ark::app {
namespace {
using State = simulation::StartupWorldRuntimeState;
namespace ref = simulation::rules;

bool fail(std::string *reason, const char *message) {
    if (reason)
        *reason = message;
    return false;
}
bool counter(int value) { return value >= 0 && value < std::numeric_limits<int>::max(); }
bool coordinate(float value) {
    // Projection casts multiplied positions to int. Bound them before any cache consumer.
    return std::isfinite(value) && std::abs(value) <= 1000000.0F;
}
bool point(ref::CombatPoint value) {
    return coordinate(value.x) && coordinate(value.height) && coordinate(value.z);
}
bool in_map(const ref::LegacyMap &map, ref::Position position) {
    return position.x >= 0 && position.y >= 0 && position.x < map.width && position.y < map.height;
}
template <class Map, class Order> bool roster(const Map &map, const Order &order) {
    if (map.size() != order.size())
        return false;
    std::set<typename Order::value_type> seen;
    for (const auto id : order)
        if (!map.count(id) || !seen.insert(id).second)
            return false;
    return true;
}
template <class Map> bool next_identity(const Map &map, std::uint64_t next) {
    if (next == 0 || next == std::numeric_limits<std::uint64_t>::max())
        return false;
    for (const auto &entry : map)
        if (entry.first >= next)
            return false;
    return true;
}
const ref::BattleActorRecord *actor(const State &s, ref::CharacterId id) {
    const auto &ai = s.scene.world.world.ai;
    const auto live = ai.battle.actors.find(id);
    if (live != ai.battle.actors.end())
        return &live->second;
    const auto retired = ai.retired_actors.find(id);
    return retired == ai.retired_actors.end() ? nullptr : &retired->second;
}
const ref::RewardEncounter *encounter(const State &s, std::uint64_t id) {
    const auto &ai = s.scene.world.world.ai;
    const auto live = ai.encounters.find(id);
    if (live != ai.encounters.end())
        return &live->second;
    const auto retired = ai.retired_encounters.find(id);
    return retired == ai.retired_encounters.end() ? nullptr : &retired->second;
}
bool path(const ref::LegacyMap &map, const ref::LegacyPathResult &route) {
    return route.error == ref::MapAccessError::none && route.cost >= 0 &&
           std::all_of(route.steps.begin(), route.steps.end(),
                       [&](const auto p) { return in_map(map, p); });
}
std::set<std::uint64_t> retired_task_facilities(const State &s) {
    const std::set<std::uint64_t> listed(s.task_order.begin(), s.task_order.end());
    std::set<std::uint64_t> result;
    // Completed task objects remain readable after their site leaves the live map.
    for (const auto &entry : s.tasks) {
        const auto id = entry.second.facility;
        if (id && *id && *id < s.next_facility_identity &&
            !s.scene.world.world.facilities.count(*id) && s.facility_original_ids.count(*id) &&
            entry.second.identity == entry.first && s.active_task != entry.first &&
            !listed.count(entry.first))
            result.insert(*id);
    }
    return result;
}
bool binding(const State &s, const ref::ArrivalBinding &value,
             const std::set<std::uint64_t> &retired) {
    const auto &world = s.scene.world.world;
    if (!in_map(world.map, value.goal))
        return false;
    const auto f = world.facilities.find(value.instance_id.value);
    if (f == world.facilities.end())
        return retired.count(value.instance_id.value) &&
               s.scripts.facilities.count(value.definition_id);
    if (f->second.placement.definition_id != value.definition_id)
        return false;
    const auto &cell =
        world.map.cells.at(static_cast<std::size_t>(value.goal.y) * world.map.width + value.goal.x);
    return cell.facility && cell.facility->instance_id == value.instance_id &&
           cell.facility->definition_id == value.definition_id;
}
bool catalog_record(const ref::ObjectCatalogRecord &value) {
    return counter(value.status) && counter(value.unlock_counter) && value.inventory >= 0 &&
           value.inventory <= 999 && counter(value.free_purchases);
}
bool same_item(const ref::ObjectCatalogRecord &a, const ref::ObjectCatalogRecord &b) {
    return a.flags == b.flags && a.status == b.status && a.unlock_counter == b.unlock_counter &&
           a.newly_unlocked == b.newly_unlocked && a.inventory == b.inventory &&
           a.free_purchases == b.free_purchases;
}

bool rebuild_map(State &s) {
    auto &world = s.scene.world.world;
    ref::WorldMapRefreshState map;
    map.map = world.map;
    map.ground_definition = s.ground_definition;
    map.special_ground_definition = s.special_ground_definition;
    map.base_variants = s.base_variants;
    map.fence_level = s.fence_level;
    map.fence_levels = s.rules->fences;
    for (const auto &d : s.rules->facilities)
        map.definitions.emplace(d.id,
                                ref::WorldMapDefinition{d.display_id, d.kind,
                                                        static_cast<ref::FacilityShape>(d.shape),
                                                        d.neighbour_effects});
    for (std::size_t n = 0; n < s.surface.size(); ++n) {
        const auto &cell = s.surface[n];
        map.surface.push_back({cell.definition, cell.updates, cell.display_definition, cell.variant,
                               cell.road_mask, cell.fragment, cell.instance, s.road_patches[n][0],
                               s.road_patches[n][1]});
    }
    for (const auto id : s.scene.world.facility_order) {
        map.facilities.push_back(world.facilities.at(id).placement);
        auto &cache = map.neighbours[id];
        const auto old = s.neighbourhood_details.find(id);
        if (old != s.neighbourhood_details.end())
            cache = old->second;
        cache.current = s.neighbourhood.at(id);
        cache.notices = s.facility_details.at(id).notices;
    }
    const auto display = ref::prepare_world_map_display(map);
    if (!display.candidate)
        return false;
    const auto roads = ref::prepare_world_map_roads(display.candidate->state);
    if (!roads.candidate)
        return false;
    const auto neighbours = ref::prepare_world_map_neighbours(roads.candidate->state, false);
    if (!neighbours.candidate)
        return false;
    const auto &rebuilt = neighbours.candidate->state;
    world.map = rebuilt.map;
    const auto history = s.neighbourhood_details;
    s.neighbourhood_details = rebuilt.neighbours;
    for (auto &entry : s.neighbourhood_details) {
        const auto old = history.find(entry.first);
        if (old != history.end())
            entry.second.previous = old->second.previous;
    }
    s.scene.world.surface.clear();
    for (std::size_t n = 0; n < s.surface.size(); ++n) {
        const auto &cell = rebuilt.surface[n];
        s.surface[n] = {cell.definition, cell.updates, cell.instance_field, cell.fragment,
                        cell.display,    cell.variant, cell.road_mask};
        s.road_patches[n] = {cell.road_quad, cell.edge_road_pair};
        s.scene.world.surface.push_back(static_cast<int>(world.map.cells[n].category));
    }
    for (const auto &entry : rebuilt.neighbours)
        s.neighbourhood[entry.first] = entry.second.current;
    return true;
}

void clear_presentation(State &s) {
    ref::WorldScriptPage main;
    main.id = 1;
    s.scripts.pages = {main};
    s.scripts.next_page_id = 2;
    s.scripts.executing_page.reset();
    s.scripts.page_mutations_locked = false;
    s.scripts.redraw_requested = false;
    s.scripts.selected_actor.reset();
    s.scripts.selected_monster.reset();
    s.scripts.selected_facility.reset();
    s.scripts.notices.clear();
    s.scripts.finance.reset();
    s.scripts.pending_completion = 0;
    s.scripts.popularity_queue.clear();
    s.scripts.human_order.clear();
    s.scripts.scene_mode = s.scripts.scene_updates = s.scripts.exploration_phase = 0;
    s.scripts.medal_count = 0;
    s.page_human_bindings.clear();
    s.task_abort_questions.clear();
    s.task_abort_answers.clear();
    s.human_pages_initialized.clear();
    s.human_detail_contexts.clear();
    s.human_page_catalogs.clear();
    s.equipment_page_catalogs.clear();
    s.human_page_selections.clear();
    s.page_job_bindings.clear();
    s.human_page_parents.clear();
    s.human_page_answers.clear();
    s.human_equipment_choices.clear();
    s.human_gift_scores.clear();
    s.human_gift_messages.clear();
    s.tax_page_residents.clear();
    s.tax_page_selection.clear();
    s.tax_page_scroll.clear();
    s.activity_pages_initialized.clear();
    s.activity_page_bindings.clear();
    s.activity_page_lists.clear();
    s.activity_page_display_humans.clear();
    s.activity_page_parents.clear();
    s.activity_page_answers.clear();
    s.activity_page_selections.clear();
    s.activity_page_scroll.clear();
    s.page_counters.clear();
    s.page_phases.clear();
    s.information_page_data.clear();
    s.task_page_lists.clear();
    s.task_recruitment_pages.clear();
    s.task_extra_pages.clear();
    s.task_page_predictions.clear();
    s.task_page_acceleration.clear();
    s.page_secondary_counters.clear();
    s.task_display_initialized.clear();
    s.deadline_initialized.clear();
    s.deadline_grades.clear();
    s.deadline_returns.clear();
    s.deadline_closed_page.reset();
    s.deadline_page.reset();
    s.award_rankings.clear();
    s.award_announced.clear();
    s.award_termination_pending.clear();
    s.award_pending_humans.clear();
    s.facility_page_bindings.clear();
    s.facility_definition_page_bindings.clear();
    s.facility_page_neighbours.clear();
    s.build_page_catalogs.clear();
    s.residence_page_candidates.clear();
    s.facility_upgrade_initialized.clear();
    s.facility_item_pages_initialized.clear();
    s.facility_item_page_items.clear();
    s.facility_item_page_lists.clear();
    s.facility_item_page_selections.clear();
    s.facility_item_response = 0;
    s.commerce_pages_initialized.clear();
    s.commerce_page_data.clear();
    s.commerce_page_lists.clear();
    s.facility_catalog_pages_initialized.clear();
    s.facility_catalog_page_data.clear();
    s.facility_catalog_page_lists.clear();
    s.facility_catalog_page_parents.clear();
    s.magic_pot_pages_initialized.clear();
    s.magic_pot_page_data.clear();
    s.magic_pot_page_lists.clear();
    s.magic_pot_page_parents.clear();
    s.magic_pot_display = {};
    s.magic_pot_comment.clear();
    s.magic_pot_output = {};
    s.rank_celebration_participants.clear();
    s.exploration_summaries.clear();
    s.exploration_displays.clear();
    s.dungeon_labels.clear();
    s.crew_summaries.clear();
    s.visual_effects.clear();
    s.delayed_effects.clear();
    s.global_effects.clear();
    s.sound_requests.clear();
    s.scene.world.hints.clear();
    s.scene.world.floating_notes.clear();
    s.confirm_input = s.cancel_input = s.menu_input = s.page_confirm_held = false;
    s.focus_held_input = 0;
    s.build_mode = 0;
    s.build_definition.reset();
    s.build_anchor.reset();
    s.build_moving_facility.reset();
    s.build_feedback_counter = 0;
    s.build_feedback_message.clear();
    s.camera_follow = s.camera_delay = 0;
    s.scene.scene_state = 0;
    s.scene.processing_phase = -1;
    s.scene.processing_subphase = 0;
    s.scene.top_is_main = true;
    auto &ai = s.scene.world.world.ai;
    ai.external_actor_roots.clear();
    ai.external_encounter_roots.clear();
    ai.facility_actor_roots.clear();
    for (const auto &f : s.scene.world.world.facilities)
        ai.facility_actor_roots.insert(ai.facility_actor_roots.end(), f.second.occupants.begin(),
                                       f.second.occupants.end());
    if (s.task.encounter)
        ai.external_encounter_roots.insert(*s.task.encounter);
    ai.battle.events.clear();
    for (const auto &event : s.scripts.event_calls)
        if (event.second > 0)
            ai.battle.events.insert(event.first);
    for (auto &entry : ai.contexts)
        entry.second.effects = {};
    for (auto &entry : s.shops)
        entry.second.notices.clear();
}
} // namespace

bool world_save_eligible(const State &s, std::string *reason) {
    // Source cancellation retains the last tool number in the main scene. That dormant
    // UI field is not persisted; scene/definition/selection still reject unfinished edits.
    if (!s.rules || s.scene.scene_state != 0 || s.scene.processing_phase != -1 ||
        s.report_state != 0 || s.deadline_page || s.deadline_closed_page || s.build_definition ||
        s.build_mode < 0 || s.build_mode >= 8 || s.build_anchor || s.build_moving_facility)
        return fail(reason, "请返回主场景，等待月报和待处理事件结束后保存");
    if (s.scripts.pages.size() != 1 ||
        s.scripts.pages.front().kind != ref::WorldScriptPageKind::scene ||
        s.scripts.pages.front().lifecycle != 2 || s.scripts.executing_page ||
        s.scripts.page_mutations_locked || !s.human_page_answers.empty() ||
        !s.task_abort_answers.empty() || !s.activity_page_answers.empty())
        return fail(reason, "请关闭当前页面并完成其中的选择后保存");
    for (const auto &entry : s.scene.world.world.ai.battle.actors)
        if (entry.second.kind == ref::ActorKind::human &&
            (entry.second.control.flags & (512U | 1024U)))
            return fail(reason, "请等待离村人物完成离开后保存");
    if (reason)
        reason->clear();
    return true;
}

WorldSaveError validate_world_save_candidate(const State &s, std::string &reason) {
    const auto invalid = [&](const char *message) {
        reason = message;
        return WorldSaveError::invalid_world;
    };
    if (!s.rules || s.rules != &simulation::startup_world_rules())
        return invalid("Save has no matching immutable world rules");
    if (!simulation::valid_startup_world_human_profiles(s))
        return invalid("Save has invalid main-character profiles");
    for (const auto &[id, profile] : s.human_profiles) {
        const auto script = s.scripts.humans.find(id);
        if (script == s.scripts.humans.end() || script->second.name != profile.name)
            return invalid("Save profile and script name disagree");
    }
    for (const auto *actors :
         {&s.scene.world.world.ai.battle.actors, &s.scene.world.world.ai.retired_actors})
        for (const auto &[id, actor] : *actors) {
            if (actor.kind != simulation::rules::ActorKind::human)
                continue;
            const auto profile = simulation::startup_world_human_profile(s, actor.definition);
            const auto metadata = s.actor_metadata.find(id);
            if (!profile || metadata == s.actor_metadata.end() ||
                metadata->second.sex != profile->sex)
                return invalid("Save profile and actor sex disagree");
        }
    const auto &rules = *s.rules;
    const auto &world = s.scene.world.world;
    const auto &ai = world.ai;
    const auto retired_facilities = retired_task_facilities(s);
    // Recipe discovery is durable; processing/display consumers must not run during restore.
    if (rules.magic_pot_recipes.size() != 40 ||
        s.magic_pot_recipes.size() != rules.magic_pot_recipes.size())
        return invalid("Save magic pot recipes do not cover the fixed dataset");
    for (const auto &definition : rules.magic_pot_recipes) {
        const auto progress = s.magic_pot_recipes.find(definition.identity);
        if (progress == s.magic_pot_recipes.end() ||
            progress->second.identity != definition.identity ||
            (progress->second.status != 0 && progress->second.status != 1))
            return invalid("Save magic pot recipe identity or status is invalid");
    }
    if (!ref::valid_world_magic_pot_state(
            s.legacy_n,
            {s.scene.calendar.year, s.scene.calendar.month, s.scene.calendar.subperiod}))
        return invalid("Save magic pot state or processing date is invalid");
    if (!ref::valid_world_calendar_state(s.scene.calendar) || !counter(s.scene.calendar.year) ||
        !counter(s.scene.calendar.month_ticks) || !counter(s.scene.frame_counter) ||
        !counter(s.scene.scene_counter) ||
        (s.scene.speed_setting != 0 && s.scene.speed_setting != 1) || s.scene.scene_state != 0 ||
        s.scene.processing_phase != -1 || s.report_state != 0 || s.deadline_page ||
        s.deadline_closed_page || s.build_mode < 0 || s.build_mode >= 8 || s.build_definition ||
        s.build_anchor || s.build_moving_facility || !s.activity_page_answers.empty() ||
        !coordinate(s.camera[0]) || !coordinate(s.camera[1]) || !coordinate(s.previous_camera[0]) ||
        !coordinate(s.previous_camera[1]) || !coordinate(s.camera_velocity[0]) ||
        !coordinate(s.camera_velocity[1]) || s.reference_viewport[2] <= 0 ||
        s.reference_viewport[3] <= 0 || s.reference_viewport[2] > 100000 ||
        s.reference_viewport[3] > 100000 || s.reference_viewport[0] < -100000 ||
        s.reference_viewport[0] > 100000 || s.reference_viewport[1] < -100000 ||
        s.reference_viewport[1] > 100000 ||
        s.simulation_steps == std::numeric_limits<std::uint64_t>::max() ||
        s.clock_parameter != 80 || s.calendar_advance != 27 || s.village_points < 0 ||
        s.village_points > 999 || s.rank < 0 || s.rank > 6 || !counter(s.medal_count) ||
        s.popularity < -50 || s.popularity == std::numeric_limits<int>::max() ||
        !counter(s.maximum_popularity) || !counter(s.arrival_counter) ||
        !counter(s.global_updates) || !counter(s.entry_updates) || !counter(s.task_subperiods) ||
        !counter(s.events_held))
        return invalid("Save calendar, scene or global ranges are invalid");
    if (!ref::valid_legacy_map(world.map) ||
        world.map.width != simulation::startup_evidence().width ||
        world.map.height != simulation::startup_evidence().height ||
        s.surface.size() != world.map.cells.size() || s.road_patches.size() != s.surface.size() ||
        s.base_variants.size() != s.surface.size() ||
        s.scene.world.map_flags.size() != s.surface.size() || s.fence_level < 0 ||
        static_cast<std::size_t>(s.fence_level) >= rules.fences.size())
        return invalid("Save map dimensions or per-cell fields are invalid");
    const auto &town = s.scene.world.town;
    // The fence level selects the source town boundary on the fixed map. Accepting an
    // arbitrary in-map rectangle would restore different routing and construction rules.
    const auto &bounds = rules.fences[s.fence_level];
    if (!counter(s.scene.world.updates) || town.left >= town.right || town.top >= town.bottom ||
        !in_map(world.map, {town.left, town.top}) ||
        !in_map(world.map, {town.right, town.bottom}) || town.left != bounds[0].x ||
        town.right != bounds[1].x || town.top != bounds[1].y || town.bottom != bounds[0].y ||
        s.scene.world.spawn_cells != simulation::startup_evidence().spawn_points)
        return invalid("Save town bounds, spawn cells or world update counter is invalid");
    std::map<int, const simulation::StartupDefinition *> definitions;
    for (const auto &d : rules.facilities)
        definitions.emplace(d.id, &d);
    if (!definitions.count(s.ground_definition) || !definitions.count(s.special_ground_definition))
        return invalid("Save references an unknown ground definition");
    for (std::size_t n = 0; n < s.surface.size(); ++n)
        if (!definitions.count(s.surface[n].definition) || !counter(s.surface[n].updates) ||
            s.base_variants[n] < -128 || s.base_variants[n] > 127)
            return invalid("Save contains an invalid surface definition or counter");
    if (!roster(world.facilities, s.scene.world.facility_order) ||
        !next_identity(world.facilities, s.next_facility_identity))
        return invalid("Save facility order or allocation identity is invalid");
    std::vector<ref::FacilityPlacement> placements;
    std::map<int, std::set<int>> ordinals;
    std::set<int> original_facilities;
    std::set<std::size_t> occupied;
    for (const auto &entry : world.facilities) {
        const auto id = entry.first;
        const auto &f = entry.second;
        const auto definition = definitions.find(f.placement.definition_id);
        if (id == 0 || f.placement.instance_id.value != id || definition == definitions.end() ||
            static_cast<int>(f.placement.shape) != definition->second->shape ||
            !s.facility_original_ids.count(id) || !s.facility_ordinals.count(id) ||
            !s.facility_residents.count(id) || !s.facility_details.count(id) ||
            !s.facility_flags.count(id) || !s.facility_difficulties.count(id) ||
            !s.facility_monthly_cash.count(id) || !s.facility_month_age.count(id) ||
            !s.facility_item_confirmations.count(id) || !s.neighbourhood.count(id) ||
            !s.dungeon_facilities.count(id) || !counter(s.facility_original_ids.at(id)) ||
            !counter(s.facility_ordinals.at(id)) ||
            !original_facilities.insert(s.facility_original_ids.at(id)).second ||
            !ordinals[f.placement.definition_id].insert(s.facility_ordinals.at(id)).second ||
            f.category < 0 || f.category > 13 || f.status < 0 || f.status > 2 ||
            !counter(s.facility_month_age.at(id)) || !counter(s.facility_difficulties.at(id)))
            return invalid("Save facility identity, definition or auxiliary record is invalid");
        const auto footprint =
            ref::facility_footprint(f.placement.shape, f.placement.orientation, f.placement.anchor,
                                    world.map.width, world.map.height);
        if (footprint.error != ref::GeometryError::none)
            return invalid("Save facility footprint is invalid");
        for (const auto &part : footprint.cells) {
            const auto index =
                static_cast<std::size_t>(part.position.y) * world.map.width + part.position.x;
            const auto &cell = world.map.cells[index];
            if (!occupied.insert(index).second || !cell.facility ||
                cell.facility->instance_id.value != id ||
                cell.facility->definition_id != f.placement.definition_id ||
                cell.facility->fragment_index != part.fragment_index ||
                !ref::legacy_surface_binding_matches(
                    cell, s.surface[index].definition,
                    definitions.at(s.surface[index].definition)->kind, s.ground_definition))
                return invalid(
                    "Save contains overlapping facilities or inconsistent tile bindings");
        }
        placements.push_back(f.placement);
        for (const auto occupant : f.occupants)
            if (!actor(s, occupant))
                return invalid("Save facility occupant is missing");
        const auto resident = s.facility_residents.at(id);
        if ((resident != -1 && !s.shop_humans.count(resident)) ||
            s.facility_details.at(id).resident_definition != resident)
            return invalid("Save facility resident is missing or inconsistent");
        const auto &progress = s.dungeon_facilities.at(id);
        const auto &detail = s.facility_details.at(id);
        if (!counter(progress.updates) || progress.progress < 0 || progress.extent < 0 ||
            !counter(detail.construction_limit) || !counter(s.facility_item_confirmations.at(id)))
            return invalid("Save facility construction or exploration counter is invalid");
        for (const auto &notice : detail.notices)
            if (notice[0] < 0 || notice[0] >= 8 || !counter(notice[1]))
                return invalid("Save facility effect history is invalid");
        for (const auto &challenge : progress.challenges)
            if (challenge[0] < 0 || challenge[1] < 0 || challenge[1] > 1 || challenge[2] < 0 ||
                challenge[2] > 3 || !counter(challenge[3]) || challenge[4] < 0 ||
                (challenge[1] == 0 &&
                 (challenge[4] > 3 || !s.catalog.count({challenge[4], challenge[5]}))) ||
                (challenge[1] == 1 && !ai.monster_growth.count(challenge[4])))
                return invalid("Save dungeon challenge has an invalid phase or definition");
    }
    for (std::size_t n = 0; n < world.map.cells.size(); ++n)
        if (world.map.cells[n].facility && !occupied.count(n))
            return invalid("Save has an orphan facility tile binding");
    if (ref::validate_facility_layout(placements, world.map.width, world.map.height) !=
        ref::GeometryError::none)
        return invalid("Save facility layout is invalid");
    for (const auto &entry : s.facility_item_confirmations)
        if (!counter(entry.second) ||
            (!world.facilities.count(entry.first) && !retired_facilities.count(entry.first)))
            return invalid("Save facility item counter references an unknown instance");
    if (s.activity_counts.size() != rules.activities.size() ||
        s.activity_flags.size() != rules.activities.size() ||
        s.scripts.activities.size() != rules.activities.size() ||
        s.human_activity_previous.size() != rules.humans.size() ||
        s.item_commerce_read.size() != rules.items.size() ||
        s.facility_commerce_read.size() != rules.facilities.size())
        return invalid("Save management tables do not cover the fixed dataset");
    for (const auto &activity : rules.activities)
        if (!s.activity_counts.count(activity.identity) ||
            !s.activity_flags.count(activity.identity) ||
            !s.scripts.activities.count(activity.identity) ||
            !counter(s.activity_counts.at(activity.identity)) ||
            !counter(s.scripts.activities.at(activity.identity).status))
            return invalid("Save village activity identity or held count is invalid");
    if (ai.professions.size() != rules.jobs.size() || ai.growth.size() != rules.humans.size() ||
        ai.battle.humans.size() != rules.humans.size() ||
        world.human_spending.size() != rules.humans.size() ||
        s.shop_humans.size() != rules.humans.size() ||
        s.scripts.humans.size() != rules.humans.size() ||
        s.scripts.professions.size() != rules.jobs.size() ||
        world.facility_uses.size() != rules.facilities.size() ||
        s.scripts.facilities.size() != rules.facilities.size() ||
        s.facility_definitions.size() != rules.facilities.size() ||
        s.facility_presence.size() != rules.facilities.size())
        return invalid("Save shared definition tables do not cover the fixed dataset");
    for (const auto &human : rules.humans) {
        const auto id = human.identity;
        if (!ai.growth.count(id) || !ai.battle.humans.count(id) || !s.shop_humans.count(id) ||
            !s.human_homes.count(id) || !s.human_presence.count(id) || !s.human_flags.count(id) ||
            !s.human_calendar.count(id) || !s.human_definition_state.count(id) ||
            !world.human_spending.count(id) || !s.human_activity_previous.count(id) ||
            !s.human_profession_changes.count(id) || !s.scripts.humans.count(id))
            return invalid("Save is missing a shared human definition");
        const auto &growth = ai.growth.at(id);
        const auto &definition = growth.definition;
        if (definition.current_profession < 0 ||
            static_cast<std::size_t>(definition.current_profession) >= rules.jobs.size() ||
            definition.profession_levels.size() != rules.jobs.size() ||
            s.human_profession_changes.at(id).size() != rules.jobs.size() ||
            !std::all_of(
                definition.profession_levels.begin(), definition.profession_levels.end(),
                [](int level) { return level >= 1 && level <= 10; }) ||
            !counter(growth.experience) || !counter(growth.pending.amount) ||
            !ref::derive_human_stats(definition, ai.professions).candidate)
            return invalid("Save shared human profession or growth is invalid");
        for (std::size_t slot = 0; slot < 4; ++slot) {
            const auto equipment = s.shop_humans.at(id).equipment[slot];
            if (equipment && !s.catalog.count({slot == 0 ? 1 : slot == 3 ? 3 : 2, *equipment}))
                return invalid("Save human equipment definition is missing");
        }
        if (!counter(s.human_calendar.at(id).absent_months) ||
            !counter(s.human_calendar.at(id).celebrations) || s.human_homes.at(id)[2] < 0 ||
            s.human_homes.at(id)[2] > 2)
            return invalid("Save human residence or calendar is invalid");
    }
    for (std::size_t n = 0; n < rules.jobs.size(); ++n)
        if (!s.scripts.professions.count(static_cast<int>(n)))
            return invalid("Save shared profession definition is missing");
    for (const auto &d : rules.facilities)
        if (!world.facility_uses.count(d.id) || !s.scripts.facilities.count(d.id) ||
            !s.facility_definitions.count(d.id) || !s.facility_presence.count(d.id) ||
            !s.residence_catalog_available.count(d.id) || !s.facility_unlock_counters.count(d.id) ||
            !s.facility_unlock_notices.count(d.id) || !s.facility_commerce_read.count(d.id) ||
            world.facility_uses.at(d.id).level < 1 || world.facility_uses.at(d.id).level > 5)
            return invalid("Save shared facility definition or level is invalid");
    std::set<ref::CharacterId> live_order;
    std::set<int> human_uids, monster_uids;
    for (const auto *order : {&ai.human_order, &ai.monster_order})
        for (const auto id : *order) {
            const auto live = ai.battle.actors.find(id);
            if (live == ai.battle.actors.end() || !live_order.insert(id).second ||
                (live->second.kind == ref::ActorKind::human) != (order == &ai.human_order))
                return invalid(
                    "Save actor roster contains a missing, duplicated or wrong-kind actor");
            auto &uids = live->second.kind == ref::ActorKind::human ? human_uids : monster_uids;
            if (!counter(live->second.legacy_id) || !uids.insert(live->second.legacy_id).second)
                return invalid("Save live actor original UID is duplicated");
        }
    if (live_order.size() != ai.battle.actors.size() || ai.next_actor_id == 0 ||
        ai.next_actor_id == std::numeric_limits<std::uint64_t>::max() || ai.next_cash_id == 0 ||
        ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() || ai.period == 0)
        return invalid("Save actor roster or maintenance allocator is invalid");
    for (const auto *actors : {&ai.battle.actors, &ai.retired_actors})
        for (const auto &entry : *actors) {
            const auto id = entry.first;
            const auto &a = entry.second;
            const auto &c = a.control;
            if (!id.value || !(id == a.id) || id.value >= ai.next_actor_id ||
                (actors == &ai.retired_actors && ai.battle.actors.count(id)) ||
                (a.kind != ref::ActorKind::human && a.kind != ref::ActorKind::monster) ||
                (a.kind == ref::ActorKind::human ? !ai.growth.count(a.definition)
                                                 : !ai.monster_growth.count(a.definition)) ||
                c.state < 0 || c.state > 20 || c.action < 0 || c.action > 11 || c.facing < 0 ||
                c.facing > 3 || a.body < 0 || a.body > 3 || !counter(c.action_counter) ||
                !counter(c.alternate_counter) || !counter(a.state_counter) || a.baseline < 0 ||
                a.baseline > 20 || a.capacity < 0 || a.hp.legacy_tick < 0 ||
                a.hp.legacy_tick > 30 || (a.hp.animating && a.hp.legacy_tick == 30) ||
                !point(a.position) || !point(a.attack_position) || !point(a.attack_destination) ||
                !point(a.decision_start) || !coordinate(a.vertical_velocity) ||
                !std::isfinite(a.perceived_distance) || a.perceived_distance < 0 ||
                !std::all_of(c.queue.begin(), c.queue.end(), ref::valid_actor_control))
                return invalid("Save actor state, movement or control queue is invalid");
            if ((a.rescue && !actor(s, *a.rescue)) || (a.follow && !actor(s, *a.follow)) ||
                (a.perceived_enemy && !actor(s, *a.perceived_enemy)) ||
                (a.encounter && !encounter(s, *a.encounter)) ||
                (a.group && !encounter(s, *a.group)))
                return invalid("Save actor has a dangling durable reference");
            if (actors == &ai.battle.actors &&
                (!ai.contexts.count(id) || !world.actors.count(id) || !s.actor_metadata.count(id) ||
                 (a.kind == ref::ActorKind::human &&
                  (!s.dungeon_actors.count(id) || !s.shop_actors.count(id)))))
                return invalid("Save live actor is missing a runtime context");
            if (actors == &ai.battle.actors && a.kind == ref::ActorKind::human &&
                (c.flags & (512U | 1024U)))
                return invalid("Save contains an unsupported departing adventurer");
        }
    for (const auto &entry : world.actors) {
        if (!ai.battle.actors.count(entry.first))
            continue; // Retired/collected objects can leave inactive adapter caches.
        const auto &c = entry.second;
        if ((c.binding && !binding(s, *c.binding, retired_facilities)) ||
            (c.destination && !in_map(world.map, *c.destination)) ||
            (c.unbound_route && !path(world.map, *c.unbound_route)) ||
            (c.journey &&
             (!binding(s, c.journey->binding, retired_facilities) ||
              !path(world.map, c.journey->route) || c.waypoint > c.journey->route.steps.size())) ||
            !coordinate(c.horizontal_velocity.x) || !coordinate(c.horizontal_velocity.z))
            return invalid("Save actor facility/path reference is invalid");
        const auto &meta = s.actor_metadata.at(entry.first);
        if (!point(meta.render_position) || meta.profession < 0 ||
            static_cast<std::size_t>(meta.profession) >= rules.jobs.size())
            return invalid("Save actor presentation cache is invalid");
        const auto *a = actor(s, entry.first);
        if (a && a->kind == ref::ActorKind::human) {
            const auto &equipment = s.shop_actors.at(entry.first);
            if (!s.catalog.count({1, equipment.weapon}) || !s.catalog.count({1, meta.weapon}) ||
                (equipment.selected_weapon && !s.catalog.count({1, *equipment.selected_weapon})) ||
                (equipment.selected_armor && !s.catalog.count({2, *equipment.selected_armor})) ||
                (equipment.selected_accessory &&
                 !s.catalog.count({3, *equipment.selected_accessory})))
                return invalid("Save actor held or selected equipment definition is missing");
            const auto &dungeon = s.dungeon_actors.at(entry.first);
            if (!counter(dungeon.retreat_state) || !counter(dungeon.retreat_updates))
                return invalid("Save dungeon actor retreat counter is invalid");
        }
    }
    if (!roster(ai.encounters, ai.encounter_order) ||
        !next_identity(ai.encounters, ai.next_encounter_id) ||
        !next_identity(ai.retired_encounters, ai.next_encounter_id) ||
        !roster(ai.projectiles, ai.projectile_order) ||
        !next_identity(ai.projectiles, ai.next_projectile_id) ||
        !roster(ai.battle.objects, world.object_order) ||
        !next_identity(ai.battle.objects, ai.battle.next_object_id))
        return invalid("Save encounter/object/projectile order or allocator is invalid");
    for (const auto *encounters : {&ai.encounters, &ai.retired_encounters})
        for (const auto &entry : *encounters) {
            const auto &e = entry.second;
            if (entry.first != e.runtime.id ||
                (encounters == &ai.retired_encounters && ai.encounters.count(entry.first)) ||
                e.runtime.state < 0 || e.runtime.state > 3 || !counter(e.runtime.counter) ||
                !counter(e.runtime.idle) || !counter(e.runtime.spawned) ||
                !counter(e.runtime.quota) || !counter(e.group.tick) || e.group.duration <= 0 ||
                e.group.cycle < 0 || e.group.cycle > 2 || e.group.alternating_side < 0 ||
                e.group.alternating_side > 1)
                return invalid("Save encounter identity or phase is invalid");
            if (e.influence && (!ref::valid_combat_influence_field(*e.influence) ||
                                e.influence->width != world.map.width * 2 ||
                                e.influence->height != world.map.height * 2))
                return invalid("Save encounter influence field is invalid");
            for (const auto member : e.members)
                if (!actor(s, member))
                    return invalid("Save encounter member is missing");
            for (const auto *members : {&e.group.humans, &e.group.monsters})
                for (const auto &member : *members) {
                    const auto *a = actor(s, member.id);
                    if (!a || (a->kind == ref::ActorKind::human) != (members == &e.group.humans))
                        return invalid("Save encounter group member is missing or wrong-kind");
                }
        }
    for (const auto &entry : ai.projectiles) {
        const auto &p = entry.second;
        if (!actor(s, p.caster) || !actor(s, p.original_target) ||
            !s.actor_metadata.count(p.caster) || !s.actor_metadata.count(p.original_target) ||
            !ai.contexts.count(p.caster) || !ai.contexts.count(p.original_target) ||
            p.kind < ref::ProjectileKind::arrow || p.kind > ref::ProjectileKind::delayed_damage ||
            !point(p.position) || !point(p.velocity) || !point(p.acceleration) ||
            !counter(p.counter) || p.facing < 0 || p.facing > 3)
            return invalid("Save projectile has an invalid state or dangling actor");
    }
    for (const auto &entry : ai.battle.objects) {
        const auto &o = entry.second;
        if (entry.first != o.id.value || o.kind < 0 || o.kind > 3 ||
            !s.catalog.count({o.kind, o.definition}) || o.state < 0 || o.state > 6 ||
            !counter(o.counter) || o.duration < 0 || o.delay < 0 || !point(o.position) ||
            !point(o.velocity) || !point(o.acceleration))
            return invalid("Save ground object identity, definition or movement is invalid");
    }
    for (const auto &entry : s.catalog)
        if (entry.first.first < 0 || entry.first.first > 3 || !catalog_record(entry.second))
            return invalid("Save catalog state is invalid");
    if (s.catalog.size() != rules.equipment.size() + rules.items.size() ||
        s.items.size() != rules.items.size() || s.shop_item_stock.size() != rules.items.size())
        return invalid("Save catalog keys do not match the fixed dataset");
    for (const auto &equipment : rules.equipment)
        if (!s.catalog.count({equipment.shop.kind, equipment.shop.id}))
            return invalid("Save equipment catalog does not cover the fixed dataset");
    for (const auto &item : rules.items)
        if (!s.catalog.count({0, item.identity}) || !s.items.count(item.identity) ||
            !s.shop_item_stock.count(item.identity) || !s.item_commerce_read.count(item.identity) ||
            !catalog_record(s.items.at(item.identity)) ||
            s.shop_item_stock.at(item.identity).definition != item.identity)
            return invalid("Save item catalog does not cover the fixed dataset");
        else if (!same_item(s.items.at(item.identity), s.catalog.at({0, item.identity})))
            return invalid("Save item owner and catalog projection disagree");
    for (const auto &monster : rules.monsters)
        if (!ai.monster_growth.count(monster.identity) ||
            !ai.battle.monsters.count(monster.identity))
            return invalid("Save monster catalog does not cover the fixed dataset");
    if (s.task_progress.definitions.size() != rules.tasks.size())
        return invalid("Save task definition table does not match the fixed dataset");
    for (const auto &task : rules.tasks)
        if (!s.task_progress.definitions.count(task.factory.identity))
            return invalid("Save task definition table is incomplete");
        else {
            const auto &definition = s.task_progress.definitions.at(task.factory.identity);
            if (definition.kind != task.factory.kind || definition.flags != task.factory.flags ||
                definition.monster_definition != task.factory.monster ||
                !counter(definition.completed))
                return invalid("Save task definition identity or completed count is invalid");
        }
    if (!next_identity(s.tasks, s.next_task_identity) || !counter(s.task_sequence))
        return invalid("Save task allocator is invalid");
    for (const auto id : s.task_order)
        if (!s.tasks.count(id))
            return invalid("Save task order references a missing task");
    for (const auto &entry : s.tasks) {
        const auto &task = entry.second;
        if (entry.first != task.identity || !s.task_progress.definitions.count(task.definition) ||
            !s.task_original_ids.count(entry.first) ||
            (task.facility && !world.facilities.count(*task.facility) &&
             (!retired_facilities.count(*task.facility) ||
              std::find(s.task_order.begin(), s.task_order.end(), entry.first) !=
                  s.task_order.end() ||
              s.active_task == entry.first)) ||
            (task.site && !in_map(world.map, *task.site)))
            return invalid("Save task references an unknown definition, site or facility");
    }
    if ((s.active_task && !s.tasks.count(*s.active_task)) ||
        (s.task.encounter && !encounter(s, *s.task.encounter)))
        return invalid("Save active task or task encounter is missing");
    for (const auto definition : s.participants)
        if (!ai.growth.count(definition))
            return invalid("Save task participant definition is missing");
    for (const auto &entry : s.sites) {
        if (!world.facilities.count(entry.first) || entry.second.occupied_cells.empty())
            return invalid("Save dungeon site references a missing facility");
        for (const auto p : entry.second.occupied_cells)
            if (!in_map(world.map, p))
                return invalid("Save dungeon site is outside the map");
    }
    for (const auto id : s.shop_order)
        if (!s.shops.count(id) || !world.facilities.count(id))
            return invalid("Save shop order references a missing shop or facility");
    const auto &scripts = simulation::startup_world_runtime_catalog();
    for (const auto &event : s.scripts.event_calls)
        if (!scripts.events.count(event.first) || !counter(event.second))
            return invalid("Save script call registry contains an unknown event or invalid count");
    for (const auto &continuation : s.scripts.continuations) {
        const auto event = scripts.events.find(continuation.event);
        const auto program = scripts.programs.find(continuation.event);
        const auto *commands = event != scripts.events.end()       ? &event->second.commands
                               : program != scripts.programs.end() ? &program->second
                                                                   : nullptr;
        if (!commands || continuation.next_instruction > commands->size() ||
            !counter(continuation.remaining_updates))
            return invalid("Save delayed script program or next instruction is invalid");
    }
    reason.clear();
    return WorldSaveError::none;
}

WorldSaveError prepare_world_save_candidate(State &candidate, const State &current,
                                            std::string &reason) {
    const auto error = validate_world_save_candidate(candidate, reason);
    if (error != WorldSaveError::none)
        return error;
    auto restored = candidate;
    clear_presentation(restored);
    // Opcode40's category is immutable dataset information, not a new player wire field.
    // Rebuild it before the restored continuations can resume their facility program.
    for (const auto &definition : restored.rules->facilities)
        restored.scripts.facilities.at(definition.id).icon = definition.legacy_icon;
    restored.scene.random = current.scene.random;
    restored.scene.framework_paused = current.scene.framework_paused;
    // Loading keeps the current session's pacing; historical saves cannot enable player speed2.
    restored.scene.speed_setting = current.scene.speed_setting;
    restored.reference_viewport = current.reference_viewport;
    if (!rebuild_map(restored) || !simulation::update_startup_world_render_cache(restored)) {
        reason = "Save map or render cache could not be rebuilt";
        return WorldSaveError::invalid_world;
    }
    candidate = std::move(restored);
    reason.clear();
    return WorldSaveError::none;
}
} // namespace ark::app
