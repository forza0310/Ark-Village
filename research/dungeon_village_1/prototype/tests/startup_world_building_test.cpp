#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_editing.hpp"
#include "dungeon_village_prototype/startup_world_expansion.hpp"
#include "dungeon_village_prototype/startup_world_facility_items.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_inheritance.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
void new_world_inheritance() {
    auto s = test_support::world_fixture();
    const auto baseline = startup_world_state_digest(s);
    std::array<std::vector<std::uint8_t>, 2> sections;
    check(install_startup_world_inheritance(s, sections) && startup_world_state_digest(s) == baseline,
          "empty source inheritance sections preserve exact default world");
    sections[0].resize(s.rules->facilities.size() * 2);
    sections[1].resize(s.rules->jobs.size() * 2);
    for (std::size_t n = 1; n < sections[0].size(); n += 2)
        sections[0][n] = 5;
    for (std::size_t n = 1; n < sections[1].size(); n += 2)
        sections[1][n] = 1;
    const auto before = s;
    check(install_startup_world_inheritance(s, sections), "full big-endian G then p inheritance installs privately");
    for (const auto &d : s.rules->facilities) {
        const auto values = startup_world_build_quote(s, d.id);
        check(values && s.scene.world.world.facility_uses.at(d.id).level == 5 &&
                  s.scripts.facilities.at(d.id).level == 5,
              "inheritance writes every stable shared definition G");
        for (std::size_t slot = 0; slot < 4; ++slot)
            check(s.scripts.facilities.at(d.id).attributes[slot] == values->definition_attributes[slot],
                  "inheritance actual economic setter rebuilds all shared attributes");
    }
    bool price_changed{};
    for (const auto &[id, f] : s.scene.world.world.facilities) {
        const auto actual = startup_world_facility_values(s, id);
        check(actual && f.price == actual->instance_attributes[0],
              "inheritance rebuilds each existing instance price with its own neighbours");
        price_changed = price_changed || f.price != before.scene.world.world.facilities.at(id).price;
    }
    check(price_changed && s.scene.random.draws() == before.scene.random.draws() &&
              s.scene.world.world.ai.accounting.funds() == before.scene.world.world.ai.accounting.funds() &&
              s.scene.world.world.ai.battle.actors.empty() && s.sound_requests.empty(),
          "real inherited economy changes price without arrival cash random or unconsumed output");
    for (std::size_t n = 0; n < s.rules->jobs.size(); ++n)
        check(s.scripts.professions.at(static_cast<int>(n)).status == 1 &&
                  s.scene.world.world.ai.professions[n].unlocked,
              "inheritance p opens profession definition not human profession level");
    for (int variant = 0; variant < 6; ++variant) {
        auto candidate = before;
        auto bad = sections;
        if (variant == 0) bad[0].pop_back();
        if (variant == 1) bad[1].push_back(0);
        if (variant == 2) bad[0][1] = 0;
        if (variant == 3) bad[0][0] = 0xff; // 大端负short，非误解为小端。
        if (variant == 4) bad[1].back() = 2; // 后段坏p不能留下前段G变化。
        if (variant == 5) candidate.neighbourhood.erase(candidate.scene.world.facility_order.front());
        const auto digest = startup_world_state_digest(candidate);
        check(!install_startup_world_inheritance(candidate, bad) &&
                  startup_world_state_digest(candidate) == digest,
              "bad inheritance length value or late reference rolls back every candidate field");
    }
    auto running = before;
    running.simulation_steps = 1;
    const auto digest = startup_world_state_digest(running);
    check(!install_startup_world_inheritance(running, sections) && startup_world_state_digest(running) == digest,
          "inherited system bytes cannot overwrite running world");
}
ref::Position empty_anchor(const StartupWorldRuntimeState &s, int definition,
                           ref::FacilityOrientation orientation = ref::FacilityOrientation::first) {
    const auto &d = s.rules->facilities.at(definition);
    const auto &map = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
            const auto f = ref::facility_footprint(static_cast<ref::FacilityShape>(d.shape),
                                                   orientation, {x, y}, map.width, map.height);
            if (f.error != ref::GeometryError::none)
                continue;
            if (std::all_of(f.cells.begin(), f.cells.end(), [&](const auto &c) {
                    return c.position.x > bounds[0].x && c.position.x < bounds[1].x &&
                           c.position.y > bounds[1].y && c.position.y < bounds[0].y &&
                           !map.cells.at(c.position.y * map.width + c.position.x).facility;
                }))
                return {x, y};
        }
    throw std::runtime_error("no legal source footprint");
}
std::uint64_t source_facility(const StartupWorldRuntimeState &s, int definition) {
    const auto found = std::find_if(
        s.scene.world.facility_order.begin(), s.scene.world.facility_order.end(), [&](auto id) {
            return s.scene.world.world.facilities.at(id).placement.definition_id == definition;
        });
    check(found != s.scene.world.facility_order.end(),
          "requested source facility exists in actual initial map");
    return *found;
}
void normal_construction() {
    auto s = test_support::world_fixture();
    const auto groups = startup_world_build_catalog(s);
    check(groups && groups->at(1) == std::vector<int>({28, 30, 31, 45}) &&
              groups->at(2) == std::vector<int>({33, 35}) &&
              groups->at(0) == std::vector<int>({24, 66}),
          "actual initial catalogue includes recruitment, excludes road/private housing, preserves "
          "source groups");
    const auto count = s.scene.world.facility_order.size();
    const auto funds = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    const auto anchor = empty_anchor(s, 28);
    check(begin_startup_world_build(s, 29).denial == StartupBuildDenial::unavailable &&
              s.scene.scene_state == 0 && !s.build_definition,
          "locked actual pair inn cannot enter mode or mutate selection");
    check(begin_startup_world_build(s, 28).error == StartupWorldRuntimeError::none &&
              s.scene.scene_state == 1 && s.build_definition == 28,
          "affordable actual inn enters source state1 without upfront payment");
    check(confirm_startup_world_build(s, {-1, 0}, ref::FacilityOrientation::first).denial ==
                  StartupBuildDenial::outside_map &&
              s.scene.world.facility_order.size() == count &&
              s.scene.world.world.ai.accounting.funds() == funds,
          "map rejection leaves placement and cash unchanged");
    const auto built = confirm_startup_world_build(s, anchor, ref::FacilityOrientation::first);
    if (!built.created)
        throw std::runtime_error(
            "construction commit error=" + std::to_string(static_cast<int>(built.error)) +
            " denial=" + std::to_string(static_cast<int>(built.denial)) +
            " anchor=" + std::to_string(anchor.x) + "," + std::to_string(anchor.y));
    check(built.error == StartupWorldRuntimeError::none && built.created &&
              s.scene.world.world.ai.accounting.funds() == funds - 1000 &&
              s.monthly_cash.at(3)[0][1] == 1000 && s.scene.random.draws() == draws &&
              s.scene.scene_state == 1,
          "actual quote pays global construction1000 once, no random; remains mode1");
    const auto id = *built.created;
    check(s.facility_residents.at(id) == -1 && s.facility_difficulties.at(id) == 0 &&
              s.facility_month_age.at(id) == 0 && s.facility_monthly_cash.at(id)[3][1] == 0 &&
              s.facility_details.at(id).construction_limit == 280 &&
              s.scene.world.world.facilities.at(id).status == 0 &&
              s.sites.at(id).occupied_cells.size() == 1,
          "all auxiliary records and actual carpenter-adjusted construction280 initialized");
    check(confirm_startup_world_build(s, anchor, ref::FacilityOrientation::first).denial ==
                  StartupBuildDenial::occupied &&
              s.scene.world.world.ai.accounting.funds() == funds - 1000,
          "second click revalidates occupancy and cannot charge twice");
    const auto paused_build = prepare_startup_world_runtime(s);
    check(paused_build.candidate &&
              paused_build.candidate->dungeon_facilities.at(id).updates == 0 &&
              paused_build.candidate->scene.calendar.units == 0 &&
              paused_build.candidate->scene.world.world.ai.battle.actors.empty(),
          "build mode updates feedback only; no construction, calendar or actor creation");
    check(cancel_startup_world_build(s) == StartupWorldRuntimeError::none &&
              s.scene.scene_state == 0 && !s.build_definition &&
              s.scene.world.world.facilities.count(id),
          "cancel leaves accepted building, returns state0 without refund");
    const auto adapter = startup_world_runtime_adapter();
    for (int n = 0; n < 280; ++n) {
        auto candidate =
            ref::prepare_world_facility_update(adapter.facilities.read(s), id, adapter.catalog);
        check(candidate.candidate && adapter.facilities.write(s, candidate.candidate->state),
              "source actual facility consumer progresses construction on qualified common calls");
        check(s.scene.world.world.facilities.at(id).status == (n == 279 ? 1 : 0),
              "construction readiness occurs at original threshold, not advance or delayed extra "
              "tick");
    }
    // o.g先初始化N20；首次完成拷入m.m并排人气，再把定义N减半为10，尚未月更。
    check(s.scene.world.world.ai.accounting.funds() == funds - 1000 &&
              s.facility_definitions.at(28).popularity_reward == 10 &&
              s.facility_details.at(id).completion_popularity == 20 &&
              !s.scene.world.popularity_queue.empty() &&
              s.scene.world.popularity_queue.front() == std::array<int, 3>{25, 20, 1},
          "first-month construction never recharges and queues real sharedN20 popularity before halving to10");
}
// 高星领取后的条件夹具：只准备p/H，不伪造原表flags或自然升星历史。
// 完工必须经过真实Owner建设和设施消费者，才能开放学校／博物馆活动。
void progression_building_unlocks() {
    for (const auto scenario : {std::array<int, 3>{63, 7, 73}, {64, 21, 75}}) {
        const int definition = scenario[0], activity = scenario[1], event = scenario[2];
        auto s = test_support::world_fixture();
        check(s.facility_presence.at(definition) == 0 &&
                  s.scripts.activities.at(activity).status == 0,
              "real new-world school and museum definitions start locked");
        s.facility_presence.at(definition) = 2;
        s.facility_free_builds.at(definition) = 1;
        const auto funds = s.scene.world.world.ai.accounting.funds();
        const auto draws = s.scene.random.draws();
        const auto count = s.scene.world.facility_order.size();
        const auto quote = startup_world_build_quote(s, definition);
        check(quote && quote->construction_cost > 0 &&
                  (definition != 64 || quote->construction_cost == 3000),
              "actual museum still costs3000G after its H entitlement, independent of200 points");
        check(begin_startup_world_build(s, definition).denial == StartupBuildDenial::none,
              "real redeemed school or museum enters ordinary construction");
        const auto built = confirm_startup_world_build(
            s, empty_anchor(s, definition), ref::FacilityOrientation::first);
        check(built.created && s.scene.world.facility_order.size() == count + 1 &&
                  s.scene.world.world.ai.accounting.funds() == funds - quote->construction_cost &&
                  s.scripts.activities.at(activity).status == 0,
              "Owner pays real construction gold and creates one unfinished instance, no early activity");
        const auto id = *built.created;
        check(cancel_startup_world_build(s) == StartupWorldRuntimeError::none,
              "accepted construction leaves build mode through normal cancellation");
        const auto limit = s.facility_details.at(id).construction_limit;
        check(limit > 0 && limit <= 2000, "real source construction has a bounded functional fixture");
        const auto adapter = startup_world_runtime_adapter();
        for (int tick = 0; tick < limit; ++tick) {
            const auto prepared = ref::prepare_world_facility_update(
                adapter.facilities.read(s), id, adapter.catalog);
            check(prepared.candidate && adapter.facilities.write(s, prepared.candidate->state),
                  "actual school or museum facility step writes through common Owner");
            check(s.scene.world.world.facilities.at(id).status == (tick + 1 == limit ? 1 : 0) &&
                      s.scripts.activities.at(activity).status == (tick + 1 == limit ? 1 : 0),
                  "source construction threshold alone opens matching activity, never a preceding tick");
        }
        check(s.scripts.event_calls.at(event) == 1 &&
                  s.scripts.activities.at(activity).pending_notice &&
                  s.activity_counts.at(activity) == 0 && s.events_held == 0 &&
                  s.scene.world.world.ai.accounting.funds() == funds - quote->construction_cost &&
                  s.scene.random.draws() == draws,
              "first real completion opens definition only, without holding activity or extra payment/random");
        if (definition == 64)
            check(s.scripts.event_calls.at(215) == 1,
                  "real museum flags additionally execute original completion news215");
        const auto events = s.scripts.event_calls;
        const auto repeated = ref::prepare_world_facility_update(
            adapter.facilities.read(s), id, adapter.catalog);
        check(repeated.candidate && adapter.facilities.write(s, repeated.candidate->state) &&
                  s.scripts.event_calls == events && s.scene.world.facility_order.size() == count + 1,
              "completed instance neither repeats first scripts nor creates another entity");
    }
}
void multi_tile_and_rollback() {
    for (const auto orientation :
         {ref::FacilityOrientation::first, ref::FacilityOrientation::second}) {
        auto s = test_support::world_fixture();
        s.facility_presence.at(49) = 1; // 杂货铺原bit4/双格；两格旅店29没有bit4，不能伪造建设资格。
        const auto p = empty_anchor(s, 49, orientation);
        check(begin_startup_world_build(s, 49).denial == StartupBuildDenial::none,
              "unlocked actual pair definition selected");
        auto broken = s;
        broken.base_variants.pop_back();
        const auto next_id = broken.next_facility_identity;
        const auto funds = broken.scene.world.world.ai.accounting.funds();
        const auto count = broken.scene.world.facility_order.size();
        check(confirm_startup_world_build(broken, p, orientation).error ==
                      StartupWorldRuntimeError::missing_source &&
                  broken.next_facility_identity == next_id &&
                  broken.scene.world.facility_order.size() == count &&
                  broken.scene.world.world.ai.accounting.funds() == funds &&
                  broken.scene.random.draws() == 0,
              "late refresh failure rolls back identity, all footprint and auxiliary state, cash "
              "and RNG");
        const auto r = confirm_startup_world_build(s, p, orientation);
        check(r.created && s.sites.at(*r.created).occupied_cells.size() == 2,
              "both actual pair orientations install all cells");
        for (const auto cell : s.sites.at(*r.created).occupied_cells)
            check(s.scene.world.world.map.cells.at(cell.y * 24 + cell.x)
                          .facility->instance_id.value == *r.created,
                  "each pair tile binds same stable instance");
    }
}
void new_shop_projection() {
    auto s = test_support::world_fixture();
    const auto old = s.shop_order;
    check(begin_startup_world_build(s, 30).denial == StartupBuildDenial::none,
          "real initial weapon shop is an affordable ordinary construction");
    const auto r =
        confirm_startup_world_build(s, empty_anchor(s, 30), ref::FacilityOrientation::first);
    check(r.created && s.shop_order.size() == old.size() + 1 && s.shop_order.back() == *r.created &&
              s.shops.at(*r.created).category == 1 && s.shops.at(*r.created).notices.empty(),
          "new weapon shop installs exact category1 and original order for object/shop consumers");
    const auto route = startup_world_runtime_routes(s);
    check(route.shops.count(*r.created) && route.shop_order == s.shop_order,
          "current route projection includes player-built shop without a second owner");
}
void details() {
    auto s = test_support::world_fixture();
    const auto id = s.scene.world.facility_order.at(0);
    const auto camera = s.camera;
    const auto draws = s.scene.random.draws();
    check(open_startup_world_facility_page(s, id) == StartupWorldRuntimeError::none,
          "actual seeded ordinary facility opens bound raw74");
    const auto page = s.scripts.pages.back().id;
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->scene.world.updates == 0 &&
              tick.candidate->scene.random.draws() == draws &&
              tick.candidate->scene.calendar.units == 0 && tick.candidate->camera == camera,
          "raw74 freezes world/date/random without silently pausing or moving camera");
    s = *tick.candidate;
    check(act_startup_world_facility_page(s, page, StartupFacilityPageAction::next) ==
                  StartupWorldRuntimeError::none &&
              s.page_phases.at(page) == 1,
          "ordinary live instance has source second page");
    check(act_startup_world_facility_page(s, page, StartupFacilityPageAction::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle != 4,
          "second-page confirm is original no-op, not fake close");
    check(act_startup_world_facility_page(s, page, StartupFacilityPageAction::cancel) ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 && s.camera == camera &&
              !s.scene.framework_paused,
          "source return closes only bound page preserving camera and explicit pause");
    check(act_startup_world_facility_page(s, page, StartupFacilityPageAction::cancel) ==
              StartupWorldRuntimeError::invalid_page,
          "stale double close rejected");
}
void facility_bonus_rows() {
    auto s = test_support::world_fixture();
    const auto target = source_facility(s, 33), shop = source_facility(s, 30);
    const auto tree1 = install_startup_world_facility(s, 71, empty_anchor(s, 71),
                                                      ref::FacilityOrientation::first);
    const auto tree2 = install_startup_world_facility(s, 71, empty_anchor(s, 71),
                                                      ref::FacilityOrientation::first);
    check(tree1.created && tree2.created, "bonus row fixture creates two distinct real source instances");
    check(open_startup_world_facility_page(s, target) == StartupWorldRuntimeError::none,
          "bonus rows use an actual bound74 page");
    const auto page = s.scripts.pages.back().id;
    // 明确页面调用点夹具：模拟曾建到10/11且其它实例已退休的合法序号，不称自然建设路径。
    s.facility_ordinals.at(*tree1.created) = 9;
    s.facility_ordinals.at(*tree2.created) = 10;
    s.facility_page_neighbours.at(page) = {
        {{*tree1.created},71}, {{shop},30}, {{*tree2.created},71}, {{*tree1.created},71}};
    const auto digest = startup_world_state_digest(s);
    const auto rows = startup_world_facility_bonus_rows(s, page);
    check(rows && rows->size() == 4 && rows->at(0).instance == *tree1.created &&
              rows->at(1).instance == shop && rows->at(2).instance == *tree2.created &&
              rows->at(3).instance == *tree1.created,
          "74 Y source order, same-definition identities and repeated page rows survive projection");
    check(rows->at(0).name == "槙树10" && rows->at(2).name == "槙树11" &&
              rows->at(0).ordinal == 9 && rows->at(0).icon == 6,
          "original name suffix is per-definition ordinal plus1, never global ID or shared level");
    check(rows->at(0).values.size() == 2 && rows->at(0).values[0].attribute == 0 &&
              rows->at(0).values[0].label == "价格" && rows->at(0).values[0].value == 20 &&
              rows->at(0).values[0].text == "+20" && rows->at(0).values[1].attribute == 1 &&
              rows->at(0).values[1].label == "品质" && rows->at(0).values[1].value == 25 &&
              rows->at(1).values.size() == 1 && rows->at(1).values[0].attribute == 2 &&
              rows->at(1).values[0].label == "魅力" && rows->at(1).values[0].text == "+10",
          "literal tenant71 y20/25 and tenant30 y10 produce exact labels/raw signed values");
    check(startup_world_state_digest(s) == digest,
          "bonus projection preserves complete Owner, neighbour totals, pages, ordinal, random and cash");
    auto scroll = s;
    for (int n = 0; n < 3; ++n)
        scroll.facility_page_neighbours.at(page).push_back({{shop},30});
    const auto scroll_digest = startup_world_state_digest(scroll);
    const auto first = startup_world_facility_bonus_window(scroll,page,0);
    const auto last = startup_world_facility_bonus_window(scroll,page,2);
    check(first && last && first->total == 7 && first->rows.size() == 5 &&
              last->first == 2 && last->rows.size() == 5 && last->rows[0].name == "槙树11" &&
              last->rows[1].name == "槙树10" && last->rows[4].definition == 30 &&
              !startup_world_facility_bonus_window(scroll,page,3) &&
              startup_world_state_digest(scroll) == scroll_digest,
          "five-row first/last views preserve repeated order, reject excess scroll and remain read-only");
    StartupWorldRules private_rules = *s.rules;
    auto conditions = s;
    conditions.rules = &private_rules;
    // 规则槽重复／顺序变化不授权重排原显示固定y位置；负数/零为条件夹具。
    private_rules.facilities[71].neighbour_effects = {{2,-3},{2,0}};
    conditions.neighbourhood.at(target) = {1234,5678,9012};
    const auto unusual = startup_world_facility_bonus_rows(conditions, page);
    check(unusual && unusual->at(0).values[0].attribute == 0 &&
              unusual->at(0).values[0].text == "+-3" &&
              unusual->at(0).values[1].attribute == 1 && unusual->at(0).values[1].text == "+0",
          "fixed display slots preserve literal plus/signed-zero and never split aggregate modifiers");
    conditions.facility_page_neighbours.at(page).clear();
    check(startup_world_facility_bonus_rows(conditions, page)->empty(),
          "empty Y stays empty even with nonzero aggregate road or other modifiers");
    for (int fault = 0; fault < 8; ++fault) {
        auto broken = s;
        auto rules = *s.rules;
        broken.rules = &rules;
        if (fault == 0) broken.facility_ordinals.erase(*tree1.created);
        if (fault == 1) broken.facility_ordinals.at(*tree1.created) = -1;
        if (fault == 2) broken.facility_ordinals.at(*tree1.created) = std::numeric_limits<int>::max();
        if (fault == 3) broken.scene.world.world.facilities.erase(*tree1.created);
        if (fault == 4) broken.facility_page_neighbours.at(page)[0].definition_id = 30;
        if (fault == 5) rules.facilities[71].neighbour_effects.resize(1);
        if (fault == 6) rules.facilities[71].legacy_icon = 7;
        if (fault == 7) broken.page_phases.erase(page);
        const auto before = startup_world_state_digest(broken);
        check(!startup_world_facility_bonus_rows(broken, page) &&
                  startup_world_state_digest(broken) == before,
              "bad source identity, ordinal, payload, icon or initialized page explicitly refuses without mutation");
    }
    auto closed = s;
    closed.scripts.pages.back().lifecycle = 4;
    check(!startup_world_facility_bonus_rows(closed,page) &&
              !startup_world_facility_bonus_rows(s, std::numeric_limits<std::uint64_t>::max()),
          "closed or unknown74 cannot render stale bonus rows");
}
void shared_upgrade() {
    auto s = test_support::world_fixture();
    const std::uint64_t id = 4; // 固定装入旅店定义28；首条是定义33，不能借用其门槛。
    const int definition = s.scene.world.world.facilities.at(id).placement.definition_id;
    const auto anchor = empty_anchor(s, definition);
    check(begin_startup_world_build(s, definition).error == StartupWorldRuntimeError::none,
          "second instance uses actual catalogue and current quote");
    const auto another = confirm_startup_world_build(s, anchor, ref::FacilityOrientation::first);
    check(another.created && cancel_startup_world_build(s) == StartupWorldRuntimeError::none,
          "second same-definition instance installed in unique Owner before upgrade");
    const auto neighbour_price = s.neighbourhood.at(*another.created)[0];
    auto &progress = s.scene.world.world.facility_uses.at(definition);
    progress.upgrade_pending = true;
    progress.completed_uses = 57;
    check(open_startup_world_facility_page(s, id) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 81 &&
              s.scene.world.world.facility_uses.at(definition).level == 1,
          "upgrade hint opens raw81 ahead of details without early level mutation");
    const auto page = s.scripts.pages.back().id;
    check(!inspect_startup_world_facility_upgrade(s, page),
          "uninitialized raw81 has no fabricated presentation projection");
    const auto funds = s.scene.world.world.ai.accounting.funds();
    const auto sound_count = s.sound_requests.size();
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->facility_upgrade_initialized.count(page) &&
              tick.candidate->sound_requests.size() == sound_count + 1 &&
              tick.candidate->sound_requests.back().id == 20 &&
              tick.candidate->sound_requests.back().operation == StartupAudioOperation::jingle &&
              tick.candidate->scene.world.world.facility_uses.at(definition).level == 2 &&
              tick.candidate->scene.world.world.facility_uses.at(definition).completed_uses == 7 &&
              tick.candidate->scene.world.world.facility_uses.at(definition).upgrade_pending &&
              tick.candidate->scene.world.updates == 0 && tick.candidate->scene.random.draws() == 0,
          "first initialization upgrades shared definition once, keeps hint until close, freezes "
          "world");
    s = *tick.candidate;
    const auto first_view = inspect_startup_world_facility_upgrade(s, page);
    check(first_view && first_view->facility == id && first_view->definition == definition &&
              first_view->level == 2 && first_view->phase == 0 && first_view->frame == 1 &&
              first_view->frame2 == 1 && first_view->attributes == s.facility_upgrade_display,
          "initialized upgrade projects frozen Owner facts and independent first update clocks");
    check(s.scene.world.world.facilities.at(*another.created).price == 337 + neighbour_price &&
              s.scene.world.world.facilities.at(id).price == 337 + s.neighbourhood.at(id)[0],
          "shared level changes effective arrival prices for both instances with their own "
          "adjacency");
    tick = prepare_startup_world_runtime(s);
    check(tick.candidate &&
              tick.candidate->scene.world.world.facility_uses.at(definition).level == 2 &&
              tick.candidate->scene.world.world.facility_uses.at(definition).completed_uses == 7,
          "repeated page update does not consume another threshold");
    s = *tick.candidate;
    check(s.page_secondary_counters.at(page) == 2,
          "second actual raw81 update advances independent clock exactly once");
    auto paused = s;
    paused.scene.framework_paused = true;
    const auto paused_tick = prepare_startup_world_runtime(paused);
    check(paused_tick.candidate && paused_tick.candidate->page_counters.at(page) == 2 &&
              paused_tick.candidate->page_secondary_counters.at(page) == 2 &&
              acknowledge_startup_world_runtime_page(paused, page) ==
                  StartupWorldRuntimeError::invalid_page,
          "framework pause freezes both upgrade clocks and rejects confirmation");
    auto wrap = s;
    wrap.page_counters.at(page) = wrap.page_secondary_counters.at(page) =
        std::numeric_limits<int>::max() - 1;
    const auto wrapped = prepare_startup_world_runtime(wrap);
    check(wrapped.candidate && wrapped.candidate->page_counters.at(page) == 0 &&
              wrapped.candidate->page_secondary_counters.at(page) == 0,
          "raw81 actual update follows original modulo INT_MAX without overflow");
    for (int fault = 0; fault < 7; ++fault) {
        auto corrupt = s;
        if (fault == 0) corrupt.page_counters.erase(page);
        if (fault == 1) corrupt.page_phases.erase(page);
        if (fault == 2) corrupt.page_secondary_counters.erase(page);
        if (fault == 3) corrupt.page_secondary_counters.at(page) = -1;
        if (fault == 4) corrupt.page_secondary_counters.at(page) = std::numeric_limits<int>::max();
        if (fault == 5) corrupt.page_phases.at(page) = 2;
        if (fault == 6) corrupt.page_counters.at(page) = -1;
        const auto before = startup_world_state_digest(corrupt);
        check(!inspect_startup_world_facility_upgrade(corrupt, page) &&
                  !prepare_startup_world_runtime(corrupt).candidate &&
                  acknowledge_startup_world_runtime_page(corrupt, page) ==
                      StartupWorldRuntimeError::missing_source &&
                  startup_world_state_digest(corrupt) == before,
              "initialized upgrade rejects missing or illegal clocks without silent repair or mutation");
    }
    const auto usage = startup_world_resource_usage(s);
    auto without_secondary = s;
    without_secondary.page_secondary_counters.erase(page);
    check(startup_world_resource_usage(without_secondary).page_payloads + 1 == usage.page_payloads,
          "resource accounting includes independent page clock ownership");
    for (int n = 0; n < 4; ++n) {
        check(acknowledge_startup_world_runtime_page(s, page) == StartupWorldRuntimeError::none,
              "source40/phase1/55/close confirmations execute");
        check(s.page_secondary_counters.at(page) == 2,
              "confirmation fast-forward and phase transition preserve independent clock");
        if (n == 1) {
            check(s.page_phases.at(page) == 1 && s.page_counters.at(page) == 0,
                  "phase transition resets only primary upgrade counter");
            const auto resumed = prepare_startup_world_runtime(s);
            check(resumed.candidate && resumed.candidate->page_counters.at(page) == 1 &&
                      resumed.candidate->page_secondary_counters.at(page) == 3,
                  "next phase update continues independent clock rather than restarting it");
        }
    }
    check(!s.scene.world.world.facility_uses.at(definition).upgrade_pending &&
              s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.accounting.funds() == funds,
          "only finished page clears shared hint without charging or resetting uses");
    check(!inspect_startup_world_facility_upgrade(s, page),
          "closed upgrade page cannot expose stale frozen presentation");
    const auto retired = prepare_startup_world_runtime(s);
    check(retired.candidate && !retired.candidate->page_counters.count(page) &&
              !retired.candidate->page_phases.count(page) &&
              !retired.candidate->page_secondary_counters.count(page) &&
              !retired.candidate->facility_upgrade_initialized.count(page),
          "actual framework retirement releases both clocks and upgrade initialization owner");
    auto early = test_support::world_fixture();
    auto &p = early.scene.world.world.facility_uses.at(definition);
    p.upgrade_pending = true;
    p.completed_uses = 57;
    check(open_startup_world_facility_page(early, id) == StartupWorldRuntimeError::none,
          "second authentic page opened for input-before-update case");
    const auto early_page = early.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(early, early_page) ==
                  StartupWorldRuntimeError::none &&
              early.scene.world.world.facility_uses.at(definition).level == 2 &&
              early.page_counters.at(early_page) == 40 &&
              early.page_secondary_counters.at(early_page) == 0,
          "input cannot skip first-time upgrade initialization");
    auto broken = test_support::world_fixture();
    broken.scene.world.world.facility_uses.at(definition).upgrade_pending = true;
    broken.scene.world.world.facility_uses.at(definition).completed_uses = 49;
    check(open_startup_world_facility_page(broken, id) == StartupWorldRuntimeError::none,
          "inconsistent hint enters actual page, consumer validates threshold");
    const auto bad_page = broken.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(broken, bad_page) ==
                  StartupWorldRuntimeError::missing_source &&
              broken.scene.world.world.facility_uses.at(definition).level == 1 &&
              broken.facility_upgrade_initialized.empty() && broken.page_counters.empty() &&
              broken.scene.random.draws() == 0,
          "failed upgrade rolls back shared level, page initialization and random");
}
void menu_and_current_quotes() {
    auto s = test_support::world_fixture();
    check(s.neighbourhood_details.size() == s.scene.world.facility_order.size() &&
              std::any_of(s.neighbourhood_details.begin(), s.neighbourhood_details.end(),
                          [](const auto &p) { return !p.second.sources.empty(); }),
          "initial Owner has actual adjacency sources, absence is not silently zero bonus");
    check(open_startup_world_build_menu(s) == StartupWorldRuntimeError::none,
          "source ordinary catalogue opens actual raw21 modal");
    const auto page = s.scripts.pages.back().id;
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->scene.world.updates == 0 &&
              tick.candidate->scene.calendar.units == 0 &&
              tick.candidate->scene.random.draws() == 0,
          "catalogue modal freezes same world/date/random Owner");
    auto &ai = s.scene.world.world.ai;
    check(ai.accounting.post_cash({ai.next_cash_id++, 1, ref::CashCategory::other,
                                   ref::CashDirection::expense, 4500}) ==
              ref::AccountingError::none,
          "explicit affordability fixture posts actual ledger expense, no table edits");
    const auto denied = select_startup_world_build_menu(s, page, 28);
    check(denied.denial == StartupBuildDenial::insufficient_funds &&
              s.scripts.pages.back().lifecycle != 4 && s.scene.scene_state == 0 &&
              !s.build_definition && ai.accounting.funds() == 500,
          "unaffordable source quote leaves menu, cash and placement intact");
    check(cancel_startup_world_build_menu(s, page) == StartupWorldRuntimeError::none &&
              cancel_startup_world_build_menu(s, page) == StartupWorldRuntimeError::invalid_page,
          "catalogue return closes once, stale retry fails");
    auto current = test_support::world_fixture();
    const auto old = startup_world_build_quote(current, 28);
    current.scene.world.world.ai.growth.at(1).definition.current_profession = 0;
    const auto changed = startup_world_build_quote(current, 28);
    check(old && changed && old->construction_ticks == changed->construction_ticks,
          "farmer profession switch does not fabricate carpenter build acceleration");
    auto stale = test_support::world_fixture();
    const auto facility = stale.scene.world.facility_order.front();
    check(open_startup_world_facility_page(stale, facility) == StartupWorldRuntimeError::none,
          "stale-page fixture begins with authenticated live facility");
    const auto pid = stale.scripts.pages.back().id;
    stale.scene.world.world.facilities.erase(facility);
    check(!update_startup_world_runtime_page(stale) &&
              act_startup_world_facility_page(stale, pid, StartupFacilityPageAction::cancel) ==
                  StartupWorldRuntimeError::invalid_page &&
              stale.page_counters.at(pid) == 0,
          "retired identity rejects whole page update and old input without counter/close partial "
          "state");
}
void residence() {
    auto s = test_support::world_fixture();
    const auto funds = s.scene.world.world.ai.accounting.funds();
    const auto p = empty_anchor(s, 24);
    check(begin_startup_world_build(s, 24).error == StartupWorldRuntimeError::none,
          "real initial recruitment definition24 enters construction without invented availability "
          "gate");
    const auto build = confirm_startup_world_build(s, p, ref::FacilityOrientation::first);
    check(build.created && s.scene.world.world.ai.accounting.funds() == funds - 100 &&
              s.facility_details.at(*build.created).construction_limit == 1,
          "recruitment source price100 and one logical construction call, distinct from home "
          "construction");
    check(cancel_startup_world_build(s) == StartupWorldRuntimeError::none,
          "accepted recruitment returns source main state0");
    const auto adapter = startup_world_runtime_adapter();
    const auto tick = ref::prepare_world_facility_update(adapter.facilities.read(s), *build.created,
                                                         adapter.catalog);
    check(tick.candidate && adapter.facilities.write(s, tick.candidate->state) &&
              s.scene.world.world.facilities.at(*build.created).status == 1,
          "actual recruitment construction completes at original threshold");
    // 住户申请资格夹具：仅提高真实定义满足度到原表门槛，不补人物实例或改原表。
    s.shop_humans.at(1).satisfaction = s.rules->humans.at(1).residence_threshold;
    for (auto &page : s.scripts.pages)
        if (page.kind != ref::WorldScriptPageKind::scene)
            page.lifecycle = 4;
    check(open_startup_world_facility_page(s, *build.created) == StartupWorldRuntimeError::none,
          "source recruitment facility opens details74");
    const auto detail = s.scripts.pages.back().id;
    check(act_startup_world_facility_page(s, detail, StartupFacilityPageAction::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 80 &&
              s.residence_page_candidates.at(s.scripts.pages.back().id) == std::vector<int>{1},
          "detail6 opens exact source candidate snapshot, no fake initial resident");
    const auto choose = s.scripts.pages.back().id;
    auto broken = s;
    broken.base_variants.pop_back();
    const auto next_id = broken.next_facility_identity;
    const auto paid = broken.scene.world.world.ai.accounting.funds();
    check(act_startup_world_residence_page(broken, choose, 1).error ==
                  StartupWorldRuntimeError::missing_source &&
              broken.next_facility_identity == next_id &&
              broken.scene.world.world.facilities.count(*build.created) &&
              broken.scene.world.world.ai.accounting.funds() == paid &&
              broken.human_homes.at(1)[2] == 0,
          "late demolition refresh failure rolls back first housing payment, identity, site and "
          "home binding");
    const auto home = act_startup_world_residence_page(s, choose, 1);
    check(home.created && !s.scene.world.world.facilities.count(*build.created) &&
              s.scene.world.world.facilities.at(*home.created).placement.definition_id == 25 &&
              s.scene.world.world.ai.accounting.funds() ==
                  funds - 100 - s.rules->humans.at(1).residence_fee &&
              s.monthly_cash.at(3)[2][1] == s.rules->humans.at(1).residence_fee &&
              s.human_homes.at(1) == std::array<int, 4>{p.x, p.y, 1, 0} &&
              s.facility_details.at(*home.created).residence_mode == 1 &&
              s.facility_residents.at(*home.created) == 1 &&
              s.scene.world.world.ai.battle.actors.empty(),
          "source80 pays only humanh then replaces full recruitment site with first real home25 "
          "and same definition");
    check(act_startup_world_residence_page(s, choose, 1).error ==
              StartupWorldRuntimeError::invalid_page,
          "retired choice cannot pay or install second home");
    const auto limit = s.facility_details.at(*home.created).construction_limit;
    const auto before = s.scene.world.world.ai.pending_completion;
    const auto C = s.shop_humans.at(1).satisfaction;
    const auto u = s.scene.world.world.ai.growth.at(1).definition.legacy_u;
    for (int n = 0; n < limit; ++n) {
        const auto result = ref::prepare_world_facility_update(
            adapter.facilities.read(s), *home.created, adapter.catalog,
            [&](const auto &current,
                const auto &request) -> std::optional<ref::WorldFacilityUpdateState> {
                if (!adapter.facilities.write(s, current))
                    return {};
                const auto completed = adapter.facility(s, request);
                if (!completed)
                    return {};
                s = *completed;
                return adapter.facilities.read(s);
            });
        check(result.candidate && adapter.facilities.write(s, result.candidate->state),
              "source qualified facility calls consume actual housing completion Owner");
    }
    check(s.scene.world.world.facilities.at(*home.created).status == 1 &&
              s.shop_humans.at(1).satisfaction == std::min(C + 5, 100) &&
              s.scene.world.world.ai.growth.at(1).definition.legacy_u == std::min(u + 15, 100) &&
              s.scene.world.world.ai.pending_completion == before + 5 &&
              s.human_calendar.at(1).celebrations == 0 && s.scripts.event_calls.at(202) == 1 &&
              s.scene.world.world.ai.battle.actors.empty(),
          "completion commits same shared definition reward5/15, event202 and no celebration or "
          "actor creation");
    const auto pages = s.scripts.pages.size();
    const auto repeat = ref::prepare_world_facility_update(adapter.facilities.read(s),
                                                           *home.created, adapter.catalog);
    check(repeat.candidate && repeat.candidate->state.scripts.pages.size() == pages &&
              repeat.candidate->state.scripts.event_calls.at(202) == 1,
          "finished home next ordinary call never repeats completion reward or pages");
    // 激活主场景后的点击调用点夹具；完成奖励及演出不由本断言伪造消费。
    for (auto &page : s.scripts.pages)
        page.lifecycle = page.kind == ref::WorldScriptPageKind::scene ? 2 : 4;
    check(open_startup_world_facility_page(s, *home.created) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 60 &&
              s.page_human_bindings.at(s.scripts.pages.back().id) == 1 &&
              s.scene.world.world.ai.battle.actors.empty(),
          "bound finished home opens original resident60, not ordinary74 or a new actor");
}
void facility_item_pages() {
    using Action = StartupFacilityItemAction;
    auto s = test_support::world_fixture();
    for (auto &[item, value] : s.items) {
        value.inventory = 0;
        s.catalog.at({0, item}) = value;
    }
    const std::uint64_t facility =
        source_facility(s, 33); // 真实初始包子铺33；库存为隔离调用点夹具。
    check(s.scene.world.world.facilities.at(facility).placement.definition_id == 33,
          "item fixture binds actual initial bun shop");
    check(begin_startup_world_build(s, 33).error == StartupWorldRuntimeError::none,
          "shared item scenario enters actual same-definition construction");
    const auto second =
        confirm_startup_world_build(s, empty_anchor(s, 33), ref::FacilityOrientation::first);
    check(second.created && cancel_startup_world_build(s) == StartupWorldRuntimeError::none,
          "second same-definition instance belongs to common Owner");
    s.items.at(1).inventory = 2;
    s.catalog.at({0, 1}) = s.items.at(1);
    const auto funds = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    const auto old_price = s.scene.world.world.facilities.at(facility).price;
    const auto other_price = s.scene.world.world.facilities.at(*second.created).price;
    check(open_startup_world_facility_page(s, facility) == StartupWorldRuntimeError::none,
          "source facility74 opens item chain");
    const auto parent = s.scripts.pages.back().id;
    const auto baseline = startup_world_resource_usage(s);
    check(act_startup_world_facility_page(s, parent, StartupFacilityPageAction::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 75,
          "normal74 confirm opens actual75 without consuming inventory");
    const auto directory = s.scripts.pages.back().id;
    check(act_startup_world_facility_item_page(s, directory, Action::confirm) ==
                  StartupWorldRuntimeError::missing_source &&
              s.items.at(1).inventory == 2,
          "new uninitialized75 cannot consume stale selection");
    auto update = prepare_startup_world_runtime(s);
    check(update.candidate &&
              update.candidate->facility_item_page_lists.at(directory) == std::vector<int>{1},
          "framework initializes75 in source item order using positive inventory only");
    s = *update.candidate;
    const auto original = s;
    check(act_startup_world_facility_item_page(s, directory, Action::select, 1) ==
                  StartupWorldRuntimeError::invalid_page &&
              s.facility_item_page_selections.at(directory) == 0 && s.items.at(1).inventory == 2,
          "out-of-range row selection does not alter current row or inventory");
    check(act_startup_world_facility_item_page(s, directory, Action::confirm, 1) ==
                  StartupWorldRuntimeError::invalid_page &&
              s.items.at(1).inventory == 2 &&
              s.scripts.next_page_id == original.scripts.next_page_id,
          "out-of-range selection rejects without inventory or page mutation");
    auto broken = s;
    broken.facility_item_page_lists.erase(directory);
    check(!prepare_startup_world_runtime(broken).candidate &&
              act_startup_world_facility_item_page(broken, directory, Action::confirm) !=
                  StartupWorldRuntimeError::none,
          "initialized missing catalogue explicitly refuses update and input");
    broken = s;
    broken.facility_page_bindings.at(parent) = 4;
    check(!prepare_startup_world_runtime(broken).candidate,
          "wrong source parent74 identity refuses75");
    broken = s;
    broken.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
    check(act_startup_world_facility_item_page(broken, directory, Action::confirm) ==
                  StartupWorldRuntimeError::script_failed &&
              broken.items.at(1).inventory == 2 &&
              broken.facility_item_confirmations.at(facility) == 0 &&
              broken.scripts.facilities.at(33).improvements == std::array<int, 4>{},
          "late76 insertion failure rolls back inventory and selected-instance count");
    check(act_startup_world_facility_item_page(s, directory, Action::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.items.at(1).inventory == 1 && s.facility_item_confirmations.at(facility) == 1 &&
              s.scripts.facilities.at(33).improvements == std::array<int, 4>{} &&
              s.scripts.pages.back().legacy_page == 76,
          "75 pays exactly one item and advances instance count before deferred76 improvement");
    const auto animation = s.scripts.pages.back().id;
    check(act_startup_world_facility_item_page(s, directory, Action::confirm) ==
                  StartupWorldRuntimeError::invalid_page &&
              s.items.at(1).inventory == 1,
          "stale75 cannot duplicate payment beneath76");
    broken = s;
    broken.neighbourhood.erase(*second.created);
    check(!prepare_startup_world_runtime(broken).candidate &&
              broken.scripts.facilities.at(33).improvements == std::array<int, 4>{} &&
              broken.scene.world.world.facilities.at(facility).price == old_price,
          "late same-definition cache failure rolls back entire76 initialization");
    update = prepare_startup_world_runtime(s);
    check(update.candidate && update.candidate->facility_item_pages_initialized.count(animation),
          "framework initializes76 once");
    s = *update.candidate;
    check(s.scripts.facilities.at(33).improvements == std::array<int, 4>{60, 8, 0, 0} &&
              s.scripts.job_counts[1] == 0 &&
              s.scene.world.world.facilities.at(facility).price == old_price + 60 &&
              s.scene.world.world.facilities.at(*second.created).price == other_price + 60 &&
              s.facility_item_response == 2 && s.facility_upgrade_display[2][1] == 8,
          "actual milk doubles33 improvement and refreshes both prices with actual initial jobs");
    for (int n = 0; n < 48; ++n) {
        update = prepare_startup_world_runtime(s);
        check(update.candidate.has_value(), "76 qualified animation advances");
        s = *update.candidate;
    }
    check(s.scripts.pages.back().legacy_page == 77 && s.items.at(1).inventory == 1 &&
              s.scripts.facilities.at(33).improvements[0] == 60,
          "49th animation update replaces76 with77 without repeating improvement");
    const auto result = s.scripts.pages.back().id;
    update = prepare_startup_world_runtime(s);
    check(update.candidate.has_value(), "77 initializes under common framework");
    s = *update.candidate;
    check(act_startup_world_facility_item_page(s, result, Action::cancel) ==
                  StartupWorldRuntimeError::invalid_page &&
              act_startup_world_facility_item_page(s, result, Action::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.page_counters.at(result) == 49,
          "77 has no cancellation and early confirmation fast-forwards only to49");
    check(act_startup_world_facility_item_page(s, result, Action::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle != 4,
          "49through54 confirmation cannot close results early");
    for (int n = 0; n < 6; ++n) {
        update = prepare_startup_world_runtime(s);
        check(update.candidate.has_value(), "77 advances toward55");
        s = *update.candidate;
    }
    check(acknowledge_startup_world_runtime_page(s, result) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.accounting.funds() == funds && s.scene.random.draws() == draws,
          "55 confirmation closes77 with no cash or random side effects");
    update = prepare_startup_world_runtime(s);
    check(update.candidate && !update.candidate->facility_item_page_items.count(result) &&
              !update.candidate->facility_item_page_items.count(animation),
          "closed76and77 retire item bindings during framework admission");
    s = *update.candidate;
    check(cancel_startup_world_runtime_page(s, directory) == StartupWorldRuntimeError::none,
          "75 cancel returns to original74 without using remaining item");
    update = prepare_startup_world_runtime(s);
    check(update.candidate && update.candidate->facility_item_page_lists.empty() &&
              update.candidate->facility_item_pages_initialized.empty() &&
              update.candidate->facility_item_page_selections.empty() &&
              startup_world_resource_usage(*update.candidate).page_payloads ==
                  baseline.page_payloads,
          "item page payloads retire fully; only original74 binding and neighbour snapshot remain");
}

void facility_item_empty_and_legend() {
    using Action = StartupFacilityItemAction;
    auto s = test_support::world_fixture();
    for (auto &[item, value] : s.items) {
        value.inventory = 0;
        s.catalog.at({0, item}) = value;
    }
    check(open_startup_world_facility_page(s, source_facility(s, 33)) ==
                  StartupWorldRuntimeError::none &&
              open_startup_world_facility_items(s, s.scripts.pages.back().id) ==
                  StartupWorldRuntimeError::none,
          "empty inventory enters real75");
    const auto empty = s.scripts.pages.back().id;
    auto update = prepare_startup_world_runtime(s);
    check(update.candidate && update.candidate->scripts.event_calls.at(15) == 1 &&
              std::any_of(update.candidate->scripts.pages.begin(),
                          update.candidate->scripts.pages.end(),
                          [empty](const auto &p) { return p.id == empty && p.lifecycle == 4; }),
          "empty75 initialization invokes actual event15 and closes only itself");
    s = test_support::world_fixture();
    for (auto &[item, value] : s.items) {
        value.inventory = 0;
        s.catalog.at({0, item}) = value;
    }
    s.items.at(1).inventory = 1;
    s.catalog.at({0, 1}) = s.items.at(1); // 门槛组合夹具，不宣称自然36个月及五次赠送轨迹。
    s.facility_item_confirmations.at(source_facility(s, 33)) = 4;
    s.facility_month_age.at(source_facility(s, 33)) = 36;
    check(open_startup_world_facility_page(s, source_facility(s, 33)) ==
                  StartupWorldRuntimeError::none &&
              open_startup_world_facility_items(s, s.scripts.pages.back().id) ==
                  StartupWorldRuntimeError::none,
          "legend threshold uses source bun-shop program");
    const auto directory = s.scripts.pages.back().id;
    update = prepare_startup_world_runtime(s);
    check(update.candidate.has_value(), "legend75 initializes");
    s = *update.candidate;
    auto broken = s;
    // opcode6保存续体而非开raw16；缺源旧续体只在后续脚本事务验证时拒绝。
    ref::WorldScriptContinuation invalid;
    invalid.event = -999;
    invalid.remaining_updates = 100;
    broken.scripts.continuations.push_back(invalid);
    const auto refused = act_startup_world_facility_item_page(broken, directory, Action::confirm);
    check(refused == StartupWorldRuntimeError::script_failed && broken.items.at(1).inventory == 1 &&
              broken.facility_item_confirmations.at(source_facility(s, 33)) == 4 &&
              broken.facility_month_age.at(source_facility(s, 33)) == 36 &&
              broken.scripts.next_page_id == s.scripts.next_page_id &&
              broken.scripts.continuations.size() == s.scripts.continuations.size() + 1,
          "late script source validation failure rolls back prior76 insertion, consumption and "
          "count");
    check(act_startup_world_facility_item_page(s, directory, Action::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.items.at(1).inventory == 0 &&
              s.facility_item_confirmations.at(source_facility(s, 33)) == 0 &&
              s.facility_month_age.at(source_facility(s, 33)) == 0 &&
              s.scripts.facilities.at(33).improvements == std::array<int, 4>{} &&
              std::any_of(s.scripts.continuations.begin(), s.scripts.continuations.end(),
                          [](const auto &c) {
                              return c.event == 2033 && c.next_instruction == 1 &&
                                     c.remaining_updates == 100;
                          }),
          "fifth item at36 stores source100-update continuation and clears counters before76 "
          "modifiesJ");
    update = prepare_startup_world_runtime(s);
    check(update.candidate && update.candidate->scripts.facilities.at(33).improvements[0] == 60 &&
              !update.candidate->facility_item_page_lists.count(directory),
          "exhausted75 retires while framework initializes its queued76 exactly once");
    s = *update.candidate;
    bool completed{};
    std::uint64_t animation{};
    for (int step = 0; step < 600 && !completed; ++step) {
        const auto tick = prepare_startup_world_runtime(s);
        check(tick.candidate.has_value(), "legend integration actual framework continues");
        s = *tick.candidate;
        const auto top = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                    [](const auto &p) { return p.lifecycle != 4; });
        check(top != s.scripts.pages.rend(), "legend integration retains one actual top page");
        if (top->legacy_page == 82) {
            animation = top->id;
            check(top->facility_definition == 33 && top->legacy_f == 0,
                  "actual75 continuation2033 creates82 bound to original bun shop definition");
            if (inspect_startup_world_facility_catalog_page(s, animation)) {
                const bool finishing = s.page_phases.at(animation) == 1 && s.page_counters.at(animation) >= 40;
                check(acknowledge_startup_world_runtime_page(s, animation) == StartupWorldRuntimeError::none,
                      "actual legend82 uses the two phase confirmation consumer");
                completed = finishing;
            } // 当轮新插页尚未初始化，玩家输入等下一个框架入口；不补载荷。
        } else if (top->legacy_page == 74) {
            check(act_startup_world_facility_page(s, top->id, StartupFacilityPageAction::cancel) == StartupWorldRuntimeError::none,
                  "legend player returns from parent details so its real delayed program can advance");
        } else if (top->kind != ref::WorldScriptPageKind::scene && top->legacy_page != 76 &&
                   (top->legacy_page != 77 || s.facility_item_pages_initialized.count(top->id))) {
            const auto error = acknowledge_startup_world_runtime_page(s, top->id);
            check(error == StartupWorldRuntimeError::none,
                  ("legend prior confirmation raw=" + std::to_string(top->legacy_page) +
                   " error=" + std::to_string(static_cast<int>(error))).c_str());
        }
        s.sound_requests.clear(); // 显式表现消费者领取本次输出，不留下积累声音。
    }
    check(completed && s.scene.world.popularity_queue.front() == std::array<int, 3>{10, 20, 1} &&
              s.facility_item_confirmations.at(source_facility(s, 33)) == 0,
          "condition fixture reaches actual75→100-delay→82→one deferred20 request without repeating gift");
    const auto retired = prepare_startup_world_runtime(s);
    check(retired.candidate && !retired.candidate->facility_catalog_pages_initialized.count(animation) &&
              !retired.candidate->facility_catalog_page_data.count(animation) &&
              !retired.candidate->facility_catalog_page_lists.count(animation),
          "actual legend page retires its initialized binding and frozen participants on next entry");
}
void road_editing() {
    auto s = test_support::world_fixture();
    check(s.facility_free_builds.size() == s.rules->facilities.size() &&
              std::all_of(s.facility_free_builds.begin(), s.facility_free_builds.end(),
                          [](const auto &v) { return v.second == 0; }),
          "real fresh world initializes all H counters to zero, without fabricated home credit");
    const ref::Position start{7, 3}, end{9, 5};
    check(begin_startup_world_road(s, 18).error == StartupWorldRuntimeError::none &&
              s.build_mode == 1,
          "source road definition enters start mode");
    check(confirm_startup_world_edit(s, start, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              s.build_mode == 2,
          "road first click stores anchor without paying");
    const auto segment = startup_world_edit_segment(s, end);
    check(segment && *segment == std::vector<ref::Position>{{7, 3}, {7, 4}, {7, 5}},
          "equal-axis road gesture selects vertical inclusive line in coordinate order");
    // 现金边界夹具；原单格10预检通过，三格费用30不能加总额预算拒绝。
    s.scene.world.world.ai.accounting = ref::PeriodAccounting(10, 0);
    const auto draws = s.scene.random.draws();
    auto broken = s;
    broken.base_variants.pop_back();
    check(confirm_startup_world_edit(broken, end, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::missing_source &&
              broken.scene.world.world.ai.accounting.funds() == 10 && broken.build_mode == 2 &&
              broken.surface.at(3 * 24 + 7).definition == s.surface.at(3 * 24 + 7).definition,
          "late road refresh failure leaves anchor, cash and all tiles unchanged");
    check(confirm_startup_world_edit(s, end, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              s.scene.world.world.ai.accounting.funds() == -20 &&
              s.monthly_cash.at(3)[0][1] == 30 && s.build_mode == 1 && !s.build_anchor &&
              s.scene.random.draws() == draws,
          "road charges changed cells once, permits original negative balance and consumes no RNG");
    for (const auto p : *segment)
        check(s.scene.world.world.map.cells.at(p.y * 24 + p.x).legacy_state == 3,
              "each selected source road cell becomes actual walkable road");
    check(cancel_startup_world_edit(s) == StartupWorldRuntimeError::none &&
              s.scene.scene_state == 0,
          "road start cancel exits editing without refund");
    check(begin_startup_world_edit(s, false).error == StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, start, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              s.build_mode == 5,
          "remove on road starts original road-removal range");
    check(cancel_startup_world_edit(s) == StartupWorldRuntimeError::none && s.build_mode == 3 &&
              !s.build_anchor,
          "range cancel returns to remove selection without touching road");
    check(confirm_startup_world_edit(s, start, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, end, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              s.scene.world.world.ai.accounting.funds() == -20 && s.build_mode == 3,
          "road removal has no refund and returns to source remove mode");
    check(cancel_startup_world_edit(s) == StartupWorldRuntimeError::none &&
              prepare_startup_world_runtime(s).candidate.has_value(),
          "real common world resumes after road topology changed");
}
StartupWorldRuntimeState first_arrival_fixture() {
    StartupSession arrival;
    for (int n = 0; n < 420; ++n)
        check(arrival.update() == StartupError::none, "source first-arrival fixture updates");
    StartupWorldRuntimeSession owner(arrival.state(), ref::WorldRandomStream::from_java_seed(1));
    auto baseline = owner.state();
    bool entered{};
    for (int n = 0; n < 2000; ++n) {
        const auto page =
            std::find_if(baseline.scripts.pages.rbegin(), baseline.scripts.pages.rend(),
                         [](const auto &p) { return p.lifecycle != 4; });
        check(page != baseline.scripts.pages.rend(), "first arrival retains a live framework page");
        if (page->kind == ref::WorldScriptPageKind::scene && baseline.scene.scene_state == 0) {
            entered = true;
            break;
        }
        if (page->kind != ref::WorldScriptPageKind::scene && page->legacy_page != 16 &&
            page->legacy_page != 56 && page->legacy_page != 57 && page->legacy_page != 97)
            check(acknowledge_startup_world_runtime_page(baseline, page->id) ==
                      StartupWorldRuntimeError::none,
                  "real first arrival tutorial uses actual page confirmation");
        const auto update = prepare_startup_world_runtime(baseline);
        check(update.candidate.has_value(),
              "first arrival script/camera advances without injected mode");
        baseline = *update.candidate;
    }
    check(entered, "first arrival source script returns naturally to ordinary main scene");
    check(!baseline.scene.world.world.ai.human_order.empty(),
          "actual source arrival installed a real actor");
    return baseline;
}
void move_remove_and_stale_actor() {
    const auto baseline = first_arrival_fixture();
    for (const bool move : {false, true})
        for (const bool using_facility : {false, true}) {
            auto s = baseline;
            s.scripts.user_flags |= 32U; // 解锁边界夹具，不冒充自然人气奖励路径。
            const auto id = source_facility(s, 28);
            const auto p = s.scene.world.world.facilities.at(id).placement.anchor;
            const auto actor_id = s.scene.world.world.ai.human_order.front();
            auto &a = s.scene.world.world.ai.battle.actors.at(actor_id);
            auto &ctx = s.scene.world.world.actors.at(actor_id);
            a.position = {static_cast<float>(p.x * 100), 0, static_cast<float>(p.y * 100)};
            s.scene.world.world.ai.contexts.at(actor_id).cell = p;
            s.scene.world.world.ai.contexts.at(actor_id).inside_town = true;
            a.control.state = using_facility ? 14 : 0;
            a.control.action = 0;
            a.control.flags = 2;
            a.control.queue.clear();
            a.state_counter = 0;
            ctx.binding = ref::ArrivalBinding{p, {id}, 28};
            ctx.destination = p;
            ctx.path_pending = !using_facility;
            ctx.unbound_route.reset();
            ctx.journey.reset();
            if (using_facility) {
                s.scene.world.world.facilities.at(id).occupants.push_back(actor_id);
                a.control.queue = {{1, 100, 0}, {24}};
            } else {
                ref::FacilityDeparture departure;
                departure.binding = *ctx.binding;
                departure.route.steps = {p};
                ctx.journey = departure;
                ctx.waypoint = 0;
            }
            const auto target = empty_anchor(s, 28);
            const auto money = s.scene.world.world.ai.accounting.funds();
            const auto raw = s.facility_original_ids.at(id);
            check(begin_startup_world_edit(s, move).error == StartupWorldRuntimeError::none,
                  "fixture enters actual Owner editing, actor remains autonomously owned");
            check(confirm_startup_world_edit(s, p, ref::FacilityOrientation::first).error ==
                      StartupWorldRuntimeError::none,
                  "actual old facility selects or removes through editor");
            std::optional<std::uint64_t> replacement;
            if (move) {
                check(confirm_startup_world_edit(s, p, ref::FacilityOrientation::first).denial ==
                          StartupBuildDenial::occupied,
                      "old footprint overlap is rejected, never treated as skipped cells");
                auto broken = s;
                broken.scene.world.world.ai.next_cash_id =
                    std::numeric_limits<std::uint64_t>::max();
                check(confirm_startup_world_edit(broken, target, ref::FacilityOrientation::first)
                                  .error == StartupWorldRuntimeError::missing_source &&
                          broken.scene.world.world.facilities.count(id) && broken.build_mode == 7 &&
                          broken.scene.world.world.ai.accounting.funds() == money,
                      "late move charge failure restores old identity, footprint, mode and money");
                const auto moved =
                    confirm_startup_world_edit(s, target, ref::FacilityOrientation::first);
                check(moved.created && *moved.created != id &&
                          s.facility_original_ids.at(*moved.created) == raw &&
                          s.scene.world.world.facilities.at(*moved.created).occupants.empty() &&
                          s.scene.world.world.ai.accounting.funds() == money - 300,
                      "moving creates new maintenance ID, retains original numeric ID and no old "
                      "occupants");
                replacement = moved.created;
            }
            check(!s.scene.world.world.facilities.count(id) && !s.facility_details.count(id) &&
                      !s.facility_original_ids.count(id) && !s.facility_monthly_cash.count(id) &&
                      !s.facility_item_confirmations.count(id) && !s.neighbourhood.count(id) &&
                      !s.dungeon_facilities.count(id) && !s.sites.count(id),
                  "retired instance auxiliary maps are removed together, not retained as fake "
                  "history");
            check(s.scene.world.world.actors.at(actor_id).binding &&
                      s.scene.world.world.actors.at(actor_id).binding->instance_id.value == id &&
                      s.scene.world.world.ai.battle.actors.at(actor_id).control.state ==
                          (using_facility ? 14 : 0),
                  "edit transaction preserves old actor binding and control until its true update");
            check(cancel_startup_world_edit(s) == StartupWorldRuntimeError::none,
                  "successful move/remove returns through original mode cancel");
            for (int n = 0; n < 3; ++n) {
                const auto update = prepare_startup_world_runtime(s);
                if (!update.candidate)
                    throw std::runtime_error(
                        "whole Owner rejects edited stale actor: move=" + std::to_string(move) +
                        " use=" + std::to_string(using_facility) +
                        " error=" + std::to_string(static_cast<int>(update.error)));
                s = *update.candidate;
            }
            check(s.scene.world.world.ai.battle.actors.at(actor_id).control.state != 14 &&
                      (!replacement ||
                       s.scene.world.world.facilities.at(*replacement).occupants.empty()),
                  "whole runtime admission and real stale path/use consumers recover without "
                  "migrating occupation");
        }
}
void residence_rebuild() {
    auto s = test_support::world_fixture();
    // 住宅绑定边界夹具，使用真实安装原语，不给人物添加新实例或改原表。
    const auto home =
        install_startup_world_facility(s, 25, empty_anchor(s, 25), ref::FacilityOrientation::first);
    check(home.created.has_value(), "source home installed for removal/rebuild boundary");
    const auto id = *home.created;
    const auto p = s.scene.world.world.facilities.at(id).placement.anchor;
    s.facility_residents.at(id) = 1;
    s.facility_details.at(id).resident_definition = 1;
    s.human_homes.at(1) = {p.x, p.y, 1, 7};
    const auto money = s.scene.world.world.ai.accounting.funds();
    check(begin_startup_world_edit(s, false).error == StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, p, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              s.human_homes.at(1) == std::array<int, 4>{0, 0, 2, 7} &&
              s.facility_free_builds.at(25) == 1 &&
              s.scene.world.world.ai.accounting.funds() == money,
          "home removal releases D0/1/2, retains D3 and grants H without refund");
    for (auto &page : s.scripts.pages)
        if (page.kind != ref::WorldScriptPageKind::scene)
            page.lifecycle = 4;
    check(cancel_startup_world_edit(s) == StartupWorldRuntimeError::none &&
              begin_startup_world_build(s, 25).error == StartupWorldRuntimeError::none,
          "returned H opens actual private-home catalogue eligibility");
    const auto rebuilt = confirm_startup_world_build(s, p, ref::FacilityOrientation::first);
    check(rebuilt.created && s.facility_free_builds.at(25) == 0 &&
              s.facility_residents.at(*rebuilt.created) == 1 &&
              s.human_homes.at(1) == std::array<int, 4>{p.x, p.y, 1, 0} &&
              s.facility_details.at(*rebuilt.created).residence_mode == 0 &&
              s.scene.world.world.ai.accounting.funds() == money - 800 && s.scene.scene_state == 0,
          "H is placement eligibility, not free cash: rebuild pays800, binds first homeless "
          "resident, no repeated welcome");
    check(begin_startup_world_build(s, 25).denial == StartupBuildDenial::unavailable &&
              prepare_startup_world_runtime(s).candidate.has_value(),
          "exhausted home credit leaves catalogue and rebuilt world resumes");
}
void retired_facility_records(const StartupWorldRuntimeState &s, std::uint64_t id) {
    check(!s.scene.world.world.facilities.count(id) && !s.facility_original_ids.count(id) &&
              !s.facility_ordinals.count(id) && !s.facility_residents.count(id) &&
              !s.facility_flags.count(id) && !s.facility_details.count(id) &&
              !s.facility_month_age.count(id) && !s.facility_monthly_cash.count(id) &&
              !s.facility_item_confirmations.count(id) && !s.neighbourhood.count(id) &&
              !s.neighbourhood_details.count(id) && !s.dungeon_facilities.count(id) &&
              !s.sites.count(id) && !s.shops.count(id) &&
              std::none_of(s.scene.world.world.map.cells.begin(),
                           s.scene.world.world.map.cells.end(),
                           [&](const auto &cell) {
                               return cell.facility && cell.facility->instance_id.value == id;
                           }),
          "map replacement retires old occupancy and auxiliary records together");
}
// 邻接变化使用真实建设/撤除入口；仅把真实首访人物投影到到达调用点，
// 不声明为自然寻路覆盖，不改价格、资金、共享属性或邻接缓存。
void neighbour_changes_reach_actual_income() {
    auto built = first_arrival_fixture();
    const auto inn = source_facility(built, 28);
    const auto anchor = built.scene.world.world.facilities.at(inn).placement.anchor;
    {
        auto initial = test_support::world_fixture();
        const auto before = initial;
        check(initialize_startup_world_neighbours(initial),
              "real initial neighbour projection remains admissible after cache synchronization");
        for (const auto &[id, facility] : initial.scene.world.world.facilities)
            check(facility.price == before.scene.world.world.facilities.at(id).price &&
                      initial.neighbourhood.at(id) == before.neighbourhood.at(id),
                  "initial projection already includes each instance neighbours and is not repriced");
        check(initial.scene.random.draws() == before.scene.random.draws(),
              "initial neighbour synchronization consumes no new random draw");
    }
    check(built.scene.world.world.facilities.at(inn).price == 300 &&
              built.neighbourhood.at(inn)[0] == 0,
          "source initial inn has independent source base price300 and no price neighbour");
    const auto bounds = built.rules->fences.at(built.fence_level);
    const auto &map = built.scene.world.world.map;
    std::optional<ref::Position> flower_cell;
    for (const auto p : std::array<ref::Position, 4>{{{anchor.x + 1, anchor.y},
                                                     {anchor.x - 1, anchor.y},
                                                     {anchor.x, anchor.y + 1},
                                                     {anchor.x, anchor.y - 1}}}) {
        if (p.x > bounds[0].x && p.x < bounds[1].x && p.y > bounds[1].y &&
            p.y < bounds[0].y && !map.cells.at(p.y * map.width + p.x).facility) {
            flower_cell = p;
            break;
        }
    }
    check(flower_cell.has_value(), "real initial inn has an empty legal adjacent decoration cell");
    const auto count = built.scene.world.facility_order.size();
    const auto draws = built.scene.random.draws();
    check(begin_startup_world_build(built, 66).error == StartupWorldRuntimeError::none,
          "actual unlocked sunflower starts through Owner build command");
    const auto planted =
        confirm_startup_world_build(built, *flower_cell, ref::FacilityOrientation::first);
    check(planted.created && cancel_startup_world_build(built) == StartupWorldRuntimeError::none,
          "actual sunflower construction commits and returns through normal cancellation");
    const auto flower = *planted.created;
    const auto quote = startup_world_facility_values(built, inn);
    check(quote && quote->instance_attributes[0] == 320 && quote->instance_attributes[1] == 10 &&
              built.scene.world.world.facilities.at(inn).price == 320 &&
              built.neighbourhood.at(inn)[0] == 20 && built.neighbourhood.at(inn)[1] == 5 &&
              built.scene.world.facility_order.size() == count + 1 &&
              built.scene.random.draws() == draws,
          "source sunflower plus20 price and plus5 quality updates existing inn without random");
    for (int missing = 0; missing < 5; ++missing) {
        auto broken = built;
        const auto actor = broken.scene.world.world.ai.human_order.front();
        const auto definition = broken.scene.world.world.ai.battle.actors.at(actor).definition;
        if (missing == 0)
            broken.scripts.facilities.erase(28);
        if (missing == 1)
            broken.scene.world.world.facility_uses.erase(28);
        if (missing == 2)
            broken.human_presence.erase(definition);
        if (missing == 3)
            broken.scene.world.world.ai.growth.erase(definition);
        if (missing == 4)
            broken.scene.world.world.ai.growth.at(definition).definition.current_profession = -1;
        const auto digest = startup_world_state_digest(broken);
        check(!refresh_startup_world_map(broken, false) &&
                  startup_world_state_digest(broken) == digest,
              "missing or invalid economy source rejects no-notice refresh without partial Owner writes");
    }

    for (const bool remove : {false, true}) {
        auto s = built;
        const int expected = remove ? 300 : 320; // 原表旅店300、向日葵邻接20，独立手写oracle。
        if (remove) {
            check(begin_startup_world_edit(s, false).error == StartupWorldRuntimeError::none &&
                      confirm_startup_world_edit(s, *flower_cell, ref::FacilityOrientation::first)
                              .error == StartupWorldRuntimeError::none &&
                      cancel_startup_world_edit(s) == StartupWorldRuntimeError::none,
                  "decoration removal uses actual Owner editor before later arrival");
            retired_facility_records(s, flower);
            check(s.scene.world.facility_order.size() == count &&
                      s.neighbourhood.at(inn)[0] == 0 && s.neighbourhood.at(inn)[1] == 0,
                  "removal retires one instance and both neighbour effects without fake history");
        }
        check(s.scene.random.draws() == draws &&
                  s.scene.world.world.facilities.at(inn).price == expected,
              "positive and negative map changes refresh arrival cache without drawing random");
        // 本无声测试显式消费建设/撤除输出；它不是持久化历史，也不作为收入oracle。
        s.sound_requests.clear();
        const auto actor = s.scene.world.world.ai.human_order.front();
        auto &a = s.scene.world.world.ai.battle.actors.at(actor);
        auto &ctx = s.scene.world.world.actors.at(actor);
        a.position = {static_cast<float>(anchor.x * 100), 0, static_cast<float>(anchor.y * 100)};
        a.control.state = 0;
        a.control.action = 0;
        a.control.flags = 2;
        a.control.queue.clear();
        a.state_counter = 0;
        s.scene.world.world.ai.contexts.at(actor).cell = anchor;
        s.scene.world.world.ai.contexts.at(actor).inside_town = true;
        ctx.binding = ref::ArrivalBinding{anchor, {inn}, 28};
        ctx.destination = anchor;
        ctx.path_pending = true;
        ctx.unbound_route.reset();
        ref::FacilityDeparture route;
        route.binding = *ctx.binding;
        route.route.steps = {anchor};
        ctx.journey = route;
        ctx.waypoint = 0;
        const auto definition = a.definition;
        const auto funds = s.scene.world.world.ai.accounting.funds();
        const auto sales = s.scene.world.world.facilities.at(inn).sales;
        const auto spending = s.scene.world.world.human_spending.at(definition);
        const auto month = s.scene.world.world.month_index;
        const auto monthly = s.facility_monthly_cash.at(inn).at(month)[0];
        const auto cash_id = s.scene.world.world.ai.next_cash_id;
        bool arrived{};
        for (int n = 0; n < 8; ++n) {
            const auto tick = prepare_startup_world_runtime(s);
            check(tick.candidate.has_value(), "complete Owner admits ordinary inn arrival fixture");
            s = *tick.candidate;
            s.sound_requests.clear(); // 每轮实际消费，无假定永久有界。
            if (!s.scene.world.world.actors.at(actor).path_pending) {
                arrived = true;
                break;
            }
        }
        check(arrived && s.scene.world.world.ai.accounting.funds() == funds + expected &&
                  s.scene.world.world.facilities.at(inn).sales == sales + expected &&
                  s.scene.world.world.human_spending.at(definition) == spending + expected &&
                  s.facility_monthly_cash.at(inn).at(month)[0] == monthly + expected &&
                  s.scene.world.world.ai.next_cash_id == cash_id + 1 && s.sound_requests.empty(),
              "actual Owner arrival posts refreshed price once to cash sales human and month bucket");
    }
}
void expansion_map_and_world() {
    auto s = test_support::world_fixture();
    const auto money = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    for (int level = 1; level <= 2; ++level) {
        const int old_left_definition = level == 1 ? 75 : 77;
        const std::array<std::uint64_t, 4> old{{source_facility(s, old_left_definition),
                                                source_facility(s, old_left_definition + 1),
                                                source_facility(s, 83), source_facility(s, 84)}};
        check(expand_startup_world_map(s), "source initial map expands through both real levels");
        const auto &town = s.scene.world.town;
        check(s.fence_level == level && town.left == (level == 1 ? 3 : 1) &&
                  town.right == (level == 1 ? 20 : 22) && town.top == 2 &&
                  town.bottom == (level == 1 ? 11 : 12) && s.scene.world.world.map.width == 24 &&
                  s.scene.world.world.map.height == 24 &&
                  s.scene.world.world.map.cells.size() == 576 && s.surface.size() == 576 &&
                  s.road_patches.size() == 576 && s.scene.world.facility_order.size() == 8,
              "source fences expand 3,11..20,2 then1,12..22,2 without resizing or entity growth");
        for (const auto id : old)
            retired_facility_records(s, id);
        for (int side = 0; side < 2; ++side) {
            const int entrance_definition = (level == 1 ? 77 : 79) + side;
            const auto top = source_facility(s, entrance_definition);
            const auto bottom = source_facility(s, 83 + side);
            const ref::Position upper{11 + side, 2}, lower{11 + side, 10 + level};
            const auto &upper_cell = s.scene.world.world.map.cells.at(upper.y * 24 + upper.x);
            const auto &lower_cell = s.scene.world.world.map.cells.at(lower.y * 24 + lower.x);
            check(s.scene.world.world.facilities.at(top).placement.anchor == upper &&
                      s.scene.world.world.facilities.at(bottom).placement.anchor == lower &&
                      upper_cell.facility->instance_id.value == top &&
                      upper_cell.legacy_state == 6 &&
                      lower_cell.facility->instance_id.value == bottom &&
                      lower_cell.legacy_state == 7 &&
                      s.surface.at(lower.y * 24 + lower.x).instance == 2 + side &&
                      s.surface.at(upper.y * 24 + upper.x).instance == -1 &&
                      s.facility_original_ids.at(bottom) == side,
                  "kind4 replacements and relocated double entrance retain source tiles/m/rawID");
        }
        check(s.scene.world.world.ai.accounting.funds() == money &&
                  s.scene.random.draws() == draws && s.sound_requests.empty(),
              "map effect alone emits no player build charges, sound or random draws");
        const auto projected = startup_world_runtime_adapter().entry.read(s);
        check(projected.generation_bounds == std::array<ref::Position, 2>{{{4, 21}, {19, 15}}},
              "expanded world entry still reads original h.m[0], not fence-level index");
        auto running = s;
        for (int n = 0; n < 3; ++n) {
            const auto step = prepare_startup_world_runtime(running);
            check(step.candidate.has_value(), "expanded Owner admits actual common world rounds");
            running = *step.candidate;
        }
    }
    // 普通建设/道路仍走真实Owner命令，位置在旧村界外、扩张后内圈，不开放定义或补款。
    const ref::Position tree{4, 3}, road{5, 3};
    const auto quote = startup_world_build_quote(s, 66);
    check(quote && begin_startup_world_build(s, 66).error == StartupWorldRuntimeError::none,
          "real available tree catalogue can start construction after expansion");
    const auto built = confirm_startup_world_build(s, tree, ref::FacilityOrientation::first);
    check(built.created && cancel_startup_world_build(s) == StartupWorldRuntimeError::none &&
              s.scene.world.world.ai.accounting.funds() == money - quote->construction_cost,
          "new interior accepts actual source construction and charges its real price");
    check(begin_startup_world_road(s, 18).error == StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, road, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, road, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              cancel_startup_world_edit(s) == StartupWorldRuntimeError::none &&
              s.scene.world.world.ai.accounting.funds() == money - quote->construction_cost - 10 &&
              s.scene.random.draws() == draws &&
              prepare_startup_world_runtime(s).candidate.has_value(),
          "expanded interior road costs10 without RNG and full Owner continues after editing");
}

// 真实任务工厂准备kind1洞穴类实例；移动到指定格只是几何条件夹具，不宣称自然任务选址。
struct TaskFacilityFixture {
    std::shared_ptr<StartupWorldRules> geometry_rules;
    StartupWorldRuntimeState state;
    std::uint64_t facility;
};
TaskFacilityFixture task_facility_fixture(ref::Position position, bool pair = false) {
    auto s = test_support::world_fixture();
    const auto created = ref::prepare_world_task_creation(startup_world_runtime_factory(s), 0);
    check(created.candidate && created.candidate->created_task &&
              write_startup_world_runtime_factory(s, created.candidate->state),
          "real original task factory creates the special facility fixture");
    const auto id = *s.tasks.at(*created.candidate->created_task).facility;
    auto &f = s.scene.world.world.facilities.at(id);
    check(f.kind == 1, "mixed-reference fixture uses actual source kind1, never a fake tree");
    for (const auto old : s.sites.at(id).occupied_cells) {
        const auto at = old.y * 24 + old.x;
        s.scene.world.world.map.cells.at(at) = {4, ref::RouteCategory::ground, {}};
        s.surface.at(at).definition = s.ground_definition;
        s.surface.at(at).instance = s.surface.at(at).fragment = -1;
    }
    std::shared_ptr<StartupWorldRules> geometry_rules;
    if (pair) {
        // 原表flag16实例均单格；局部双格只覆盖“任一占地”，不改发布原表或声称原版双格洞穴。
        geometry_rules = std::make_shared<StartupWorldRules>(*s.rules);
        geometry_rules->facilities.at(f.placement.definition_id).shape =
            static_cast<int>(ref::FacilityShape::pair);
        s.rules = geometry_rules.get();
        f.placement.shape = ref::FacilityShape::pair;
    }
    f.placement.anchor = position;
    s.tasks.at(*created.candidate->created_task).site = position;
    const auto footprint =
        ref::facility_footprint(f.placement.shape, f.placement.orientation, position, 24, 24);
    check(footprint.error == ref::GeometryError::none, "fixture footprint is inside real map");
    s.sites.at(id).occupied_cells.clear();
    for (const auto &cell : footprint.cells) {
        const auto at = cell.position.y * 24 + cell.position.x;
        check(!s.scene.world.world.map.cells.at(at).facility,
              "fixture never overwrites an existing real instance");
        s.scene.world.world.map.cells.at(at) = {
            8, ref::RouteCategory::terminal,
            ref::FacilityTileBinding{{id}, f.placement.definition_id, cell.fragment_index}};
        s.surface.at(at).definition = f.placement.definition_id;
        s.surface.at(at).variant = cell.fragment_index;
        s.surface.at(at).instance = 3; // 原m方向独立于x实例，检验铺撤不得清除。
        s.sites.at(id).occupied_cells.push_back(cell.position);
    }
    check(refresh_startup_world_map(s, false), "source task fixture has coherent map and identity");
    return {std::move(geometry_rules), std::move(s), id};
}
void expansion_cleanup_and_rejections() {
    for (const bool pair : {false, true}) {
        auto fixture =
            task_facility_fixture(pair ? ref::Position{4, 2} : ref::Position{4, 3}, pair);
        auto &s = fixture.state;
        const auto old_order = s.scene.world.facility_order;
        const auto old_next = s.next_facility_identity;
        check(!expand_startup_world_map(s) && s.fence_level == 0 &&
                  s.scene.world.facility_order == old_order &&
                  s.next_facility_identity == old_next &&
                  s.scene.world.world.facilities.count(fixture.facility) && !s.tasks.empty(),
              "unreachable relocated task-site retirement refuses without dangling task reference");
        // 以下仅验证flag16/占地扫描；固定原任务区域不在扩张范围，不保留伪造任务关系。
        s.tasks.clear();
        s.task_order.clear();
        s.task_original_ids.clear();
        check(expand_startup_world_map(s),
              "flag16 removal scans past later non16 kind4 and checks every footprint cell");
        retired_facility_records(s, fixture.facility);
    }
    const auto baseline = test_support::world_fixture();
    for (int fault = 0; fault < 5; ++fault) {
        auto broken = baseline;
        const auto id = source_facility(broken, 75);
        const auto p = broken.scene.world.world.facilities.at(id).placement.anchor;
        if (fault == 0)
            broken.scene.world.world.map.cells.at(p.y * 24 + p.x).facility->instance_id.value = 999;
        else if (fault == 1)
            broken.facility_original_ids.erase(id);
        else if (fault == 2)
            broken.neighbourhood_details.erase(id);
        else if (fault == 3)
            broken.scene.world.world.facility_uses.erase(77);
        else
            broken.base_variants.pop_back();
        check(!expand_startup_world_map(broken) && broken.fence_level == 0 &&
                  broken.scene.world.town.left == 6 &&
                  broken.scene.world.facility_order == baseline.scene.world.facility_order &&
                  broken.next_facility_identity == baseline.next_facility_identity &&
                  broken.scene.random.draws() == baseline.scene.random.draws() &&
                  broken.scene.world.world.ai.accounting.funds() ==
                      baseline.scene.world.world.ai.accounting.funds(),
              "invalid binding or missing required projection rejects expansion atomically");
    }
}
void expansion_human_cleanup() {
    auto baseline = first_arrival_fixture();
    ref::EncounterCreationInput encounter;
    encounter.kind = 0;
    encounter.center = {10, 18};
    encounter.year_index = 0;
    encounter.month_index = 3;
    const auto spawned = prepare_startup_world_runtime_encounter(baseline, encounter);
    check(spawned && !spawned->scene.world.world.ai.monster_order.empty(),
          "actual source encounter provides a real monster for bl/bm separation");
    baseline = *spawned;
    const auto human = baseline.scene.world.world.ai.human_order.front();
    const auto monster = baseline.scene.world.world.ai.monster_order.front();
    struct Case {
        ref::Position cell;
        bool old_ax;
        bool cleanup;
    };
    for (const auto &test : std::array<Case, 4>{{{{4, 3}, false, true},
                                                 {{3, 2}, false, true},
                                                 {{2, 3}, false, false},
                                                 {{4, 3}, true, false}}}) {
        auto s = baseline;
        auto &ai = s.scene.world.world.ai;
        auto &a = ai.battle.actors.at(human);
        a.position.height = 9;
        a.control.state = 0;
        a.control.flags = 1121; // 1024|97，原r清97、置514并插等待120。
        a.control.queue = {{1, 9, 0}};
        ai.contexts.at(human).cell = test.cell;
        ai.contexts.at(human).inside_town = test.old_ax;
        ai.contexts.at(monster).cell = {4, 3};
        ai.contexts.at(monster).inside_town = false;
        auto &route = s.scene.world.world.actors.at(human);
        route.unbound_route = ref::LegacyPathResult{ref::MapAccessError::none, {{4, 3}, {5, 3}}, 1};
        const auto old_actor = a;
        const auto old_monster = ai.battle.actors.at(monster);
        const auto old_humans = ai.human_order, old_monsters = ai.monster_order;
        const auto draws = s.scene.random.draws();
        check(expand_startup_world_map(s), "source expansion accepts actual actor-owner fixture");
        const auto &after = s.scene.world.world.ai;
        const auto &h = after.battle.actors.at(human);
        const auto &m = after.battle.actors.at(monster);
        check(after.human_order == old_humans && after.monster_order == old_monsters &&
                  h.object_slot == old_actor.object_slot &&
                  s.scene.world.world.actors.at(human).unbound_route->steps ==
                      std::vector<ref::Position>{{4, 3}, {5, 3}} &&
                  m.control.state == old_monster.control.state &&
                  m.control.flags == old_monster.control.flags &&
                  m.control.queue == old_monster.control.queue &&
                  m.position.height == old_monster.position.height &&
                  s.scene.random.draws() == draws,
              "expansion preserves both rosters, monster state, human N/path and random stream");
        if (test.cleanup)
            check(h.control.state == 19 && h.control.flags == 1538 && h.position.height == 0 &&
                      h.control.queue == std::vector<ref::LegacyActorControl>{{1, 120, 0}, {8, 5}},
                  "only old !ax human inside new closed bounds receives exact r state19/120/act5");
        else
            check(h.control.state == old_actor.control.state &&
                      h.control.flags == old_actor.control.flags &&
                      h.control.queue == old_actor.control.queue && h.position.height == 9,
                  "old ax=true or outside new closed bounds leaves human control unchanged");
    }
}
void road_special_reference() {
    auto fixture = task_facility_fixture({7, 3});
    auto s = std::move(fixture.state);
    const auto id = fixture.facility;
    const ref::Position p{7, 3};
    const auto at = p.y * 24 + p.x;
    const auto original = s.scene.world.world.map.cells.at(at).facility;
    const auto raw = s.facility_original_ids.at(id);
    const auto order = s.scene.world.facility_order;
    const auto draws = s.scene.random.draws();
    const auto funds = s.scene.world.world.ai.accounting.funds();
    check(begin_startup_world_road(s, 18).error == StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, p, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none,
          "actual road selects a source kind1 task-site cell");
    auto broken = s;
    broken.scene.world.world.map.cells.at(at).facility->instance_id.value = 999999;
    check(confirm_startup_world_edit(broken, p, ref::FacilityOrientation::first).error !=
                  StartupWorldRuntimeError::none &&
              broken.surface.at(at).definition == original->definition_id &&
              broken.scene.world.world.ai.accounting.funds() == funds &&
              broken.scene.random.draws() == draws && broken.build_mode == 2,
          "stale special reference rejects whole road candidate before cash or mode publication");
    const auto paved = confirm_startup_world_edit(s, p, ref::FacilityOrientation::first);
    if (paved.error != StartupWorldRuntimeError::none)
        throw std::runtime_error("source kind1 road transaction failed: error=" +
                                 std::to_string(static_cast<int>(paved.error)) +
                                 " definition=" + std::to_string(original->definition_id) +
                                 " instance=" + std::to_string(id));
    check(paved.error == StartupWorldRuntimeError::none && s.surface.at(at).definition == 18 &&
              s.surface.at(at).instance == 3 &&
              s.scene.world.world.map.cells.at(at).legacy_state == 3 &&
              s.scene.world.world.map.cells.at(at).facility->instance_id.value == id &&
              s.scene.world.world.map.cells.at(at).facility->definition_id ==
                  original->definition_id &&
              s.scene.world.facility_order == order && s.facility_original_ids.at(id) == raw &&
              s.scene.world.world.ai.accounting.funds() == funds - 10 &&
              s.scene.random.draws() == draws,
          "one changed road tile preserves kind1 x/definition/rawID/m and costs exactly10");
    check(cancel_startup_world_edit(s) == StartupWorldRuntimeError::none &&
              begin_startup_world_edit(s, false).error == StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, p, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              confirm_startup_world_edit(s, p, ref::FacilityOrientation::first).error ==
                  StartupWorldRuntimeError::none &&
              s.surface.at(at).definition == s.ground_definition &&
              s.surface.at(at).instance == 3 &&
              s.scene.world.world.map.cells.at(at).legacy_state == 4 &&
              s.scene.world.world.map.cells.at(at).facility->instance_id.value == id &&
              s.scene.world.facility_order == order && s.facility_original_ids.at(id) == raw &&
              s.scene.world.world.ai.accounting.funds() == funds - 10 &&
              s.scene.random.draws() == draws,
          "removing road restores S ground while retaining source cave and has no refund or RNG");
}
void commerce_definition_preview() {
    auto s = test_support::page_fixture(85);
    const auto parent = s.scripts.pages.back().id;
    const auto initialized = prepare_startup_world_runtime(s);
    check(initialized.candidate && !initialized.candidate->commerce_page_lists.at(parent).empty(),
          "real85 initialization supplies eligible source definitions for preview");
    s = *initialized.candidate;
    const int d = s.commerce_page_lists.at(parent).front();
    const auto row = s.commerce_page_data.at(parent);
    const auto money = s.scene.world.world.ai.accounting.funds();
    const auto count = s.scene.world.facility_order.size();
    const auto flags = s.facility_commerce_read.at(d);
    const auto shared = s.scripts.facilities.at(d).attributes;
    check(open_startup_world_facility_definition(s, d) == StartupWorldRuntimeError::none,
          "85 inspect opens definition-only74 with no installed instance prerequisite");
    const auto page = s.scripts.pages.back().id;
    check(valid_startup_world_facility_page(s, s.scripts.pages.back()) &&
              s.facility_definition_page_bindings.at(page) == d &&
              !s.facility_page_bindings.count(page) && !s.facility_page_neighbours.count(page) &&
              startup_world_facility_page_count(s, s.scripts.pages.back()) == 1,
          "preview retains only definition identity and has no artificial instance or neighbours");
    for (int fault = 0; fault < 5; ++fault) {
        auto broken = s;
        if (fault == 0)
            broken.facility_definition_page_bindings.erase(page);
        if (fault == 1)
            broken.page_counters.erase(page);
        if (fault == 2)
            broken.page_phases.erase(page);
        if (fault == 3)
            broken.facility_page_bindings[page] = source_facility(broken, 33);
        if (fault == 4) {
            ref::WorldScriptPage wrong;
            wrong.id = broken.scripts.next_page_id++;
            wrong.kind = ref::WorldScriptPageKind::raw_page;
            wrong.legacy_page = 83;
            broken.scripts.pages.insert(broken.scripts.pages.end() - 1, wrong);
        }
        check(
            !prepare_startup_world_runtime(broken).candidate &&
                act_startup_world_facility_page(broken, page, StartupFacilityPageAction::confirm) !=
                    StartupWorldRuntimeError::none,
            "damaged initialized definition preview explicitly refuses without map.at exceptions");
    }
    check(act_startup_world_facility_page(s, page, static_cast<StartupFacilityPageAction>(99)) ==
                  StartupWorldRuntimeError::invalid_page &&
              open_startup_world_facility_items(s, page) != StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle != 4 && s.scene.world.facility_order.size() == count,
          "unknown preview action and forced instance-item entry reject without closing or "
          "fabricating instance");
    check(act_startup_world_facility_page(s, page, StartupFacilityPageAction::next) ==
                  StartupWorldRuntimeError::none &&
              s.page_phases.at(page) == 0 &&
              act_startup_world_facility_page(s, page, StartupFacilityPageAction::confirm) ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 && s.commerce_page_data.at(parent) == row &&
              s.scene.world.world.ai.accounting.funds() == money &&
              s.facility_commerce_read.at(d) == flags &&
              s.scripts.facilities.at(d).attributes == shared &&
              s.scene.world.facility_order.size() == count,
          "read-only74 cannot turn page or consume goods; confirm closes only itself preserving85");
    const auto retired = prepare_startup_world_runtime(s);
    check(retired.candidate && !retired.candidate->facility_definition_page_bindings.count(page) &&
              retired.candidate->commerce_page_data.at(parent) == row,
          "definition preview payload retires on real framework update while85 selection survives");
}
} // namespace
int main() {
    try {
        normal_construction();
        progression_building_unlocks();
        new_world_inheritance();
        multi_tile_and_rollback();
        new_shop_projection();
        details();
        facility_bonus_rows();
        shared_upgrade();
        menu_and_current_quotes();
        residence();
        road_editing();
        move_remove_and_stale_actor();
        neighbour_changes_reach_actual_income();
        expansion_map_and_world();
        expansion_cleanup_and_rejections();
        expansion_human_cleanup();
        road_special_reference();
        residence_rebuild();
        commerce_definition_preview();
        facility_item_pages();
        facility_item_empty_and_legend();
        std::cout << "startup world building: " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
