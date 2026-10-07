#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_expansion.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_tax.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
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
const auto fixture = test_support::page_fixture;
void page_tick(StartupWorldRuntimeState &s);
void facility_commodity_pages() {
    using A = StartupFacilityCatalogAction;
    using E = StartupWorldRuntimeError;
    for (const auto setting : std::array<std::array<int, 3>, 3>{{{1, 1, 0}, {4, 2, 1}, {5, 3, 3}}}) {
        auto s = fixture(79);
        s.scripts.pages.back().lifecycle = 0; // 新插入页必须先经过真实初始化。
        const auto id = s.scripts.pages.back().id;
        s.scripts.pages.back().legacy_f = setting[0];
        StartupWorldRules rules = *s.rules; // 私有排序夹具，不改冻结原表或共享规则。
        s.rules = &rules;
        std::vector<int> source_ids;
        for (auto &d : rules.equipment)
            if (d.shop.kind == setting[1]) {
                auto &c = s.catalog.at({setting[1], d.shop.id});
                c.status = 0;
                c.newly_unlocked = true;
                if (source_ids.size() < 6) {
                    static constexpr int order[]{2, 2, 1, 3, 4, 0};
                    d.gift_order = order[source_ids.size()];
                    c.status = 1;
                    source_ids.push_back(d.shop.id);
                }
            }
        check(source_ids.size() == 6, "real equipment category supplies six independent order inputs");
        const std::vector<int> expected{source_ids[5], source_ids[2], source_ids[1],
                                        source_ids[0], source_ids[3], source_ids[4]};
        s.catalog.at({0, 0}).newly_unlocked = true;
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto draws = s.scene.random.draws();
        const auto date = s.scene.calendar.units;
        check(!inspect_startup_world_facility_catalog_page(s, id),
              "79 query cannot initialize its own payload or change NEW flags");
        page_tick(s);
        auto view = inspect_startup_world_facility_catalog_page(s, id);
        check(view && view->entries == expected && view->selection == 0 &&
                  view->first_visible == 0 && view->phase == 0 && view->binding == -1,
              "79 original swap traversal reverses these equal keys instead of stable sorting");
        check(act_startup_world_facility_catalog_page(s, id, A::select, 4) == E::none,
              "79 selects fifth original ordered row");
        view = inspect_startup_world_facility_catalog_page(s, id);
        check(view && view->selection == 4 && view->first_visible == 1,
              "79 four-row viewport scrolls one row at the fifth choice");
        check(act_startup_world_facility_catalog_page(s, id, A::previous_tab) == E::none &&
                  s.page_phases.at(id) == 1 &&
                  act_startup_world_facility_catalog_page(s, id, A::next_tab) == E::none &&
                  s.page_phases.at(id) == 0,
              "79 left/right cycle exactly two attribute pages without moving selection");
        check(act_startup_world_facility_catalog_page(s, id, A::select, 6) == E::invalid_page &&
                  s.facility_catalog_page_data.at(id)[1] == 4,
              "79 out of catalogue selection explicitly rejects without partial scroll");
        check(act_startup_world_facility_catalog_page(s, id, A::inspect) == E::none,
              "79 information input inserts actual72 rather than purchasing equipment");
        const auto child = s.scripts.pages.back().id;
        page_tick(s);
        view = inspect_startup_world_facility_catalog_page(s, child);
        check(view && view->raw == 72 && view->mode == setting[2] &&
                  view->binding == expected[4] && view->phase == 4 &&
                  s.facility_catalog_page_parents.at(child) == id &&
                  s.catalog.at({setting[1], expected[4]}).newly_unlocked,
              "72 binds selected original equipment and retains parent category NEW flags");
        for (int fault = 0; fault < 3; ++fault) {
            auto broken = s;
            if (fault == 0)
                broken.facility_catalog_page_parents.at(child) = 999;
            else if (fault == 1) {
                broken.scripts.pages.front().legacy_page = 79;
                broken.facility_catalog_page_parents.at(child) = 1;
            }
            else
                broken.facility_catalog_page_data.at(id)[1] = 99;
            check(!inspect_startup_world_facility_catalog_page(broken, child) &&
                      act_startup_world_facility_catalog_page(broken, child, A::cancel) == E::missing_source &&
                      broken.scene.world.world.ai.accounting.funds() == cash &&
                      broken.catalog.at({setting[1], expected[4]}).newly_unlocked,
                  "72 rejects missing/wrong or damaged parent with explicit failure and no charge");
        }
        check(cancel_startup_world_runtime_page(s, child) == E::none &&
                  s.catalog.at({setting[1], expected[4]}).newly_unlocked,
              "72 return closes only information and leaves complete category NEW untouched");
        page_tick(s);
        check(!s.facility_catalog_page_parents.count(child) &&
                  !s.facility_catalog_page_data.count(child) &&
                  !s.facility_catalog_page_lists.count(child),
              "72 retired payload and parent reference are consumed by framework cleanup");
        view = inspect_startup_world_facility_catalog_page(s, id);
        check(view && view->selection == 4 && view->first_visible == 1,
              "information returns to original79 selection and scroll");
        for (bool cancel : {false, true}) {
            auto close_state = s;
            const auto error = cancel ? cancel_startup_world_runtime_page(close_state, id)
                                      : acknowledge_startup_world_runtime_page(close_state, id);
            bool all_clear = true;
            for (const auto &entry : close_state.catalog)
                if (entry.first.first == setting[1])
                    all_clear = all_clear && !entry.second.newly_unlocked;
            check(error == E::none && all_clear && close_state.catalog.at({0, 0}).newly_unlocked &&
                      close_state.scene.world.world.ai.accounting.funds() == cash &&
                      close_state.scene.random.draws() == draws && close_state.scene.calendar.units == date,
                  "79 confirm and cancel clear all category NEW including unavailable rows only");
            check(act_startup_world_facility_catalog_page(close_state, id, A::confirm) == E::invalid_page,
                  "retired79 cannot clear or buy again through stale input");
        }
        for (int fault = 0; fault < 4; ++fault) {
            auto broken = s;
            if (fault == 0) broken.facility_catalog_page_data.erase(id);
            else if (fault == 1) broken.facility_catalog_page_lists.erase(id);
            else if (fault == 2) broken.facility_catalog_page_data.at(id)[1] = 999;
            else broken.page_phases.at(id) = 2;
            check(!prepare_startup_world_runtime(broken).candidate &&
                      act_startup_world_facility_catalog_page(broken, id, A::confirm) == E::missing_source &&
                      broken.scene.random.draws() == draws &&
                      broken.catalog.at({setting[1], expected[4]}).newly_unlocked,
                  "initialized79 missing/bad payload rejects explicitly, preserving NEW/random");
        }
        auto late_failure = s;
        late_failure.scripts.pages.front().id = 0; // 下层页身份损坏，候选清r不得越过Owner关闭校验。
        check(act_startup_world_facility_catalog_page(late_failure, id, A::confirm) == E::script_failed &&
                  late_failure.catalog.at({setting[1], expected[4]}).newly_unlocked &&
                  late_failure.scripts.pages.back().lifecycle != 4,
              "79 late close failure rolls back whole-category NEW clearing and page retirement");
    }
    auto empty = fixture(79);
    empty.scripts.pages.back().lifecycle = 0;
    empty.scripts.pages.back().legacy_f = 1;
    for (auto &entry : empty.catalog)
        if (entry.first.first == 1)
            entry.second.status = 0;
    check(!prepare_startup_world_runtime(empty).candidate &&
              empty.facility_catalog_pages_initialized.empty() &&
              empty.facility_catalog_page_lists.empty(),
          "79 genuinely empty category rejects initialization without retained half payload");
}
void facility_reputation_pages() {
    using A = StartupFacilityCatalogAction;
    using E = StartupWorldRuntimeError;
    for (int mode : {0, 1}) {
        auto s = fixture(82);
        s.scripts.pages.back().lifecycle = 0;
        const auto id = s.scripts.pages.back().id;
        s.scripts.pages.back().legacy_f = mode;
        s.scripts.pages.back().facility_definition = mode == 0 ? 36 : 45;
        StartupWorldRules rules = *s.rules;
        s.rules = &rules;
        // 原序/当前职业夹具；场上无人仍应展示共享定义，不借实例或出生职业。
        std::swap(rules.humans.at(1), rules.humans.at(4));
        const int wanted_type = mode == 0 ? 1 : 0;
        const auto selected_job = std::find_if(rules.jobs.begin(), rules.jobs.end(),
                                              [wanted_type](const auto &j) { return j.type == wanted_type; });
        const auto excluded_job = std::find_if(rules.jobs.begin(), rules.jobs.end(),
                                              [wanted_type](const auto &j) { return j.type != wanted_type; });
        check(selected_job != rules.jobs.end() && excluded_job != rules.jobs.end(),
              "real profession table supplies both target and excluded categories");
        for (auto &entry : s.human_presence)
            entry.second = 0;
        for (int human : {1, 2, 3, 4}) {
            s.human_presence.at(human) = 1;
            s.scene.world.world.ai.growth.at(human).definition.current_profession =
                static_cast<int>(selected_job - rules.jobs.begin());
        }
        s.human_presence.at(5) = 1;
        s.scene.world.world.ai.growth.at(5).definition.current_profession =
            static_cast<int>(excluded_job - rules.jobs.begin());
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto random = s.scene.random.draws();
        const auto popularity = s.popularity;
        const auto date = s.scene.calendar.units;
        s.scene.world.popularity_queue = {{7, 3, 0}};
        check(!inspect_startup_world_facility_catalog_page(s, id),
              "82 query does not fabricate frozen members before initialization");
        page_tick(s);
        auto view = inspect_startup_world_facility_catalog_page(s, id);
        check(view && view->entries == std::vector<int>({4, 2, 3}) && view->phase == 0 &&
                  view->counter == 1 && view->binding == (mode == 0 ? 36 : 45) &&
                  s.scene.world.world.ai.human_order.empty() && s.scene.random.draws() == random &&
                  s.sound_requests == std::vector<int>{4},
              "82 first update freezes original-order first3 current-job definitions,sound4,no draw");
        s.sound_requests.clear(); // 消费已发声音，不把输出留为持久历史。
        page_tick(s);
        check(s.sound_requests.empty(), "82 later phase0 update does not replay its count1 sound");
        for (int fault = 0; fault < 5; ++fault) {
            auto broken = s;
            if (fault == 0) broken.facility_catalog_page_data.erase(id);
            else if (fault == 1) broken.facility_catalog_page_lists.erase(id);
            else if (fault == 2) broken.scripts.pages.back().facility_definition.reset();
            else if (fault == 3) broken.page_phases.at(id) = 2;
            else broken.facility_catalog_page_lists.at(id).push_back(1);
            check(!prepare_startup_world_runtime(broken).candidate &&
                      act_startup_world_facility_catalog_page(broken, id, A::confirm) == E::missing_source &&
                      broken.popularity == popularity &&
                      broken.scene.world.popularity_queue == std::vector<std::array<int, 3>>{{7, 3, 0}} &&
                      broken.scene.random.draws() == random,
                  "initialized82 rejects damaged binding/list/phase with no eager popularity or draw");
        }
        check(cancel_startup_world_runtime_page(s, id) == E::none && s.page_counters.at(id) == 40 &&
                  s.page_phases.at(id) == 0 && s.scripts.pages.back().lifecycle != 4,
              "82 early return is a fast-forward to40,not an effect-free cancellation");
        check(acknowledge_startup_world_runtime_page(s, id) == E::none &&
                  s.page_phases.at(id) == 1 && s.page_counters.at(id) == 0 &&
                  s.scene.world.popularity_queue == std::vector<std::array<int, 3>>{{7, 3, 0}},
              "82 first40 confirmation starts second phase without awarding popularity");
        page_tick(s);
        check(s.sound_requests.empty() && s.page_counters.at(id) == 1,
              "82 second-phase count1 does not replay phase0 sound4");
        check(acknowledge_startup_world_runtime_page(s, id) == E::none &&
                  s.page_phases.at(id) == 1 && s.page_counters.at(id) == 40 &&
                  s.popularity == popularity,
              "82 second phase early confirmation only fast-forwards its own40 gate");
        auto late_failure = s;
        late_failure.scripts.pages.front().id = 0; // 完整页栈校验失败，已候选头插I必须回滚。
        check(act_startup_world_facility_catalog_page(late_failure, id, A::confirm) == E::script_failed &&
                  late_failure.scene.world.popularity_queue ==
                      std::vector<std::array<int, 3>>{{7, 3, 0}} &&
                  late_failure.scripts.pages.back().lifecycle != 4 &&
                  late_failure.scene.random.draws() == random,
              "82 close failure rolls back already-prepared delayed20 request and lifecycle");
        check(cancel_startup_world_runtime_page(s, id) == E::none &&
                  s.scripts.pages.back().lifecycle == 4 &&
                  s.scene.world.popularity_queue == std::vector<std::array<int, 3>>{{10, 20, 1}, {7, 3, 0}} &&
                  s.popularity == popularity && s.scene.random.draws() == random &&
                  s.scene.calendar.units == date && s.scene.world.world.ai.accounting.funds() == cash,
              "82 final return head-inserts10/20/1 once and closes,without immediate popularity");
        check(acknowledge_startup_world_runtime_page(s, id) == E::invalid_page &&
                  s.scene.world.popularity_queue.size() == 2,
              "stale82 final confirm cannot repeat the20-point delayed request");
    }
    auto fallback = fixture(82);
    fallback.scripts.pages.back().lifecycle = 0;
    const auto id = fallback.scripts.pages.back().id;
    fallback.scripts.pages.back().facility_definition = 36;
    for (auto &entry : fallback.human_presence)
        entry.second = 0;
    page_tick(fallback);
    check(fallback.facility_catalog_page_lists.at(id) == std::vector<int>{0} &&
              fallback.human_presence.at(0) == 0 && fallback.scene.random.draws() == 0,
          "82 no eligible person falls back to definition0 without opening/creating/drawing it");
}
StartupWorldRuntimeState human_fixture(int raw) {
    auto s = fixture(raw);
    s.page_human_bindings[s.scripts.pages.back().id] = 1;
    // 管理页合同夹具：绕开首次说明的模态，仅避免它遮住被测栈顶。
    for (int event : {99, 111, 112, 119})
        s.scripts.event_calls[event] = 1;
    return s;
}
void page_tick(StartupWorldRuntimeState &s) {
    const auto r = prepare_startup_world_runtime(s);
    check(r.candidate.has_value(), "managed page actual framework update succeeds");
    s = *r.candidate;
}
void human_details_and_gifts() {
    using A = StartupHumanPageAction;
    using E = StartupWorldRuntimeError;
    auto s = human_fixture(60);
    const auto id = s.scripts.pages.back().id;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    const auto date = s.scene.calendar.units;
    const auto old = startup_world_human_details(s, 1);
    check(!startup_world_human_page_ready(s, id),
          "new human page read-only readiness does not construct a catalogue or advance state");
    check(old && old->equipment == s.shop_humans.at(1).equipment,
          "human query reads current sole definition and equipment, not spawn metadata");
    page_tick(s);
    check(startup_world_human_page_ready(s, id),
          "only successful framework initialization makes detail payload ready");
    for (int tab = 0; tab < 4; ++tab)
        check(act_startup_world_human_page(s, id, A::view_tab, tab) == E::none &&
                  s.page_phases.at(id) == tab,
              "all four detail tabs have explicit current page input");
    check(act_startup_world_human_page(s, id, A::view_tab, 4) == E::invalid_page &&
              s.scene.random.draws() == draws && s.scene.calendar.units == date &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "detail tabs and rejected tab do not advance world/date/random/cash");
    check(act_startup_world_human_page(s, id, A::gifts) == E::none,
          "actual human detail opens equipment gift catalogue");
    const auto gift = s.scripts.pages.back().id;
    page_tick(s);
    const auto &list = s.equipment_page_catalogs.at(gift)[0];
    check(!list.empty(), "real initial weapon catalogue is nonempty");
    const int weapon = list.front();
    s.catalog.at({1, weapon}).free_purchases = 1;
    s.catalog.at({1, weapon}).inventory = 7; // 两种库存刻意不同，防止接错ObjectCatalog字段。
    check(act_startup_world_human_page(s, gift, A::confirm) == E::none,
          "gift catalogue opens exact selected weapon confirmation");
    const auto question = s.scripts.pages.back().id;
    const auto satisfaction = s.shop_humans.at(1).satisfaction;
    for (int fault = 0; fault < 4; ++fault) {
        auto invalid = s;
        auto &parent_page =
            *std::find_if(invalid.scripts.pages.begin(), invalid.scripts.pages.end(),
                          [&](const auto &p) { return p.id == gift; });
        if (fault == 0)
            parent_page.lifecycle = 4;
        if (fault == 1)
            parent_page.legacy_page = 60;
        if (fault == 2)
            invalid.page_human_bindings.at(gift) = 2;
        if (fault == 3)
            invalid.human_equipment_choices.at(gift)[1] = -1;
        check(acknowledge_startup_world_runtime_page(invalid, question) == E::missing_source &&
                  invalid.human_page_answers.empty() &&
                  invalid.scene.world.world.ai.accounting.funds() == cash &&
                  invalid.catalog.at({1, weapon}).free_purchases == 1,
              "65 rejects retired, wrong-kind, different-human or mismatched-choice parent "
              "atomically");
    }
    check(acknowledge_startup_world_runtime_page(s, question) == E::none &&
              s.catalog.at({1, weapon}).free_purchases == 1 &&
              s.shop_humans.at(1).satisfaction == satisfaction &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "raw65 confirm only returns K0; no eager charge, inventory or reward");
    check(act_startup_world_human_page(s, gift, A::confirm) == E::invalid_page,
          "resumed parent answer cannot be bypassed by another input");
    auto broken = s;
    broken.scene.random =
        ref::WorldRandomStream::from_raw({}); // 晚期文案抽签无票，整个消费必须回滚。
    const auto result = prepare_startup_world_runtime(broken);
    check(!result.candidate && broken.catalog.at({1, weapon}).free_purchases == 1 &&
              broken.shop_humans.at(1).satisfaction == satisfaction &&
              broken.human_page_answers.at(gift) == 0,
          "late gift dialogue failure rolls back stock, reward and parent answer");
    page_tick(s);
    check(s.shop_humans.at(1).equipment[0] == weapon &&
              s.catalog.at({1, weapon}).free_purchases == 0 &&
              s.catalog.at({1, weapon}).inventory == 7 &&
              s.scene.world.world.ai.accounting.funds() == cash &&
              s.shop_humans.at(1).reselect[0] == 6 && s.scene.random.draws() == draws + 1,
          "resumed64 commits stock-only gift, final setter, cooldown6 and one draw2");
    check(acknowledge_startup_world_runtime_page(s, question) == E::invalid_page,
          "retired65 cannot grant twice");
    // 取消问答复用同一目录，不发礼物或随机；不另建一次性bug套件。
    s = human_fixture(64);
    const auto parent = s.scripts.pages.back().id;
    page_tick(s);
    check(act_startup_world_human_page(s, parent, A::confirm) == E::none,
          "cash-backed weapon quote opens65");
    const auto cancelled = s.scripts.pages.back().id;
    const auto before = s.scene.world.world.ai.accounting.funds();
    check(cancel_startup_world_runtime_page(s, cancelled) == E::none,
          "65 cancel returns K1 instead of deleting parent");
    page_tick(s);
    check(s.scene.world.world.ai.accounting.funds() == before && s.scene.random.draws() == 0 &&
              s.scripts.pages.back().id == parent,
          "resumed64 cancellation keeps cash/random/catalogue unchanged");
    const int selected_weapon = s.equipment_page_catalogs.at(parent)[0].front();
    s.catalog.at({1, selected_weapon}).newly_unlocked = true;
    check(act_startup_world_human_page(s, parent, A::inspect_equipment) == E::none &&
              !startup_world_human_page_ready(s, s.scripts.pages.back().id) &&
              s.catalog.at({1, selected_weapon}).newly_unlocked,
          "selected equipment has read-only raw73 details");
    page_tick(s);
    check(s.scripts.pages.back().legacy_page == 73 &&
              acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) == E::none &&
              s.scene.world.world.ai.accounting.funds() == before && s.scene.random.draws() == 0,
          "equipment details confirm only closes without gift side effects");
}
void human_profession_and_mastery() {
    using A = StartupHumanPageAction;
    using E = StartupWorldRuntimeError;
    auto s = human_fixture(61);
    const auto id = s.scripts.pages.back().id;
    const int old_job = s.scene.world.world.ai.growth.at(1).definition.current_profession;
    const auto original_job_counts = s.scripts.job_counts;
    // 最小缓存夹具：同定义的活跃/保留实例上限随共享重算，当前HP保持原值。
    ref::BattleActorRecord active;
    active.id = {501};
    active.definition = 1;
    active.capacity = s.scene.world.world.ai.growth.at(1).derived.combat[0];
    active.hp.target = active.hp.displayed = 9;
    s.scene.world.world.ai.battle.actors.emplace(active.id, active);
    auto retained = active;
    retained.id = {502};
    s.scene.world.world.ai.retired_actors.emplace(retained.id, retained);
    s.village_points = 999;
    s.scene.world.world.ai.growth.at(1).definition.legacy_u = 7; // 3点奖励跨十位，强制实际67接线。
    s.human_calendar.at(1).celebrations = 99;
    for (auto &job : s.scripts.professions)
        job.second.status = 1;
    page_tick(s);
    const auto &list = s.human_page_catalogs.at(id);
    const auto found =
        std::find_if(list.begin(), list.end(), [&](int job) { return job != old_job; });
    check(found != list.end(), "full actual profession directory has an alternative for sex");
    const int job = *found;
    check(act_startup_world_human_page(s, id, A::select, static_cast<int>(found - list.begin())) ==
                  E::none &&
              act_startup_world_human_page(s, id, A::confirm) == E::none,
          "actual61 accepts distinct affordable profession and opens62");
    const auto question = s.scripts.pages.back().id;
    const int cost = s.rules->jobs.at(job).change_points;
    const int effort = s.scene.world.world.ai.growth.at(1).definition.legacy_u;
    auto wrong_parent = s;
    wrong_parent.page_human_bindings.at(id) = 2;
    check(acknowledge_startup_world_runtime_page(wrong_parent, question) == E::missing_source &&
              wrong_parent.village_points == 999 &&
              wrong_parent.human_profession_changes.at(1).at(job) == 0 &&
              wrong_parent.human_page_answers.empty(),
          "62 cannot charge or reward through another human's profession parent");
    check(acknowledge_startup_world_runtime_page(s, question) == E::none &&
              s.village_points == 999 - cost && s.human_profession_changes.at(1).at(job) == 1 &&
              s.scene.world.world.ai.growth.at(1).definition.current_profession == old_job &&
              s.scene.world.world.ai.growth.at(1).definition.legacy_u == effort + 3,
          "62 pays points, rewards old profession, increments R but does not switch t yet");
    const auto animation =
        std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                     [](const auto &p) { return p.lifecycle != 4 && p.legacy_page == 63; });
    check(animation != s.scripts.pages.end(), "confirmation queues real63 animation");
    const auto aid = animation->id;
    page_tick(s);
    check(s.human_pages_initialized.count(aid) && s.page_counters.at(aid) == 0 &&
              s.scene.random.draws() == 1 && s.scripts.pages.back().legacy_page == 67,
          "framework initializes hidden63 before updating result67; draw5 does not wait for focus");
    // 领域67已单独覆盖；这里只按合法确认消掉它，保留63的真实计数时点。
    for (int n = 0; n < 120 && s.scripts.pages.back().id != aid; ++n) {
        page_tick(s);
        if (s.scripts.pages.back().legacy_page == 67)
            check(acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) == E::none,
                  "effort result drains before profession animation");
    }
    check(s.scripts.pages.back().id == aid, "63 becomes actual top after reward result");
    for (int n = s.page_counters[aid]; n < 54; ++n)
        page_tick(s);
    check(s.scene.world.world.ai.growth.at(1).definition.current_profession == old_job,
          "frame54 retains original profession");
    const auto cache = s.scene.world.world.ai.growth.at(1).derived.attributes;
    page_tick(s);
    check(s.page_counters.at(aid) == 55 &&
              s.scene.world.world.ai.growth.at(1).definition.current_profession == job &&
              s.scene.world.world.ai.growth.at(1).derived.attributes == cache &&
              s.scene.random.draws() == 1,
          "exact55 switches shared t only; no eager recalculation or repeated dialogue draw");
    s.scene.world.world.ai.growth.at(1).experience = 9;
    s.scene.world.world.ai.growth.at(1).pending = {4, 8};
    check(acknowledge_startup_world_runtime_page(s, aid) == E::none &&
              s.scene.world.world.ai.growth.at(1).experience == 9,
          "early63 confirm does not clear current experience");
    while (s.page_counters.at(aid) < 197)
        page_tick(s);
    check(acknowledge_startup_world_runtime_page(s, aid) == E::none &&
              s.scene.world.world.ai.growth.at(1).experience == 0 &&
              s.scene.world.world.ai.growth.at(1).pending.amount == 0 &&
              s.scene.world.world.ai.growth.at(1).pending.counter == 8 &&
              s.scene.random.draws() == 1,
          "197 confirm recomputes and closes, clears L/N only, preserves O and random");
    const int changed_capacity = s.scene.world.world.ai.growth.at(1).derived.combat[0];
    check(s.scene.world.world.ai.battle.actors.at(active.id).capacity == changed_capacity &&
              s.scene.world.world.ai.retired_actors.at(retained.id).capacity == changed_capacity &&
              s.scene.world.world.ai.battle.actors.at(active.id).hp.target == 9 &&
              s.scene.world.world.ai.retired_actors.at(retained.id).hp.displayed == 9,
          "197 refresh propagates shared maximum HP to live and retained actors without healing");
    const auto old_type = s.rules->jobs.at(old_job).type;
    const auto new_type = s.rules->jobs.at(job).type;
    // a/h.d()统计p!=0定义；新局另一名已开放人物仍贡献旧职业人数。
    check(s.scripts.job_counts.at(new_type) ==
                  original_job_counts.at(new_type) + (old_type == new_type ? 0 : 1) &&
              s.scripts.job_counts.at(old_type) ==
                  original_job_counts.at(old_type) - (old_type == new_type ? 0 : 1),
          "final profession refresh synchronizes shared h.C read by later route and construction");
    page_tick(s);
    check(s.scripts.pages.back().lifecycle == 4 && s.scripts.pages.back().id == id &&
              !s.human_page_answers.count(id),
          "61 closes only when its update consumes returned K0");
    const auto usage = startup_world_resource_usage(s);
    check(usage.pages <= 2 && usage.page_payloads < 12,
          "closed child payloads retire instead of accumulating for detail chain");
    s = human_fixture(70);
    const auto mastery = s.scripts.pages.back().id;
    const auto extra = s.scene.world.world.ai.growth.at(1).definition.extra;
    for (int phase = 0; phase < 2; ++phase) {
        check(acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
                  s.page_counters.at(mastery) == 40 &&
                  s.scene.world.world.ai.growth.at(1).definition.extra == extra,
              "mastery phase early confirmation fastforwards only");
        check(acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
                  s.page_phases.at(mastery) == phase + 1,
              "mastery independently enters each following phase");
    }
    check(act_startup_world_human_page(s, mastery, A::select, 1) == E::none &&
              acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
              s.scene.world.world.ai.growth.at(1).definition.extra == extra,
          "final choice still waits its own40 before granting");
    check(acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
              s.scripts.pages.back().lifecycle == 4 &&
              acknowledge_startup_world_runtime_page(s, mastery) == E::invalid_page,
          "mastery final return grants once and stale duplicate rejects");
}
void tax_pages() {
    using E = StartupWorldRuntimeError;
    auto s = fixture(90);
    const auto id = s.scripts.pages.back().id;
    for (int n = 0; n < 6; ++n) {
        s.human_presence.at(n) = 1;
        s.human_homes.at(n)[2] = 1;
        s.human_calendar.at(n).legacy_G = (n + 1) * 10;
        s.scene.world.world.ai.battle.humans.at(n).battle_reward_stat = 100 + n;
    }
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    page_tick(s);
    check(s.tax_page_residents.at(id) == std::vector<int>({0, 1, 2, 3, 4, 5}) &&
              act_startup_world_tax_page(s, id, StartupWorldTaxAction::select, 5) == E::none &&
              s.tax_page_scroll.at(id) == 1,
          "90 freezes source residents and keeps selected row in five-row viewport");
    s.human_calendar.at(5).legacy_G = 90;
    s.human_homes.at(5)[2] = 0;
    const auto view = inspect_startup_world_tax_page(s, id);
    check(view && view->rows.size() == 6 && view->rows.back().amount == 90 && view->total == 240,
          "90 frozen identities still read current G without refreshing eligibility");
    check(cancel_startup_world_runtime_page(s, id) == E::invalid_page &&
              acknowledge_startup_world_runtime_page(s, id) == E::none &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "tax list has no cancellation and confirmation never pays");
    s = fixture(98);
    const auto payment = s.scripts.pages.back().id;
    s.human_presence.at(1) = 1;
    s.human_homes.at(1)[2] = 1;
    s.human_calendar.at(1).legacy_G = 140;
    s.human_calendar.at(2).legacy_G = 90;
    s.scene.world.world.ai.battle.humans.at(2).battle_reward_stat = 700;
    check(acknowledge_startup_world_runtime_page(s, payment) == E::invalid_page,
          "automatic98 cannot be executed by manual confirm");
    page_tick(s);
    check(s.scene.world.world.ai.accounting.funds() == cash + 140 &&
              s.monthly_cash.at(s.scene.calendar.month)[4][0] == 140 &&
              s.human_calendar.at(2).legacy_G == 0 &&
              s.scene.world.world.ai.battle.humans.at(2).battle_reward_stat == 0 &&
              s.scene.random.draws() == draws && s.scripts.pages.back().lifecycle == 4,
          "98 pays current eligible residents once to other income, clears every F/G and no draw");
    check(s.scripts.notices.back().message == 28, "automatic tax receipt submits source notice28");
    auto broken = fixture(98);
    broken.human_presence.at(1) = 1;
    broken.human_homes.at(1)[2] = 1;
    broken.human_calendar.at(1).legacy_G = 10;
    broken.monthly_cash.at(broken.scene.calendar.month)[4][0] = std::numeric_limits<int>::max();
    check(!prepare_startup_world_runtime(broken).candidate &&
              broken.human_calendar.at(1).legacy_G == 10 &&
              broken.scene.world.world.ai.accounting.funds() == cash &&
              broken.page_counters.empty(),
          "tax ledger overflow rolls back cash, F/G, counter and close");
}
void summary() {
    auto s = fixture(30);
    const auto id = s.scripts.pages.back().id;
    s.exploration_summaries[id] = {30, 1, 0, {}};
    const auto funds = s.scene.world.world.ai.accounting.funds();
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->page_counters.at(id) == 1 &&
              tick.candidate->sound_requests == std::vector<int>{4},
          "page30 first phase sound4");
    check(tick.candidate->scene.calendar.units == s.scene.calendar.units &&
              tick.candidate->scene.random.draws() == s.scene.random.draws() &&
              tick.candidate->scene.world.updates == s.scene.world.updates &&
              tick.checkpoints.empty(),
          "nonmain page freezes world/date/random/checkpoints");
    s = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 40 && s.page_phases[id] == 0,
          "early confirm skips40 only");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 0 && s.page_phases.at(id) == 1,
          "ready confirm enters phase1");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 40 && s.scripts.pages.back().lifecycle != 4,
          "second phase also requires independent fastforward");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.accounting.funds() == funds,
          "fourth immediate confirm closes without paying again");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "closed summary cannot be acknowledged twice");
    tick = prepare_startup_world_runtime(s);
    check(tick.candidate && !tick.candidate->exploration_summaries.count(id) &&
              !s.exploration_summaries.empty(),
          "framework retires closed result payload only on successful next Owner commit");
    s = fixture(32);
    const auto missing = s.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(s, missing) ==
                  StartupWorldRuntimeError::missing_source &&
              s.scripts.pages.back().lifecycle == 2 && s.page_counters.empty(),
          "missing summary is rejected without partial mutation");
    s.exploration_summaries[missing] = {32, 1, 0, {{0, 1}}};
    check(acknowledge_startup_world_runtime_page(s, missing) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4,
          "page32 closes authenticated summary");
}
void gift() {
    auto s = fixture(94);
    auto &page = s.scripts.pages.back();
    const auto id = page.id;
    page.legacy_r = 3;
    page.legacy_s = 0;
    s.facility_presence.at(0) = 0;
    s.facility_free_builds[0] = 0;
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->sound_requests == std::vector<int>{5},
          "gift first update sound5 and no eager claim");
    s = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 40 && s.facility_free_builds.at(0) == 0,
          "gift first confirm only skips animation");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.facility_free_builds.at(0) == 1 && s.facility_presence.at(0) == 2 &&
              s.facility_unlock_notices.at(0) && s.scripts.pages.back().lifecycle == 4,
          "gift authentic claim writes unique catalog and closes");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page &&
              s.facility_free_builds.at(0) == 1,
          "gift duplicate confirm cannot grant twice");
}
// 95领域矩阵在gift套件；这里只验证同一Owner的资金/定义桥与回写失败。
void unlock_rewards() {
    using E = StartupWorldRuntimeError;
    auto s = fixture(95);
    const auto id = s.scripts.pages.back().id;
    s.scripts.pages.back().legacy_r = 0;
    s.scripts.pages.back().legacy_s = 3000;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto entries = s.scene.world.world.ai.accounting.entries().size();
    const auto month = s.scene.calendar.month;
    const auto income = s.monthly_cash.at(month)[4][0];
    const auto draws = s.scene.random.draws();
    const auto date = s.scene.calendar.units;
    page_tick(s);
    check(s.page_counters.at(id) == 1 && s.sound_requests == std::vector<int>{5} &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "95 actual framework first tick emits sound without prepaying");
    s.sound_requests.clear(); // 明确的表现输出消费者，不作为业务前置。
    check(acknowledge_startup_world_runtime_page(s, id) == E::none &&
              s.page_counters.at(id) == 40 && s.sound_requests.empty() &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "95 early confirmation only fast-forwards, never repeats update sound");
    auto paused = s;
    paused.scene.framework_paused = true;
    const auto frozen = prepare_startup_world_runtime(paused);
    check(frozen.candidate && frozen.candidate->page_counters.at(id) == 40 &&
              acknowledge_startup_world_runtime_page(paused, id) == E::invalid_page &&
              cancel_startup_world_runtime_page(s, id) == E::invalid_page,
          "95 respects framework pause and cannot be cancelled into a reward");
    auto broken = s;
    broken.scene.world.world.ai.next_cash_id = std::numeric_limits<std::uint64_t>::max();
    check(acknowledge_startup_world_runtime_page(broken, id) == E::script_failed &&
              broken.scripts.pages.back().lifecycle == 2 && broken.page_counters.at(id) == 40 &&
              broken.scene.world.world.ai.accounting.funds() == cash &&
              broken.scene.world.world.ai.accounting.entries().size() == entries &&
              broken.monthly_cash.at(month)[4][0] == income && broken.scene.random.draws() == draws,
          "95 late cash ledger rejection rolls back candidate reward, close and statistics");
    check(acknowledge_startup_world_runtime_page(s, id) == E::none &&
              s.scene.world.world.ai.accounting.funds() == cash + 3000 &&
              s.scene.world.world.ai.accounting.entries().size() == entries + 1 &&
              s.monthly_cash.at(month)[4][0] == income + 3000 && s.cash_peak == cash + 3000 &&
              s.scripts.pages.back().lifecycle == 4 && s.scripts.notices.empty() &&
              s.scene.random.draws() == draws && s.scene.calendar.units == date,
          "95 cash reaches one ledger entry, other income and peak without opcode0 notice34");
    check(acknowledge_startup_world_runtime_page(s, id) == E::invalid_page,
          "95 closed reward rejects duplicate confirmation");
    page_tick(s);
    check(!s.page_counters.count(id) && !s.page_phases.count(id) &&
              std::none_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                           [&](const auto &p) { return p.id == id; }),
          "95 uses actual framework retirement with no leftover counter or page reference");
    for (const int kind : {1, 3, 4, 9, 10, 11}) {
        auto next = fixture(95);
        auto &page = next.scripts.pages.back();
        const auto reward = page.id;
        page.legacy_r = kind;
        page.legacy_s = kind == 1 ? 100 : 0;
        next.page_counters[reward] = 40;
        next.village_points = 10;
        next.medal_count = 2;
        next.facility_free_builds[0] = 0;
        next.scripts.professions.at(0).status = 0;
        next.scripts.professions.at(0).pending_notice = false;
        next.scripts.activities.at(0).status = 0;
        next.scripts.activities.at(0).pending_notice = false;
        const auto before = next.scene.world.world.ai.accounting.funds();
        check(acknowledge_startup_world_runtime_page(next, reward) == E::none &&
                  next.scripts.pages.back().lifecycle == 4 &&
                  next.scene.world.world.ai.accounting.funds() == before &&
                  next.scene.random.draws() == 0,
              "95 noncash effects use shared Owner and no unrelated cash/random changes");
        if (kind == 1)
            check(next.village_points == 110, "95 points write back sole village points field");
        else if (kind == 3)
            check(next.facility_free_builds.at(0) == 1,
                  "95 building entitlement shares existing building owner");
        else if (kind == 4)
            check(next.scripts.professions.at(0).status == 1 &&
                      next.scripts.professions.at(0).pending_notice &&
                      next.scene.world.world.ai.professions.at(0).unlocked,
                  "95 profession unlock and growth projection stay synchronized");
        else if (kind == 9)
            check((next.scripts.user_flags & 32U) != 0,
                  "95 layout unlock writes original user flag32 only");
        else if (kind == 10)
            check(next.medal_count == 3 && next.scripts.medal_count == 0 &&
                      next.scripts.notices.empty(),
                  "95 medal uses unique owner and does not replay opcode29 notice21");
        else
            check(next.scripts.activities.at(0).status == 1 &&
                      next.scripts.activities.at(0).pending_notice &&
                      next.activity_counts.at(0) == 0 && next.events_held == 0,
                  "95 activity unlock does not hold or pay for the activity");
    }
}
void focus_and_pause() {
    auto s = fixture(56);
    s.scene.framework_paused = true;
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->page_counters.empty(),
          "framework pause freezes page too");
    s.scene.framework_paused = false;
    tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->scripts.pages.back().lifecycle == 4 &&
              tick.candidate->scene.random.draws() == 0 && tick.candidate->scene.world.updates == 0,
          "camera page without monster closes without fake world round");
    s = fixture(1234);
    const auto id = s.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::missing_source &&
              s.scripts.pages.back().lifecycle == 2 && s.page_counters.empty(),
          "unimplemented rawpage action explicitly rejects without mutation");
}
void crew_initialization() {
    auto s = fixture(31);
    const auto id = s.scripts.pages.back().id;
    s.scripts.pages.back().lifecycle = 0;
    s.scripts.pages.back().task_identity = 1;
    s.scripts.pages.back().task_definition = 0;
    s.tasks.emplace(1, ref::DungeonFinishTask{1, 0, 0, 0, {}, {}});
    s.participants = {1, 3}; // 明确成果页夹具，不宣称正常新局已接受该任务。
    s.task_progress.definitions.at(0).flags |= 2U;
    s.scene.world.world.ai.battle.humans.at(1).task_kills = 4;
    s.scene.world.world.ai.battle.humans.at(3).task_kills = 2;
    const auto result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->crew_summaries.at(id) == s.participants &&
              result.candidate->scene.world.world.ai.battle.humans.at(1).task_kills == 1 &&
              result.candidate->scene.world.world.ai.battle.humans.at(3).task_kills == 0 &&
              result.candidate->scripts.pages.back().lifecycle == 2,
          "frame page initialization performs original X/H once before update");
    const auto funds = s.scene.world.world.ai.accounting.funds();
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.crew_summaries.at(id) == s.participants && s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.accounting.funds() == funds,
          "early input cannot bypass initialization and never pays task reward again");
    auto malformed = fixture(31);
    const auto missing = malformed.scripts.pages.back().id;
    check(!prepare_startup_world_runtime(malformed).candidate &&
              acknowledge_startup_world_runtime_page(malformed, missing) ==
                  StartupWorldRuntimeError::missing_source &&
              malformed.crew_summaries.empty() && malformed.scripts.pages.back().lifecycle == 2,
          "missing task-bound raw31 payload cannot become fake close success");
}
void task_focus_and_introduction() {
    for (int delay : {0, 1, 3, 90}) {
        auto waiting = fixture(16);
        const auto waiting_id = waiting.scripts.pages.back().id;
        waiting.scripts.pages.back().legacy_l = delay;
        for (int count = 1; count <= std::max(1, delay); ++count) {
            const auto tick = prepare_startup_world_runtime(waiting);
            check(tick.candidate && tick.candidate->page_counters.at(waiting_id) == count &&
                      (tick.candidate->scripts.pages.back().lifecycle == 4) ==
                          (count == std::max(1, delay)) &&
                      tick.candidate->scene.world.updates == 0 &&
                      tick.candidate->scene.calendar.units == 0 &&
                      tick.candidate->scene.random.draws() == 0,
                  "opcode7 raw16 honors exact page delay without world/date/random advance");
            waiting = *tick.candidate;
        }
    }
    auto s = fixture(57);
    const auto id = s.scripts.pages.back().id;
    auto result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->scripts.pages.back().lifecycle == 4,
          "raw57 empty task list automatically closes");
    s.task_order = {1};
    s.tasks.emplace(1, ref::DungeonFinishTask{1, 0, 0, 0, {}, ref::Position{1, 1}});
    result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->scripts.pages.back().lifecycle == 4 &&
              result.candidate->camera == s.camera,
          "raw57 unbound first task closes rather than aiming at task site");
    const auto facility = s.scene.world.facility_order.front();
    s.tasks.at(1).facility = facility;
    const auto target = startup_world_runtime_facility_target(s, facility);
    check(target.has_value(), "actual startup bound facility has source focus center");
    s.camera = {(*target)[0] - 5, (*target)[1]};
    s.previous_camera = s.camera;
    const auto units = s.scene.calendar.units;
    const auto draws = s.scene.random.draws();
    result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->camera == *target &&
              result.candidate->scripts.pages.back().lifecycle != 4,
          "raw57 distance equals step moves once before close");
    result = prepare_startup_world_runtime(*result.candidate);
    check(result.candidate && result.candidate->scripts.pages.back().lifecycle == 4 &&
              result.candidate->scene.calendar.units == units &&
              result.candidate->scene.random.draws() == draws &&
              result.candidate->scene.world.updates == 0,
          "raw57 reached focus closes automatically with world/date/random frozen");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::missing_source,
          "automatic focus cannot be deleted by manual confirm");
    s.tasks.at(1).facility = 99999;
    check(!prepare_startup_world_runtime(s).candidate && s.camera[0] == (*target)[0] - 5,
          "stale bound focus rejects without camera or page partial write");
    s = fixture(89);
    const auto intro = s.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(s, intro) == StartupWorldRuntimeError::none &&
              s.page_counters.at(intro) == 40 && s.scripts.pages.back().lifecycle != 4,
          "raw89 early confirm only fastforwards40");
    check(acknowledge_startup_world_runtime_page(s, intro) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 && s.scripts.event_calls.empty(),
          "raw89 next confirm closes without repeating monster introduction program");
}
void rank_conditions() {
    for (int rank = 0; rank < 5; ++rank) {
        auto s = fixture(49);
        const auto id = s.scripts.pages.back().id;
        s.rank = rank;
        s.rank_met.fill(true); // 旧显示缓存不是本次查询输入。
        s.rank_values.fill(-1);
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto tick = prepare_startup_world_runtime(s);
        check(tick.candidate && tick.candidate->rank == rank &&
                  tick.candidate->page_counters.at(id) == 1 &&
                  tick.candidate->rank_values != s.rank_values,
              "raw49 initializes actual shared rank criteria without a monthly prompt");
        check(tick.candidate->scripts.event_calls == s.scripts.event_calls &&
                  tick.candidate->scene.calendar.units == s.scene.calendar.units &&
                  tick.candidate->scene.random.draws() == s.scene.random.draws() &&
                  tick.candidate->scene.world.updates == s.scene.world.updates,
              "rank display initialization neither triggers36 nor advances world/random/date");
        s = *tick.candidate;
        s.rank_met.fill(true); // 即使所有条件已满足，页49也不是页48的晋级入口。
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.rank == rank && (s.scripts.user_flags & 8) != 0 &&
                  s.scripts.pages.back().lifecycle == 4 &&
                  s.scene.world.world.ai.accounting.funds() == cash,
              "raw49 confirm marksu8 and closes only, no rank advance or cash mutation");
        check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::invalid_page,
              "rank page duplicate confirmation rejects");
        const auto resumed = prepare_startup_world_runtime(s);
        check(resumed.candidate &&
                  std::none_of(resumed.candidate->scripts.pages.begin(),
                               resumed.candidate->scripts.pages.end(),
                               [&](const auto &p) { return p.id == id; }) &&
                  resumed.candidate->simulation_steps == s.simulation_steps + 1,
              "rank page framework removal admits actual main script/world routing");
        auto running = *resumed.candidate;
        for (int frame = 0; frame < 10 && running.scene.world.updates == s.scene.world.updates;
             ++frame) {
            // 初局页夹具尚未执行事件7，先保留真实介绍/等待，不强令同次世界更新。
            const auto &top = running.scripts.pages.back();
            if (top.kind != ref::WorldScriptPageKind::scene && top.lifecycle != 4)
                check(acknowledge_startup_world_runtime_page(running, top.id) ==
                          StartupWorldRuntimeError::none,
                      "resumed actual opening dialogue is acknowledged normally");
            const auto next = prepare_startup_world_runtime(running);
            check(next.candidate.has_value(), "post-rank main continuation stays executable");
            running = *next.candidate;
        }
        check(running.scene.world.updates > s.scene.world.updates,
              "post-rank real script continuation eventually resumes world/date");
    }
    auto early = fixture(49);
    const auto id = early.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(early, id) == StartupWorldRuntimeError::none &&
              early.page_counters.at(id) == 0 && early.scripts.pages.back().lifecycle == 4,
          "early raw49 confirm cannot skip source initialization");
    auto max = fixture(49);
    const auto max_id = max.scripts.pages.back().id;
    max.rank = 5;
    max.rank_values = {1, 2, 3, 4};
    const auto result = prepare_startup_world_runtime(max);
    check(result.candidate && result.candidate->scripts.event_calls.at(48) == 1 &&
              result.candidate->rank == 5 && result.candidate->rank_values == max.rank_values &&
              (result.candidate->scripts.user_flags & 8) == 0,
          "rank5 initialization runs48 then closes without resetting old criteria or confirmu8");
    check(result.candidate->scripts.pages.front().lifecycle == 3 &&
              std::any_of(result.candidate->scripts.pages.begin(),
                          result.candidate->scripts.pages.end(),
                          [&](const auto &p) { return p.id == max_id && p.lifecycle == 4; }),
          "rank5 replacement talk preserves page anchor and marks only old rank page closed");
    auto malformed = fixture(49);
    const auto bad_id = malformed.scripts.pages.back().id;
    malformed.rank = -1;
    check(!prepare_startup_world_runtime(malformed).candidate &&
              acknowledge_startup_world_runtime_page(malformed, bad_id) ==
                  StartupWorldRuntimeError::missing_source &&
              malformed.page_counters.empty() && (malformed.scripts.user_flags & 8) == 0 &&
              malformed.scripts.pages.back().lifecycle != 4,
          "invalid rank rejects without page, criteria or flag partial writes");
}
void rank_promotion() {
    auto s = fixture(48);
    const auto id = s.scripts.pages.back().id;
    s.scripts.pages.back().legacy_f = 1;
    auto initialized = prepare_startup_world_runtime(s);
    check(initialized.candidate && initialized.candidate->rank == 0,
          "promotion page initialization only refreshes source criteria");
    s = *initialized.candidate;
    const auto draws = s.scene.random.draws();
    const auto cash = s.scene.world.world.ai.accounting.funds();
    check(act_startup_world_runtime_rank_page(s, id, 1) == StartupWorldRuntimeError::none &&
              s.rank == 0 && (s.scripts.user_flags & 8) == 0 && s.scripts.event_calls.at(159) == 1,
          "condition explanation does not promote or marku8");
    // 条件缓存/页面调用点夹具，不伪造自然新局已满足晋级条件。
    auto ready = fixture(48);
    const auto ready_id = ready.scripts.pages.back().id;
    ready.scripts.pages.back().legacy_f = 1;
    ready.page_counters[ready_id] = 1;
    ready.rank_met.fill(true);
    ready.scene.calendar.year = 2;
    ready.scene.calendar.month = 7;
    check(act_startup_world_runtime_rank_page(ready, ready_id) == StartupWorldRuntimeError::none &&
              ready.rank == 1 && ready.rank_history[0] == std::array<int, 2>{2, 7} &&
              (ready.scripts.user_flags & 8) != 0 && ready.scripts.event_calls.at(37) == 1 &&
              ready.scripts.event_calls.at(53) == 1 && ready.scripts.event_calls.at(42) == 1 &&
              ready.scripts.event_calls.at(41) == 1 && ready.scripts.event_calls.at(206) == 1,
          "actual promotion commits rank/date and manual/facility/title/tutorial script chain");
    const auto closed = std::find_if(ready.scripts.pages.begin(), ready.scripts.pages.end(),
                                     [&](const auto &p) { return p.id == ready_id; });
    check(closed != ready.scripts.pages.end() && closed->lifecycle == 4 &&
              ready.scene.world.world.ai.accounting.funds() == cash &&
              ready.scene.random.draws() == draws && ready.scene.world.updates == 0,
          "promotion closes only source page, no invented cost/world update/shuffle before raw50 "
          "initialization");
    check(std::any_of(ready.scripts.pages.begin(), ready.scripts.pages.end(),
                      [](const auto &p) { return p.legacy_page == 50; }) &&
              std::any_of(ready.scripts.notices.begin(), ready.scripts.notices.end(),
                          [](const auto &n) { return n.message == 18; }) &&
              std::any_of(ready.scripts.notices.begin(), ready.scripts.notices.end(),
                          [](const auto &n) { return n.message == 6; }),
          "rank1 creates actual celebration and definition/activity notices");
    for (const auto &activity : ready.rules->activities)
        if (activity.parameters[6] == 1)
            check(ready.scripts.activities.at(activity.identity).status == 1 &&
                      ready.scripts.activities.at(activity.identity).pending_notice,
                  "matched original activity j opens shared definition and retains new notice");
    check(act_startup_world_runtime_rank_page(ready, ready_id) ==
                  StartupWorldRuntimeError::invalid_page &&
              ready.rank == 1,
          "closed promotion cannot advance rank twice");
    auto denied = fixture(48);
    const auto denied_id = denied.scripts.pages.back().id;
    denied.page_counters[denied_id] = 1;
    denied.rank_met = {true, true, true, false};
    check(act_startup_world_runtime_rank_page(denied, denied_id) ==
                  StartupWorldRuntimeError::none &&
              denied.rank == 0 && denied.scripts.event_calls.at(39) == 1 &&
              denied.scripts.pages[1].lifecycle != 4,
          "three-condition refusal preserves rank and source page, invokes39");
    auto broken = fixture(48);
    const auto bad_id = broken.scripts.pages.back().id;
    broken.page_counters[bad_id] = 1;
    broken.rank_met.fill(true);
    broken.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
    check(
        act_startup_world_runtime_rank_page(broken, bad_id) ==
                StartupWorldRuntimeError::script_failed &&
            broken.rank == 0 && broken.rank_history[0] == std::array<int, 2>{0, 0} &&
            broken.scripts.notices.empty() && broken.scripts.event_calls.empty() &&
            (broken.scripts.user_flags & 8) == 0 && broken.scene.random.draws() == 0,
        "late page-ID exhaustion rolls back rank/history/unlocks/notices/userflag/scripts/random");
    auto celebration = fixture(50);
    const auto celebration_id = celebration.scripts.pages.back().id;
    celebration.human_presence.at(1) = 1;
    celebration.human_presence.at(2) = 0; // 原表2同为p1；本条件夹具只保留一名入场者。
    celebration.human_presence.at(3) = 2; // p2不入庆典，不按p!=0扩宽。
    celebration.scene.random = ref::WorldRandomStream::from_raw({0});
    auto tick = prepare_startup_world_runtime(celebration);
    check(tick.candidate &&
              tick.candidate->rank_celebration_participants.at(celebration_id) ==
                  std::vector<std::array<int, 5>>{{1, 287, 143, 3, 0}} &&
              tick.candidate->scene.random.draws() == 1 &&
              tick.candidate->sound_requests == std::vector<int>{3},
          "celebration initializes current p1 definitions, one shuffle draw and first music3");
    celebration = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(celebration, celebration_id) ==
                  StartupWorldRuntimeError::none &&
              celebration.page_counters.at(celebration_id) == 1 &&
              celebration.page_phases.at(celebration_id) == 0,
          "early celebration confirm does not fastforward or close");
    for (const auto threshold : {135, 20}) {
        celebration.page_counters[celebration_id] = threshold - 1;
        tick = prepare_startup_world_runtime(celebration);
        check(tick.candidate && tick.candidate->page_counters.at(celebration_id) == 0,
              "source phase threshold automatically transitions and resets local counter");
        celebration = *tick.candidate;
    }
    celebration.page_counters[celebration_id] = 139;
    check(acknowledge_startup_world_runtime_page(celebration, celebration_id) ==
                  StartupWorldRuntimeError::none &&
              celebration.scripts.pages.back().lifecycle != 4,
          "last celebration phase139 cannot close before140");
    tick = prepare_startup_world_runtime(celebration);
    check(tick.candidate.has_value(), "last celebration threshold update valid");
    celebration = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(celebration, celebration_id) ==
                  StartupWorldRuntimeError::none &&
              celebration.scripts.pages.back().lifecycle == 4 &&
              celebration.sound_requests.back() == 1 && celebration.scene.world.updates == 0 &&
              celebration.scene.random.draws() == 1,
          "final confirm refreshes original music and closes, no repeat shuffle/world update");
    auto exhausted = fixture(50);
    exhausted.human_presence.at(1) = 1;
    exhausted.scene.random = ref::WorldRandomStream::from_raw({});
    check(!prepare_startup_world_runtime(exhausted).candidate &&
              exhausted.rank_celebration_participants.empty() && exhausted.page_counters.empty(),
          "celebration random exhaustion fails whole Owner without synthetic participants");
}
void annual_termination() {
    auto s = fixture(87);
    const auto id = s.scripts.pages.back().id;
    s.human_presence.at(1) = 1;
    const auto issued = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                  startup_world_runtime_scripts(s), {108, {}, {}});
    check(issued.candidate && write_startup_world_runtime_scripts(s, issued.candidate->state) &&
              s.medal_count == 1 && s.scripts.medal_count == 0,
          "real opcode29 commits medal to sole Owner, not persistent script duplicate");
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->medal_count == 2 &&
              tick.candidate->sound_requests == std::vector<int>{3} &&
              tick.candidate->award_rankings.count(id) && tick.candidate->award_announced.at(id),
          "raw87 actual page initialization increments j once and first update sounds3 only");
    s = *tick.candidate;
    const auto repeated = prepare_startup_world_runtime(s);
    check(repeated.candidate && repeated.candidate->medal_count == 2 &&
              repeated.candidate->sound_requests == std::vector<int>{3},
          "raw87 later updates neither award another medal nor replay first sound");
    check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::missing_source &&
              act_startup_world_runtime_award_page(s, id,
                                                   ref::WorldAwardAction::confirm_termination) ==
                  StartupWorldRuntimeError::missing_source &&
              s.medal_count == 2,
          "ordinary confirm and absent termination question cannot silently end ceremony");
    check(act_startup_world_runtime_award_page(s, id, ref::WorldAwardAction::request_termination) ==
                  StartupWorldRuntimeError::none &&
              s.award_termination_pending.at(id),
          "explicit test player requests the source termination question");
    check(act_startup_world_runtime_award_page(s, id, ref::WorldAwardAction::reject_termination) ==
                  StartupWorldRuntimeError::none &&
              !s.award_termination_pending.at(id) && s.scripts.pages.back().lifecycle != 4,
          "termination no answer retains actual page and medals");
    check(act_startup_world_runtime_award_page(s, id, ref::WorldAwardAction::request_termination) ==
                  StartupWorldRuntimeError::none &&
              act_startup_world_runtime_award_page(s, id,
                                                   ref::WorldAwardAction::confirm_termination) ==
                  StartupWorldRuntimeError::none &&
              s.medal_count == 2 && s.scripts.event_calls.at(22) == 1 &&
              s.sound_requests.back() == 1 && s.scene.world.world.ai.accounting.funds() == cash &&
              s.scene.calendar.units == 0 && s.scene.random.draws() == 0,
          "termination preserves unused medal and cash/date/RNG, runs22 then source BGM refresh");
    check(std::any_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                      [&](const auto &p) { return p.id == id && p.lifecycle == 4; }) &&
              !s.scripts.executing_page,
          "termination closes only parent award page and releases callback root");
    const auto reissued = ref::prepare_world_script(
        startup_world_runtime_catalog(), startup_world_runtime_scripts(s), {108, {}, {}});
    check(reissued.candidate && reissued.candidate->state.medal_count == 3 &&
              write_startup_world_runtime_scripts(s, reissued.candidate->state) &&
              s.medal_count == 3 && s.scripts.medal_count == 0,
          "script after ceremony reads actual retained medals and increments same field");
    auto malformed = fixture(87);
    const auto bad = malformed.scripts.pages.back().id;
    malformed.medal_count = std::numeric_limits<int>::max();
    const auto overflow = ref::prepare_world_script(
        startup_world_runtime_catalog(), startup_world_runtime_scripts(malformed), {108, {}, {}});
    check(!overflow.candidate && malformed.medal_count == std::numeric_limits<int>::max() &&
              malformed.scripts.medal_count == 0 && malformed.scripts.notices.empty(),
          "real medal overflow returns no partial shared field or notification");
    check(!prepare_startup_world_runtime(malformed).candidate &&
              act_startup_world_runtime_award_page(malformed, bad,
                                                   ref::WorldAwardAction::request_termination) ==
                  StartupWorldRuntimeError::missing_source &&
              malformed.award_rankings.empty() && malformed.page_counters.empty(),
          "late award initialization error cannot leave partial medal/list/page state");
}
void annual_award() {
    auto s = fixture(87);
    s.human_presence.at(1) = 1;
    const auto parent = s.scripts.pages.back().id;
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate.has_value(), "actual annual page initialized");
    s = *tick.candidate;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto old_u = s.scene.world.world.ai.growth.at(1).definition.legacy_u;
    const auto old_C = s.shop_humans.at(1).satisfaction;
    const auto completion = s.scene.world.world.ai.pending_completion;
    check(act_startup_world_runtime_award_page(s, parent, ref::WorldAwardAction::request_award) ==
                  StartupWorldRuntimeError::none &&
              s.award_pending_humans.at(parent) == 1 && s.medal_count == 1,
          "request binds definition and preserves unique medal");
    auto overflow = s;
    overflow.human_calendar.at(1).celebrations = std::numeric_limits<int>::max();
    const auto counter = overflow.page_counters.at(parent);
    check(act_startup_world_runtime_award_page(overflow, parent,
                                               ref::WorldAwardAction::confirm_award) ==
                  StartupWorldRuntimeError::missing_source &&
              overflow.medal_count == 1 && overflow.page_counters.at(parent) == counter &&
              overflow.scripts.pages.size() == s.scripts.pages.size() &&
              overflow.award_pending_humans.at(parent) == 1,
          "late reward overflow rolls back medal, pending question, counter and page stack");
    check(act_startup_world_runtime_award_page(s, parent, ref::WorldAwardAction::confirm_award) ==
                  StartupWorldRuntimeError::none &&
              s.medal_count == 0 && s.human_calendar.at(1).celebrations == 1 &&
              s.scripts.event_calls.at(58) == 1 &&
              s.scene.world.world.ai.pending_completion == completion + 10 &&
              s.shop_humans.at(1).satisfaction == std::min(old_C + 10, 100) &&
              s.scene.world.world.ai.growth.at(1).definition.legacy_u ==
                  std::min(old_u + 10, 100) &&
              s.page_counters.at(parent) == 0 && s.award_announced.at(parent),
          "yes commits reward and event58 once, preserves announced flag at parent counter0");
    check(s.scripts.pages.back().legacy_page == 88 &&
              s.page_human_bindings.at(s.scripts.pages.back().id) == 1,
          "framework inserts each child after same callback anchor: raw88 above later67");
    const auto display = s.scripts.pages.back().id;
    s.page_phases[display] = 1;
    s.page_counters[display] = 212;
    s.scene.random = ref::WorldRandomStream::from_raw({1});
    auto exhausted = s;
    exhausted.scene.random = ref::WorldRandomStream::from_raw({});
    check(acknowledge_startup_world_runtime_page(exhausted, display) ==
                  StartupWorldRuntimeError::missing_source &&
              exhausted.scripts.pages.back().id == display &&
              exhausted.scripts.pages.back().lifecycle != 4 && exhausted.scene.random.draws() == 0,
          "late display random failure keeps source page and global random unchanged");
    check(
        acknowledge_startup_world_runtime_page(s, display) == StartupWorldRuntimeError::none &&
            s.scene.random.draws() == 1 && s.scripts.event_calls.at(25) == 1 &&
            s.scripts.pages.back().speaker_kind == 1 &&
            s.scripts.pages.back().speaker_definition == 1 &&
            s.scene.world.world.ai.accounting.funds() == cash && s.scene.calendar.units == 0 &&
            s.scene.world.world.ai.battle.actors.empty(),
        "raw88 selects actual event25 on sole RNG and binds last page without world/cash advance");
    for (int n = 0; n < 100 && !ref::world_script_seen(s.scripts, 22); ++n) {
        const auto next = prepare_startup_world_runtime(s);
        check(next.candidate.has_value(), "award return chain keeps valid Owner");
        s = *next.candidate;
        if (s.scripts.pages.back().kind == ref::WorldScriptPageKind::dialogue)
            check(acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) ==
                      StartupWorldRuntimeError::none,
                  "actual award dialogue returns");
        else if (s.scripts.pages.back().legacy_page == 67) {
            const auto effort = s.scripts.pages.back().id;
            s.page_counters[effort] = 73;
            check(acknowledge_startup_world_runtime_page(s, effort) ==
                      StartupWorldRuntimeError::none,
                  "effort page returns after original award dialogue at threshold73");
        }
    }
    check(ref::world_script_seen(s.scripts, 22) && s.medal_count == 0 &&
              s.human_calendar.at(1).celebrations == 1,
          "zero medals close ceremony after child return, never repeat reward");
}
void task_display() {
    for (int raw : {99, 100}) {
        auto s = fixture(raw);
        const auto id = s.scripts.pages.back().id;
        auto missing = prepare_startup_world_runtime(s);
        check(
            !missing.candidate && s.scene.random.draws() == 0,
            "task display requires actual bound monster definition; page number cannot invent it");
        s.scripts.pages.back().monster_definition = 0;
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.page_counters.at(id) == 0 && s.scripts.pages.back().lifecycle != 4 &&
                  s.scene.random.draws() == (raw == 100 ? 19U : 0U),
              "early confirm initializes true display once but never skips forty counter");
        const auto initial_draws = s.scene.random.draws();
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.scene.random.draws() == initial_draws,
              "repeated early confirmation does not reinitialize or run same-frame F");
        for (int count = 1; count <= 40; ++count) {
            const auto tick = prepare_startup_world_runtime(s);
            check(tick.candidate && tick.candidate->page_counters.at(id) == count &&
                      tick.candidate->scene.calendar.units == 0 &&
                      tick.candidate->scene.world.updates == 0,
                  "presentation page advances separately while shared world and calendar stay "
                  "frozen");
            s = *tick.candidate;
        }
        const auto draws = s.scene.random.draws();
        check((raw == 99 ? draws == 0 : draws > initial_draws) &&
                  acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.scripts.pages.back().lifecycle == 4 && s.scene.random.draws() == draws,
              "ready confirm closes true display without extra presentation random");
    }
}
void event_message_and_shop_return() {
    for (int command : {0, 1, 2}) {
        auto s = fixture(11);
        const auto id = s.scripts.pages.back().id;
        s.scripts.pages.back().message_commands = {command};
        s.scripts.pages.back().paragraphs = {"调用点夹具"};
        const auto funds = s.scene.world.world.ai.accounting.funds();
        const auto tick = prepare_startup_world_runtime(s);
        check(
            tick.candidate && tick.candidate->page_counters.at(id) == 1 &&
                tick.candidate->sound_requests ==
                    (command == 2 ? std::vector<int>{} : std::vector<int>{command == 0 ? 4 : 6}) &&
                tick.candidate->scene.world.updates == 0 &&
                tick.candidate->scene.random.draws() == 0,
            "raw11 source sound only on first page update, no world/random tick");
        s = *tick.candidate;
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.page_counters.at(id) == 40 && s.scripts.pages.back().lifecycle != 4,
              "raw11 early confirm only fast-forwards to40");
        auto locked = s;
        locked.scripts.page_mutations_locked = true;
        // kairo/android/a/b.c(a)无l守卫；l限制新增页，不限制关闭已有页。
        check(acknowledge_startup_world_runtime_page(locked, id) ==
                      StartupWorldRuntimeError::none &&
                  locked.scripts.pages.back().lifecycle == 4 &&
                  locked.scene.world.world.ai.accounting.funds() == funds,
              "source page creation lock does not prevent raw11 existing-page close");
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.scripts.pages.back().lifecycle == 4 &&
                  s.scene.world.world.ai.accounting.funds() == funds,
              "raw11 second confirm only closes, no extra reward/payment");
    }
    auto invalid = fixture(11);
    const auto invalid_id = invalid.scripts.pages.back().id;
    check(!prepare_startup_world_runtime(invalid).candidate &&
              acknowledge_startup_world_runtime_page(invalid, invalid_id) ==
                  StartupWorldRuntimeError::missing_source &&
              invalid.page_counters.empty(),
          "raw11 missing authenticated message payload rejects without partial update");
    auto s = fixture(83);
    const auto id = s.scripts.pages.back().id;
    const auto funds = s.scene.world.world.ai.accounting.funds();
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::missing_source,
          "shop confirm does not silently mean cancel or fabricate purchase");
    auto locked = s;
    locked.scripts.page_mutations_locked = true;
    check(cancel_startup_world_runtime_page(locked, id) == StartupWorldRuntimeError::none &&
              locked.scripts.pages.back().lifecycle == 4,
          "source page creation lock does not prevent raw83 existing-page cancel");
    check(cancel_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 && s.scripts.pages.front().lifecycle != 4 &&
              s.scene.random.draws() == 0 && s.scene.world.world.ai.accounting.funds() == funds,
          "explicit shop cancel preserves lower scene, money and random");
    check(cancel_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "retired shop cannot be cancelled twice");
    auto unknown = fixture(1234);
    check(cancel_startup_world_runtime_page(unknown, unknown.scripts.pages.back().id) ==
                  StartupWorldRuntimeError::invalid_page &&
              unknown.scripts.pages.back().lifecycle != 4,
          "cancel only supports proven raw83 source route");
}
void popularity_return() {
    auto s = fixture(97);
    const auto id = s.scripts.pages.back().id;
    s.scripts.pages.back().lifecycle = 1;
    s.popularity_display = true;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto rewards = s.popularity_rewards;
    check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::missing_source &&
              s.popularity_display && s.scripts.pages.back().lifecycle == 1,
          "raw97 early player input cannot bypass actual framework update");
    check(!update_startup_world_runtime_page(s),
          "raw97 direct consumer preserves lifecycle2 requirement");
    s.scene.framework_paused = true;
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->popularity_display &&
              tick.candidate->scripts.pages.back().lifecycle != 4,
          "raw97 framework pause does not clear reward display");
    s.scene.framework_paused = false;
    tick = prepare_startup_world_runtime(s);
    check(tick.candidate && !tick.candidate->popularity_display &&
              tick.candidate->scripts.pages.back().lifecycle == 4,
          "raw97 actual first update automatically clears R and closes");
    check(tick.candidate->scene.calendar.units == s.scene.calendar.units &&
              tick.candidate->scene.world.updates == s.scene.world.updates &&
              tick.candidate->scene.random.draws() == s.scene.random.draws() &&
              tick.candidate->scene.world.world.ai.accounting.funds() == cash &&
              tick.candidate->sound_requests.empty() && tick.checkpoints.empty(),
          "raw97 does not advance world or replay cash/random/reward effects");
    check(tick.candidate->popularity_rewards.size() == rewards.size(),
          "raw97 preserves reward catalog size");
    for (std::size_t index = 0; index < rewards.size(); ++index)
        check(tick.candidate->popularity_rewards[index].status == rewards[index].status &&
                  tick.candidate->popularity_rewards[index].pending_notice ==
                      rewards[index].pending_notice,
              "raw97 leaves each claimed reward and pending notice untouched");
    s = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "retired raw97 cannot repeat its effect through player confirmation");
}
void unlocked_visitor() {
    auto s = fixture(59);
    s.scripts.pages.back().legacy_f = 2;
    const auto id = s.scripts.pages.back().id;
    const auto actors = s.scene.world.world.ai.human_order;
    const auto priority = s.human_calendar.at(2).absent_months;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 0 && s.scripts.pages.back().lifecycle != 4 &&
              s.human_calendar.at(2).absent_months == priority,
          "raw59 early confirm does not fastforward or change arrival priority");
    for (int count = 1; count <= 70; ++count) {
        const auto tick = prepare_startup_world_runtime(s);
        check(tick.candidate && tick.candidate->page_counters.at(id) == count &&
                  tick.candidate->scripts.pages.back().lifecycle != 4 &&
                  tick.candidate->scene.world.world.ai.human_order == actors &&
                  tick.candidate->scene.random.draws() == s.scene.random.draws() &&
                  tick.candidate->scene.calendar.units == s.scene.calendar.units &&
                  tick.candidate->scene.world.world.ai.accounting.funds() == cash &&
                  tick.candidate->human_calendar.at(2).absent_months == priority,
              "raw59 page clock advances without auto-close, spawn, reward or world tick");
        s = *tick.candidate;
        check(s.sound_requests == std::vector<int>{5},
              "raw59 sound5 occurs only on first actual update");
        if (count < 70)
            check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                      s.page_counters.at(id) == count && s.scripts.pages.back().lifecycle != 4,
                  "raw59 confirms below70 preserve page and exact counter");
    }
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.human_calendar.at(2).absent_months == 10 && s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.human_order == actors,
          "raw59 ready confirm writes aq10 and closes without direct arrival");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "raw59 closed page rejects duplicate confirm");
    auto bad = fixture(59);
    bad.scripts.pages.back().legacy_f = -1;
    check(!prepare_startup_world_runtime(bad).candidate && bad.page_counters.empty() &&
              acknowledge_startup_world_runtime_page(bad, bad.scripts.pages.back().id) ==
                  StartupWorldRuntimeError::missing_source &&
              bad.scripts.pages.back().lifecycle != 4,
          "raw59 invalid definition rolls back update and confirmation");
}
StartupWorldRuntimeState village_fixture(int activity = 23) {
    auto s = fixture(51);
    s.scripts.event_calls[100] = 1; // 已读说明的明确页面夹具，非自然玩家轨迹。
    s.village_points = 200;
    s.quarter_counter = 3;
    s.scripts.activities.at(activity).status = 1;
    page_tick(s);
    const auto id = s.scripts.pages.back().id;
    const auto &list = s.activity_page_lists.at(id);
    const auto chosen = std::find(list.begin(), list.end(), activity);
    check(chosen != list.end(), "activity fixture selects an actual source definition");
    check(act_startup_world_village_activity_page(s, id, StartupVillageActivityAction::select,
                                                  static_cast<int>(chosen - list.begin())) ==
              StartupWorldRuntimeError::none,
          "activity selection goes through current page input");
    return s;
}
void village_activity_initialization() {
    using A = StartupVillageActivityAction;
    using E = StartupWorldRuntimeError;
    for (int fault = 0; fault < 3; ++fault) {
        auto s = fixture(51);
        const auto id = s.scripts.pages.back().id;
        if (fault == 0)
            s.scene.random = ref::WorldRandomStream::from_raw({0});
        // 原框架锁仅抑制插页，不使事件失败；用ID耗尽制造真正脚本失败。
        if (fault == 1)
            s.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
        if (fault == 2)
            s.activity_counts.erase(23);
        const auto next_id = s.scripts.next_page_id;
        check(!inspect_startup_world_village_activity_page(s, id) &&
                  act_startup_world_village_activity_page(s, id, A::confirm) == E::missing_source,
              "new51 is not initialized by drawing or input before framework entry");
        const auto initialized = prepare_startup_world_runtime(s);
        check(!initialized.candidate, ("initial51 rejects fault " + std::to_string(fault)).c_str());
        check(s.scene.random.draws() == 0 && !ref::world_script_seen(s.scripts, 100) &&
                  s.scripts.next_page_id == next_id && s.scripts.pages.size() == 2 &&
                  s.activity_pages_initialized.empty() && s.activity_page_lists.empty() &&
                  s.activity_page_display_humans.empty(),
              ("initial51 rolls back entry for fault " + std::to_string(fault) +
               "; draws=" + std::to_string(s.scene.random.draws()) +
               ", seen100=" + std::to_string(ref::world_script_seen(s.scripts, 100)) +
               ", pages=" + std::to_string(s.scripts.pages.size()))
                  .c_str());
    }
    auto s = fixture(51);
    s.scene.random = ref::WorldRandomStream::from_raw({0, 0});
    const auto id = s.scripts.pages.back().id;
    page_tick(s);
    check(ref::world_script_seen(s.scripts, 100) && s.scene.random.draws() == 2 &&
              s.activity_pages_initialized.count(id) && s.scripts.pages.back().id != id &&
              !inspect_startup_world_village_activity_page(s, id) &&
              act_startup_world_village_activity_page(s, id, A::confirm) == E::invalid_page,
          "first51 initialization draws once and actual introduction blocks its input");
    auto empty = fixture(51);
    empty.scripts.event_calls[100] = 1;
    for (auto &[human, presence] : empty.human_presence) {
        (void)human;
        presence = 0;
    }
    page_tick(empty);
    const auto empty_id = empty.scripts.pages.back().id;
    check(inspect_startup_world_village_activity_page(empty, empty_id).has_value() &&
              empty.scene.random.draws() == 0 &&
              empty.activity_page_display_humans.at(empty_id) == std::array<int, 2>{0, 0},
          "empty display pool preserves valid zero placeholders without random draws");
    empty.activity_page_display_humans.at(empty_id)[0] = -1;
    check(!inspect_startup_world_village_activity_page(empty, empty_id) &&
              act_startup_world_village_activity_page(empty, empty_id, A::confirm) ==
                  E::missing_source,
          "empty display pool cannot hide a forged human definition reference");
}
void village_activity_pages() {
    using A = StartupVillageActivityAction;
    using E = StartupWorldRuntimeError;
    auto s = village_fixture();
    const auto parent = s.scripts.pages.back().id;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    check(s.activity_page_lists.at(parent) == std::vector<int>{23} && s.scene.random.draws() == 2 &&
              s.scene.world.world.ai.human_order.empty(),
          "initial catalogue only cleaning23; two draws use open definitions without creating "
          "actors");
    for (int fault = 0; fault < 9; ++fault) {
        auto broken = s;
        if (fault == 0)
            broken.activity_page_lists.erase(parent);
        if (fault == 1)
            broken.activity_page_selections.erase(parent);
        if (fault == 2)
            broken.activity_page_scroll.erase(parent);
        if (fault == 3)
            broken.page_counters.erase(parent);
        if (fault == 4)
            broken.activity_page_selections.at(parent) = 9;
        if (fault == 5)
            broken.activity_page_display_humans.erase(parent);
        if (fault == 6)
            broken.activity_page_display_humans.at(parent)[0] = -1;
        if (fault == 7)
            broken.scene.world.world.ai.growth.erase(s.activity_page_display_humans.at(parent)[0]);
        if (fault == 8)
            broken.human_presence.erase(s.activity_page_display_humans.at(parent)[0]);
        check(!inspect_startup_world_village_activity_page(broken, parent) &&
                  act_startup_world_village_activity_page(broken, parent, A::confirm) != E::none &&
                  !prepare_startup_world_runtime(broken).candidate &&
                  broken.village_points == 200 && broken.events_held == 0 &&
                  broken.scene.random.draws() == 2,
              "initialized51 missing or invalid payload explicitly rejects without mutation or "
              "exception");
    }
    for (const int slots : {0, 1}) {
        auto denied = s;
        denied.quarter_counter = slots;
        denied.village_points = 0;
        check(act_startup_world_village_activity_page(denied, parent, A::confirm) == E::none &&
                  ref::world_script_seen(denied.scripts, slots == 0 ? 50 : 12) &&
                  denied.events_held == 0 && denied.activity_counts.at(23) == 0,
              "51 preserves source denial priority and does not start an event");
    }
    s.scripts.activities.at(23).pending_notice = true;
    check(acknowledge_startup_world_runtime_page(s, parent) == E::none,
          "common confirmation routes51 into real52");
    const auto child = s.scripts.pages.back().id;
    page_tick(s);
    check(!s.scripts.activities.at(23).pending_notice && s.village_points == 200 &&
              s.events_held == 0,
          "51 clears notice but defers charge and F to52");
    for (int fault = 0; fault < 8; ++fault) {
        auto broken = s;
        if (fault == 0)
            broken.activity_page_bindings.erase(child);
        if (fault == 1)
            broken.activity_page_parents.erase(child);
        if (fault == 2)
            broken.activity_page_selections.at(child) = 2;
        if (fault == 3)
            broken.page_counters.erase(child);
        if (fault == 4)
            broken.scripts.pages[1].legacy_page = 54;
        if (fault == 5)
            broken.scripts.pages[1].lifecycle = 4;
        if (fault == 6)
            broken.activity_page_lists.at(parent).clear();
        if (fault == 7)
            broken.activity_page_bindings.at(child) = 0;
        check(!inspect_startup_world_village_activity_page(broken, child) &&
                  act_startup_world_village_activity_page(broken, child, A::confirm) != E::none &&
                  broken.village_points == 200 && broken.activity_page_answers.empty() &&
                  broken.events_held == 0 && broken.quarter_counter == 3,
              "52 rejects incomplete or wrong parent/selection instead of treating corruption as "
              "cancel");
    }
    auto locked_start = s;
    locked_start.scripts.page_mutations_locked = true;
    const auto next_id = locked_start.scripts.next_page_id;
    check(act_startup_world_village_activity_page(locked_start, child, A::confirm) ==
                  E::script_failed &&
              locked_start.village_points == 200 && locked_start.events_held == 0 &&
              locked_start.activity_counts.at(23) == 0 &&
              !(locked_start.activity_flags.at(23) & 4U) &&
              locked_start.activity_page_answers.empty() &&
              locked_start.scripts.next_page_id == next_id &&
              locked_start.scripts.pages.back().id == child &&
              locked_start.scripts.pages.back().lifecycle != 4,
          "52 insertion failure rolls back tentative charge, m/F, flags and parent answer");
    check(cancel_startup_world_runtime_page(s, child) == E::none &&
              s.activity_page_answers.at(parent) == 1 && s.village_points == 200,
          "52 cancel returns K1 without spending");
    check(act_startup_world_village_activity_page(s, parent, A::confirm) == E::invalid_page,
          "parent answer cannot be bypassed before its resume update");
    page_tick(s);
    check(s.activity_page_answers.empty() && !s.activity_page_parents.count(child),
          "resume consumes K1 and retired52 loses parent binding");
    check(acknowledge_startup_world_runtime_page(s, parent) == E::none, "open52 again");
    page_tick(s);
    check(acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) == E::none,
          "52 starts actual selected activity");
    const auto animation = s.scripts.pages.back().id;
    page_tick(s);
    check(s.village_points == 180 && s.events_held == 1 && s.activity_counts.at(23) == 1 &&
              (s.activity_flags.at(23) & 4U) && s.quarter_counter == 3 &&
              s.scene.world.popularity_queue.empty() &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "52 commits charge/m/F only; q and popularity still wait for53");
    for (int fault = 0; fault < 6; ++fault) {
        auto broken = s;
        if (fault == 0)
            broken.activity_page_bindings.erase(animation);
        if (fault == 1)
            broken.activity_page_selections.erase(animation);
        if (fault == 2)
            broken.activity_page_scroll.erase(animation);
        if (fault == 3)
            broken.page_counters.erase(animation);
        if (fault == 4)
            broken.activity_page_selections.at(animation) = 1;
        if (fault == 5)
            broken.activity_counts.erase(23);
        check(!inspect_startup_world_village_activity_page(broken, animation) &&
                  act_startup_world_village_activity_page(broken, animation, A::confirm) ==
                      E::missing_source &&
                  !prepare_startup_world_runtime(broken).candidate && broken.quarter_counter == 3 &&
                  broken.village_points == 180 && broken.events_held == 1 &&
                  broken.scene.world.popularity_queue.empty() && broken.scene.random.draws() == 2,
              "initialized53 missing or invalid payload rejects before timer or effects");
    }
    s.page_counters.at(animation) = 69;
    const auto sounds = s.sound_requests.size();
    page_tick(s);
    check(s.page_counters.at(animation) == 70 && s.sound_requests.size() == sounds + 1 &&
              s.sound_requests.back() == 5,
          "53 update70 produces sound5 once");
    page_tick(s);
    check(s.sound_requests.size() == sounds + 1, "sound70 does not replay on71");
    s.page_counters.at(animation) = 119;
    check(acknowledge_startup_world_runtime_page(s, animation) == E::none &&
              s.page_counters.at(animation) == 119 && s.quarter_counter == 3,
          "119 confirmation does not fast-forward or complete");
    check(cancel_startup_world_runtime_page(s, animation) == E::invalid_page,
          "53 cannot be cancelled into a completed activity");
    auto paused = s;
    paused.scene.framework_paused = true;
    check(acknowledge_startup_world_runtime_page(paused, animation) == E::invalid_page &&
              paused.quarter_counter == 3,
          "paused activity rejects player mutation");
    const auto paused_update = prepare_startup_world_runtime(paused);
    check(paused_update.candidate && paused_update.candidate->page_counters.at(animation) == 119 &&
              paused_update.candidate->sound_requests == paused.sound_requests &&
              paused_update.candidate->scene.random.draws() == paused.scene.random.draws() &&
              paused_update.candidate->scene.calendar.units == paused.scene.calendar.units,
          "paused framework preserves activity timer, queued sounds, random and calendar");
    page_tick(s);
    check(acknowledge_startup_world_runtime_page(s, animation) == E::none &&
              s.quarter_counter == 2 &&
              s.scene.world.popularity_queue == std::vector<std::array<int, 3>>{{10, 10, 1}} &&
              s.scene.random.draws() == 2,
          "120 completes cleaning with one global request and no extra draws");
    check(acknowledge_startup_world_runtime_page(s, animation) == E::invalid_page,
          "stale completed53 cannot consume q twice");
    page_tick(s);
    page_tick(s);
    check(s.activity_pages_initialized.empty() && s.activity_page_lists.empty() &&
              s.activity_page_bindings.empty() && s.activity_page_parents.empty() &&
              s.activity_page_answers.empty() && s.activity_page_selections.empty() &&
              s.activity_page_scroll.empty() && s.activity_page_display_humans.empty(),
          "completed empty catalogue retires all eight transient record families");
}
void village_expansion_pages() {
    using A = StartupVillageActivityAction;
    using E = StartupWorldRuntimeError;
    // 页面条件夹具：25/26为真实原表活动，开放状态和200点沿village_fixture明确准备。
    // 费用/时序的自然玩家路径由continuous的natural_expansion另验。
    for (const auto scenario :
         {std::array<int, 4>{25, 0, 1, 100}, {26, 1, 2, 200}, {25, 2, 2, 100}}) {
        auto s = village_fixture(scenario[0]);
        while (s.fence_level < scenario[1])
            check(expand_startup_world_map(s),
                  "expansion page fixture uses actual prior map effects");
        const auto parent = s.scripts.pages.back().id;
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto draws = s.scene.random.draws();
        check(acknowledge_startup_world_runtime_page(s, parent) == E::none,
              "type3 catalogue opens confirmation without charging or expanding");
        page_tick(s);
        const auto question = s.scripts.pages.back().id;
        check(s.fence_level == scenario[1] && s.village_points == 200 && s.events_held == 0,
              "type3 51 only opens52 and keeps resources");
        check(acknowledge_startup_world_runtime_page(s, question) == E::none,
              "type3 52 starts actual source expansion activity");
        page_tick(s);
        const auto animation = s.scripts.pages.back().id;
        check(s.scripts.pages.back().legacy_page == 53 && s.fence_level == scenario[1] &&
                  s.village_points == 200 - scenario[3] && s.quarter_counter == 3 &&
                  s.events_held == 1 && s.activity_counts.at(scenario[0]) == 1 &&
                  (s.activity_flags.at(scenario[0]) & 4U),
              "52 charges source points and held count but does not consume q or change map");
        s.page_counters.at(animation) = 119; // 明确演出计数边界夹具，不跳过效果提交。
        check(acknowledge_startup_world_runtime_page(s, animation) == E::none &&
                  s.page_counters.at(animation) == 119 && s.fence_level == scenario[1] &&
                  s.quarter_counter == 3,
              "type3 119 confirmation cannot complete or fast-forward");
        page_tick(s);
        if (scenario[1] == 0) {
            auto broken = s;
            // 两个kind4已重建、首个新外入口已创建后才耗尽维护ID，证明晚期整候选回滚。
            broken.next_facility_identity = std::numeric_limits<std::uint64_t>::max() - 3;
            const auto order = broken.scene.world.facility_order;
            const auto originals = broken.facility_original_ids;
            const auto serial = broken.next_facility_identity;
            check(acknowledge_startup_world_runtime_page(broken, animation) != E::none &&
                      broken.fence_level == 0 && broken.quarter_counter == 3 &&
                      broken.village_points == 100 && broken.events_held == 1 &&
                      broken.activity_counts.at(25) == 1 &&
                      broken.scene.world.facility_order == order &&
                      broken.facility_original_ids == originals &&
                      broken.next_facility_identity == serial &&
                      broken.scene.random.draws() == draws &&
                      broken.scripts.pages.back().id == animation &&
                      broken.scripts.pages.back().lifecycle != 4,
                  "late expansion failure preserves paid52 but rolls back "
                  "q/map/entities/page/random");
            for (std::size_t n = 0; n < s.surface.size(); ++n) {
                const auto &before = s.scene.world.world.map.cells[n];
                const auto &after = broken.scene.world.world.map.cells[n];
                check(before.legacy_state == after.legacy_state &&
                          before.category == after.category &&
                          before.facility.has_value() == after.facility.has_value() &&
                          (!before.facility ||
                           (before.facility->instance_id == after.facility->instance_id &&
                            before.facility->definition_id == after.facility->definition_id &&
                            before.facility->fragment_index == after.facility->fragment_index)) &&
                          s.surface[n].definition == broken.surface[n].definition &&
                          s.surface[n].instance == broken.surface[n].instance &&
                          s.surface[n].fragment == broken.surface[n].fragment &&
                          s.surface[n].road_mask == broken.surface[n].road_mask,
                      "late type3 failure leaves each original surface and binding unchanged");
            }
        }
        check(acknowledge_startup_world_runtime_page(s, animation) == E::none &&
                  s.fence_level == scenario[2] && s.quarter_counter == 2 &&
                  s.village_points == 200 - scenario[3] &&
                  s.scene.world.world.ai.accounting.funds() == cash &&
                  s.scene.random.draws() == draws && s.scene.world.world.map.cells.size() == 576,
              "120 applies type3 once; capped level is a successful no-op without refund");
        check(std::none_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                           [](const auto &p) { return p.lifecycle != 4 && p.legacy_page == 54; }) &&
                  acknowledge_startup_world_runtime_page(s, animation) == E::invalid_page,
              "type3 has no54 result draw and stale53 cannot repeat payment or expansion");
        page_tick(s);
        check(!s.activity_page_bindings.count(animation) &&
                  !s.activity_pages_initialized.count(animation) &&
                  !s.activity_page_parents.count(question),
              "type3 child pages and expansion payload retire on real parent resumption");
        const auto &remaining = s.activity_page_lists.at(parent);
        check(std::find(remaining.begin(), remaining.end(), scenario[0]) == remaining.end(),
              "held source bit2 expansion disappears from parent catalogue");
        check(act_startup_world_village_activity_page(s, parent, A::cancel) == E::none,
              "source expansion returns through normal catalogue cancellation");
    }
    auto forged = fixture(54);
    forged.activity_page_bindings[forged.scripts.pages.back().id] = 25;
    check(!prepare_startup_world_runtime(forged).candidate,
          "type3 cannot initialize a forged human-result54 payload");
}
void village_activity_effect_rollback() {
    using A = StartupVillageActivityAction;
    using E = StartupWorldRuntimeError;
    for (const int activity : {0, 4}) {
        auto s = village_fixture(activity);
        // 条件夹具：将真实独立W的定义0纳入开放池，不改W身份或创建场上实例。
        // 活跃／保留实例的完整同步组合由人物管理套件负责，这里只检查村办接线。
        if (activity == 4)
            s.human_presence.at(s.focus_actor.actor.definition) = 1;
        check(acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) == E::none,
              "open effect52");
        page_tick(s);
        check(acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) == E::none,
              "start effect53");
        const auto id = s.scripts.pages.back().id;
        page_tick(s);
        s.page_counters.at(id) = 120;
        const auto old_c = s.shop_humans.at(1).satisfaction;
        const auto old_extra = s.scene.world.world.ai.growth.at(1).definition.extra;
        const auto old_focus = s.focus_actor.actor;
        for (int fault = 0; fault < 2; ++fault) {
            auto broken = s;
            if (fault == 0)
                broken.scene.random = ref::WorldRandomStream::from_raw({0});
            else
                broken.scripts.page_mutations_locked = true;
            const auto draws = broken.scene.random.draws();
            check(act_startup_world_village_activity_page(broken, id, A::confirm) != E::none &&
                      broken.quarter_counter == 3 &&
                      broken.shop_humans.at(1).satisfaction == old_c &&
                      broken.scene.world.world.ai.growth.at(1).definition.extra == old_extra &&
                      broken.scene.random.draws() == draws && broken.scripts.pages.back().id == id,
                  "late second draw or script failure rolls back q, all human effects, random and "
                  "pages");
        }
        s.scene.random = ref::WorldRandomStream::from_raw({0, 0});
        check(acknowledge_startup_world_runtime_page(s, id) == E::none && s.quarter_counter == 2 &&
                  s.scene.random.draws() == 2 && s.scripts.pages.back().legacy_page == 54,
              "kind0/1 completion consumes same stream and opens real result54");
        if (activity == 4) {
            const auto &actor = s.focus_actor.actor;
            const auto &hp = actor.hp;
            const auto &old_hp = old_focus.hp;
            check(actor.capacity != old_focus.capacity &&
                      actor.capacity ==
                          s.scene.world.world.ai.growth.at(actor.definition).derived.combat[0] &&
                      hp.requested_delta == old_hp.requested_delta &&
                      hp.displayed == old_hp.displayed && hp.origin == old_hp.origin &&
                      hp.target == old_hp.target && hp.animating == old_hp.animating &&
                      hp.legacy_tick == old_hp.legacy_tick,
                  "kind1 activity refreshes real W maximum HP from shared growth without changing "
                  "six current HP slots");
        }
        const auto result = s.scripts.pages.back().id;
        page_tick(s);
        const auto view = inspect_startup_world_village_activity_page(s, result);
        check(view && view->display_humans[0] == view->display_humans[1] && !view->entries.empty(),
              "two draws permit the same definition and54 freezes open human references");
        auto no_longer_open = s;
        for (auto &[human, presence] : no_longer_open.human_presence) {
            (void)human;
            presence = 0;
        }
        const auto frozen = inspect_startup_world_village_activity_page(no_longer_open, result);
        check(frozen && frozen->entries == view->entries &&
                  frozen->display_humans == view->display_humans &&
                  no_longer_open.scene.random.draws() == 2,
              "54 preserves frozen valid references when current presence changes, without "
              "redrawing");
        for (int fault = 0; fault < 7; ++fault) {
            auto broken = s;
            if (fault == 0)
                broken.activity_page_bindings.erase(result);
            if (fault == 1)
                broken.activity_page_lists.erase(result);
            if (fault == 2)
                broken.activity_page_display_humans.erase(result);
            if (fault == 3)
                broken.page_counters.erase(result);
            if (fault == 4)
                broken.activity_page_selections.at(result) = -1;
            if (fault == 5)
                broken.human_activity_previous.erase(view->entries.front());
            if (fault == 6)
                broken.activity_page_lists.at(result).push_back(view->entries.front());
            check(!inspect_startup_world_village_activity_page(broken, result) &&
                      act_startup_world_village_activity_page(broken, result, A::cancel) ==
                          E::missing_source &&
                      !prepare_startup_world_runtime(broken).candidate &&
                      broken.quarter_counter == 2 && broken.scene.random.draws() == 2 &&
                      broken.scripts.pages.back().lifecycle != 4,
                  "initialized54 rejects missing, duplicate or invalid frozen result payload "
                  "without closing");
        }
        const auto effects = s.scene.world.world.ai.growth.at(1).definition.extra;
        const auto satisfaction = s.shop_humans.at(1).satisfaction;
        check(cancel_startup_world_runtime_page(s, result) == E::none && s.quarter_counter == 2 &&
                  s.scene.world.world.ai.growth.at(1).definition.extra == effects &&
                  s.shop_humans.at(1).satisfaction == satisfaction && s.scene.random.draws() == 2,
              "54 only closes; no repeated rewards or draws");
    }
}

