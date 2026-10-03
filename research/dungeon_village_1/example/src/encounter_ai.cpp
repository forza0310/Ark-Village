#include "dungeon_village_reference/encounter_ai.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
int slot(std::size_t index, std::size_t count, bool monster) {
    const auto base =
        count <= 7 ? static_cast<int>(index * 3) + 10 : 10 + static_cast<int>(index * 22 / count);
    return base + (monster ? 40 : 0);
}
} // namespace
BattleGroupResult prepare_battle_group_step(const BattleGroupState &s,
                                            const std::vector<int> &tickets) {
    if (s.tick < 0 || s.tick >= std::numeric_limits<int>::max() || s.duration <= 0 || s.cycle < 0 ||
        s.cycle > 2 || s.alternating_side < 0 || s.alternating_side > 1)
        return {EncounterAiError::invalid_input, std::nullopt};
    for (const auto &roster : {s.humans, s.monsters})
        for (const auto &m : roster)
            if (m.id.value == 0)
                return {EncounterAiError::invalid_input, std::nullopt};
    BattleGroupStep c;
    c.state = s;
    ++c.state.tick;
    if (c.state.tick == s.duration / 2) {
        c.state.alternating_side = 1 - s.alternating_side;
        for (std::size_t n = 0; n < s.humans.size(); ++n)
            c.assignments.push_back(
                {s.humans[n].id, slot(n, s.humans.size(), false), std::nullopt});
    }
    if (c.state.tick >= s.duration) {
        c.state.tick = 0;
        c.state.cycle = (s.cycle + 1) % 3;
        c.state.duration = 80;
        for (auto *roster : {&c.state.humans, &c.state.monsters})
            roster->erase(std::remove_if(roster->begin(), roster->end(),
                                         [](const auto &m) { return !(m.flags & 128U); }),
                          roster->end());
        if (c.state.humans.empty() || c.state.monsters.empty()) {
            c.disband = true;
            for (auto *roster : {&c.state.humans, &c.state.monsters}) {
                for (const auto &m : *roster)
                    c.release_group_flag.push_back(m.id);
                roster->clear();
            }
        } else {
            if (c.state.monsters.size() > 1) {
                if (tickets.size() < c.state.monsters.size())
                    return {EncounterAiError::missing_ticket, std::nullopt};
                for (std::size_t n = 0; n < c.state.monsters.size(); ++n)
                    if (tickets[n] < 0 || tickets[n] >= 100)
                        return {EncounterAiError::invalid_ticket, std::nullopt};
                c.consumed_tickets = c.state.monsters.size();
            }
            for (std::size_t n = 0; n < c.state.monsters.size(); ++n) {
                int posture = 0;
                if (c.state.monsters.size() > 1) {
                    const bool boss = (c.state.monsters[n].flags & 16384U) != 0;
                    const int first = boss ? 50 : 60;
                    const int second = boss ? 67 : 80;
                    posture = tickets[n] < first ? 0 : tickets[n] < second ? 1 : 2;
                }
                c.assignments.push_back(
                    {c.state.monsters[n].id, slot(n, c.state.monsters.size(), true), posture});
            }
        }
    }
    return {EncounterAiError::none, c};
}
MonsterChoiceResult prepare_monster_choice(const std::vector<MonsterChoiceDefinition> &table,
                                           int progress, bool active_task,
                                           std::optional<int> ticket) {
    if (progress < 0)
        return {EncounterAiError::invalid_input, std::nullopt};
    std::set<int> ids;
    for (const auto &d : table)
        if (d.id < 0 || d.required_progress < 0 || d.growth_counter < 0 || !ids.insert(d.id).second)
            return {EncounterAiError::invalid_input, std::nullopt};
    MonsterChoiceCandidate c;
    for (std::size_t n = 1; n < table.size(); ++n) {
        const auto &current = table[n - 1];
        const auto &next = table[n];
        if (!(next.flags & 4U) && next.required_progress <= progress && current.unlocked &&
            !next.unlocked) {
            if (current.growth_counter >= 8)
                c.unlock_definition = next.id;
            break;
        }
    }
    for (auto p = table.rbegin(); p != table.rend() && c.eligible.size() < 5; ++p)
        if (!(p->flags & 4U) && (p->unlocked || c.unlock_definition == p->id))
            c.eligible.push_back(p->id);
    if (c.eligible.empty())
        return {EncounterAiError::no_candidates, std::nullopt};
    if (!ticket)
        return {EncounterAiError::missing_ticket, std::nullopt};
    if (*ticket < 0 || static_cast<std::size_t>(*ticket) >= c.eligible.size())
        return {EncounterAiError::invalid_ticket, std::nullopt};
    c.selected = c.eligible[static_cast<std::size_t>(*ticket)];
    const auto selected =
        std::find_if(table.begin(), table.end(), [&](const auto &d) { return d.id == c.selected; });
    c.request_introduction = !selected->introduced && !(selected->flags & 1U) && !active_task;
    return {EncounterAiError::none, c};
}
MonsterCountResult prepare_normal_monster_count(const MonsterCountInput &i) {
    if (i.year_index < 0 || i.month_index < 0 || i.month_index > 11 || i.nearby_people < 0)
        return {EncounterAiError::invalid_input, std::nullopt};
    if (!i.count_ticket || (i.nearby_people > 0 && !i.nearby_ticket))
        return {EncounterAiError::missing_ticket, std::nullopt};
    if (*i.count_ticket < 0 || *i.count_ticket >= 100 ||
        (i.nearby_people > 0 && (*i.nearby_ticket < 0 || *i.nearby_ticket >= i.nearby_people)))
        return {EncounterAiError::invalid_ticket, std::nullopt};
    const bool late = i.year_index >= 4;
    const int ticket = *i.count_ticket;
    int count = late          ? ticket < 50   ? 1
                                : ticket < 80 ? 2
                                : ticket < 95 ? 3
                                              : 4
                : ticket < 80 ? 1
                : ticket < 95 ? 2
                              : 3;
    if (i.nearby_people > 0)
        count += std::min(*i.nearby_ticket, 3);
    // The early-year override happens after both random branches, not before their consumption.
    if (i.year_index == 0 && i.month_index < 7)
        count = 1;
    return {EncounterAiError::none, MonsterCountCandidate{count, i.nearby_people > 0}};
}
} // namespace dungeon_village_reference
