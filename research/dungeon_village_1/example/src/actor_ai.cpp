#include "dungeon_village_reference/actor_ai.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace dungeon_village_reference {
BattleGateResult prepare_battle_gate(const BattleGateInput &input) {
    if ((input.kind != ActorKind::human && input.kind != ActorKind::monster) ||
        input.state_counter < 0)
        return {ActorAiError::invalid_input, std::nullopt};
    BattleGateCandidate result;
    if ((input.flags & (512U | 1024U)) != 0)
        return {ActorAiError::none, result};
    if (input.state_counter < 5)
        result.legacy_diagnostic_write = 1;
    else if (!input.in_move_area)
        result.legacy_diagnostic_write = 2;
    else if (input.kind == ActorKind::human && input.has_object)
        result.legacy_diagnostic_write = 3;
    else if (!input.has_enemy) {
        if (input.kind == ActorKind::human)
            result.legacy_diagnostic_write = 4;
    } else {
        result.allowed = input.inside_town == input.enemy_inside_town;
        if (input.kind == ActorKind::human)
            result.legacy_diagnostic_write = result.allowed ? 6 : 5;
    }
    return {ActorAiError::none, result};
}

EnemySelectionResult select_combat_enemy(const EnemySelectionInput &input) {
    if (!character_world_cell(input.position))
        return {ActorAiError::invalid_input, std::nullopt};
    std::map<CharacterId, EnemySnapshot> ids;
    std::vector<EnemySelectionCandidate> eligible;
    for (const auto &enemy : input.opposite_roster) {
        if (enemy.id.value == 0 || enemy.legacy_state < 0 || enemy.legacy_state > 20 ||
            !character_world_cell(enemy.position))
            return {ActorAiError::invalid_input, std::nullopt};
        const auto inserted = ids.emplace(enemy.id, enemy);
        if (!inserted.second) {
            const auto &previous = inserted.first->second;
            if (!input.active_battle_group || previous.legacy_state != enemy.legacy_state ||
                previous.in_move_area != enemy.in_move_area ||
                previous.position.x != enemy.position.x ||
                previous.position.z != enemy.position.z ||
                previous.encounter_id != enemy.encounter_id)
                return {ActorAiError::invalid_input, std::nullopt};
        }
        const int state = enemy.legacy_state;
        if (!enemy.in_move_area || state == 2 || state == 3 || state == 8 || state == 9 ||
            state == 14 || state == 15 || state == 16)
            continue;
        if (!input.active_battle_group &&
            (!input.encounter_id || enemy.encounter_id != input.encounter_id))
            continue;
        const float x = input.position.x - enemy.position.x;
        const float z = input.position.z - enemy.position.z;
        eligible.push_back({enemy.id, std::sqrt(x * x + z * z)});
    }
    if (eligible.empty())
        return {ActorAiError::none, std::nullopt};
    // Only the first reverse-scan exchange pass affects the returned head. Strict less-than means
    // an already-minimal head wins; otherwise the rightmost equal minimum wins, not a stable sort.
    auto best = eligible.front();
    for (std::size_t i = eligible.size(); i > 1; --i)
        if (eligible[i - 1].world_distance < best.world_distance)
            best = eligible[i - 1];
    return {ActorAiError::none, best};
}

HumanIdleResult prepare_human_idle(const HumanIdleInput &input) {
    if (input.outside_counter < 0 || input.capacity_hp <= 0)
        return {ActorAiError::invalid_input, std::nullopt};
    HumanIdleCandidate result;
    const auto hp = std::clamp<std::int64_t>(input.reported_hp, 0, input.capacity_hp);
    result.hp_percent = static_cast<int>(hp * 100 / input.capacity_hp);
    const int average = (result.hp_percent + std::clamp(input.effort, 0, 100)) / 2;
    result.reselect_threshold = 500 + average * 700 / 100;
    if ((input.flags & 16U) != 0)
        return {ActorAiError::none, result};
    if (input.active_task && input.definition_task_flag) {
        result.action = IdleAiAction::activity;
        result.activity = 1;
    } else if (input.battle_event_ready) {
        result.action = IdleAiAction::battle_prepare;
    } else if (input.carrying_character) {
        result.action = IdleAiAction::activity;
        result.activity = 4;
        result.clear_path = true;
    } else {
        if (input.nearby_event && input.outside_counter % 100 == 0) {
            result.action = IdleAiAction::activity;
            result.activity = 6;
        }
        if (result.hp_percent < 25 || input.outside_counter >= result.reselect_threshold) {
            result.action = IdleAiAction::activity;
            result.activity = 0;
        }
        result.run_spawn_probe = true;
    }
    return {ActorAiError::none, result};
}

MonsterIdleResult prepare_monster_idle(const MonsterIdleInput &input) {
    if (input.mode < 0 || input.mode > 4 || input.inside_counter < 0)
        return {ActorAiError::invalid_input, std::nullopt};
    MonsterIdleCandidate result;
    if (input.mode == 1 || input.mode == 4) {
        result.advance_path = true;
        if (input.mode == 1 && !input.path_returned_true && input.inside_counter >= 500)
            result.action = MonsterIdleAction::cleanup;
    } else if (input.battle_gate) {
        result.action = MonsterIdleAction::enter_battle;
    }
    return {ActorAiError::none, result};
}
} // namespace dungeon_village_reference
