#include "dungeon_village_reference/encounter_lifecycle.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <set>

namespace dungeon_village_reference {
namespace {
bool fits(std::int64_t value) { return value >= 0 && value <= std::numeric_limits<int>::max(); }
bool nearby(Position a, Position b) {
    return std::abs(static_cast<std::int64_t>(a.x) - b.x) <= 1 &&
           std::abs(static_cast<std::int64_t>(a.y) - b.y) <= 1;
}
} // namespace
EncounterStepResult prepare_encounter_step(const EncounterStepInput &i) {
    const auto &s = i.state;
    if (s.state < 0 || s.state > 3 || s.counter < 0 ||
        s.counter == std::numeric_limits<int>::max() || s.idle < 0 ||
        s.idle == std::numeric_limits<int>::max() || s.spawned < 0 || s.quota < 0 || s.reward < 0 ||
        i.humans.size() > 1000000 || i.monsters.size() > 1000000)
        return {EncounterAiError::invalid_input, std::nullopt};
    std::set<CharacterId> ids;
    std::map<int, std::pair<int, int>> definition_rewards;
    for (const auto &h : i.humans) {
        if (h.definition < 0 || h.state < 0 || h.state > 20 || h.attack_count < 0 ||
            h.recent_reward < 0 || h.recent_kills < 0 || !ids.insert(h.id).second)
            return {EncounterAiError::invalid_input, std::nullopt};
        const auto inserted = definition_rewards.emplace(
            h.definition, std::make_pair(h.recent_reward, h.recent_kills));
        if (!inserted.second &&
            inserted.first->second != std::make_pair(h.recent_reward, h.recent_kills))
            return {EncounterAiError::invalid_input, std::nullopt};
    }
    EncounterStepCandidate c;
    c.state = s;
    ++c.state.counter;
    c.humans = i.humans;
    auto req = [&](EncounterRequestKind kind, int parameter = 0, int value = 0, int delay = 0,
                   std::optional<CharacterId> actor = std::nullopt) {
        c.requests.push_back({kind, actor, parameter, value, delay});
    };
    EncounterAiError ticket_error = EncounterAiError::none;
    auto draw = [&](std::size_t bound) -> std::optional<int> {
        if (bound == 0 || bound > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            ticket_error = EncounterAiError::no_candidates;
            return std::nullopt;
        }
        if (c.consumed_tickets >= i.tickets.size()) {
            ticket_error = EncounterAiError::missing_ticket;
            return std::nullopt;
        }
        const auto &t = i.tickets[c.consumed_tickets++];
        if (t.bound != static_cast<int>(bound) || t.value < 0 || t.value >= t.bound) {
            ticket_error = EncounterAiError::invalid_ticket;
            return std::nullopt;
        }
        return t.value;
    };
    auto retire = [&] {
        c.state.state = 1;
        c.state.counter = 0;
    };
    auto cancel = [&] {
        c.cancelled = true;
        for (const auto &m : i.monsters)
            if (m.encounter == s.id)
                req(EncounterRequestKind::cancel_monster, 3, 1, 0, m.id);
        retire();
        req(EncounterRequestKind::refresh_map);
    };
    if (s.state == 1) {
        c.remove = c.state.counter >= 100;
        return {EncounterAiError::none, c};
    }
    if (s.state == 2)
        return {EncounterAiError::none, c};
    if (s.state == 0 && i.town_overlap) {
        cancel();
        return {EncounterAiError::none, c};
    }
    if (s.state == 3 && !i.task_exists) {
        req(EncounterRequestKind::clear_task);
        cancel();
        return {EncounterAiError::none, c};
    }
    if (i.group_exists)
        req(EncounterRequestKind::update_group);
    for (const auto &h : i.humans)
        if (nearby(h.cell, s.center))
            ++c.nearby_humans; // Includes town/down/rescued, unlike victory list.
    for (const auto &m : i.monsters)
        if (m.encounter == s.id)
            ++c.linked_monsters;
    if (s.state == 0 && c.linked_monsters < 5 && s.spawned < 5)
        if (!draw(1000))
            return {ticket_error,
                    std::nullopt}; // Fixed source empty generation branch still draws.
    if (s.state == 3 && s.spawned < s.quota && c.linked_monsters < c.nearby_humans &&
        !i.quest_cells.empty()) {
        for (const auto &cell : i.quest_cells)
            if (cell.state != 4 || !nearby(cell.cell, s.center) || cell.cell == s.center)
                return {EncounterAiError::invalid_input, std::nullopt};
        const auto cell_ticket = draw(i.quest_cells.size());
        if (!cell_ticket)
            return {ticket_error, std::nullopt};
        const auto &cell = i.quest_cells[static_cast<std::size_t>(*cell_ticket)];
        if (!cell.inside_town) {
            const bool last = s.spawned == s.quota - 1;
            int definition = i.quest.boss_definition;
            if (!last) {
                const auto def_ticket = draw(i.quest.normal_definitions.size());
                if (!def_ticket)
                    return {ticket_error, std::nullopt};
                definition = i.quest.normal_definitions[static_cast<std::size_t>(*def_ticket)];
            }
            if (definition < 0 || s.spawned == std::numeric_limits<int>::max())
                return {EncounterAiError::invalid_input, std::nullopt};
            c.spawn = EncounterSpawnCandidate{cell.cell, definition, last && (i.quest.flags & 2U)};
            ++c.state.spawned;
            ++c.linked_monsters;
        }
    }
    req(EncounterRequestKind::snapshot_influence); // p=q=human field, r=s=monster field.
    if (s.state == 0 && (c.state.spawned == 0 || c.linked_monsters != 0)) {
        ++c.state.idle;
        if (c.state.idle >= 600)
            cancel();
        return {EncounterAiError::none, c};
    }
    if (s.state == 3 &&
        (c.state.spawned == 0 || c.state.spawned < s.quota || c.linked_monsters != 0))
        return {EncounterAiError::none, c};
    c.victory = true;
    retire();
    if (s.state == 3 && c.state.reward <= 0 && (i.quest.flags & 2U)) {
        if (i.quest.boss_reward < 0)
            return {EncounterAiError::invalid_input, std::nullopt};
        c.state.reward = i.quest.boss_reward;
    }
    const int share = static_cast<int>(static_cast<std::int64_t>(c.state.reward) * 50 / 100);
    auto reward_actor = [&](VictoryHuman &h, int ordinal) -> bool {
        auto &recent = definition_rewards.at(h.definition);
        bool expression_gate = true;
        int delay = 44;
        int expression_delay{};
        if (s.state == 0) {
            if (h.attack_count >= 2) {
                h.state = 10;
                req(EncounterRequestKind::state10, 10, 0, 0, h.id);
            }
            h.attack_count = 0;
            req(EncounterRequestKind::reset_attack_count, 0, 0, 0, h.id);
            delay = 50 + ordinal * 4;
            expression_delay = 5 + ordinal * 4;
        } else {
            expression_gate = h.state != 2 && h.state != 16 && h.state != 0;
            if (expression_gate) {
                req(EncounterRequestKind::reset_hp, 0, 0, 0, h.id);
                h.state = 10;
                req(EncounterRequestKind::state10, 10, 0, 0, h.id);
            }
        }
        if (expression_gate) {
            const auto ticket = draw(100);
            if (!ticket)
                return false;
            const int threshold = recent.second == 0   ? 15
                                  : recent.second == 1 ? (s.state == 0 ? 40 : 30)
                                                       : 60;
            if (*ticket < threshold)
                req(s.state == 0 ? EncounterRequestKind::expression5
                                 : EncounterRequestKind::task_victory_expression,
                    5, 0, expression_delay, h.id);
        }
        const auto amount = static_cast<std::int64_t>(recent.first) + share;
        if (!fits(amount)) {
            ticket_error = EncounterAiError::invalid_input;
            return false;
        }
        recent = {0, 0};
        for (auto &same_definition : c.humans)
            if (same_definition.definition == h.definition) {
                same_definition.recent_reward = 0;
                same_definition.recent_kills = 0;
            }
        req(EncounterRequestKind::reset_recent_reward_and_kills, 0, 0, 0, h.id);
        req(EncounterRequestKind::reward_display, 24, static_cast<int>(amount), delay - 28, h.id);
        req(EncounterRequestKind::reward_accumulation, 0, static_cast<int>(amount), delay, h.id);
        if (s.state == 3) {
            h.flags &= ~2048U;
            req(EncounterRequestKind::clear_boost2048, 2048, 0, 0, h.id);
        }
        return true;
    };
    if (s.state == 0) {
        int ordinal{};
        for (auto &h : c.humans)
            if (nearby(h.cell, s.center) && !h.inside_town && h.state != 2)
                if (!reward_actor(h, ordinal++))
                    return {ticket_error, std::nullopt};
        if (!i.event91_present)
            req(EncounterRequestKind::event, 91);
        req(EncounterRequestKind::refresh_map);
        return {EncounterAiError::none, c};
    }
    for (int def : i.quest.participants) {
        if (def < 0)
            return {EncounterAiError::invalid_input, std::nullopt};
        for (auto &h : c.humans)
            if (h.definition == def) {
                if (!reward_actor(h, 0))
                    return {ticket_error, std::nullopt};
                break;
            }
    }
    req(EncounterRequestKind::refresh_map);
    req(EncounterRequestKind::page30, 30, share);
    const auto first = draw(i.quest.participants.size());
    if (!first)
        return {ticket_error, std::nullopt};
    const auto second = draw(i.quest.participants.size());
    if (!second)
        return {ticket_error, std::nullopt};
    req(EncounterRequestKind::page31, i.quest.participants[static_cast<std::size_t>(*first)],
        i.quest.participants[static_cast<std::size_t>(*second)]);
    if (i.quest.flags & 2U) {
        if (!fits(static_cast<std::int64_t>(i.quest.event_offset) + 145))
            return {EncounterAiError::invalid_input, std::nullopt};
        req(EncounterRequestKind::event, i.quest.event_offset + 145);
        req(EncounterRequestKind::event, 152, i.quest.boss_definition);
    }
    req(EncounterRequestKind::event, 126);
    req(EncounterRequestKind::completion_delta, i.quest.completion_delta);
    req(EncounterRequestKind::mark_task_complete);
    req(EncounterRequestKind::clear_task);
    req(EncounterRequestKind::refresh_global);
    if (!i.event128_present) {
        req(EncounterRequestKind::refresh_task_catalog);
        req(EncounterRequestKind::event, 128);
    } else if (!i.feature16) {
        req(EncounterRequestKind::set_feature16, 16);
    }
    if (!i.event205_present)
        req(EncounterRequestKind::event, 205);
    return {EncounterAiError::none, c};
}
std::optional<DelayedRewardState> prepare_delayed_reward(const DelayedRewardState &s, int amount,
                                                         int delay) {
    if (s.amount < 0 || s.counter == std::numeric_limits<int>::min() || amount < 0 || delay < 0)
        return std::nullopt;
    std::int64_t remaining = s.amount;
    if (s.counter >= 0 && s.counter < 9)
        remaining -= static_cast<std::int64_t>(s.counter) * s.amount / 9;
    remaining += amount;
    if (!fits(remaining))
        return std::nullopt;
    return DelayedRewardState{static_cast<int>(remaining), -delay};
}
std::optional<DelayedRewardStep> advance_delayed_reward(const DelayedRewardState &s) {
    if (s.amount < 0 || s.counter == std::numeric_limits<int>::max())
        return std::nullopt;
    DelayedRewardStep c{s, 0};
    if (s.amount == 0)
        return c; // Original a(character) returns BEFORE O increment when N==0.
    ++c.state.counter;
    if (c.state.counter >= 0) {
        const auto amount = static_cast<std::int64_t>(s.amount);
        const int now = std::clamp(c.state.counter, 0, 9);
        const int previous = std::clamp(c.state.counter - 1, 0, 9);
        c.increment = static_cast<int>(now * amount / 9 - previous * amount / 9);
    }
    // Level/job/HP/upgrade side effects between increments and N clearing are owner requests,
    // so preserve N here rather than guessing when all current definition changes have finished.
    return c;
}
std::optional<DelayedRewardState> complete_delayed_reward_step(const DelayedRewardStep &s,
                                                               bool increased) {
    if (s.state.amount < 0 || s.increment < 0)
        return std::nullopt;
    if (increased || s.state.counter >= 9)
        return DelayedRewardState{};
    return s.state;
}
} // namespace dungeon_village_reference
