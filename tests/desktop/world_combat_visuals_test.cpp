// Presentation fixtures exercise real owner fields without spawning actors or paying rewards.
#include "../support/world_fixture.hpp"
#include "ark/presentation/world_combat_visuals.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
namespace sim = ark::simulation;
namespace rules = sim::rules;
using namespace ark::desktop;
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
sim::StartupWorldRuntimeState fixture() {
    sim::StartupWorldRuntimeState state;
    auto &ai = state.scene.world.world.ai;
    rules::BattleActorRecord actor;
    actor.id = {1};
    actor.definition = 7;
    actor.kind = rules::ActorKind::human;
    ai.battle.actors.emplace(actor.id, actor);
    ai.contexts.emplace(actor.id, rules::RewardActorContext{});
    rules::RewardHumanDefinition growth;
    growth.definition.profession_levels = {1};
    growth.pending = {10, -50};
    growth.experience = 6;
    ai.growth.emplace(actor.definition, growth);
    ai.professions.resize(1);
    ai.accounting = rules::PeriodAccounting(1234, 12);
    state.scene.random = rules::WorldRandomStream::from_java_seed(123);
    return state;
}
std::vector<OverlaySprite> sprites(const OverlayPlan &plan) {
    std::vector<OverlaySprite> result;
    for (const auto &entry : plan)
        if (const auto *sprite = std::get_if<OverlaySprite>(&entry))
            result.push_back(*sprite);
    return result;
}
const OverlayRectangle *orange(const OverlayPlan &plan) {
    for (const auto &entry : plan)
        if (const auto *rectangle = std::get_if<OverlayRectangle>(&entry);
            rectangle && rectangle->rgb == std::array<unsigned char, 3>{255, 109, 15})
            return rectangle;
    return nullptr;
}
void damage() {
    auto state = fixture();
    auto &actor = state.scene.world.world.ai.battle.actors.at({1});
    actor.damage_total = 127;
    actor.hit_count = 3;
    actor.label_timer = 16;
    auto out = sprites(world_actor_combat_visuals(state, {1}));
    check(out.size() == 3 && out[0].name == "number07.seb" && out[0].frame == 1 &&
              out[1].frame == 2 && out[2].frame == 7 && out[0].x == -12 && out[1].x == -4 &&
              out[2].x == 4 && out[0].y == -38,
          "Human damage must use accumulated red digits centered on the ordinary body anchor");
    actor.kind = rules::ActorKind::monster;
    out = sprites(world_actor_combat_visuals(state, {1}));
    check(out.size() == 3 && out[0].name == "number06.seb",
          "Monster damage must use the yellow row instead of human red");
    actor.label_timer = 1;
    check(world_actor_combat_visuals(state, {1}).size() == 3,
          "Last source hit-label tick should remain visible");
    actor.label_timer = 0;
    check(world_actor_combat_visuals(state, {1}).empty(), "Expired damage must not remain visible");
    actor.label_timer = 16;
    actor.miss_label = true;
    check(world_actor_combat_visuals(state, {1}).empty(), "MISS must not become a damage number");
    actor.miss_label = false;
    actor.control.flags |= 1U;
    check(world_actor_combat_visuals(state, {1}).empty(),
          "Hidden actor must not leak damage labels");
    actor.control.flags &= ~1U;
    actor.damage_total = 0;
    out = sprites(world_actor_combat_visuals(state, {1}));
    check(out.size() == 1 && out[0].frame == 0, "Zero-damage hit retains its actual zero label");
}
void experience() {
    auto state = fixture();
    auto &ai = state.scene.world.world.ai;
    auto &effect = ai.contexts.at({1}).effects.display;
    auto &growth = ai.growth.at(7);
    effect = {{24, -1, 0, 999, 0, 0}};
    check(world_actor_combat_visuals(state, {1}).empty(), "Delayed EXP must not draw early");
    effect[0][1] = 0;
    auto plan = world_actor_combat_visuals(state, {1});
    auto out = sprites(plan);
    check(out.size() == 4 && out[0].name == "wnd_exp.seb" && out[0].x == -23.5F &&
              out[0].y == -34 && out[1].frame == 14 && out[2].frame == 1 && out[3].frame == 0,
          "EXP uses remaining owner N, not the obsolete original amount stored in cd24");
    check(orange(plan) && orange(plan)->width == 15 && orange(plan)->height == 2 &&
              orange(plan)->x == -15 && orange(plan)->y == -22,
          "EXP bar must show L6 over source threshold12, independently of pending N10");
    growth.pending.counter = 1;
    out = sprites(world_actor_combat_visuals(state, {1}));
    check(out.size() == 3 && out.back().frame == 9,
          "After the growth consumer pays one of N10, the display must retain the remaining nine");
    growth.pending.counter = 8;
    out = sprites(world_actor_combat_visuals(state, {1}));
    check(out.back().frame == 2, "Eighth EXP step has consumed eight of N10, retaining two");
    growth.pending.counter = 9;
    out = sprites(world_actor_combat_visuals(state, {1}));
    check(out.back().frame == 0, "Ninth EXP step must display zero remaining");
    growth.pending.counter = 99;
    check(sprites(world_actor_combat_visuals(state, {1})).back().frame == 0,
          "EXP descending map must clamp its final endpoint");
    effect[0][1] = 54;
    check(!sprites(world_actor_combat_visuals(state, {1})).empty(),
          "EXP label should survive through tick54");
    effect[0][1] = 55;
    plan = world_actor_combat_visuals(state, {1});
    check(sprites(plan).empty() && orange(plan), "Tick55 hides the label but retains its EXP bar");
    effect[0][1] = 71;
    check(orange(world_actor_combat_visuals(state, {1})), "EXP bar should survive through tick71");
    effect[0][1] = 72;
    check(world_actor_combat_visuals(state, {1}).empty(), "Expired cd24 must not render");
    effect[0][1] = 55;
    growth.definition.profession_levels[0] = 10;
    growth.experience = 0;
    plan = world_actor_combat_visuals(state, {1});
    check(orange(plan) && orange(plan)->width == 31,
          "Mastered profession retains full EXP bar even when remaining L is zero");
    effect = {{14, -1}};
    check(world_actor_combat_visuals(state, {1}).empty(), "Delayed level-up should not appear");
    for (const int count : {0, 9, 47}) {
        effect[0][1] = count;
        out = sprites(world_actor_combat_visuals(state, {1}));
        check(out.size() == 1 && out[0].name == "ef_lvUp.seb" && out[0].frame == 0 &&
                  out[0].x == 0 && out[0].y == -23,
              "Real cd14 displays its source static badge without guessing bounce/SEB offsets");
    }
    effect[0][1] = 48;
    check(world_actor_combat_visuals(state, {1}).empty(), "Expired level-up should disappear");
}
void experience_conservation() {
    auto state = fixture();
    auto &ai = state.scene.world.world.ai;
    auto &growth = ai.growth.at(7);
    ai.contexts.at({1}).effects.display = {{24, 12, 0, 999, 0, 0}};
    // Exercise the actual maintained nine-step consumer rather than duplicating the display
    // arithmetic. These are presentation-policy checks, not proof of APK glyph rounding.
    for (const int amount : {1, 6, 9, 10, 17, 100, 9999}) {
        rules::DelayedRewardState pending{amount, -1};
        int paid{};
        for (int step_index = 0; step_index <= 9; ++step_index) {
            const auto step = rules::advance_delayed_reward(pending);
            check(step.has_value(), "Maintained experience consumer rejected visual fixture");
            pending = step->state;
            paid += step->increment;
            growth.pending = pending;
            int shown{};
            for (const auto &sprite : sprites(world_actor_combat_visuals(state, {1})))
                if (sprite.name == "number05.seb" && sprite.frame < 10)
                    shown = shown * 10 + sprite.frame;
            check(shown + paid == amount,
                  "Displayed remainder and actual consumed experience must conserve the award");
        }
    }
}
void pending_level_badge() {
    auto state = fixture();
    auto &ai = state.scene.world.world.ai;
    auto &growth = ai.growth.at(7);
    auto &effects = ai.contexts.at({1}).effects.display;
    growth.notice_pending = true;
    effects = {{24, 0, 0, 10, 0, 0}};
    for (const auto &[count, frame] :
         {std::pair{0, 0}, {5, 0}, {6, 1}, {11, 1}, {12, 0}, {54, 1}, {55, 1}, {71, 1}}) {
        effects.front()[1] = count;
        const auto plan = world_actor_combat_visuals(state, {1});
        const auto out = sprites(plan);
        const auto badge = std::find_if(out.begin(), out.end(), [](const auto &sprite) {
            return sprite.name == "ef_lvUp.seb";
        });
        check(badge != out.end() && badge->frame == frame && badge->x == 0 && badge->y == -23,
              "Pending P badge follows cd24's six-tick halves on the ordinary badge anchor");
        check(orange(plan) && (count < 55 || out.size() == 1),
              "P badge survives label expiry with the actual experience bar");
    }
    for (const int count : {-1, 72}) {
        effects.front()[1] = count;
        check(world_actor_combat_visuals(state, {1}).empty(),
              "Pending badge cannot outlive or precede its cd24 record");
    }
    effects = {{24, 6, 0, 10, 0, 0}};
    growth.notice_pending = false;
    const auto no_pending = sprites(world_actor_combat_visuals(state, {1}));
    check(std::none_of(no_pending.begin(), no_pending.end(),
                       [](const auto &sprite) { return sprite.name == "ef_lvUp.seb"; }),
          "Experience alone does not invent a pending level badge");
    // The actual level-up consumer owns cd14; its frame is independent of pending P.
    effects = {{14, 6}};
    const auto actual = sprites(world_actor_combat_visuals(state, {1}));
    check(actual.size() == 1 && actual.front().frame == 0,
          "Actual cd14 stays frame0 when the removed cd24 would have shown frame1");
    effects = {{24, 6, 0, 10, 0, 0}};
    growth.notice_pending = true;
    ai.battle.actors.at({1}).control.flags |= 1U;
    check(world_actor_combat_visuals(state, {1}).empty(),
          "Hidden actor suppresses the pending badge along with its ordinary overlays");
}
void cash() {
    std::vector<int> facility{2, -10, 120, 140, 300, -4000, 666};
    std::vector<int> death{3, -2, 120, 140, 300, 0, 0};
    check(world_cash_visuals(facility).empty() && world_cash_visuals(death).empty(),
          "Both cash kinds retain their independent negative delays");
    facility[1] = death[1] = 0;
    auto out = sprites(world_cash_visuals(facility));
    check(out.size() == 4 && out[0].frame == 3 && out[0].x == 11.5F && out[0].y == -26 &&
              out.back().frame == 20 && out.back().x == 35.5F,
          "X2 facility income retains source offset, digits and G suffix");
    out = sprites(world_cash_visuals(death));
    check(out.size() == 4 && out[0].x == -16.5F && out[0].y == -39 && out.back().frame == 20 &&
              out.back().x == 7.5F,
          "X3 death income must remain centered at the death point Y-39");
    facility[1] = 1;
    check(sprites(world_cash_visuals(facility))[0].y == -29,
          "Facility upward motion retains source signed integer truncation");
    for (const int count : {6, 19}) {
        facility[1] = death[1] = count;
        check(sprites(world_cash_visuals(facility))[0].y == -36 &&
                  sprites(world_cash_visuals(death))[0].y == -39,
              "Facility and monster cash must not share motion after their start");
    }
    facility[1] = death[1] = 20;
    check(world_cash_visuals(facility).empty() && world_cash_visuals(death).empty(),
          "Both cash text consumers expire at20, not coin-animation23");
    check(world_cash_visuals({4, 0, 120, 140, -320, 29}).empty(),
          "Coin animation must not masquerade as text without its complete movement contract");
    bool rejected{};
    try {
        (void)world_cash_visuals({3, 0, 120, 140, -1, 0, 0});
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "Malformed negative reward must not create a fake display");
}
void immutable_owner() {
    auto state = fixture();
    auto &ai = state.scene.world.world.ai;
    ai.battle.actors.at({1}).damage_total = 42;
    ai.battle.actors.at({1}).label_timer = 16;
    ai.contexts.at({1}).effects.display = {{24, 12, 0, 10, 0, 0}};
    ai.growth.at(7).notice_pending = true;
    state.visual_effects = {{3, 2, 120, 140, 300, 0, 0}};
    const auto before = state;
    for (int rendered_frame = 0; rendered_frame < 100; ++rendered_frame) {
        (void)world_actor_combat_visuals(state, {1});
        (void)world_cash_visuals(state.visual_effects.front());
    }
    const auto &old = before.scene.world.world.ai;
    check(ai.accounting.funds() == 1234 && ai.accounting.entries().empty() &&
              ai.accounting.village_points() == 12 &&
              state.scene.random.draws() == before.scene.random.draws() &&
              ark::test::same_world_clock(state, before) &&
              ai.battle.actors.at({1}).label_timer == 16 &&
              ai.battle.actors.at({1}).damage_total == 42 &&
              ai.growth.at(7).pending.amount == old.growth.at(7).pending.amount &&
              ai.growth.at(7).pending.counter == old.growth.at(7).pending.counter &&
              ai.growth.at(7).experience == old.growth.at(7).experience &&
              ai.growth.at(7).notice_pending == old.growth.at(7).notice_pending &&
              ai.battle.actors.at({1}).hp.target == old.battle.actors.at({1}).hp.target &&
              ai.contexts.at({1}).effects.display == old.contexts.at({1}).effects.display &&
              state.visual_effects == before.visual_effects,
          "Repeated drawing must not pay rewards, draw random or advance source clocks/HP/XP");
}
} // namespace
int main() {
    damage();
    experience();
    experience_conservation();
    pending_level_badge();
    cash();
    immutable_owner();
    std::cout << "PASS world combat visual plans " << checks << " checks\n";
}
