#include "dungeon_village_reference/world_encounters.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
bool town_boundary_or_inside(Position p, const TownBounds &b) {
    return p.x >= b.left && p.x <= b.right && p.y >= b.top && p.y <= b.bottom;
}
} // namespace
WorldEventMapResult prepare_world_event_map(const AiRewardState &s, const WorldMapFacts &f) {
    if (!valid_world_map_facts(f) || s.encounter_order.size() != s.encounters.size())
        return {AiRewardError::invalid_input, {}};
    auto next = f;
    for (auto &flags : next.flags)
        flags &= ~2U;
    std::set<std::uint64_t> seen;
    for (const auto key : s.encounter_order) {
        const auto found = s.encounters.find(key);
        if (!seen.insert(key).second || found == s.encounters.end() ||
            found->second.runtime.id != key)
            return {AiRewardError::stale_encounter, {}};
        const auto &e = found->second.runtime;
        if (e.state != 0 && e.state != 3)
            continue;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const auto x = static_cast<std::int64_t>(e.center.x) + dx;
                const auto y = static_cast<std::int64_t>(e.center.y) + dy;
                if (x < 0 || x >= f.map.width || y < 0 || y >= f.map.height ||
                    town_boundary_or_inside({static_cast<int>(x), static_cast<int>(y)}, f.town))
                    continue;
                next.flags[static_cast<std::size_t>(y * f.map.width + x)] |= 2U;
            }
    }
    return {AiRewardError::none, next};
}
std::optional<int> prepare_task_encounter_quota(int base, int completions, std::uint32_t flags) {
    if (base < 0 || completions < 0)
        return {};
    const auto result =
        static_cast<std::int64_t>(base) +
        ((flags & 4U) ? std::min<std::int64_t>(static_cast<std::int64_t>(completions) * 2, 10) : 0);
    return result <= std::numeric_limits<int>::max() ? std::optional<int>(static_cast<int>(result))
                                                     : std::nullopt;
}
WorldEventEntryResult prepare_world_event_entry(const AiRewardState &s, const WorldMapFacts &f,
                                                const WorldEventEntryInput &i) {
    const auto fail = [](AiRewardError error) -> WorldEventEntryResult { return {error, {}}; };
    const auto gate = prepare_world_event_gate(s, i.actor, f, i.task);
    if (!gate.candidate)
        return fail(gate.error);
    WorldEventEntryCandidate c{
        gate.candidate->state,         f,     i.task, gate.candidate->gate, {},
        EncounterCreationDenial::none, false, false};
    if (!c.gate.request_task_encounter)
        return {AiRewardError::none, c};
    if (i.task.center.x < 0 || i.task.center.y < 0 || i.task.center.x > 9998 ||
        i.task.center.y > 9998)
        return fail(AiRewardError::invalid_input);
    EncounterCreationInput create;
    create.kind = 2;
    create.draw = i.draw;
    create.center = i.task.center;
    for (int offset = -1; offset <= 1; ++offset)
        create.upper_band_town[static_cast<std::size_t>(offset + 1)] =
            town_boundary_or_inside({create.center.x + offset, create.center.y - 1}, f.town);
    const auto created = prepare_encounter_creation(c.state, create);
    if (!created.candidate)
        return fail(created.error);
    c.state = created.candidate->state;
    c.denial = created.candidate->denial;
    c.created = created.candidate->created;
    if (c.created) {
        const auto quota =
            prepare_task_encounter_quota(i.base_quota, i.task_completions, i.task_flags);
        if (!quota)
            return fail(AiRewardError::invalid_input);
        c.state.encounters.at(*c.created).runtime.quota = *quota;
        c.task.encounter = c.created;
        const auto map = prepare_world_event_map(c.state, f);
        if (!map.facts)
            return fail(map.error);
        c.facts = *map.facts;
        c.music2 = c.notice24 = true;
    }
    return {AiRewardError::none, c};
}
WorldEncounterUpdateResult prepare_world_encounter_update(const AiRewardState &s,
                                                          const WorldMapFacts &f,
                                                          EncounterCommitInput i,
                                                          const CombatInfluenceCandidate &field) {
    const auto fail = [](AiRewardError error) -> WorldEncounterUpdateResult { return {error, {}}; };
    if (!valid_world_map_facts(f) || !valid_combat_influence_field(field) ||
        field.width != f.map.width * 2 || field.height != f.map.height * 2)
        return fail(AiRewardError::invalid_input);
    const auto event = s.encounters.find(i.encounter);
    if (event == s.encounters.end())
        return fail(AiRewardError::stale_encounter);
    const auto center = event->second.runtime.center;
    i.town_overlap = false;
    for (int offset = -1; offset <= 1; ++offset) {
        const auto x = static_cast<std::int64_t>(center.x) + offset;
        const auto y = static_cast<std::int64_t>(center.y) - 1;
        i.town_overlap |=
            x >= f.town.left && x <= f.town.right && y >= f.town.top && y <= f.town.bottom;
    }
    i.cells.clear();
    constexpr Position offsets[]{{-1, 1}, {0, 1},   {1, 1},  {-1, 0},
                                 {1, 0},  {-1, -1}, {0, -1}, {1, -1}};
    for (const auto offset : offsets) {
        const auto x = static_cast<std::int64_t>(center.x) + offset.x;
        const auto y = static_cast<std::int64_t>(center.y) + offset.y;
        if (x < 0 || x >= f.map.width || y < 0 || y >= f.map.height)
            continue;
        const auto cell = Position{static_cast<int>(x), static_cast<int>(y)};
        if (f.map.cells[static_cast<std::size_t>(y * f.map.width + x)].legacy_state == 4)
            i.cells.push_back({cell, 4, inside_town(cell, f.town)});
    }
    i.snapshot_field = field;
    const auto updated = prepare_encounter_reward_commit(s, i);
    if (!updated.candidate)
        return fail(updated.error);
    WorldEncounterUpdateCandidate c{updated.candidate->state, f,
                                    updated.candidate->encounter_requests,
                                    updated.candidate->removed};
    if (std::any_of(c.requests.begin(), c.requests.end(), [](const EncounterRequest &r) {
            return r.kind == EncounterRequestKind::refresh_map;
        })) {
        // Cancellation/victory has already set k1; no source refresh request removes bn.
        const auto map = prepare_world_event_map(c.state, f);
        if (!map.facts)
            return fail(map.error);
        c.facts = *map.facts;
    }
    return {AiRewardError::none, c};
}
} // namespace dungeon_village_reference
