#include "ark/simulation/rules/combat_commit.hpp"
#include "ark/simulation/rules/world_random.hpp"
#include "ark/simulation/rules/world_random_consumers.hpp"
#include "ark/simulation/rules/world_scripts.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
struct PrivateDraw {
    WorldRandomStream stream;
    std::vector<int> bounds;
    explicit PrivateDraw(std::vector<std::int32_t> raw)
        : stream(WorldRandomStream::from_raw(std::move(raw))) {}
    CombatRandomDraw provider() {
        return [this](int bound) -> std::optional<int> {
            bounds.push_back(bound);
            const auto result = stream.draw(bound);
            if (result.error != WorldRandomError::none)
                return {};
            return result.ticket;
        };
    }
    WorldCombatExpressionConsumer expressions() {
        return [this](CharacterId, const ActorEffectState &state, int expression,
                      int delay) -> std::optional<ActorEffectState> {
            const auto draw = provider();
            const auto probability = draw(1000);
            if (!probability)
                return {};
            const int count = reference_world_expression_variants(expression, true);
            ActorExpressionInput input{state, expression, delay, *probability, count, {}};
            auto result = prepare_actor_expression(input);
            if (result.error == ActorEffectError::missing_ticket) {
                input.variant_ticket = draw(count);
                result = prepare_actor_expression(input);
            }
            return result.candidate ? std::optional<ActorEffectState>(result.candidate->state)
                                    : std::nullopt;
        };
    }
};
CombatStrategyInput policy() {
    CombatStrategyInput input;
    input.flags = 128;
    input.in_move_area = input.sensed_enemy = input.same_town_side = input.fresh_enemy = true;
    input.attack_slot = input.group_tick = 10;
    input.weapon_range = input.monster_range = 100;
    return input;
}
AiRewardState world() {
    AiRewardState state;
    BattleActorRecord human;
    human.id = {1};
    human.definition = 1;
    human.control.flags = 2;
    human.control.state = 1;
    human.control.action = 1;
    human.capacity = 100;
    human.hp = {0, 100, 100, 100, false, 0};
    human.baseline = 5;
    human.position = {0, 0, 0};
    human.encounter = 0;
    auto monster = human;
    monster.id = {2};
    monster.definition = 7;
    monster.kind = ActorKind::monster;
    monster.baseline = 17;
    monster.position = {20, 0, 0};
    state.battle.actors = {{human.id, human}, {monster.id, monster}};
    state.human_order = {human.id};
    state.monster_order = {monster.id};
    state.contexts = {{human.id, {{0, 0}, false, {}, {}, true, {0, 0}}},
                      {monster.id, {{0, 0}, false, {}, {}, true, {0, 0}}}};
    state.battle.humans.emplace(1, HumanBattleRecord{});
    state.battle.monsters.emplace(7, MonsterBattleRecord{});
    RewardHumanDefinition growth;
    growth.derived.combat = {100, 100, 100, 100};
    growth.derived.available_spells = {true, false, false, false};
    state.growth.emplace(1, growth);
    RewardMonsterDefinition definition;
    definition.base_hp = 100;
    definition.base_attack = definition.base_defense = 100;
    state.monster_growth.emplace(7, definition);
    state.encounters.emplace(0, RewardEncounter{{0, {0, 0}, 0, 0, 0, 1, 1, 0}, {{2}}, false, {}});
    return state;
}
void strategy_gates() {
    for (int tick = 0; tick < 24; ++tick) {
        auto input = policy();
        input.group_tick = tick;
        PrivateDraw draw({99, 5});
        input.draw = draw.provider();
        check(prepare_combat_strategy(input).candidate.has_value() &&
                  draw.bounds == (tick == 10 ? std::vector<int>{100} : std::vector<int>{}),
              "only human attack slot consumes policy100");
    }
    auto input = policy();
    input.spells[0] = input.spells[3] = input.healing_target = true;
    PrivateDraw mixed({99, 5});
    input.draw = mixed.provider();
    check(prepare_combat_strategy(input).candidate->decision == CombatDecision::healing_spell &&
              mixed.bounds == std::vector<int>({100, 10}),
          "both available spell kinds consume secondary10 only after spell branch chosen");
    for (int guard = 0; guard < 5; ++guard) {
        auto guarded = policy();
        PrivateDraw draw({99});
        guarded.draw = draw.provider();
        if (guard == 0)
            guarded.flags |= 4;
        if (guard == 1)
            guarded.in_move_area = false;
        if (guard == 2)
            guarded.fresh_enemy = false;
        if (guard == 3)
            guarded.flags = 0;
        if (guard == 4)
            guarded.kind = ActorKind::monster;
        check(prepare_combat_strategy(guarded).candidate.has_value() && draw.bounds.empty(),
              "lock/area/fresh/group/monster guards do not speculatively draw human policy");
    }
    PrivateDraw explicit_draw({7});
    input.policy_ticket = 99;
    input.healing_ticket = 5;
    input.draw = explicit_draw.provider();
    check(prepare_combat_strategy(input).candidate && explicit_draw.bounds.empty(),
          "explicit optional tickets remain independent fixtures and precede fallback provider");
}
void damage_and_setup() {
    PrivateDraw physical({0});
    const auto damage = prepare_physical_damage(
        {ActorKind::human, 100, 100, false, false, {}, physical.provider()});
    check(damage.candidate && damage.candidate->base == 81 && damage.candidate->value == 73 &&
              physical.bounds == std::vector<int>{16},
          "physical bound derived from actual integer-ratio base before requesting jitter");
    PrivateDraw spell({1, 99, 0});
    const auto magic =
        prepare_spell_damage({100, {false, true, true}, false, {}, {}, {}, spell.provider()});
    check(magic.candidate && magic.candidate->effect == 6 && magic.candidate->damage.value == 81 &&
              spell.bounds == std::vector<int>({2, 100, 18}),
          "sparse learned order selects spell before enhancement before coefficient-specific span");
    PrivateDraw heal({0});
    const auto amount = prepare_healing_amount(0, {}, heal.provider());
    check(amount.candidate && amount.candidate->value == -1 && heal.bounds == std::vector<int>{2},
          "healing has one span draw, no learned/enhancement draws nor fabricated positive floor");
    PrivateDraw setup({9, 49, 29, 9, 99});
    const auto attack = prepare_human_attack({0, 0, 1, 0, 0, false, {}, setup.provider()});
    check(attack.candidate && attack.candidate->combo_count == 1 && !attack.candidate->miss &&
              setup.bounds == std::vector<int>({100, 100, 100, 100, 100}),
          "four combo perturbations precede miss100 even when combo clamps back to1");
    PrivateDraw invalid({1});
    check(!prepare_spell_damage({100, {}, false, {}, {}, {}, invalid.provider()}).candidate &&
              invalid.bounds.empty(),
          "no learned magic rejects before any extraction");
    check(!prepare_human_attack({0, 9, 1, 0, 0, false, {}, invalid.provider()}).candidate &&
              invalid.bounds.empty(),
          "invalid weapon is not an actual attack random branch");
}
void drop_chain() {
    auto state = world();
    state.battle.actors.at({1}).control.queue = {{14}};
    state.battle.actors.at({1}).control.action_counter = 5;
    state.battle.actors.at({2}).hp.target = 1;
    PrivateDraw draw({0, 0, 99, 30, 0});
    WorldAttackInput input;
    input.actor = {1};
    input.weapon = {0, 100, 1, 0, 0};
    input.draw = draw.provider();
    DropSelectionInput selection;
    selection.definitions = {{4, 0, 1, 2, 0}};
    input.drop_selection = selection;
    const auto result = prepare_world_attack_control(state, input);
    check(result.candidate && result.candidate->objects.size() == 1 &&
              draw.bounds == std::vector<int>({16, 100, 100, 60, 1}) &&
              state.battle.actors.at({2}).hp.target == 1 && state.battle.objects.empty(),
          "actual direct lethal chain: damage span, chance100, equipment100, rank60, nonempty "
          "selection");
    for (bool first_visit : {false, true}) {
        auto guarded = state;
        guarded.battle.actors.at({1}).miss = !first_visit;
        if (first_visit)
            guarded.battle.actors.at({1}).control.flags |= 8192;
        PrivateDraw private_draw({0, 0});
        input.draw = private_draw.provider();
        const auto hit = prepare_world_attack_control(guarded, input);
        check(hit.candidate && hit.candidate->objects.empty() &&
                  private_draw.bounds ==
                      (first_visit ? std::vector<int>{16, 100} : std::vector<int>{16}),
              "miss still computes damage but no drop chance; first visit consumes lethal chance "
              "then suppresses selection");
    }
    DropSelectionInput empty;
    PrivateDraw no_candidates({99, 59});
    empty.draw = no_candidates.provider();
    const auto none = prepare_drop_selection(empty);
    check(none.candidate && !none.candidate->selected &&
              no_candidates.bounds == std::vector<int>({100, 60}),
          "empty drop catalogue still consumes equipment/rank but no zero-bound selection");
    PrivateDraw failure({0, 0, 99});
    input.draw = failure.provider();
    check(!prepare_world_attack_control(state, input).candidate &&
              state.battle.actors.at({1}).attack_armed &&
              state.battle.actors.at({2}).hp.target == 1,
          "late rank tape failure publishes no HP/control/drop mutation; private raw candidate "
          "must be discarded");
}
void lazy_attack_windows() {
    for (int counter = 0; counter <= 16; ++counter) {
        auto state = world();
        state.battle.actors.at({1}).control.queue = {{14}};
        state.battle.actors.at({1}).control.action_counter = counter;
        PrivateDraw draw({0});
        WorldAttackInput input;
        input.actor = {1};
        input.weapon = {0, 100, 1, 0, 0};
        input.draw = draw.provider();
        const auto result = prepare_world_attack_control(state, input);
        check(result.candidate &&
                  draw.bounds ==
                      (counter >= 5 && counter <= 8 ? std::vector<int>{16} : std::vector<int>{}),
              "physical jitter only on actual armed hit window, not idle animation frames");
    }
    for (int counter : {1, 25, 26, 27, 42}) {
        auto state = world();
        state.battle.actors.at({1}).control.queue = {{15}};
        state.battle.actors.at({1}).control.action = 4;
        state.battle.actors.at({1}).control.action_counter = counter;
        PrivateDraw draw({0, 99, 0});
        WorldAttackInput input;
        input.actor = {1};
        input.weapon = {0, 100, 1, 0, 0};
        input.facing = 2;
        input.draw = draw.provider();
        const auto result = prepare_world_attack_control(state, input);
        check(result.candidate && draw.bounds == (counter == 26 ? std::vector<int>{1, 100, 10}
                                                                : std::vector<int>{}),
              "magic selection/enhancement/jitter only at26 with actual current target");
    }
}
void projectiles() {
    auto state = world();
    state.monster_growth.at(7).base_defense = 50;
    state.battle.actors.at({2}).capacity = 500;
    state.battle.actors.at({2}).hp = {0, 500, 500, 500, false, 0};
    state.projectiles.emplace(
        10, *prepare_projectile(ProjectileKind::arrow, {1}, {2}, {}, {1000, 0, 0}, 0).candidate);
    state.projectile_order = {10};
    WorldProjectileInput input;
    input.projectile = 10;
    input.box = CollisionBox{-1, 0, 2, 2};
    input.monster_boxes[0] = CollisionBox{-10, 10, 20, 20};
    PrivateDraw contact({0});
    input.draw = contact.provider();
    auto result = prepare_world_projectile(state, input);
    check(result.candidate && result.candidate->step.damage_target == CharacterId{2} &&
              contact.bounds == std::vector<int>{24},
          "arrow actual collision derives current physical base120/span24, not launch jitter");
    auto far = state;
    far.battle.actors.at({2}).position.x = 1000;
    PrivateDraw no_contact({0});
    input.draw = no_contact.provider();
    result = prepare_world_projectile(far, input);
    check(result.candidate && !result.candidate->step.damage_target && no_contact.bounds.empty(),
          "travelling projectile consumes no damage/drop RNG before collision");
}
void group_before_event() {
    auto state = world();
    auto extra = state.battle.actors.at({2});
    extra.id = {3};
    state.battle.actors.emplace(extra.id, extra);
    state.contexts.emplace(extra.id, state.contexts.at({2}));
    state.monster_order.push_back(extra.id);
    state.battle.actors.at({1}).control.flags |= 128;
    state.battle.actors.at({2}).control.flags |= 128;
    state.battle.actors.at({3}).control.flags |= 128;
    auto &event = state.encounters.at(0);
    event.group_exists = true;
    event.group.tick = 159;
    event.group.humans = {{{1}, 128}};
    event.group.monsters = {{{2}, 128}, {{3}, 128}};
    PrivateDraw draw({10, 90, 999});
    EncounterCommitInput input;
    input.encounter = 0;
    input.draw = draw.provider();
    const auto result = prepare_encounter_reward_commit(state, input);
    check(result.candidate && draw.bounds == std::vector<int>({100, 100, 1000}) &&
              result.candidate->state.battle.actors.at({2}).monster_posture == 0 &&
              result.candidate->state.battle.actors.at({3}).monster_posture == 2,
          "real group posture extraction precedes event's proven empty-spawn1000 attempt");
    input.town_overlap = true;
    PrivateDraw cancelled({10, 90, 999});
    input.draw = cancelled.provider();
    const auto cancel = prepare_encounter_reward_commit(state, input);
    check(cancel.candidate && cancelled.bounds.empty(),
          "town cancellation guard prevents unused group/event extraction");
}
void victory_interleave() {
    auto state = world();
    state.battle.actors.erase({2});
    state.monster_order.clear();
    state.encounters.at(0).members.clear();
    auto &event = state.encounters.at(0).runtime;
    event.state = 3;
    event.spawned = event.quota = 1;
    state.task_active = true;
    state.battle.participants = {1, 1};
    PrivateDraw draw({0, 0, 0, 0, 0, 0, 0});
    EncounterCommitInput input;
    input.encounter = 0;
    input.draw = draw.provider();
    input.random_request = [&](const AiRewardState &owner,
                               const EncounterRequest &request) -> std::optional<AiRewardState> {
        auto next = owner;
        if (!request.actor)
            return {};
        const auto probability = input.draw(1000);
        if (!probability)
            return {};
        // 变体数1是明确表现夹具，不称为真实平台素材目录。
        ActorExpressionInput expression{next.contexts.at(*request.actor).effects,
                                        request.parameter,
                                        request.delay,
                                        *probability,
                                        1,
                                        {}};
        auto result = prepare_actor_expression(expression);
        if (result.error == ActorEffectError::missing_ticket) {
            expression.variant_ticket = input.draw(1);
            result = prepare_actor_expression(expression);
        }
        if (!result.candidate)
            return {};
        next.contexts.at(*request.actor).effects = result.candidate->state;
        return next;
    };
    const auto result = prepare_encounter_reward_commit(state, input);
    check(result.candidate && draw.bounds == std::vector<int>({100, 1000, 1, 100, 1000, 2, 2}),
          "duplicate participant second expression sees earlier24: no variant draw; summary draws "
          "follow both actors");
    check(result.candidate->state.contexts.at({1}).effects.display.size() == 3 &&
              state.contexts.at({1}).effects.display.empty(),
          "two reward displays and one expression share same private effects owner");
    PrivateDraw missing({0, 0});
    input.draw = missing.provider();
    input.random_request = {};
    check(!prepare_encounter_reward_commit(state, input).candidate &&
              missing.bounds == std::vector<int>{100},
          "provider mode requires real expression consumer at original slot, not postponed "
          "notification");
}
void source_combat_expressions() {
    auto state = world();
    for (const bool suppressed : {false, true}) {
        auto setup_state = state;
        if (suppressed)
            setup_state.contexts.at({1}).effects.display = {{24, 0, 0, 0, 0, 0}};
        PrivateDraw draw(suppressed ? std::vector<std::int32_t>{0, 9, 49, 29, 9, 99}
                                    : std::vector<std::int32_t>{0, 0, 9, 49, 29, 9, 99});
        WorldAttackSetupInput input;
        input.actor = {1};
        input.target = {2};
        input.weapon = {0, 100, 1, 0, 0};
        input.facing = 1;
        input.draw = draw.provider();
        input.expression = draw.expressions();
        const auto result = prepare_world_attack_setup(setup_state, input);
        check(result.candidate &&
                  draw.bounds == (suppressed
                                      ? std::vector<int>{1000, 100, 100, 100, 100, 100}
                                      : std::vector<int>{1000, 4, 100, 100, 100, 100, 100}) &&
                  result.candidate->requests.empty(),
              "human source c1 first1000/optional4 precedes five attack100; no deferred duplicate "
              "expression");
        input.expression = {};
        PrivateDraw missing({0, 0, 0, 0, 0});
        input.draw = missing.provider();
        check(!prepare_world_attack_setup(setup_state, input).candidate && missing.bounds.empty(),
              "shared-random human attack refuses missing c1 consumer before combo extraction");
    }
    for (const bool down : {false, true}) {
        auto hit_state = state;
        hit_state.battle.actors.at({2}).control.queue = {{17}};
        hit_state.battle.actors.at({2}).control.action = 6;
        hit_state.battle.actors.at({2}).control.action_counter = 12;
        hit_state.battle.actors.at({2}).attack_position = hit_state.battle.actors.at({2}).position;
        hit_state.battle.actors.at({2}).attack_destination =
            hit_state.battle.actors.at({1}).position;
        if (down)
            hit_state.battle.actors.at({1}).hp.target = 1;
        PrivateDraw draw(down ? std::vector<std::int32_t>{0, 0, 0, 0}
                              : std::vector<std::int32_t>{0, 0, 0});
        WorldAttackInput input;
        input.actor = {2};
        input.draw = draw.provider();
        input.expression = draw.expressions();
        const auto result = prepare_world_attack_control(hit_state, input);
        check(
            result.candidate && result.candidate->hit->landed &&
                draw.bounds ==
                    (down ? std::vector<int>{16, 1000, 2, 1000} : std::vector<int>{16, 1000, 7}) &&
                result.candidate->state.contexts.at({1}).effects.display.size() == 1 &&
                result.candidate->state.contexts.at({1}).effects.display[0][1] == (down ? -16 : 0),
            "monster jitter precedes human downc2/16 then landedc0 suppressed by earliercd12; "
            "normalhit c0variant7");
        check(hit_state.contexts.at({1}).effects.display.empty(),
              "expression fields are private alongside hit HP, not external side effects");
        PrivateDraw incomplete({0, 0});
        input.draw = incomplete.provider();
        input.expression = incomplete.expressions();
        check(
            !prepare_world_attack_control(hit_state, input).candidate &&
                hit_state.contexts.at({1}).effects.display.empty(),
            "late expression tape failure discards all hit/down/statistic/effect candidate fields");
    }
}
void source_hit_events() {
    auto state = world();
    state.pending_completion = 37;
    state.battle.global_downs = 4;
    state.battle.humans.at(1).recent_reward = 99;
    state.battle.humans.at(1).recent_kills = 4;
    state.battle.monsters.at(7).human_kills = 3;
    state.battle.actors.at({1}).hp.target = 1;
    state.battle.actors.at({2}).control.queue = {{17}};
    state.battle.actors.at({2}).control.action = 6;
    state.battle.actors.at({2}).control.action_counter = 12;
    state.battle.actors.at({2}).attack_position = state.battle.actors.at({2}).position;
    state.battle.actors.at({2}).attack_destination = state.battle.actors.at({1}).position;
    PrivateDraw draw({0, 0, 0, 0});
    WorldAttackInput input;
    input.actor = {2};
    input.draw = draw.provider();
    input.expression = draw.expressions();
    int events{};
    input.event = [&](const AiRewardState &source,
                      int event) -> std::optional<WorldCombatExternalWriteback> {
        ++events;
        check(event == 131 && source.battle.events.count(131) && source.battle.global_downs == 5 &&
                  source.battle.actors.at({1}).control.state == 2 &&
                  source.contexts.at({1}).effects.display[0][1] == -16 &&
                  source.battle.humans.at(1).recent_reward == 99 &&
                  source.battle.monsters.at(7).human_kills == 3 &&
                  draw.bounds == std::vector<int>{16, 1000, 2},
              "131 source case after state2/down-expression/global5 but before "
              "recent-reset/monsterkill/c0 extraction");
        // 明确脚本控制夹具，用实际解释器验证即时22和续体；非APK的131脚本内容。
        WorldScriptCatalog catalog;
        catalog.events.emplace(131, WorldScriptDefinition{131, "fixture", 0, {}, {{22}, {6, 2}}});
        WorldScriptState scripts;
        scripts.pending_completion = source.pending_completion;
        const auto result = prepare_world_script(catalog, scripts, {131, {}, {}});
        if (!result.candidate)
            return {};
        auto fields = encounter_external_writeback(source);
        fields.pending_completion = result.candidate->state.pending_completion;
        fields.external_actor_roots.insert({1});
        return WorldCombatExternalWriteback{fields, result.candidate->state.popularity_queue};
    };
    const auto down = prepare_world_attack_control(state, input);
    check(down.candidate && events == 1 && draw.bounds == std::vector<int>{16, 1000, 2, 1000} &&
              down.candidate->state.pending_completion == 0 &&
              down.candidate->popularity_queue == std::vector<std::array<int, 3>>{{10, 37, 1}} &&
              down.candidate->state.battle.humans.at(1).recent_reward == 0 &&
              down.candidate->state.battle.monsters.at(7).human_kills == 4 &&
              down.candidate->state.external_actor_roots.count({1}),
          "131 real script typed globals/I/roots survive subsequent internal counters and no event "
          "replay");
    input.event = {};
    PrivateDraw missing({0, 0, 0, 0});
    input.draw = missing.provider();
    input.expression = missing.expressions();
    check(!prepare_world_attack_control(state, input).candidate &&
              missing.bounds == std::vector<int>{16, 1000, 2} && state.battle.global_downs == 4 &&
              state.contexts.at({1}).effects.display.empty(),
          "shared event131 missing consumer refuses before c0 and returns no partial "
          "down/expression/stats");
    auto boss = world();
    boss.battle.actors.at({1}).control.queue = {{14}};
    boss.battle.actors.at({1}).control.action_counter = 5;
    boss.battle.actors.at({2}).hp.target = 1;
    boss.battle.monsters.at(7).flags = 4;
    boss.battle.monsters.at(7).rank = 5;
    PrivateDraw death({0, 99});
    input = {};
    input.actor = {1};
    input.weapon = {0, 100, 1, 0, 0};
    input.draw = death.provider();
    int bosses{};
    input.event = [&](const AiRewardState &source,
                      int event) -> std::optional<WorldCombatExternalWriteback> {
        ++bosses;
        check(
            event == 217 && source.battle.actors.at({2}).control.state == 3 &&
                source.battle.humans.at(1).kills == 1 &&
                source.battle.defeated_definitions.empty() &&
                death.bounds == std::vector<int>{16, 100},
            "217 original slot after kill/dropchance but before UserData.N global monster record");
        auto fields = encounter_external_writeback(source);
        fields.events.insert(300);
        return WorldCombatExternalWriteback{fields, {}};
    };
    const auto killed = prepare_world_attack_control(boss, input);
    check(killed.candidate && bosses == 1 && killed.candidate->state.battle.events.count(217) &&
              killed.candidate->state.battle.events.count(300) &&
              killed.candidate->state.battle.defeated_definitions == std::vector<int>{7},
          "typed event additions merge without restoring old HP/control/kill record");
    input.event = [](const AiRewardState &, int) -> std::optional<WorldCombatExternalWriteback> {
        return {};
    };
    PrivateDraw rejected({0, 99});
    input.draw = rejected.provider();
    check(
        !prepare_world_attack_control(boss, input).candidate &&
            boss.battle.actors.at({2}).hp.target == 1 && boss.battle.humans.at(1).kills == 0 &&
            boss.battle.defeated_definitions.empty(),
        "late217 external script failure rolls back HP/death/drop/statistics/global list together");
}
} // namespace
int main() {
    try {
        strategy_gates();
        damage_and_setup();
        drop_chain();
        lazy_attack_windows();
        projectiles();
        group_before_event();
        victory_interleave();
        source_combat_expressions();
        source_hit_events();
        std::cout << "world_combat_random: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_combat_random: " << e.what() << '\n';
        return 1;
    }
}
