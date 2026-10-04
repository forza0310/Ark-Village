#include "ark/simulation/startup_world_runtime.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
StartupWorldRuntimeState fixture() {
    StartupSession startup;
    StartupWorldRuntimeSession session(startup.state(), ref::WorldRandomStream::from_java_seed(17));
    auto s = session.state();
    // 只为独立表现调用夹具装入两名对象；不宣称原版新局会有这名怪物。
    ref::BattleActorRecord human;
    human.id = {1};
    human.kind = ref::ActorKind::human;
    human.definition = 0;
    human.position = {100, 0, 200};
    ref::BattleActorRecord monster;
    monster.id = {2};
    monster.kind = ref::ActorKind::monster;
    monster.definition = 0;
    monster.position = {200, 0, 200};
    s.scene.world.world.ai.battle.actors.emplace(human.id, human);
    s.scene.world.world.ai.battle.actors.emplace(monster.id, monster);
    s.scene.world.world.ai.contexts.emplace(human.id, ref::RewardActorContext{});
    s.scene.world.world.ai.contexts.emplace(monster.id, ref::RewardActorContext{});
    s.actor_metadata.emplace(human.id, StartupWorldActorMetadata{0, 0, 0, {50, 80}});
    s.actor_metadata.emplace(monster.id, StartupWorldActorMetadata{0, 0, 0, {60, 70}});
    s.actor_metadata.at(human.id).cached_screen_position = {120, 160};
    s.actor_metadata.at(monster.id).cached_screen_position = {130, 170};
    s.shop_actors.emplace(human.id, ref::ShopActorRecord{});
    return s;
}
void contact_and_spells() {
    auto s = fixture();
    const auto adapter = startup_world_runtime_adapter();
    ref::WorldNonactorRequest r;
    r.kind = ref::WorldNonactorRequestKind::projectile_contact;
    r.identity = 77;
    r.caster = ref::CharacterId{1};
    r.target = ref::CharacterId{2};
    r.source_position = ref::CombatPoint{100, 12, 200};
    auto fields = adapter.nonactors.request(s, r);
    check(fields && fields->target_effects &&
              fields->target_effects->display ==
                  std::vector<ref::ActorEffectRecord>{{10, 0, 114, 13, 0}},
          "arrow contact uses target edge20, raw projection, y10 and target kind");
    check(s.visual_effects.empty() && s.sound_requests.empty(),
          "contact does not create global20 or cast sound");
    r.source_position = s.scene.world.world.ai.battle.actors.at({2}).position;
    fields = adapter.nonactors.request(s, r);
    check(fields && fields->target_effects->display[0] == ref::ActorEffectRecord{10, 0, 0, 10, 0},
          "coincident source reproduces Java NaN integer cast0 without C++ undefined cast");
    r.kind = ref::WorldNonactorRequestKind::projectile_visual;
    r.visual = 5;
    r.source_position = ref::CombatPoint{100, 25, 200};
    s.scene.world.world.ai.contexts.at({2}).effects.display = {{10, 0, 1, 2, 0}};
    fields = adapter.nonactors.request(s, r);
    check(fields && fields->target_effects &&
              fields->target_effects->display ==
                  std::vector<ref::ActorEffectRecord>{{16, 0, 5, 105, 7}, {10, 0, 1, 2, 0}} &&
              s.sound_requests == std::vector<int>{18},
          "spell5 midpoint has height0, inserts cd16 first and emits source18");
    r.visual = 8;
    fields = adapter.nonactors.request(s, r);
    check(fields &&
              fields->target_effects->delayed ==
                  std::vector<ref::ActorEffectRecord>{
                      {5, 0, 105, 7}, {5, 3, 100, 12}, {5, 6, 110, 15}} &&
              s.sound_requests.size() == 1,
          "spell8 schedules exact ce5 delay0/3/6 without eager sounds");
    r.target.reset();
    check(!adapter.nonactors.request(s, r), "missing spell target rejects typed consumption");
    r.visual = 22;
    fields = adapter.nonactors.request(s, r);
    check(fields && s.visual_effects.back() == ref::ActorEffectRecord{22, 0, 90, 40, 0},
          "spell ground22 is global X, uses supplied original projectile position");
}
void hit_object_and_projection() {
    auto s = fixture();
    const auto adapter = startup_world_runtime_adapter();
    ref::WorldNonactorRequest r;
    r.kind = ref::WorldNonactorRequestKind::projectile_hit;
    r.identity = 77;
    r.caster = ref::CharacterId{1};
    r.target = ref::CharacterId{2};
    r.hit = ref::HitRequest{ref::HitRequestKind::face_attacker, 0};
    const auto fields = adapter.nonactors.request(s, r);
    check(fields && fields->target_facing == 3 &&
              s.scene.world.world.ai.battle.actors.at({2}).control.facing == 0,
          "miss facing reads cached u not n and returns constrained typed writeback");
    check(startup_world_actor_direction(s, {1}, {2}) == 1 &&
              startup_world_actor_direction(s, {2}, {1}) == 3,
          "facing resolves ordered self/target cached u; attacker and miss victim use opposite "
          "order");
    r.hit = ref::HitRequest{ref::HitRequestKind::attack_sound, 14};
    check(adapter.nonactors.request(s, r) && s.sound_requests == std::vector<int>{14},
          "successful attack sound retained for presentation consumption");
    r.kind = ref::WorldNonactorRequestKind::object;
    r.hit.reset();
    r.object = ref::ObjectCommitRequest{ref::ObjectCommitRequestKind::ground_effect, 0, 0, {}};
    r.source_position = ref::CombatPoint{100, 0, 200};
    check(adapter.nonactors.request(s, r) &&
              s.visual_effects.back() == ref::ActorEffectRecord{12, 0, 90, 15, 0},
          "ground pickup uses actual global12 not generic contact20");
    ref::GroundObjectState object;
    object.id = {77};
    object.kind = 0;
    object.definition = 0;
    s.scene.world.world.ai.battle.objects.emplace(77, object);
    r.object = ref::ObjectCommitRequest{ref::ObjectCommitRequestKind::notice, 2, 0, {}};
    const auto old_inventory = s.catalog.at({0, 0}).inventory;
    check(adapter.nonactors.request(s, r) && s.scripts.notices.back().counter == -10 &&
              s.scripts.notices.back().replacement == s.rules->items.at(0).name &&
              s.catalog.at({0, 0}).inventory == old_inventory,
          "pickup hint uses original item name/delay and does not regrant inventory");
    s.camera = {90, 15};
    check(startup_world_view_projection(s, {100, 0, 200}) == ref::Position{120, 160} &&
              startup_world_actor_visible(s, {1}) == true,
          "240x320 reference viewport projection and actual actor bounds");
    check(update_startup_world_render_cache(s) &&
              s.actor_metadata.at({1}).render_position.x == 100 &&
              startup_world_hit_sound_visible(s, {1}) == true,
          "render cache writes o only after a complete frame and sound reads retained o");
    s.scene.world.world.ai.battle.actors.at({1}).control.flags |= 1U;
    check(startup_world_actor_visible(s, {1}) == false,
          "hidden bit1 suppresses renderer eligibility");
    check(startup_world_hit_sound_visible(s, {1}) == true,
          "hidden source does not erase previously rendered o for hit sound eligibility");
    check(!startup_world_actor_visible(s, {999}), "missing actor is not guessed visible");
    const auto drops = startup_world_drop_selection(s, {1});
    check(drops && drops->definitions.size() == s.rules->items.size() + s.rules->equipment.size() &&
              s.scene.random.draws() == 0,
          "drop catalogue uses all source definitions without eager random tickets");
    ref::ProjectileState projectile;
    projectile.kind = ref::ProjectileKind::arrow;
    projectile.caster = {1};
    projectile.original_target = {2};
    s.scene.world.world.ai.projectiles.emplace(77, projectile);
    const auto input = adapter.nonactors.projectile(s, 77);
    check(input && input->box && input->box->x_offset == -30 && input->box->z_offset == 20 &&
              input->monster_boxes[3]->width == 40 && input->caster_visible &&
              input->drop_selection,
          "arrow provider uses n.a(0,8) and body3 n.a(1,6), current visibility and catalogue");
}
void actor_presentation_and_frame_cache() {
    auto s = fixture();
    s.camera = {90, 15};
    const auto adapter = startup_world_runtime_adapter();
    ref::WorldActorPresentationRequest r;
    r.actor = {1};
    r.attack = ref::WorldAttackRequest{ref::WorldAttackVisual::spell_source, {1}, {}, 0};
    auto next = adapter.actors.presentation(s, r);
    check(next && next->scene.world.world.ai.contexts.at({1}).effects.display.back() ==
                      ref::ActorEffectRecord{4, 0, 50, 79},
          "spell preparation uses cached u.y-1 at the actual command point");
    r.attack = ref::WorldAttackRequest{
        ref::WorldAttackVisual::healing_target, {1}, ref::CharacterId{2}, 0};
    r.target = ref::CharacterId{2};
    next = adapter.actors.presentation(s, r);
    check(next &&
              next->scene.world.world.ai.contexts.at({1}).effects.display.back() ==
                  ref::ActorEffectRecord{6, 0, 60, 70} &&
              next->scene.world.world.ai.contexts.at({2}).effects.display.empty(),
          "heal target effect belongs to caster cd at target old u, not target cd");
    r.attack = ref::WorldAttackRequest{ref::WorldAttackVisual::telegraph, {2}, {}, 23};
    r.actor = {2};
    r.target.reset();
    next = adapter.actors.presentation(s, r);
    check(next && next->scene.world.world.ai.contexts.at({2}).effects.display.back() ==
                      ref::ActorEffectRecord{23, 0, 60, 70},
          "monster telegraph preserves original cd23 payload");
    r.actor = {1};
    r.attack.reset();
    r.shop = ref::ShopWorldRequest{ref::ShopWorldRequestKind::cash_display, 300, 0, {}};
    s.scene.world.world.ai.contexts.at({1}).cell = {3, 4};
    const auto cash_before = s.scene.world.world.ai.accounting.funds();
    next = adapter.actors.presentation(s, r);
    check(next &&
              next->visual_effects.back() ==
                  ref::ActorEffectRecord{2, -10, 210, 15, 300, -4000, 666} &&
              next->scene.world.world.ai.accounting.funds() == cash_before,
          "facility income display retains seven original curve fields without crediting twice");
    r.shop = ref::ShopWorldRequest{ref::ShopWorldRequestKind::delivered_item_notice, 0, 2, {}};
    next = adapter.actors.presentation(s, r);
    check(next && next->scripts.notices.back().counter == -1 &&
              next->scripts.notices.back().replacement == s.rules->items.at(0).name,
          "delivered inventory notice has source delay1, distinct from ground pickup delay10");
    r.shop.reset();
    r.sound = ref::WorldMiscSoundRequest{{1}, 8, {120, 160}};
    next = adapter.actors.presentation(s, r);
    check(next && next->sound_requests == std::vector<int>{8},
          "front33 presentation sound consumed before same-FIFO continuation");
    auto &monster = s.scene.world.world.ai.battle.actors.at({2});
    monster.control.action = 3;
    monster.attack_position = {100, 8, 300};
    s.actor_metadata.at({2}).render_position.height = 2;
    check(update_startup_world_render_cache(s) &&
              s.actor_metadata.at({2}).render_position.x == 100 &&
              s.actor_metadata.at({2}).render_position.z == 300 &&
              s.actor_metadata.at({2}).render_position.height == 2,
          "visible monster E uses au.xz but preserves o.height");
    monster.physics_pause = 1;
    monster.position.height = 12;
    check(update_startup_world_render_cache(s) &&
              s.actor_metadata.at({2}).render_position.height == 12,
          "P positive render copies full n including height after E projection");
    monster.control.flags |= 1U;
    monster.position = {300, 0, 200};
    check(update_startup_world_render_cache(s) &&
              s.actor_metadata.at({2}).render_position.x == 200 &&
              s.actor_metadata.at({2}).render_position.height == 12,
          "hidden render returns before o refresh, retaining previous frame cache");
    const auto before_sounds = s.sound_requests.size();
    s.actor_metadata.at({1}).cached_screen_position = {-100, -100};
    check(
        emit_startup_world_actor_sound(s, {1}, 17) && s.sound_requests.size() == before_sounds,
        "spatial n.a(sound,bm) clips using retained screen coordinate without pretending failure");
    r = {};
    r.actor = {2};
    r.definition = 0;
    r.lifecycle = ref::LifecycleRequest{ref::LifecycleRequestKind::normal_death_rewards, 0};
    const int reward = s.scene.world.world.ai.battle.monsters.at(0).statF;
    next = adapter.actors.presentation(s, r);
    check(next &&
              next->visual_effects ==
                  std::vector<ref::ActorEffectRecord>{
                      {20, 0, 60, 70}, {3, -2, 60, 70, reward, 0, 0}, {4, -2, 60, 70, -320, 29}} &&
              next->sound_requests.back() == 23,
          "normal death retains exact X20/X3/X4 source order and sound23, no duplicate cash grant");
    r.lifecycle = ref::LifecycleRequest{ref::LifecycleRequestKind::cancelled_death_effect, 0};
    next = adapter.actors.presentation(s, r);
    check(next && next->visual_effects == std::vector<ref::ActorEffectRecord>{{21, 0, 60, 70}} &&
              next->sound_requests == s.sound_requests,
          "cancelled death only creates global21 with no normal death reward sound");
    r.lifecycle = ref::LifecycleRequest{ref::LifecycleRequestKind::landing_effect, 18};
    next = adapter.actors.presentation(s, r);
    check(next &&
              next->scene.world.world.ai.contexts.at({2}).effects.display.back() ==
                  ref::ActorEffectRecord{18, 0, 60, 70} &&
              next->visual_effects.empty(),
          "special entry landing creates actor cd18 not global20");
}
void tail_direction_projection() {
    auto s = fixture();
    const auto adapter = startup_world_runtime_adapter();
    auto actor = s.scene.world.world.ai.battle.actors.at({1});
    actor.control.facing = 3;
    actor.position.height = 80.F;
    // 新n包含高度，整数投影为(90,95)；两个轴均严格变化才覆盖朝向。
    for (const auto entry : std::vector<std::array<int, 3>>{{89, 94, 0},
                                                            {89, 96, 1},
                                                            {91, 96, 2},
                                                            {91, 94, 3},
                                                            {90, 94, 3},
                                                            {90, 96, 3},
                                                            {89, 95, 3},
                                                            {91, 95, 3},
                                                            {90, 95, 3}}) {
        s.actor_metadata.at({1}).cached_view = {entry[0], entry[1]};
        const auto direction = adapter.actors.projected_facing(s, {1}, actor);
        check(direction && *direction == entry[2] &&
                  s.actor_metadata.at({1}).cached_view == ref::Position{entry[0], entry[1]},
              "tail uses new position including height and old u without publishing cache early");
    }
    check(!adapter.actors.projected_facing(s, {999}, actor),
          "tail direction refuses missing source cache instead of guessing initial facing");
    const auto published = adapter.actors.tail_cache(s, {1}, actor);
    check(published && published->actor_metadata.at({1}).cached_view == ref::Position{90, 95} &&
              published->scene.world.world.ai.battle.actors.at({1}).position.height == 0,
          "published u uses retained physics snapshot, not current n after r cleared height");
}
} // namespace
int main() {
    try {
        contact_and_spells();
        hit_object_and_projection();
        actor_presentation_and_frame_cache();
        tail_direction_projection();
        std::cout << "startup_world_runtime_nonactors: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "startup_world_runtime_nonactors: " << error.what() << '\n';
        return 1;
    }
}
