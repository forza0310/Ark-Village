#include "dungeon_village_prototype/startup_world_building.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
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
    check(s.scene.world.world.ai.accounting.funds() == funds - 1000 &&
              s.facility_definitions.at(28).popularity_reward == 1,
          "construction completion does not charge again and consumes source sharedN minimum");
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
    const auto funds = s.scene.world.world.ai.accounting.funds();
    const auto sound_count = s.sound_requests.size();
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->facility_upgrade_initialized.count(page) &&
              tick.candidate->sound_requests.size() == sound_count + 1 &&
              tick.candidate->sound_requests.back() == 20 &&
              tick.candidate->scene.world.world.facility_uses.at(definition).level == 2 &&
              tick.candidate->scene.world.world.facility_uses.at(definition).completed_uses == 7 &&
              tick.candidate->scene.world.world.facility_uses.at(definition).upgrade_pending &&
              tick.candidate->scene.world.updates == 0 && tick.candidate->scene.random.draws() == 0,
          "first initialization upgrades shared definition once, keeps hint until close, freezes "
          "world");
    s = *tick.candidate;
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
    for (int n = 0; n < 4; ++n)
        check(acknowledge_startup_world_runtime_page(s, page) == StartupWorldRuntimeError::none,
              "source40/phase1/55/close confirmations execute");
    check(!s.scene.world.world.facility_uses.at(definition).upgrade_pending &&
              s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.accounting.funds() == funds,
          "only finished page clears shared hint without charging or resetting uses");
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
              early.page_counters.at(early_page) == 40,
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
} // namespace
int main() {
    try {
        normal_construction();
        multi_tile_and_rollback();
        details();
        shared_upgrade();
        menu_and_current_quotes();
        residence();
        std::cout << "startup world building: " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
