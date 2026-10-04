#include "dungeon_village_reference/encounter_creation.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
bool within(Position a, Position b, int distance) {
    return std::abs(static_cast<std::int64_t>(a.x) - b.x) <= distance &&
           std::abs(static_cast<std::int64_t>(a.y) - b.y) <= distance;
}
} // namespace
EncounterCreationResult prepare_encounter_creation(const AiRewardState &s,
                                                   const EncounterCreationInput &i) {
    auto fail = [](AiRewardError error) { return EncounterCreationResult{error, std::nullopt}; };
    if (i.kind < 0 || i.kind > 2 || i.center.x < 0 || i.center.y < 0 || i.center.x > 9998 ||
        i.center.y > 9998 || s.monster_limit < 0 || s.monster_progress < 0 ||
        s.monster_order.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return fail(AiRewardError::invalid_input);
    EncounterCreationCandidate c{s, {}, EncounterCreationDenial::none, false, false, false, 0,
                                 0, {}};
    const auto deny = [&](EncounterCreationDenial reason) {
        c.denial = reason;
        return EncounterCreationResult{AiRewardError::none, c};
    };
    if (i.probe) {
        if (i.kind != 0)
            return fail(AiRewardError::invalid_input);
        const auto actor = s.battle.actors.find(i.probe->actor);
        const auto context = s.contexts.find(i.probe->actor);
        if (actor == s.battle.actors.end() || context == s.contexts.end() ||
            !(actor->second.id == i.probe->actor) || !(context->second.cell == i.center))
            return fail(AiRewardError::stale_actor);
        const auto gate = prepare_spawn_probe(
            {actor->second.kind, actor->second.control.state, context->second.inside_town,
             i.probe->destination_inside_town, i.center.y, i.probe->minimum_y,
             static_cast<int>(s.monster_order.size()), s.monster_limit, i.probe->ticket});
        if (!gate.candidate)
            return fail(AiRewardError::preparation_failed);
        c.consumed_probe = gate.candidate->consumes_ticket;
        if (!gate.candidate->request_event_probe)
            return deny(EncounterCreationDenial::probe);
        // L success only admits f.b. k.a first checks the actual current cell and all bq centers.
        if (context->second.inside_town || i.probe->logical_state != 4)
            return deny(EncounterCreationDenial::cell);
        for (const auto center : i.probe->task_centers)
            if (within(i.center, center, 2))
                return deny(EncounterCreationDenial::task_overlap);
        for (const auto &[id, e] : s.encounters) {
            (void)id;
            if (within(i.center, e.runtime.center, 2))
                return deny(EncounterCreationDenial::event_overlap);
        }
    }
    if (std::any_of(i.upper_band_town.begin(), i.upper_band_town.end(), [](bool v) { return v; }))
        return deny(EncounterCreationDenial::town);
    if (i.kind == 0 && s.monster_order.size() >= static_cast<std::size_t>(s.monster_limit))
        return deny(EncounterCreationDenial::limit);
    if (i.kind == 0 || i.kind == 2)
        for (const auto &[id, e] : s.encounters) {
            (void)id;
            if (within(i.center, e.runtime.center, i.kind == 0 ? 4 : 2))
                return deny(EncounterCreationDenial::event_overlap);
        }
    if (s.next_encounter_id == 0 ||
        s.next_encounter_id == std::numeric_limits<std::uint64_t>::max() ||
        s.encounters.count(s.next_encounter_id) ||
        s.retired_encounters.count(s.next_encounter_id) || s.legacy_encounter_counter < 0 ||
        s.legacy_encounter_counter >= std::numeric_limits<int>::max())
        return fail(AiRewardError::invalid_input);
    int legacy = (s.legacy_encounter_counter + 1) % std::numeric_limits<int>::max();
    std::set<int> used;
    for (const auto &[id, e] : s.encounters) {
        if (e.runtime.id != id || e.legacy_id < 0)
            return fail(AiRewardError::stale_encounter);
        used.insert(e.legacy_id);
    }
    while (used.count(legacy)) {
        if (legacy == std::numeric_limits<int>::max())
            return fail(AiRewardError::invalid_input);
        ++legacy;
    }
    RewardEncounter event;
    event.legacy_id = legacy;
    event.runtime.id = s.next_encounter_id;
    event.runtime.center = i.center;
    c.state.encounters.emplace(s.next_encounter_id, event);
    c.state.encounter_order.push_back(s.next_encounter_id);
    c.state.legacy_encounter_counter = legacy;
    c.created = c.state.next_encounter_id++;
    if (i.kind == 0) {
        int nearby{};
        std::set<CharacterId> people;
        for (const auto id : s.human_order) {
            if (!people.insert(id).second || !s.contexts.count(id) || !s.battle.actors.count(id) ||
                s.battle.actors.at(id).kind != ActorKind::human)
                return fail(AiRewardError::stale_actor);
            if (within(s.contexts.at(id).cell, i.center, 1))
                ++nearby;
        }
        const auto count = prepare_normal_monster_count(
            {i.year_index, i.month_index, nearby, i.count_ticket, i.nearby_ticket});
        if (!count.candidate ||
            i.monsters.size() < static_cast<std::size_t>(count.candidate->count))
            return fail(AiRewardError::preparation_failed);
        c.consumed_count = true;
        c.consumed_nearby = count.candidate->consumes_nearby_ticket;
        if (s.monster_definition_order.size() != s.monster_growth.size())
            return fail(AiRewardError::invalid_input);
        for (int n = 0; n < count.candidate->count; ++n) {
            std::vector<MonsterChoiceDefinition> catalogue;
            std::set<int> definitions;
            for (const auto id : c.state.monster_definition_order) {
                const auto d = c.state.monster_growth.find(id);
                const auto b = c.state.battle.monsters.find(id);
                if (!definitions.insert(id).second || d == c.state.monster_growth.end() ||
                    b == c.state.battle.monsters.end() || d->second.status < 0 ||
                    d->second.status > 1)
                    return fail(AiRewardError::invalid_input);
                catalogue.push_back({id, b->second.flags, d->second.required_progress,
                                     d->second.status == 1, d->second.growth,
                                     d->second.introduced});
            }
            const auto &draw = i.monsters[static_cast<std::size_t>(n)];
            const auto choice = prepare_monster_choice(catalogue, c.state.monster_progress,
                                                       c.state.task_active, draw.definition_ticket);
            if (!choice.candidate)
                return fail(AiRewardError::preparation_failed);
            ++c.consumed_definitions; // Always draw catalogue length, even forced def13.
            if (choice.candidate->unlock_definition) {
                auto &d = c.state.monster_growth.at(*choice.candidate->unlock_definition);
                d.status = 1;
                d.newly_unlocked = true;
            }
            int selected = choice.candidate->selected;
            if (choice.candidate->request_introduction) {
                auto &d = c.state.monster_growth.at(selected);
                if (d.has_introduction_script)
                    c.requests.push_back(
                        {EncounterCreationRequestKind::definition_script, selected});
                c.requests.push_back({EncounterCreationRequestKind::page89, selected});
                d.introduced = true;
            }
            if (i.source_force_definition13)
                selected =
                    13; // Source override AFTER selection/intro, not an alternate draw branch.
            const auto spawn = prepare_encounter_monster_spawn(
                c.state, *c.created, {i.center, selected, false}, draw.offset_tickets);
            if (!spawn.candidate)
                return fail(spawn.error);
            c.state = spawn.candidate->state;
            c.consumed_offsets += 2;
        }
        c.state.encounters.at(*c.created).runtime.state = 0;
        c.requests.push_back({EncounterCreationRequestKind::refresh_map, 0});
    } else {
        c.state.encounters.at(*c.created).runtime.state = i.kind == 1 ? 2 : 3;
        if (i.kind == 2) {
            c.state.battle.quest_encounters.insert(*c.created);
            c.requests.push_back({EncounterCreationRequestKind::refresh_map, 0});
        }
    }
    return {AiRewardError::none, c};
}
} // namespace dungeon_village_reference
