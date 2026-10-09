#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "support/world_fixture.hpp"
#include "support/audio_requests.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
StartupWorldRuntimeState fixture() {
    auto s = test_support::world_fixture(ref::WorldRandomStream::from_java_seed(17));
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
              test_support::audio_ids(s.sound_requests) == std::vector<int>{18} &&
              s.sound_requests.front().operation == StartupAudioOperation::ordinary_play,
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
    check(adapter.nonactors.request(s, r) && test_support::audio_ids(s.sound_requests) == std::vector<int>{14} &&
              s.sound_requests.front().operation == StartupAudioOperation::ordinary_play,
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
    check(next && test_support::audio_ids(next->sound_requests) == std::vector<int>{8} &&
              next->sound_requests.front().operation == StartupAudioOperation::ordinary_play,
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
              next->sound_requests.back().id == 23 &&
              next->sound_requests.back().operation == StartupAudioOperation::ordinary_play,
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
StartupWorldRuntimeState shop_projection_fixture() {
    auto s = fixture();
    s.scene.world.world.actors.emplace(ref::CharacterId{1}, ref::RescueActorContext{});
    s.human_flags.at(0) |= 2U;
    // 条件投影夹具：三种商店覆盖关系，不虚构成自然建设或世界存档资格。
    s.shops[1001] = {1, {{7, 91}}};
    s.shops[1002] = {4, {{3, 6}, {7, 2}}};
    s.shops[1003] = {5, {{7, 9}}};
    s.facility_details[1001].notices = {{7, 4}, {1, 8}, {7, 3}};
    s.facility_details.erase(1002);
    s.facility_details[1003].notices.clear();
    s.shop_order = {1003, 1001, 1002};
    return s;
}
void narrow_shop_projection() {
    auto s = shop_projection_fixture();
    const auto adapter = startup_world_runtime_adapter();
    const auto equal = [](const auto &a, const auto &b) {
        if (a.size() != b.size()) return false;
        auto x = a.begin(), y = b.begin();
        for (; x != a.end(); ++x, ++y)
            if (x->first != y->first || x->second.category != y->second.category ||
                x->second.notices != y->second.notices) return false;
        return true;
    };
    const auto before = startup_world_state_digest(s);
    const auto full = startup_world_runtime_routes(s); // 原完整公开投影，保留独立world/facts处理。
    const auto narrow = adapter.nonactors.read_routes(s);
    check(equal(full.shops, narrow.objects.shops) && narrow.objects.shop_order == s.shop_order,
          "窄商店投影逐字段等于旧完整routes oracle，商店顺序不按ID重排");
    check(narrow.objects.shops.at(1001).notices == std::vector<std::array<int, 2>>{{7, 4}, {1, 8}, {7, 3}} &&
              narrow.objects.shops.at(1002).notices == std::vector<std::array<int, 2>>{{3, 6}, {7, 2}} &&
              narrow.objects.shops.at(1003).notices.empty(),
          "实际details覆盖保留重复种类原序，缺details保留旧notice，空details真正清空");
    check(full.world.actors.at({1}).definition_task_flag &&
              !s.scene.world.world.actors.at({1}).definition_task_flag &&
              startup_world_state_digest(s) == before,
          "完整投影仍写私有任务旗标，窄读不改Owner/随机/目录/实体");
    const auto rejection = [](const auto &read) {
        try { read(); }
        catch (const std::invalid_argument &error) { return std::string(error.what()); }
        return std::string{};
    };
    for (int variant = 0; variant < 2; ++variant) {
        auto bad = s;
        if (variant == 0) bad.human_flags.erase(0);
        else bad.scene.world.world.actors.erase({1});
        const auto saved = startup_world_state_digest(bad);
        const auto old_error = rejection([&] { (void)startup_world_runtime_routes(bad); });
        const auto new_error = rejection([&] { (void)adapter.nonactors.read_routes(bad); });
        check(old_error == "共同人物缺原任务旗标投影" && new_error == old_error &&
                  startup_world_state_digest(bad) == saved,
              "缺human flag或RescueActorContext仍在原投影时点拒绝，旧输入完整不变");
    }
    // 原检查使用world.actors，不能误加强为ai.contexts；非human也不新增该要求。
    auto reward_context_absent = s;
    reward_context_absent.scene.world.world.ai.contexts.erase({1});
    const auto old_without_reward = startup_world_runtime_routes(reward_context_absent);
    const auto new_without_reward = adapter.nonactors.read_routes(reward_context_absent);
    check(equal(old_without_reward.shops, new_without_reward.objects.shops) &&
              !s.scene.world.world.actors.count({2}),
          "保留原拒绝范围，不把reward context或怪物上下文改成新门槛");
}

// 只保留被替换的外层组合，不重写规则算法；固定同一DLL中的完整routes作为旧路径。
ref::WorldNonactorScheduleState full_nonactor_projection(const StartupWorldRuntimeState &s) {
    ref::WorldNonactorScheduleState r{s.scene.world, s.scene.random, {}};
    r.objects.catalog = s.catalog;
    r.objects.shops = startup_world_runtime_routes(s).shops;
    r.objects.shop_order = s.shop_order;
    r.objects.item_rewards = s.item_rewards;
    return r;
}
std::uint64_t projection_checksum(const ref::WorldNonactorScheduleState &p) {
    std::uint64_t hash = 14695981039346656037ULL;
    const auto mix = [&](std::uint64_t value) { hash = (hash ^ value) * 1099511628211ULL; };
    mix(p.common.world.ai.battle.actors.size());
    mix(p.common.world.facilities.size());
    mix(p.common.world.map.cells.size());
    mix(p.common.world.ai.accounting.funds());
    const auto random = p.random.snapshot();
    mix(random.engine_state); mix(random.cursor); mix(random.tape_mode);
    for (auto raw : random.tape) mix(static_cast<std::uint32_t>(raw));
    for (const auto &[id, value] : p.objects.catalog) {
        mix(id.first); mix(id.second); mix(value.flags); mix(value.status);
        mix(value.unlock_counter); mix(value.newly_unlocked); mix(value.inventory); mix(value.free_purchases);
    }
    for (const auto &[id, shop] : p.objects.shops) {
        mix(id); mix(shop.category); mix(shop.notices.size());
        for (const auto &notice : shop.notices) { mix(notice[0]); mix(notice[1]); }
    }
    for (auto id : p.objects.shop_order) mix(id);
    mix(p.objects.item_rewards);
    return hash;
}
int projection_benchmark() {
    using Clock = std::chrono::steady_clock;
    const auto s = shop_projection_fixture();
    const auto adapter = startup_world_runtime_adapter();
    const auto before = startup_world_state_digest(s);
    constexpr int warmup = 64, pairs = 7, iterations = 1000;
    for (int i = 0; i < warmup; ++i)
        check(projection_checksum(full_nonactor_projection(s)) ==
                  projection_checksum(adapter.nonactors.read_routes(s)), "诊断预热两路径checksum相同");
    struct Sample { double milliseconds{}; std::uint64_t checksum{}; };
    const auto measure = [&](bool full) {
        Sample sample;
        const auto start = Clock::now();
        for (int i = 0; i < iterations; ++i) {
            // 构造、使用与销毁都在区间内，两路径消费完全相同的投影字段。
            const auto p = full ? full_nonactor_projection(s) : adapter.nonactors.read_routes(s);
            sample.checksum = (sample.checksum ^ projection_checksum(p) ^ static_cast<std::uint64_t>(i)) *
                              1099511628211ULL;
        }
        sample.milliseconds = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        return sample;
    };
    std::cout << "projection_benchmark fixture=conditional-shop-projection pairs=" << pairs
              << " iterations=" << iterations << " warmup=" << warmup
              << " qualification=microbenchmark-not-natural-game-speed\n";
    std::cout << "pair,order,iterations,full_ms,narrow_ms,full_checksum,narrow_checksum\n";
    std::cout << std::fixed << std::setprecision(6);
    for (int pair = 0; pair < pairs; ++pair) {
        Sample full, narrow;
        if (pair % 2 == 0) { full = measure(true); narrow = measure(false); }
        else { narrow = measure(false); full = measure(true); }
        check(full.checksum == narrow.checksum, "诊断实际消费checksum不一致");
        std::cout << pair + 1 << ',' << (pair % 2 == 0 ? "full-first" : "narrow-first") << ','
                  << iterations << ',' << full.milliseconds << ',' << narrow.milliseconds << ','
                  << full.checksum << ',' << narrow.checksum << '\n';
    }
    check(startup_world_state_digest(s) == before, "诊断不修改单一输入Owner/随机");
    std::cout << "projection_benchmark input_digest=" << before << " unchanged=true\n";
    return 0; // 快慢不影响成功资格，只有行为/checksum或只读性错误才失败。
}
} // namespace
int main(int argc, const char **argv) {
    try {
        if (argc > 1) {
            if (argc == 2 && std::string(argv[1]) == "projection_benchmark") return projection_benchmark();
            throw std::runtime_error("未知nonactor测试模式");
        }
        contact_and_spells();
        hit_object_and_projection();
        actor_presentation_and_frame_cache();
        tail_direction_projection();
        narrow_shop_projection();
        std::cout << "startup_world_runtime_nonactors: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "startup_world_runtime_nonactors: " << error.what() << '\n';
        return 1;
    }
}
