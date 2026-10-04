#include "ark/simulation/rules/combat_commit.hpp"
#include "ark/simulation/rules/world_perception.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
AiRewardState fixture() {
    AiRewardState s;
    BattleActorRecord h;
    h.id = {1};
    h.capacity = 100;
    h.hp = {0, 50, 50, 50, false, 0};
    h.position = {250, 0, 250};
    h.encounter = 0;
    h.control.state = 1;
    h.baseline = 5;
    h.attack_cooldown = 3;
    auto m = h;
    m.id = {2};
    m.kind = ActorKind::monster;
    m.position = {260, 0, 250};
    m.baseline = 17;
    s.battle.actors = {{h.id, h}, {m.id, m}};
    s.human_order = {{1}};
    s.monster_order = {{2}};
    s.contexts.emplace(h.id, RewardActorContext{{0, 0}, false, {}, {}, true, {5, 5}});
    s.contexts.emplace(m.id, RewardActorContext{{2, 2}, false, {}, {}, true, {5, 5}});
    s.encounters.emplace(0, RewardEncounter{{0, {2, 2}}, {}, true, {}});
    s.encounter_order = {0};
    return s;
}
WorldMapFacts facts() {
    return {{5, 5, std::vector<LegacyMapCell>(25)},
            std::vector<int>(25, 1),
            std::vector<std::uint32_t>(25),
            {0, 4, 0, 4}};
}
void prefix_and_cache() {
    auto s = fixture();
    const auto f = facts();
    auto r = prepare_world_perception_prefix(s, {1}, f);
    check(r.candidate && r.candidate->area.allowed && r.candidate->enemy &&
              r.candidate->enemy->id == CharacterId{2} && r.candidate->sensed_distance == 10 &&
              !r.candidate->state.contexts.at({1}).inside_town &&
              !r.candidate->state.contexts.at({1}).low_hp &&
              r.candidate->state.battle.actors.at({1}).attack_cooldown == 2 &&
              r.candidate->state.battle.actors.at({1}).decision_start.x == 250,
          "c uses cached s for town, actual n for K/e, strict halfHP and cooldown-before-sense");
    s.contexts.at({2}).move_area = false;
    r = prepare_world_perception_prefix(s, {1}, f);
    check(r.candidate && !r.candidate->enemy &&
              r.candidate->sensed_distance == std::numeric_limits<float>::max(),
          "opponent old aB0 excluded even though its current position would pass K");
    const auto monster = prepare_world_perception_prefix(s, {2}, f);
    check(monster.candidate && monster.candidate->state.contexts.at({2}).move_area,
          "opponent own c refreshes K independently later");
    s = monster.candidate->state;
    check(prepare_world_perception_prefix(s, {1}, f).candidate->enemy.has_value(),
          "later e observes already updated current opponent cache");
    s = fixture();
    s.battle.actors.at({1}).blocked_battle_steps = 150;
    s.battle.actors.at({1}).hp.target = 49;
    r = prepare_world_perception_prefix(s, {1}, f);
    check(r.candidate && r.candidate->restored_baseline &&
              r.candidate->state.battle.actors.at({1}).control.state == 5 &&
              !r.candidate->state.battle.actors.at({1}).encounter && !r.candidate->enemy &&
              r.candidate->state.contexts.at({1}).low_hp,
          "K before baseline150, enemy lookup AFTER human baseline clears db");
    s = fixture();
    s.battle.actors.at({1}).control.flags |= 128U;
    s.battle.actors.at({1}).group = 0;
    s.encounters.at(0).group.monsters = {{{2}, 128U}, {{2}, 128U}};
    s.battle.actors.at({2}).control.state = 3;
    s.retired_actors.emplace(CharacterId{2}, s.battle.actors.at({2}));
    s.battle.actors.erase({2});
    s.monster_order.clear();
    r = prepare_world_perception_prefix(s, {1}, f);
    check(r.candidate && !r.candidate->enemy &&
              !(r.candidate->state.battle.actors.at({1}).control.flags & 128U),
          "retired group corpse retains cached context, filtered state3 clears caller128 without "
          "errors");
}
void geometry() {
    auto s = fixture();
    auto f = facts();
    for (int state : {0, 1, 4, 5, 14, 17})
        for (float x :
             {-101.0F, -50.0F, -1.0F, 0.0F, 99.0F, 100.0F, 199.0F, 200.0F, 499.0F, 500.0F}) {
            auto &a = s.battle.actors.at({1});
            a.control.state = state;
            a.position = {x, 0, 250};
            const auto r = query_world_move_area(s, {1}, f);
            // Independent two-stage truncation; do not use the tested coordinate helper.
            bool bounds = true, square = true;
            for (const auto offset :
                 {Position{0, 1}, Position{1, 0}, Position{0, -1}, Position{-1, 0}}) {
                const int cellx = static_cast<int>((x + offset.x) / 50.0F) / 2;
                const int celly = static_cast<int>((250.0F + offset.y) / 50.0F) / 2;
                bounds &= cellx >= 0 && cellx < 5 && celly >= 0 && celly < 5;
                square &= cellx >= 1 && cellx <= 3 && celly >= 1 && celly <= 3;
            }
            const bool constrained = state == 1 || state == 4;
            check(r && r->allowed == (bounds && (!constrained || square)),
                  "world K original unit probes/two truncations/event square guards");
        }
    s = fixture();
    f.surface[12] = 3;
    auto r = query_world_move_area(s, {1}, f);
    check(r && !r->allowed && r->legacy_diagnostic == 2,
          "surface3 rejection independent from logical ground/path eligibility");
    f.surface[12] = 1;
    f.map.cells[12].legacy_state = 0;
    check(!query_world_move_area(s, {1}, f)->allowed, "logical0 independently blocks area");
    s = fixture();
    s.battle.actors.at({1}).position = {-99, 0, -99};
    const auto p = prepare_world_actor_projection(s, {1});
    check(p.candidate && p.candidate->state.contexts.at({1}).cell == Position{0, 0} &&
              p.candidate->state.contexts.at({1}).half_cell == Position{-1, -1},
          "d whole and half projections each truncate toward zero, not floor or whole*2");
}
void healing_and_influence() {
    auto s = fixture();
    auto f = facts();
    s.battle.actors.at({1}).hp.target = 1;
    check(!query_world_healing_target(s, {1}).target, "J reads old ak false despite now-low HP");
    s.contexts.at({1}).low_hp = true;
    s.battle.actors.at({1}).hp.target = 100;
    check(query_world_healing_target(s, {1}).target == CharacterId{1},
          "J includes self with old ak true despite now-full targetHP");
    const auto c = prepare_world_perception_prefix(s, {1}, f);
    check(c.candidate && !query_world_healing_target(c.candidate->state, {1}).target,
          "only own c refreshes ak for subsequent J");
    const auto field = prepare_world_influence(s, f);
    check(field.candidate && field.candidate->width == 10 && field.candidate->height == 10,
          "global influence prepared from old cached movement/half cells");
    auto moved = s;
    moved.battle.actors.at({2}).position = {450, 0, 450};
    const auto before_projection = prepare_world_influence(moved, f);
    check(before_projection.candidate &&
              before_projection.candidate->human_field == field.candidate->human_field,
          "world influence not eagerly projected from changed n");
    const auto projected = prepare_world_actor_projection(moved, {2});
    const auto after_projection = prepare_world_influence(projected.candidate->state, f);
    check(after_projection.candidate &&
              after_projection.candidate->human_field != field.candidate->human_field,
          "explicit d projection changes next h.e cached field placement");
    s.monster_order.push_back({2});
    check(!prepare_world_influence(s, f).candidate,
          "duplicate running roster invalid, unlike permitted battle-group repeated refs");
    s.human_order.push_back({1});
    check(query_world_healing_target(s, {1}).error == AiRewardError::invalid_input,
          "invalid current healing roster is an error, not ordinary no-target success");
}
void event_and_physics() {
    auto s = fixture();
    auto f = facts();
    s.contexts.at({1}).cell = {2, 2};
    f.flags[12] = 2;
    s.battle.actors.at({1}).state_counter = 5;
    s.battle.actors.at({1}).control.flags = 1024U;
    auto gate = prepare_world_event_gate(s, {1}, f);
    check(gate.candidate && gate.candidate->gate.ready && gate.candidate->gate.bind_encounter == 0,
          "world F accepts1024 and binds current original-order event, unlike G");
    s.encounters.at(0).runtime.center = {4, 4};
    gate = prepare_world_event_gate(s, {1}, f);
    check(gate.candidate && gate.candidate->gate.ready && !gate.candidate->gate.bind_encounter &&
              gate.candidate->state.battle.actors.at({1}).encounter == 0,
          "event flagged cell with no center match retains old db and still F true");
    s.task_active = true;
    f.flags[12] = 0;
    gate = prepare_world_event_gate(s, {1}, f, {true, 1, {2, 2}, {}});
    check(gate.candidate && gate.candidate->gate.ready &&
              gate.candidate->gate.request_task_encounter &&
              gate.candidate->state.encounters.size() == 1,
          "new task creation request returns F true without eager spawn/bind/quota");
    s.battle.actors.at({1}).object_slot = -2;
    gate = prepare_world_event_gate(s, {1}, f, {true, 1, {2, 2}, {}});
    check(gate.candidate && !gate.candidate->gate.ready,
          "rescue sentinel still blocks human F before task creation");
    s = fixture();
    f = facts();
    auto &a = s.battle.actors.at({1});
    a.position = {450, 10, 250};
    a.decision_start = {250, 0, 250};
    a.vertical_velocity = 0;
    const auto physics = prepare_world_physics_projection(s, {1}, f);
    check(physics.candidate && physics.candidate->queried_area &&
              physics.candidate->diagnostic == 7 &&
              physics.candidate->state.battle.actors.at({1}).position.x == 250 &&
              physics.candidate->state.battle.actors.at({1}).position.height > 0 &&
              physics.candidate->state.battle.actors.at({1}).blocked_battle_steps == 1 &&
              physics.candidate->state.contexts.at({1}).cell == Position{2, 2} &&
              !physics.candidate->state.contexts.at({1}).inside_town,
          "freshK rejects new battle position, restores only horizontal bu and projects without ax "
          "refresh");
    a.physics_pause = 1;
    a.encounter = 999;
    const auto paused = prepare_world_physics_projection(s, {1}, f);
    check(paused.candidate && !paused.candidate->queried_area &&
              paused.candidate->state.battle.actors.at({1}).physics_pause == 0 &&
              paused.candidate->state.battle.actors.at({1}).position.x == 450 &&
              paused.candidate->state.battle.actors.at({1}).position.height == 10,
          "old P1 skips K even with missing db, still projects changed n");
    s = fixture();
    s.battle.actors.at({1}).control.state = 16;
    s.battle.actors.at({1}).position.height = 16;
    const auto carried = prepare_world_physics_projection(s, {1}, f);
    check(carried.candidate && !carried.candidate->queried_area &&
              carried.candidate->state.battle.actors.at({1}).position.height == 16,
          "state16 keeps follow height, no gravity/K but same d projection");
}
void combat_move() {
    auto s = fixture();
    auto f = facts();
    s.battle.actors.at({1}).position = {275, 0, 275};
    s.battle.actors.at({2}).position = {375, 0, 275};
    const auto r = prepare_world_combat_move(s, {1}, {2}, f, false);
    check(r.candidate && r.candidate->target && r.candidate->target->x == 325 &&
              r.candidate->target->z == 275 &&
              r.candidate->state.battle.actors.at({1}).position.x > 275 &&
              r.candidate->state.contexts.at({1}).half_cell == Position{5, 5},
          "nine samples approach chooses half-center then6.7 move without cached t reprojection");
    check(!s.encounters.at(0).influence &&
              r.candidate->state.encounters.at(0).human_scratch.size() == 100,
          "new event zero field/scratch constructed only on candidate, original untouched");
    auto field = *prepare_world_influence(s, f).candidate;
    field.human_field.assign(100, 100);
    field.monster_field.assign(100, 0);
    field.human_field[5 * 10 + 6] = 0;
    const auto copied = prepare_world_encounter_influence(s, 0, field);
    check(copied.candidate &&
              copied.candidate->state.encounters.at(0).human_scratch == field.human_field,
          "event snapshot copies p/q and r/s only at explicit event update point");
    s = copied.candidate->state;
    s.encounters.at(0).human_scratch.assign(100, 999);
    const auto low = prepare_world_combat_move(s, {1}, {2}, f, true);
    check(low.candidate && low.candidate->target && low.candidate->target->x == 325 &&
              low.candidate->state.encounters.at(0).human_scratch == field.human_field,
          "low influence copies p into q before evaluation, not prior actor scratch values");
    s.battle.actors.at({1}).control.flags |= 64U;
    const auto stopped = prepare_world_combat_move(s, {1}, {2}, f, true);
    check(stopped.candidate && stopped.candidate->target &&
              stopped.candidate->state.battle.actors.at({1}).position.x == 275,
          "scoring still selects target when64 prevents actual displacement");
    s = fixture();
    s.contexts.at({1}).half_cell = {0, 0};
    s.battle.actors.at({1}).position = {25, 0, 25};
    s.battle.actors.at({1}).decision_start = s.battle.actors.at({1}).position;
    const auto invalid = prepare_world_combat_move(s, {1}, {2}, f, true);
    check(invalid.candidate && invalid.candidate->scores.selected == 0 &&
              invalid.candidate->target->x == -25,
          "invalid low-mode score0 can select out-of-grid first sample, source behavior retained");
    const auto revert = prepare_world_physics_projection(invalid.candidate->state, {1}, f);
    check(revert.candidate && revert.candidate->diagnostic == 7 &&
              revert.candidate->state.battle.actors.at({1}).position.x == 25,
          "d freshK can reverse c score movement, rather than nav pre-filtering it");
    s = fixture();
    s.battle.actors.at({1}).encounter.reset();
    const auto no_event = prepare_world_combat_move(s, {1}, {2}, f, false);
    check(no_event.candidate && no_event.candidate->diagnostic == 4 && !no_event.candidate->target,
          "no db returns original no-move diagnostic, not invented approach route");
}
void repair_and_preemption() {
    auto s = fixture();
    auto f = facts();
    auto &a = s.battle.actors.at({1});
    a.control.state = 0;
    a.control.action = 7;
    a.control.action_counter = 80;
    a.control.alternate_counter = 29;
    a.rescue = CharacterId{2};
    a.object_slot = -2;
    const auto repair = prepare_world_reference_preemption(s, {1}, f, false, false, {});
    check(repair.candidate && !repair.candidate->state.battle.actors.at({1}).rescue &&
              repair.candidate->state.battle.actors.at({1}).object_slot == -1 &&
              repair.candidate->state.battle.actors.at({1}).hp.target == 100 &&
              repair.candidate->state.battle.actors.at({1}).control.action == 0 &&
              repair.candidate->state.battle.actors.at({1}).control.alternate_counter == 29,
          "R.R null repair resets three HP slots/k/l but preserves i and previously sensed ak");
    check(!repair.candidate->state.contexts.at({1}).low_hp && a.rescue.has_value(),
          "reference repair doesn't repeat lowHP sensing or mutate input");
    a.control.flags = 512U;
    a.rescue.reset();
    a.object_slot = 4;
    const auto slot = prepare_world_reference_preemption(s, {1}, f, false, false, {});
    check(slot.candidate && slot.candidate->state.battle.actors.at({1}).object_slot == -1 &&
              slot.candidate->state.battle.actors.at({1}).hp.target == 50 &&
              slot.candidate->state.battle.actors.at({1}).control.action == 7,
          "512 with no R only clears N, does not restore HP or action");
    s = fixture();
    s.battle.actors.at({1}).control.state = 5;
    s.contexts.at({1}).cell = {2, 2};
    auto human = s.battle.actors.at({1});
    human.id = {3};
    human.control.state = 2;
    human.position = {499, 0, 499};
    s.battle.actors.emplace(human.id, human);
    RewardActorContext cached;
    cached.cell = {4, 4};
    s.contexts.emplace(human.id, cached);
    s.human_order.push_back(human.id);
    GroundObjectState object;
    object.id = {0};
    object.state = 3;
    object.cached_cell = {2, 2};
    s.battle.objects.emplace(0, object);
    auto r = prepare_world_reference_preemption(s, {1}, f, true, false, {});
    check(
        r.candidate && r.candidate->state.battle.actors.at({1}).control.state == 13 &&
            !r.candidate->state.battle.actors.at({1}).encounter &&
            r.candidate->state.battle.actors.at({1}).state_counter == 0,
        "cached per-axis2 rescue preempts object scan; no object metadata consumed after success");
    s.contexts.at({3}).inside_town = true;
    r = prepare_world_reference_preemption(s, {1}, f, true, false, {0});
    check(r.candidate && r.candidate->state.battle.actors.at({1}).control.state == 11,
          "different cached town side excludes rescue, object cached h still eligible");
    s.contexts.at({3}).inside_town = false;
    f.flags[24] = 2;
    r = prepare_world_reference_preemption(s, {1}, f, true, false, {0});
    check(r.candidate && r.candidate->state.battle.actors.at({1}).control.state == 11,
          "current map bit2 on down cached cell excludes rescue");
    f.flags[24] = 0;
    r = prepare_world_reference_preemption(s, {1}, f, true, true, {0});
    check(r.candidate && r.candidate->state.battle.actors.at({1}).control.state == 11,
          "definition task flag suppresses rescue but not object's separate scan");
    s.battle.actors.at({1}).object_slot = -2;
    s.battle.actors.at({1}).rescue = CharacterId{3};
    s.battle.actors.at({3}).rescue = CharacterId{2};
    r = prepare_world_reference_preemption(s, {1}, f, true, false, {});
    check(r.candidate && r.candidate->state.battle.actors.at({1}).object_slot == -2 &&
              r.candidate->state.battle.actors.at({1}).control.state == 5,
          "R.R only tests nonnull, not equality; valid carrying prevents both idle scans");
    s.battle.actors.at({1}).rescue = CharacterId{999};
    check(!prepare_world_reference_preemption(s, {1}, f, true, false, {}).candidate,
          "unresolvable R is explicit error, no invented object or partial repaired candidate");
}
void enemy_event_identity() {
    auto s = fixture();
    auto other = s.encounters.at(0);
    other.runtime.id = 7;
    s.encounters.emplace(7, other);
    s.battle.actors.at({2}).encounter = 7;
    auto r = query_current_combat_enemy(s, {1});
    check(r.candidate && r.candidate->id == CharacterId{2},
          "ordinary e compares original f164b, not different stable event object identities");
    s.encounters.at(7).legacy_id = 1;
    check(!query_current_combat_enemy(s, {1}).candidate,
          "different original event IDs excluded even with equal event center");
    s.battle.actors.at({1}).control.flags |= 128U;
    s.battle.actors.at({1}).group.reset();
    r = query_current_combat_enemy(s, {1});
    check(r.error == ActorAiError::none && !r.candidate,
          "128 with null dc is source ordinary empty e, not a malformed world error");
    const auto prefix = prepare_world_perception_prefix(s, {1}, facts());
    check(prefix.candidate && !(prefix.candidate->state.battle.actors.at({1}).control.flags & 128U),
          "own c repairs lingering128 after empty-null-dc enemy lookup");
    s.battle.actors.at({1}).control.flags = 0;
    s.battle.actors.at({1}).encounter.reset();
    s.monster_order = {{999}};
    r = query_current_combat_enemy(s, {1});
    check(r.error == ActorAiError::none && !r.candidate,
          "absent db returns before scanning unrelated opponent roster");
}
void battle_preparation_world() {
    auto s = fixture();
    auto f = facts();
    auto &a = s.battle.actors.at({1});
    a.control.state = 18;
    a.state_counter = 5;
    a.control.flags = 4U | 16U;
    a.perceived_enemy = CharacterId{2};
    a.group.reset();
    auto r = prepare_world_battle_preparation(s, {1}, f);
    check(r.state && r.preparation->action == BattlePreparationAction::battle &&
              r.state->battle.actors.at({1}).control.state == 1 &&
              r.state->battle.actors.at({1}).encounter == 0 &&
              r.state->battle.actors.at({1}).state_counter == 0 &&
              !(r.state->battle.actors.at({1}).control.flags & (4U | 16U)),
          "state18 current G wins before unrelated map flag/db/bn checks and applies c1");
    a.perceived_enemy.reset();
    a.state_counter = 19;
    a.control.alternate_counter = 33;
    s.contexts.at({1}).cell = {2, 2};
    f.flags[12] = 2;
    r = prepare_world_battle_preparation(s, {1}, f);
    check(r.state && r.preparation->action == BattlePreparationAction::keep &&
              r.state->battle.actors.at({1}).control.state == 18,
          "valid event/map identity keeps battle preparation when G can't enter");
    auto old = s.encounters.at(0);
    s.encounters.erase(0);
    s.retired_encounters.emplace(0, old);
    old.runtime.id = 7;
    s.encounters.emplace(7, old);
    s.encounter_order = {7};
    r = prepare_world_battle_preparation(s, {1}, f);
    check(r.state && r.preparation->action == BattlePreparationAction::keep,
          "state18 matches live bn originalID+center even if db holds distinct retired object");
    s.encounters.at(7).runtime.center = {3, 2};
    r = prepare_world_battle_preparation(s, {1}, f);
    check(r.state && r.preparation->clear_encounter &&
              r.state->battle.actors.at({1}).control.state == 5 &&
              !r.state->battle.actors.at({1}).encounter &&
              r.state->battle.actors.at({1}).state_counter == 19 &&
              r.state->battle.actors.at({1}).control.alternate_counter == 33,
          "same originalID wrong center clears db then b preserving B and alternate i");
    s.encounters.at(7).runtime.center = {2, 2};
    s.battle.actors.at({1}).control.flags |= 512U;
    r = prepare_world_battle_preparation(s, {1}, f);
    check(r.state && r.preparation->action == BattlePreparationAction::restore_baseline &&
              r.preparation->clear_encounter,
          "512 follows identity scan and restores baseline even when original identity valid");
    s = fixture();
    s.battle.actors.at({1}).control.state = 18;
    s.battle.actors.at({1}).encounter = 999;
    s.contexts.at({1}).cell = {2, 2};
    f.flags[12] = 0;
    r = prepare_world_battle_preparation(s, {1}, f);
    check(r.state && r.preparation->action == BattlePreparationAction::restore_baseline,
          "map unflagged early return doesn't dereference stale db");
}
} // namespace
int main() {
    prefix_and_cache();
    geometry();
    healing_and_influence();
    event_and_physics();
    combat_move();
    repair_and_preemption();
    enemy_event_identity();
    battle_preparation_world();
    std::cout << "world perception checks: " << checks << '\n';
}
