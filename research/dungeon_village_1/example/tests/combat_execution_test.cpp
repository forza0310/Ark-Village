#include "dungeon_village_reference/actor_lifecycle.hpp"
#include "dungeon_village_reference/combat_execution.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
bool near(float a, float b) { return std::abs(a - b) < 0.001F; }
bool has(const HitCandidate &c, HitRequestKind kind) {
    for (const auto &r : c.requests)
        if (r.kind == kind)
            return true;
    return false;
}
HitTargetState target(ActorKind kind, int hp = 100) {
    HitTargetState s;
    s.kind = kind;
    s.capacity = 100;
    s.hp.target = hp;
    s.hp.displayed = 99;
    s.hp.origin = 99;
    return s;
}
void setup_and_windows() {
    AttackSetupInput i;
    i.weapon_combo = 4;
    i.tickets = {99, 99, 99, 99, 99};
    check(prepare_human_attack(i).candidate->combo_count == 1, "zero dexterity minimum1");
    i.dexterity = 500;
    check(prepare_human_attack(i).candidate->combo_count == 4, "dexterity maps to weapon combo");
    i.tickets = {0, 0, 99, 99, 99};
    check(prepare_human_attack(i).candidate->combo_count == 7, "four sequential perturbations");
    i.boosted = true;
    check(prepare_human_attack(i).candidate->combo_count == 11, "boost1.7 truncation");
    i.miss_low = 100;
    i.miss_high = 100;
    auto r = prepare_human_attack(i);
    check(r.candidate->miss && r.candidate->combo_count == 1, "miss overrides combo after boost");
    check(r.candidate->queue ==
              std::vector<LegacyActorControl>{{3, 1}, {14}, {3, 0}, {1, 10, 0}, {7, 4}},
          "post attack wait10 and clear lock in source order");
    i.tickets[3] = 100;
    check(prepare_human_attack(i).error == CombatAiError::invalid_ticket,
          "invalid perturbation rejected even if miss forces count1");
    constexpr int finishes[] = {12, 14, 12, 12};
    for (int weapon = 0; weapon < 4; ++weapon)
        for (int count : {1, 2, 5})
            for (int index = 0; index < count; ++index)
                for (int tick = 0; tick <= 20; ++tick)
                    for (bool found : {false, true})
                        for (bool armed : {false, true}) {
                            HumanAttackFrame f{weapon, tick, index, count, armed, found, 50, 51};
                            const auto c = prepare_human_attack_frame(f).candidate;
                            const int effective = tick + (index > 0 ? 1 : 0);
                            check(c && c->request_damage == (weapon != 1 && armed && found &&
                                                             effective >= 5 && effective <= 8),
                                  "direct inclusive5..8 after extra combo increment");
                            check(c->request_arrow == (weapon == 1 && found && effective == 8),
                                  "arrow exact8 without ai/range gate");
                            const bool restart = index < count - 1 &&
                                                 effective >= finishes[weapon] - 5 &&
                                                 !(weapon == 1 && effective == 8 && !found);
                            check(c->combo_index == index + (restart ? 1 : 0) &&
                                      c->counter == (restart ? 0 : effective),
                                  "combo restart and absent bow-target bypass");
                            check(c->completed ==
                                      (index == count - 1 && effective >= finishes[weapon]),
                                  "completion is distinct from hit window");
                        }
    HumanAttackFrame exact{0, 5, 0, 1, true, true, 50, 50};
    check(!prepare_human_attack_frame(exact).candidate->request_damage,
          "exact range excludes direct hit");
    for (int tick = 0; tick <= 50; ++tick)
        for (bool found : {false, true}) {
            const auto c = prepare_spell_frame(tick, found);
            check(c && c->prepare_visual == (tick == 1) && c->query_target == (tick == 26) &&
                      c->request_effect == (tick == 26 && found) && c->completed == (tick >= 42),
                  "spell exact preparation/commit and separate completion");
        }
    check(prepare_healing_amount(0, 0).candidate->value == -1 &&
              prepare_healing_amount(0, 1).candidate->value == 0,
          "zero magic can produce negative healing, no invented minimum");
    check(prepare_healing_amount(100, 19).candidate->value == 109,
          "healing uses magic directly, no offensive spell boost");
    check(prepare_healing_amount(100, std::nullopt).error == CombatAiError::missing_ticket,
          "healing requires fresh ticket");
}
void monster_windows() {
    MonsterAttackFrame f;
    f.origin = {0, 0, 0};
    f.destination = {100, 0, 0};
    f.previous_position = {70, 0, 0};
    f.range = 31;
    f.preferred = CharacterId{2};
    f.humans = {{{1}, {100, 0, 0}, {0, 0}, true}, {{2}, {100, 0, 0}, {999, 999}, false}};
    for (int action : {3, 6})
        for (int tick = 0; tick < 60; ++tick) {
            f.action = action;
            f.counter = tick;
            const auto c = prepare_monster_attack_frame(f).candidate;
            check(c && c->completed == (tick >= (action == 3 ? 56 : 30)),
                  "monster exact completion boundary");
            const bool window = tick >= (action == 3 ? 34 : 12) && tick <= (action == 3 ? 42 : 20);
            check(c->damage_target.has_value() == window && c->armed == !window,
                  "monster inclusive hit window, target consumes ai");
            if (window)
                check(c->damage_target == CharacterId{2},
                      "preferred roster target bypasses fallback state/half-grid gate");
        }
    f.action = 6;
    f.counter = 12;
    f.range = 30;
    check(!prepare_monster_attack_frame(f).candidate->damage_target,
          "range equal excluded for preferred and fallback");
    f.range = 31;
    f.preferred = CharacterId{99};
    check(prepare_monster_attack_frame(f).candidate->damage_target == CharacterId{1},
          "absent preferred fallback takes original roster first eligible");
    f.humans[0].half_cell = {3, 3};
    check(!prepare_monster_attack_frame(f).candidate->damage_target,
          "fallback half Manhattan6 excluded");
    f.humans[0].half_cell = {3, 2};
    check(prepare_monster_attack_frame(f).candidate->damage_target == CharacterId{1},
          "fallback half Manhattan5 included");
    f.previous_position = {0, 0, 0};
    auto c = prepare_monster_attack_frame(f).candidate;
    check(!c->damage_target && near(c->position.x, 100),
          "hit reads preceding au even though current au arrives at target");
    f.counter = 9;
    c = prepare_monster_attack_frame(f).candidate;
    check(near(c->position.x, 80) && near(c->position.height, 20),
          "monster6 curved attack at tick9 uses displacement80, height20");
    f.counter = 16;
    c = prepare_monster_attack_frame(f).candidate;
    check(near(c->position.x, 100.0F - 5500.0F / 70.0F), "monster6 curved return at tick16");
    f.counter = 20;
    check(near(prepare_monster_attack_frame(f).candidate->position.x, 0),
          "monster attack returns origin at20 before finish30");
    f.action = 3;
    f.counter = 31;
    check(near(prepare_monster_attack_frame(f).candidate->position.x, 50),
          "action3 outbound is linear, not action6 parabola");
    f.counter = 38;
    check(near(prepare_monster_attack_frame(f).candidate->position.x, 50),
          "action3 return is linear");
}
void projectile_cases() {
    for (int facing = 0; facing < 4; ++facing) {
        auto s =
            prepare_projectile(ProjectileKind::arrow, {1}, {2}, {0, 99, 0}, {100, 0, 0}, facing)
                .candidate;
        constexpr int heights[] = {4, 12, 12, 4};
        check(s && near(s->velocity.x, 17) && near(s->position.height, heights[facing]),
              "arrow17 and facing-specific absolute height, ignoring caster elevation");
    }
    check(prepare_projectile(ProjectileKind::arrow, {1}, {2}, {}, {}, 0).error ==
              CombatAiError::invalid_input,
          "coincident launch rejected without NaN");
    auto vertical =
        prepare_projectile(ProjectileKind::spell, {1}, {2}, {}, {0, 0, 300}, 0, 4, 30).candidate;
    check(vertical && near(vertical->velocity.height, 20) &&
              near(vertical->acceleration.height, -5),
          "vertical dx/vx NaN maps to Java0 then travel4, not length-based30");
    ProjectileContext ctx;
    ctx.box = {0, 0, 0, 0};
    ctx.monsters = {{{3}, {17, 0, 0}, {0, 0, 0, 0}}, {{2}, {17, 0, 0}, {0, 0, 0, 0}}};
    auto arrow = prepare_projectile(ProjectileKind::arrow, {1}, {2}, {}, {100, 0, 0}, 0).candidate;
    auto a = advance_projectile(*arrow, ctx).candidate;
    check(a && a->remove && a->damage_target == CharacterId{3} && a->physical_damage &&
              a->contact_effect && a->state.counter == 0,
          "arrow postmove contact uses first bm, not originally targeted ID, no tick increment");
    ctx.original_target_reference = false;
    a = advance_projectile(*arrow, ctx).candidate;
    check(a->remove && !a->damage_target && near(a->state.position.x, 0),
          "original target null deletes before movement even with another collision");
    ctx.original_target_reference = true;
    ctx.monsters = {{{3}, {17, 0, -2}, {0, 0, 1, 2}}};
    ctx.box = {0, 0, 1, 0};
    check(!advance_projectile(*arrow, ctx).candidate->damage_target,
          "legacy rectangle depth runs negative z, not positive");
    ctx.monsters[0].position.z = 2;
    check(advance_projectile(*arrow, ctx).candidate->damage_target == CharacterId{3},
          "inclusive bottom-depth rectangle edge contact");
    ctx.monsters.clear();
    auto falling = *arrow;
    falling.position.height = 0;
    falling.velocity.height = -1;
    check(advance_projectile(falling, ctx).candidate->remove, "unhit arrow below ground removed");
    for (int tick : {98, 99, 100}) {
        arrow->counter = tick;
        a = advance_projectile(*arrow, ctx).candidate;
        check(a->remove == (tick >= 99), "all types lifetime100 after counter increment");
    }
    constexpr int delays[] = {6, 12, 4, 19, 24, 11};
    for (int effect = 4; effect <= 9; ++effect) {
        auto spell =
            prepare_projectile(ProjectileKind::spell, {1}, {2}, {}, {100, 0, 0}, 0, effect, 30)
                .candidate;
        spell->position = {0, 0, 0};
        spell->velocity = {0, -1, 0};
        spell->acceleration = {};
        ctx.monsters = {{{3}, {}, {0, 0, 0, 0}}};
        auto c = advance_projectile(*spell, ctx).candidate;
        check(c && c->remove && !c->damage_target && c->spawned &&
                  c->spawned->original_target == CharacterId{3} &&
                  c->spawned->delay == delays[effect - 4] && c->spawned->damage == 30 &&
                  c->visual_effect == effect && c->state.counter == 0,
              "landed spell spawns distinct delayed damage, correct visual lifetime");
        auto delay = *c->spawned;
        for (int tick = 0; tick <= delay.delay; ++tick) {
            delay.counter = tick;
            const auto d = advance_projectile(delay, ctx).candidate;
            check(d->remove == (tick == delay.delay) &&
                      d->damage_target.has_value() == (tick == delay.delay),
                  "delayed damage checks OLD counter, exact delay and only once on removal");
        }
        ctx.monsters.clear();
        c = advance_projectile(*spell, ctx).candidate;
        check(c->remove && c->ground_effect22 && !c->spawned,
              "missed landing requests ground22, no damage instance");
        spell->velocity.height = 0;
        c = advance_projectile(*spell, ctx).candidate;
        check(!c->remove && !c->ground_effect22, "exact ground0 does not count as spell landing");
    }
}
void hits() {
    HitContext i;
    i.attacker_miss = true;
    for (int timer : {0, 1, 16}) {
        auto s = target(ActorKind::human);
        s.label_timer = timer;
        auto c = prepare_hit(s, 1000, i).candidate;
        check(c && !c->landed && !c->lethal && c->target.hp.target == 100 &&
                  c->target.hit_flash == 7 && c->target.label_timer == (timer == 0 ? 16 : timer) &&
                  c->target.miss_label == (timer == 0) && c->requests.size() == 1 &&
                  c->requests[0].kind == HitRequestKind::face_attacker,
              "miss flashes7 and faces attacker, preserves active label and HP, no stats");
    }
    i = {};
    i.attacker_visible = true;
    for (auto kind : {ActorKind::human, ActorKind::monster})
        for (int damage : {0, 1, 69, 70, 99, 100, 101}) {
            auto s = target(kind);
            s.flags = 16 | 128 | 2048 | 16384;
            i.drop_ticket = 99;
            const auto c = prepare_hit(s, damage, i).candidate;
            check(c && c->landed && c->lethal == (damage >= 100) &&
                      c->target.hp.target == 100 - damage && c->target.hp.origin == 100 &&
                      c->target.hp.displayed == 100 && c->target.hp.animating &&
                      c->target.damage_total == damage && c->target.hit_count == 1,
                  "hit uses target HP, records zero damage as hit, restarts HP animation");
            check(c->consumed_drop_ticket == (kind == ActorKind::monster && damage >= 100),
                  "drop random only on lethal monster hit");
            if (damage < 100)
                check(!(c->target.flags & 16) &&
                          bool(c->target.flags & 4096) ==
                              (kind == ActorKind::monster && 100 - damage < 100 / 3),
                      "nonlethal clears16 and monster boost strict target HP threshold");
        }
    auto human = target(ActorKind::human);
    i.attacker_kind = ActorKind::monster;
    i.target_carries_rescued_actor = true;
    i.rescue_reference = true;
    i.victim_participant = true;
    i.global_down_count = 4;
    auto c = prepare_hit(human, 100, i).candidate;
    const std::vector<HitRequestKind> sequence = {HitRequestKind::attack_sound,
                                                  HitRequestKind::state,
                                                  HitRequestKind::expression,
                                                  HitRequestKind::reset_down_timer,
                                                  HitRequestKind::drop_rescued_actor,
                                                  HitRequestKind::clear_rescue_links,
                                                  HitRequestKind::participant_down_count,
                                                  HitRequestKind::global_down_count,
                                                  HitRequestKind::event131,
                                                  HitRequestKind::reset_human_definition,
                                                  HitRequestKind::monster_human_kills};
    check(c->requests.size() == sequence.size(), "human down complete ordered requests");
    for (std::size_t n = 0; n < sequence.size(); ++n)
        check(c->requests[n].kind == sequence[n], "human down request order");
    i.rescue_reference = false;
    c = prepare_hit(human, 100, i).candidate;
    check(!has(*c, HitRequestKind::drop_rescued_actor) &&
              has(*c, HitRequestKind::clear_rescue_links),
          "missing rescued reference still clears sentinel and links");
    i.event131_present = true;
    check(!has(*prepare_hit(human, 100, i).candidate, HitRequestKind::event131),
          "event131 duplicate suppression");
    auto monster = target(ActorKind::monster);
    i = {};
    i.attacker_first_visit = true;
    i.task_encounter = true;
    i.killer_participant = true;
    i.boss_flags4 = true;
    i.monster_rank = 5;
    check(prepare_hit(monster, 100, i).error == CombatAiError::missing_ticket,
          "first-visit drop suppression still requires consumed ticket");
    for (int ticket = 0; ticket < 100; ++ticket)
        for (bool first_visit : {false, true}) {
            i.drop_ticket = ticket;
            i.attacker_first_visit = first_visit;
            c = prepare_hit(monster, 100, i).candidate;
            check(c && c->consumed_drop_ticket &&
                      has(*c, HitRequestKind::spawn_drop) == (ticket < 6 && !first_visit) &&
                      has(*c, HitRequestKind::participant_task_kills) &&
                      has(*c, HitRequestKind::event217),
                  "drop6/100 before8192, task participant and boss event");
        }
    monster.flags = 16384;
    i = {};
    i.target_action = 7;
    check(prepare_hit(monster, 1, i).candidate->target.flags & 4096,
          "g reports0 for k7, enabling rage even with high HP target");
    monster.damage_total = std::numeric_limits<int>::max();
    check(prepare_hit(monster, 1, i).error == CombatAiError::invalid_input &&
              monster.hp.target == 100,
          "overflow returns no partial candidate and preserves input");
}
void integrated_chain() {
    CombatStrategyInput policy;
    policy.in_move_area = true;
    policy.sensed_enemy = true;
    policy.same_town_side = true;
    policy.fresh_enemy = true;
    policy.flags = 128;
    policy.weapon_range = 100;
    policy.sensed_distance = 50;
    policy.policy_ticket = 0;
    check(prepare_combat_strategy(policy).candidate->decision == CombatDecision::physical_attack,
          "strategy selects physical request at assigned slot");
    AttackSetupInput setup;
    setup.weapon_combo = 1;
    setup.tickets = {99, 99, 99, 99, 99};
    const auto prepared = prepare_human_attack(setup).candidate;
    HumanAttackFrame frame{0, 0, 0, prepared->combo_count, true, true, 50, 100};
    auto victim = target(ActorKind::monster, 80);
    int committed{};
    for (int tick = 1; tick <= 12; ++tick) {
        frame.counter = tick;
        const auto c = prepare_human_attack_frame(frame).candidate;
        frame.armed = c->armed;
        if (c->request_damage) {
            const auto damage =
                prepare_physical_damage({ActorKind::human, 100, 100, false, false, 8}).candidate;
            HitContext context;
            context.drop_ticket = 99;
            const auto hit = prepare_hit(victim, damage->value, context).candidate;
            check(damage->value == 81 && hit->lethal && hit->target.hp.target == -1,
                  "window5 physical81 causes lethal targetHP, not displayed99");
            victim = hit->target;
            ++committed;
        }
        check(c->completed == (tick == 12), "hit5 does not end animation before12");
    }
    check(committed == 1, "armed hit cannot repeat at6/7/8");
    TimedLifecycleInput corpse;
    corpse.state = 3;
    corpse.old_counter = 11;
    check(!prepare_timed_lifecycle(corpse).candidate->delete_instance,
          "lethal hit state3 does not immediately remove corpse");
    corpse.old_counter = 12;
    check(prepare_timed_lifecycle(corpse).candidate->delete_instance,
          "death reward/removal is a later lifecycle step");
    auto delayed =
        prepare_projectile(ProjectileKind::delayed_damage, {1}, {2}, {}, {}, 0, 0, 81, 6).candidate;
    ProjectileContext context;
    for (int tick = 0; tick < 6; ++tick) {
        auto next = advance_projectile(*delayed, context).candidate;
        check(!next->remove && !next->damage_target, "delayed projectile no early HP effect");
        delayed = next->state;
    }
    auto next = advance_projectile(*delayed, context).candidate;
    HitContext hit_context;
    hit_context.attacker_miss = true; // Source reads live caster.w even for queued magic damage.
    auto hit = prepare_hit(target(ActorKind::monster), next->state.damage, hit_context).candidate;
    check(next->damage_target == CharacterId{2} && !hit->landed && hit->target.hp.target == 100,
          "delayed hit reads current miss flag, not captured launch state");
}
} // namespace
int main() {
    try {
        setup_and_windows();
        monster_windows();
        projectile_cases();
        hits();
        integrated_chain();
        std::cout << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "combat_execution: " << e.what() << '\n';
        return 1;
    }
}