// 合并到既有startup_world_pages_test.cpp；加commerce头并在main调用以下两函数。
void commerce_transactions() {
    using A = StartupCommerceAction;
    using E = StartupWorldRuntimeError;
    auto s = fixture(83);
    const auto menu = s.scripts.pages.back().id;
    check(initialize_startup_world_commerce_pages(s),
          "83 initializes three actual commerce choices");
    check(act_startup_world_commerce_page(s, menu, A::select, 3) == E::invalid_page,
          "83 rejects an out-of-range selection rather than opening an arbitrary page");
    check(act_startup_world_commerce_page(s, menu, static_cast<A>(99)) == E::invalid_page,
          "unknown commerce command cannot be interpreted as a purchase");
    // 固定新局A全0；仅注入最小补货夹具，价格400、已有2个马铃薯均读冻结原表。
    check(s.items.at(0).inventory == 2 && s.shop_item_stock.at(0).quantity == 0 &&
              s.rules->items.at(0).commerce_price == 400,
          "commerce oracle preserves raw potato inventory2 stock0 price400");
    s.shop_item_stock.at(0).quantity = 2;
    check(act_startup_world_commerce_page(s, menu, A::confirm) == E::none,
          "83 buy choice actually opens84 without charging");
    const auto buy = s.scripts.pages.back().id;
    check(initialize_startup_world_commerce_pages(s), "84 builds stock catalogue once");
    check(s.commerce_page_lists.at(buy) == std::vector<int>{0}, "buy catalogue uses A not z or p");
    check(act_startup_world_commerce_page(s, buy, A::next_tab) == E::none,
          "buy holdings tab is a view of the same transaction direction");
    for (int fault = 0; fault < 7; ++fault) {
        auto broken = s;
        if (fault == 0)
            broken.commerce_page_data.erase(buy);
        if (fault == 1)
            broken.commerce_page_lists.erase(buy);
        if (fault == 2)
            broken.commerce_page_data.at(buy)[2] = 8;
        if (fault == 3)
            broken.page_counters.erase(buy);
        if (fault == 4)
            broken.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
        if (fault == 5)
            broken.scene.world.world.ai.next_cash_id = std::numeric_limits<std::uint64_t>::max();
        if (fault == 6)
            broken.monthly_cash[3][3][1] = std::numeric_limits<int>::max();
        const auto entries = broken.scene.world.world.ai.accounting.entries().size();
        const auto pages = broken.scripts.pages.size();
        check(act_startup_world_commerce_page(broken, buy, A::confirm) != E::none &&
                  broken.scene.world.world.ai.accounting.funds() == 5000 &&
                  broken.scene.world.world.ai.accounting.entries().size() == entries &&
                  broken.items.at(0).inventory == 2 && broken.shop_item_stock.at(0).quantity == 2 &&
                  broken.scripts.pages.size() == pages && broken.scene.random.draws() == 0,
              "commerce missing payload, cash overflow and late page insertion failures roll back "
              "all domains");
    }
    auto paused = s;
    paused.scene.framework_paused = true;
    check(act_startup_world_commerce_page(paused, buy, A::confirm) == E::invalid_page &&
              paused.scene.world.world.ai.accounting.funds() == 5000,
          "paused commerce cannot charge");
    check(act_startup_world_commerce_page(s, buy, A::confirm) == E::none &&
              s.scene.world.world.ai.accounting.funds() == 4600 && s.items.at(0).inventory == 3 &&
              s.shop_item_stock.at(0).quantity == 1 && s.village_points == 10 &&
              s.monthly_cash[3][3][1] == 400 && s.scene.random.draws() == 0,
          "84 holdings tab buys one potato using cash category3, never village points");
    const auto result = s.scripts.pages.back().id;
    check(s.scripts.pages.back().legacy_page == 86, "trade has actual result86");
    const auto updated = update_startup_world_commerce_page(s, result);
    check(updated.has_value(), "86 actual update closes without confirmation or a second trade");
    s = *updated;
    check(s.scene.world.world.ai.accounting.funds() == 4600 &&
              act_startup_world_commerce_page(s, result, A::confirm) == E::invalid_page,
          "retired86 cannot repeat trade");
    check(act_startup_world_commerce_page(s, buy, A::confirm) == E::none &&
              s.commerce_page_data.at(buy)[5] == 0 && s.items.at(0).inventory == 3 &&
              s.scene.world.world.ai.accounting.funds() == 4600,
          "first parent confirmation clears20 feedback rather than purchasing twice");
    s.item_commerce_read.at(0) = false;
    check(act_startup_world_commerce_page(s, buy, A::cancel) == E::none &&
              s.item_commerce_read.at(0) && s.items.at(0).inventory == 3,
          "cancel marks remaining catalogue B read without consuming inventory");

    s = fixture(84);
    s.scripts.pages.back().legacy_f = 1;
    const auto sell = s.scripts.pages.back().id;
    check(initialize_startup_world_commerce_pages(s) && s.commerce_page_lists.at(sell).front() == 0,
          "sell catalogue reads actual initial holdings");
    check(act_startup_world_commerce_page(s, sell, A::confirm) == E::none &&
              s.scene.world.world.ai.accounting.funds() == 5200 && s.items.at(0).inventory == 1 &&
              s.shop_item_stock.at(0).quantity == 0 && s.village_points == 10 &&
              s.monthly_cash[3][3][0] == 200 && s.cash_peak == 5200 && s.maximum_income == 0,
          "sale returns half-price200 cash, updates peak, not merchant stock or highest facility "
          "income");
    check(s.scene.world.world.ai.accounting.entries().rbegin()->second.category ==
              ref::CashCategory::shop,
          "commerce ledger records shop rather than generic other category");
}
void commerce_facility_and_projection() {
    using A = StartupCommerceAction;
    using E = StartupWorldRuntimeError;
    auto s = fixture(85);
    const auto shop = s.scripts.pages.back().id;
    check(initialize_startup_world_commerce_pages(s),
          "85 initializes rank-limited facility catalogue");
    const auto &list = s.commerce_page_lists.at(shop);
    const auto found = std::find(list.begin(), list.end(), 34);
    check(found != list.end() && s.rules->facility_initial.at(34).capacity == 30 &&
              s.facility_presence.at(34) == 0,
          "rank0 cold drink shop oracle costs30 village points");
    const int selected = static_cast<int>(found - list.begin());
    check(act_startup_world_commerce_page(s, shop, A::select, selected) == E::none,
          "85 selection binds original facility identity34");
    const auto initial_read = s.facility_commerce_read;
    const auto attributes = s.scripts.facilities.at(34).attributes;
    // 从真实85动作打开定义详情；两种返回都不能借用已建实例、扣款或清NEW。
    for (const auto action :
         {StartupFacilityPageAction::cancel, StartupFacilityPageAction::confirm}) {
        check(act_startup_world_commerce_page(s, shop, A::inspect) == E::none,
              "85 inspect action opens actual definition preview rather than purchasing");
        const auto preview = s.scripts.pages.back().id;
        check(s.scripts.pages.back().legacy_page == 74 && s.scripts.pages.back().legacy_g == 1 &&
                  s.facility_definition_page_bindings.at(preview) == 34 &&
                  !s.facility_page_bindings.count(preview) &&
                  !s.facility_page_neighbours.count(preview) &&
                  valid_startup_world_facility_page(s, s.scripts.pages.back()) &&
                  startup_world_facility_page_count(s, s.scripts.pages.back()) == 1 &&
                  s.scripts.facilities.at(34).attributes == attributes &&
                  s.scene.world.world.ai.accounting.funds() == 5000 && s.village_points == 10 &&
                  s.facility_commerce_read == initial_read && s.facility_free_builds.at(34) == 0,
              "74 preview reads shared definition only, with no invented instance or commerce side "
              "effects");
        check(
            act_startup_world_facility_page(s, preview, action) == E::none,
            "74 definition confirm and cancel both return without opening item or shop consumers");
        const auto resumed = inspect_startup_world_commerce_page(s, shop);
        check(
            resumed && resumed->selection == selected && resumed->entries.at(selected) == 34 &&
                s.scene.world.world.ai.accounting.funds() == 5000 && s.village_points == 10 &&
                s.facility_commerce_read == initial_read && s.scene.random.draws() == 0,
            "definition preview restores the same85 selection and preserves money, NEW and random");
        page_tick(s);
        check(s.scripts.pages.back().id == shop &&
                  !s.facility_definition_page_bindings.count(preview),
              "framework retires definition preview binding before original85 purchase continues");
    }
    // 精确支付边界夹具；不宣称自然取得村点。
    s.village_points = 30;
    auto broken = s;
    broken.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
    check(act_startup_world_commerce_page(broken, shop, A::confirm) != E::none &&
              broken.village_points == 30 && broken.facility_free_builds.at(34) == 0 &&
              broken.facility_presence.at(34) == 0,
          "late93 insertion failure keeps points and unlocks unchanged");
    check(act_startup_world_commerce_page(s, shop, A::confirm) == E::none &&
              s.village_points == 0 && s.scene.world.world.ai.accounting.funds() == 5000 &&
              s.facility_free_builds.at(34) == 0 && s.facility_presence.at(34) == 0,
          "85 spends points now while93 reward remains pending");
    const auto reward = s.scripts.pages.back().id;
    check(s.scripts.pages.back().legacy_page == 93 && initialize_startup_world_commerce_pages(s),
          "facility redemption opens actual93r3");
    check(act_startup_world_commerce_page(s, reward, A::cancel) == E::invalid_page,
          "93 does not invent cancellation after spending points");
    check(act_startup_world_commerce_page(s, reward, A::confirm) == E::none &&
              s.page_counters.at(reward) == 40 && s.facility_free_builds.at(34) == 0,
          "early93 confirm only fast-forwards40");
    check(act_startup_world_commerce_page(s, reward, A::confirm) == E::none &&
              s.facility_free_builds.at(34) == 1 && s.facility_presence.at(34) == 2 &&
              s.facility_unlock_notices.at(34) && s.village_points == 0 &&
              s.scene.world.world.ai.accounting.funds() == 5000,
          "ready93 unlocks once and grants one free build without further payment");
    check(act_startup_world_commerce_page(s, reward, A::confirm) == E::invalid_page,
          "retired93 cannot grant another free build");

    s = fixture(83);
    // 模拟已发生的原奖励解锁，与仍保留旧p/q/r的补货辅助记录刻意不同。
    s.items.at(2).status = 1;
    s.items.at(2).unlock_counter = 6;
    s.items.at(2).newly_unlocked = true;
    s.catalog.at({0, 2}) = s.items.at(2);
    s.shop_item_stock.at(2).presence = 0;
    s.shop_item_stock.at(2).legacy_q = 0;
    s.shop_item_stock.at(2).newly_available = false;
    const auto adapter = startup_world_runtime_adapter();
    auto projection = adapter.maintenance.read(s);
    const auto actual = std::find_if(projection.shop_items.begin(), projection.shop_items.end(),
                                     [](const auto &v) { return v.definition == 2; });
    check(actual != projection.shop_items.end() && actual->presence == 1 && actual->legacy_q == 6 &&
              actual->newly_available && adapter.maintenance.write(s, projection) &&
              s.items.at(2).status == 1 && s.items.at(2).unlock_counter == 6 &&
              s.catalog.at({0, 2}).newly_unlocked && s.shop_item_stock.at(2).presence == 1,
          "maintenance reads current shared item definition, never overwrites reward from stale "
          "stock mirror");
}
void ordinary_item_pages() {
    using A = StartupHumanPageAction;
    using E = StartupWorldRuntimeError;
    auto s = human_fixture(64);
    const auto menu = s.scripts.pages.back().id;
    page_tick(s);
    check(s.items.at(0).inventory == 2 && s.items.at(0).free_purchases == 0,
          "real potato starts with two ordinary items, no equipment free-purchase tokens");
    check(act_startup_world_human_page(s, menu, A::equipment_slot, 4) == E::none &&
              s.equipment_page_catalogs.at(menu)[4] == std::vector<int>({0, 29}),
          "fifth64 tab lists original ordinary inventory without an injected stock");
    const auto before_extra = s.scene.world.world.ai.growth.at(1).definition.extra;
    const auto before_cash = s.scene.world.world.ai.accounting.funds();
    const auto before_draws = s.scene.random.draws();
    s.items.at(29).newly_unlocked = s.catalog.at({0, 29}).newly_unlocked = true;
    for (int fault = 0; fault < 5; ++fault) {
        auto bad = s;
        if (fault == 0)
            bad.equipment_page_catalogs.erase(menu);
        if (fault == 1)
            bad.human_page_selections.at(menu) = 100;
        if (fault == 2)
            bad.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
        if (fault == 3)
            bad.scene.random = ref::WorldRandomStream::from_raw({});
        if (fault == 4)
            bad.catalog.erase({0, 29});
        const auto original_draws = bad.scene.random.draws();
        check(act_startup_world_human_page(bad, menu, A::confirm) != E::none &&
                  bad.items.at(0).inventory == 2 && bad.catalog.at({0, 0}).inventory == 2 &&
                  bad.scene.world.world.ai.growth.at(1).definition.extra == before_extra &&
                  bad.scene.world.world.ai.accounting.funds() == before_cash &&
                  bad.scene.random.draws() == original_draws &&
                  bad.scripts.pages.size() == s.scripts.pages.size(),
              ("ordinary gift rollback fault=" + std::to_string(fault)).c_str());
    }
    check(
        act_startup_world_human_page(s, menu, A::confirm) == E::none &&
            s.items.at(0).inventory == 1 && s.catalog.at({0, 0}).inventory == 1 &&
            s.items.at(0).free_purchases == 0 && !s.items.at(29).newly_unlocked &&
            !s.catalog.at({0, 29}).newly_unlocked &&
            s.scene.world.world.ai.growth.at(1).definition.extra[0] == before_extra[0] + 4 &&
            s.scene.world.world.ai.accounting.funds() == before_cash &&
            s.scene.random.draws() == before_draws + 1 &&
            std::none_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                         [](const auto &p) { return p.legacy_page == 65; }),
        "ordinary64 commits immediately: consumes one potato, source extra4, no65 or cash charge");
    // 经过实际页消费者退回64；没有直接弹栈，也没有跳过66/69计数。
    for (int n = 0; n < 500 && s.scripts.pages.back().id != menu; ++n) {
        page_tick(s);
        const auto &p = s.scripts.pages.back();
        if (p.id != menu && p.lifecycle != 4)
            check(acknowledge_startup_world_runtime_page(s, p.id) == E::none,
                  "ordinary gift result follows actual timed page consumers");
        s.sound_requests.clear();
    }
    check(s.scripts.pages.back().id == menu, "ordinary results retire to original64");
    check(act_startup_world_human_page(s, menu, A::confirm) == E::none &&
              s.items.at(0).inventory == 0 && s.catalog.at({0, 0}).inventory == 0 &&
              s.equipment_page_catalogs.at(menu)[4] == std::vector<int>{29} &&
              s.items.at(29).inventory == 3,
          "last potato is removed while three original recovery items remain in parent catalogue");

    s = human_fixture(66);
    const auto page = s.scripts.pages.back().id;
    s.human_equipment_choices[page] = {4, 29};
    s.items.at(29).inventory = s.catalog.at({0, 29}).inventory = 0;
    s.human_gift_scores[page] = 50;
    s.human_gift_messages[page] = "谢谢";
    check(initialize_startup_world_human_pages(s),
          "ordinary recovery display accepts already-consumed stock");
    ref::BattleActorRecord first;
    first.id = {800};
    first.kind = ref::ActorKind::human;
    first.definition = 1;
    first.capacity = 100;
    first.hp.target = first.hp.displayed = 20;
    auto second = first;
    second.id = {801};
    s.scene.world.world.ai.battle.actors[first.id] = first;
    s.scene.world.world.ai.battle.actors[second.id] = second;
    s.scene.world.world.ai.human_order = {first.id, second.id};
    s.scene.world.world.ai.retired_actors[{802}] = first;
    s.focus_actor.actor = first;
    s.page_counters[page] = 74;
    const auto healed = update_startup_world_human_page(s, page);
    check(healed && healed->scene.world.world.ai.battle.actors.at(first.id).hp.target == 70 &&
              healed->scene.world.world.ai.battle.actors.at(second.id).hp.target == 20 &&
              healed->scene.world.world.ai.retired_actors.at({802}).hp.target == 20 &&
              healed->focus_actor.actor.hp.target == 20,
          "counter75 heals first live same-definition actor only, never duplicates or retired/W");
    s = *healed;
    const auto displayed = s.scene.world.world.ai.battle.actors.at(first.id).hp.displayed;
    check(acknowledge_startup_world_runtime_page(s, page) == E::none &&
              acknowledge_startup_world_runtime_page(s, page) == E::none &&
              s.scene.world.world.ai.battle.actors.at(first.id).hp.target == 70 &&
              s.scene.world.world.ai.battle.actors.at(first.id).hp.displayed == displayed,
          "repeated66 confirm neither heals again nor advances HP animation");

    s = human_fixture(69);
    const auto effect_page = s.scripts.pages.back().id;
    s.human_equipment_choices[effect_page] = {4, 0};
    check(initialize_startup_world_human_pages(s), "69 binds a real attribute item");
    check(acknowledge_startup_world_runtime_page(s, effect_page) == E::none &&
              s.page_counters.at(effect_page) == 39 && s.scripts.pages.back().lifecycle != 4,
          "69 early confirm stops at source39");
    s.page_counters.at(effect_page) = 44;
    check(acknowledge_startup_world_runtime_page(s, effect_page) == E::none &&
              s.scripts.pages.back().lifecycle != 4,
          "69 does not close before45");
    const auto effect_tick = update_startup_world_human_page(s, effect_page);
    check(effect_tick.has_value(), "69 actual update reaches45");
    s = *effect_tick;
    check(acknowledge_startup_world_runtime_page(s, effect_page) == E::none &&
              s.scripts.pages.back().lifecycle == 4,
          "69 closes at45 without granting again");
}
void commerce_after_reward_writeback() {
    using E = StartupWorldRuntimeError;
    using C = StartupCommerceAction;
    for (int source = 0; source < 2; ++source) {
        auto s = test_support::world_fixture();
        const auto adapter = startup_world_runtime_adapter();
        auto projected = adapter.nonactors.read_routes(s);
        const auto grant = ref::prepare_object_grant(
            projected.objects, 0, 0,
            source == 0 ? ref::ObjectGrantOrigin::ground_pickup : ref::ObjectGrantOrigin::direct);
        check(grant.candidate.has_value(),
              "real ordinary-item reward produces catalogue inventory increment");
        if (source == 0) {
            projected.objects = grant.candidate->state;
            auto broken = s;
            broken.items.erase(0);
            check(!adapter.nonactors.write_routes(broken, projected) &&
                      broken.catalog.at({0, 0}).inventory == 2 &&
                      broken.item_rewards == s.item_rewards,
                  "missing pickup item mirror rejects writer without publishing partial catalogue "
                  "reward");
            check(adapter.nonactors.write_routes(s, projected),
                  "nonactor writer publishes real pickup grant");
        } else {
            auto finished = startup_world_runtime_finish(s);
            finished.dungeon.catalog = grant.candidate->state.catalog;
            finished.dungeon.item_rewards = grant.candidate->state.item_rewards;
            auto broken = s;
            broken.items.erase(0);
            check(!write_startup_world_runtime_finish(broken, finished) &&
                      broken.catalog.at({0, 0}).inventory == 2 &&
                      broken.item_rewards == s.item_rewards,
                  "missing finish item mirror rejects writer without publishing partial catalogue "
                  "reward");
            check(write_startup_world_runtime_finish(s, finished),
                  "finish writer publishes real direct item grant");
        }
        check(s.items.at(0).inventory == 3 && s.catalog.at({0, 0}).inventory == 3 &&
                  s.items.at(0).status == s.catalog.at({0, 0}).status &&
                  s.items.at(0).newly_unlocked == s.catalog.at({0, 0}).newly_unlocked,
              source == 0
                  ? "pickup writer synchronizes ordinary inventory before commerce initialization"
                  : "finish writer synchronizes ordinary inventory before commerce initialization");
        // 最小补货夹具只给两件A；开放与83/120/121页栈来自原128延迟程序的真实36指令。
        s.shop_item_stock.at(0).quantity = 2;
        auto program = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                 startup_world_runtime_scripts(s), {128, {}, {}});
        check(program.candidate.has_value(), "fixed event128 starts its actual delayed program");
        auto scripts = program.candidate->state;
        bool inserted{};
        for (int tick = 0; tick < 501 && !inserted; ++tick) {
            const auto resumed = ref::prepare_world_script_continuations(
                startup_world_runtime_catalog(), scripts, true);
            check(resumed.candidate.has_value(), "event128 admits one actual continuation update");
            scripts = resumed.candidate->state;
            inserted = std::any_of(scripts.pages.begin(), scripts.pages.end(), [](const auto &p) {
                return p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == 83;
            });
        }
        check(inserted && (scripts.user_flags & 16U) &&
                  write_startup_world_runtime_scripts(s, scripts),
              "actual opcode36 creates commerce with lower original script pages and real unlock");
        const auto top = [&]() -> ref::WorldScriptPage {
            const auto found = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                            [](const auto &p) { return p.lifecycle != 4; });
            check(found != s.scripts.pages.rend(),
                  "commerce reward sequence retains a live framework root");
            return *found;
        };
        const auto reach = [&](int raw) {
            for (int n = 0; n < 100; ++n) {
                const auto before = top();
                if (raw == -1 && before.kind == ref::WorldScriptPageKind::scene)
                    return before.id;
                page_tick(s);
                const auto p = top();
                if (p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == raw)
                    return p.id;
                if (p.kind != ref::WorldScriptPageKind::scene && p.legacy_page != 86)
                    check(acknowledge_startup_world_runtime_page(s, p.id) == E::none,
                          "original commerce script dialogue is consumed through its real page "
                          "action");
            }
            throw std::runtime_error("reward commerce page sequence exceeded fixture bound");
        };
        const auto menu = reach(83);
        check(act_startup_world_commerce_page(s, menu, C::confirm) == E::none,
              "script-created83 opens buy84 after real item reward");
        const auto buy = reach(84);
        check(s.commerce_page_lists.at(buy) == std::vector<int>{0} &&
                  act_startup_world_commerce_page(s, buy, C::confirm) == E::none &&
                  s.items.at(0).inventory == 4 && s.catalog.at({0, 0}).inventory == 4 &&
                  s.scene.world.world.ai.accounting.funds() == 4600,
              "84 purchase follows reward once with strict mirrored catalogue and exact400 cash "
              "charge");
        check(top().legacy_page == 86, "reward-backed transaction inserts actual86 result");
        page_tick(s); // 86自动结束，不以玩家确认跳过。
        page_tick(s); // 框架退休86再恢复父84。
        check(top().id == buy && act_startup_world_commerce_page(s, buy, C::cancel) == E::none,
              "86 retires and original84 can cancel after successful real transaction");
        check(act_startup_world_commerce_page(s, menu, C::cancel) == E::none,
              "cancel only original83 and retain its lower event36 dialogue pages");
        (void)reach(-1);
        check(open_startup_world_human_page(s, 1) == E::none,
              "post-commerce original main root opens existing human definition without injected "
              "actor");
        const auto human = reach(60);
        check(act_startup_world_human_page(s, human, StartupHumanPageAction::gifts) == E::none,
              "human60 opens its actual gift catalogue after commerce return");
        const auto gifts = reach(64);
        check(
            act_startup_world_human_page(s, gifts, StartupHumanPageAction::equipment_slot, 4) ==
                    E::none &&
                s.items.at(0).inventory == 4 && s.catalog.at({0, 0}).inventory == 4 &&
                s.scene.world.world.ai.accounting.funds() == 4600,
            "fifth64 tab reads actual post-reward purchase inventory without another grant or fee");
    }
}

} // namespace
int main() {
    try {
        facility_commodity_pages();
        facility_reputation_pages();
        ordinary_item_pages();
        commerce_after_reward_writeback();
        commerce_transactions();
        commerce_facility_and_projection();
        village_activity_initialization();
        village_activity_pages();
        village_expansion_pages();
        village_activity_effect_rollback();
        summary();
        gift();
        unlock_rewards();
        focus_and_pause();
        crew_initialization();
        rank_conditions();
        rank_promotion();
        task_focus_and_introduction();
        annual_termination();
        annual_award();
        task_display();
        event_message_and_shop_return();
        popularity_return();
        unlocked_visitor();
        human_details_and_gifts();
        human_profession_and_mastery();
        tax_pages();
        std::cout << "startup world pages checks: " << checks << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
