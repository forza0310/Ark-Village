#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_expansion.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_tax.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "support/world_fixture.hpp"
#include "support/audio_requests.hpp"

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
                  test_support::audio_ids(s.sound_requests) == std::vector<int>{4} &&
                  s.sound_requests.front().operation == StartupAudioOperation::jingle,
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
    if (!r.candidate) {
        std::cerr << "page update error=" << static_cast<int>(r.error) << " stack=";
        for (const auto &page : s.scripts.pages)
            std::cerr << page.id << ':' << page.legacy_page << '/' << page.lifecycle
                      << "/initialized=" << s.human_pages_initialized.count(page.id) << ' ';
        std::cerr << '\n';
    }
    check(r.candidate.has_value(), "managed page actual framework update succeeds");
    s = *r.candidate;
}
void information_menu_pages() {
    using E=StartupWorldRuntimeError;
    auto s=test_support::world_fixture();
    check(open_startup_world_information_menu(s)==E::none,
          "real scene opens maintained information9 without fabricating a main-menu3");
    const auto id=s.scripts.pages.back().id;
    check(s.scripts.pages.back().legacy_page==9 && !s.page_phases.count(id) &&
              !s.page_counters.count(id) && !inspect_startup_world_information_page(s,id),
          "new9 waits for framework initialization, query does not install payload");
    auto frozen=startup_world_state_digest(s);
    StartupInformationInput confirm;confirm.confirm=true;
    check(input_startup_world_information_page(s,id,confirm)!=E::none &&
              startup_world_state_digest(s)==frozen,"uninitialized9 input atomically rejects");
    auto paused_entry=s;paused_entry.scene.framework_paused=true;
    page_tick(paused_entry);
    check(paused_entry.scripts.pages.back().lifecycle==0 &&
              !paused_entry.page_phases.count(id) && !paused_entry.page_counters.count(id),
          "pause before first information callback retains uninitialized lifecycle and absent payload");
    paused_entry.scene.framework_paused=false;page_tick(paused_entry);
    check(inspect_startup_world_information_page(paused_entry,id).has_value(),
          "unpaused fresh information page initializes through its real framework entry");
    page_tick(s);
    auto view=inspect_startup_world_information_page(s,id);
    check(view && view->raw==9 && view->selection_or_period==0 && view->frame==2 && !view->income,
          "raw9 framework global count then menu count gives first frame2");
    constexpr std::array<int,5> tags{15,14,16,17,18},targets{35,34,36,37,38};
    for(int row=0;row<5;++row)
        check(view->entries[row].tag==tags[row] && view->entries[row].target_raw==targets[row] &&
                  view->entries[row].implemented,
              "raw9 retains all five original ordered rows; implemented is maintenance capability only");
    page_tick(s);
    check(s.page_counters.at(id)==3,"raw9 second update reaches original menu count cap3");
    StartupInformationInput both;both.up=true;both.down=true;both.confirm=true;
    check(input_startup_world_information_page(s,id,both)==E::none && s.page_phases.at(id)==4,
          "raw9 simultaneous up/down/confirm takes up only and wraps to row4 without opening child");
    StartupInformationInput down;down.down=true;
    check(input_startup_world_information_page(s,id,down)==E::none && s.page_phases.at(id)==0,
          "raw9 down wraps row4 back to row0");
    for(int fault=0;fault<12;++fault) {
        auto bad=s;
        StartupInformationInput input=down;
        if(fault==0)bad.page_phases.erase(id);
        if(fault==1)bad.page_counters.erase(id);
        if(fault==2)bad.page_phases.at(id)=-1;
        if(fault==3)bad.page_phases.at(id)=5;
        if(fault==4)bad.page_counters.at(id)=-1;
        if(fault==5)bad.page_counters.at(id)=4;
        if(fault==6)bad.scripts.pages.back().lifecycle=1;
        if(fault==7)bad.scripts.pages.back().lifecycle=3;
        if(fault==8)bad.scripts.pages.back().lifecycle=4;
        if(fault==9)bad.scene.framework_paused=true;
        if(fault==10){input={};input.select_row=5;}
        if(fault==11){input={};input.select_row=2;input.confirm=true;}
        const auto digest=startup_world_state_digest(bad);
        check(input_startup_world_information_page(bad,id,input)!=E::none &&
                  startup_world_state_digest(bad)==digest,
              "raw9 bad payload/selection/lifecycle/pause or mixed row/key input rejects whole action");
    }
    auto missing=s;missing.page_phases.erase(id);missing.page_counters.erase(id);
    check(!prepare_startup_world_runtime(missing).candidate,
          "initialized9 with both payload maps missing cannot masquerade as a fresh page");
    StartupInformationInput select;select.select_row=2;
    check(input_startup_world_information_page(s,id,select)==E::none &&
              input_startup_world_information_page(s,id,confirm)==E::none,
          "raw9 income row opens actual36");
    check(s.scripts.pages.back().legacy_page==36 &&
              std::any_of(s.scripts.pages.begin(),s.scripts.pages.end(),[&](const auto &p){return p.id==id&&p.lifecycle==4;}),
          "income selection retires9 after pushing36");
    const auto income=s.scripts.pages.back().id;
    page_tick(s);
    check(inspect_startup_world_information_page(s,income).has_value() &&
              !s.page_phases.count(id) && !s.page_counters.count(id),
          "framework removes retired9 and its two maps while initializing36");
    // 独立原raw3父页条件夹具：不把维护scene快捷入口冒称完整主菜单实现。
    auto parent=fixture(3);parent.scripts.executing_page.reset();parent.scripts.pages.back().lifecycle=2;
    const auto main=parent.scripts.pages.back().id;
    check(open_startup_world_information_menu(parent)==E::none,"actual raw3 parent accepts information entry");
    const auto menu=parent.scripts.pages.back().id;page_tick(parent);
    auto cancelled=parent;StartupInformationInput back;back.left=true;
    check(input_startup_world_information_page(cancelled,menu,back)==E::none &&
              cancelled.scripts.pages.back().lifecycle==4 &&
              std::any_of(cancelled.scripts.pages.begin(),cancelled.scripts.pages.end(),[&](const auto &p){
                  return p.id==main&&p.lifecycle!=4;}),"raw9 left retires only itself and retains actual raw3 parent");
    check(input_startup_world_information_page(parent,menu,select)==E::none &&
              input_startup_world_information_page(parent,menu,confirm)==E::none,
          "raw3-to9 actual stack selects36");
    for(const auto retired:{main,menu})
        check(std::any_of(parent.scripts.pages.begin(),parent.scripts.pages.end(),[&](const auto &p){
                  return p.id==retired&&p.lifecycle==4;}),"raw36 selection retires both actual menu3 and9");
}
void income_information_pages() {
    using E=StartupWorldRuntimeError;
    auto s=test_support::world_fixture();
    check(open_startup_world_information_menu(s)==E::none,"income test opens real information entry");
    const auto menu=s.scripts.pages.back().id;page_tick(s);
    StartupInformationInput select;select.select_row=2;
    StartupInformationInput confirm;confirm.confirm=true;
    check(input_startup_world_information_page(s,menu,select)==E::none &&
              input_startup_world_information_page(s,menu,confirm)==E::none,"income test selects original row2");
    const auto id=s.scripts.pages.back().id;page_tick(s);
    // 仅注入权威桶作投影条件夹具，不声称这些收入已自然赚取；金额算式矩阵归纯查询套件。
    s.monthly_cash={};const int month=s.scene.calendar.month,other=(month+1)%12;
    s.monthly_cash[month][0]={123,7};s.monthly_cash[other][0]={20,3};
    auto before=startup_world_state_digest(s);
    auto view=inspect_startup_world_information_page(s,id);
    check(view && view->raw==36 && view->selection_or_period==0 && view->income &&
              view->income->rows[0].income==123 && view->income->rows[0].expense==7 &&
              startup_world_state_digest(s)==before,"month view reads current Owner bucket without writes");
    const auto count=s.page_counters.at(id);
    const auto updates=s.scene.world.updates;
    const auto units=s.scene.calendar.units;
    const auto draws=s.scene.random.draws();
    page_tick(s);
    check(s.page_counters.at(id)==count+1 && s.scene.world.updates==updates &&
              s.scene.calendar.units==units && s.scene.random.draws()==draws,
          "modal36 advances only page count while world/date/random remain frozen");
    StartupInformationInput left;left.left=true;
    check(input_startup_world_information_page(s,id,left)==E::none && s.page_phases.at(id)==1,
          "36 left from month wraps to year");
    before=startup_world_state_digest(s);view=inspect_startup_world_information_page(s,id);
    check(view && view->income && view->income->rows[0].income==143 && view->income->rows[0].expense==10 &&
              startup_world_state_digest(s)==before,"year view reads same current Owner across twelve buckets");
    StartupInformationInput both;both.left=true;both.right=true;
    check(input_startup_world_information_page(s,id,both)==E::none && s.page_phases.at(id)==1,
          "36 left/right are independent in one input, two flips retain year");
    StartupInformationInput right;right.right=true;
    check(input_startup_world_information_page(s,id,right)==E::none && s.page_phases.at(id)==0,
          "36 right from year wraps to month");
    auto wrapping=s;wrapping.page_counters.at(id)=std::numeric_limits<int>::max()-1;
    const auto wrapped=update_startup_world_information_page(wrapping,id);
    check(wrapped && wrapped->page_counters.at(id)==0,"36 legal count upper endpoint advances moduloINT_MAX");
    for(int fault=0;fault<10;++fault) {
        auto bad=s;StartupInformationInput input=right;
        if(fault==0)bad.page_phases.erase(id);
        if(fault==1)bad.page_counters.erase(id);
        if(fault==2)bad.page_phases.at(id)=2;
        if(fault==3)bad.page_counters.at(id)=-1;
        if(fault==4)bad.page_counters.at(id)=std::numeric_limits<int>::max();
        if(fault==5)bad.scripts.pages.back().lifecycle=1;
        if(fault==6)bad.scripts.pages.back().lifecycle=3;
        if(fault==7)bad.scripts.pages.back().lifecycle=4;
        if(fault==8)bad.scene.framework_paused=true;
        if(fault==9){input={};input.select_row=0;}
        const auto digest=startup_world_state_digest(bad);
        check(input_startup_world_information_page(bad,id,input)!=E::none && startup_world_state_digest(bad)==digest,
              "36 bad payload/lifecycle/pause/selection rejects; INT_MAX rejection is maintenance overflow policy");
    }
    auto paused=s;paused.scene.framework_paused=true;
    const auto pause_frame=paused.page_counters.at(id);page_tick(paused);
    check(paused.page_counters.at(id)==pause_frame && paused.scene.world.updates==updates &&
              paused.scene.random.draws()==draws,"framework pause freezes information counter too");
    auto double_close=s;both.confirm=true;
    check(input_startup_world_information_page(double_close,id,both)==E::none &&
              double_close.page_phases.at(id)==0 && double_close.scripts.pages.back().lifecycle==4,
          "same input left/right both run before confirm retires36");
    left.confirm=true;
    check(input_startup_world_information_page(s,id,left)==E::none && s.page_phases.at(id)==1 &&
              s.scripts.pages.back().lifecycle==4,"single direction changes period before same-input confirm closes");
    before=startup_world_state_digest(s);
    check(input_startup_world_information_page(s,id,confirm)!=E::none && startup_world_state_digest(s)==before,
          "stale closed36 input cannot flip or close again");
    const auto steps=s.simulation_steps;page_tick(s);
    check(!s.page_phases.count(id) && !s.page_counters.count(id) && s.simulation_steps==steps+1,
          "closed36 releases both maps and restores actual main-scene simulation admission");
    // 初局介绍仍按真实确认消耗，允许新局脚本先出现；不篡改事件seen去强求同轮经营。
    for(int n=0;n<16 && s.scripts.pages.back().kind!=ref::WorldScriptPageKind::scene;++n) {
        const auto top=s.scripts.pages.back();
        if(top.lifecycle!=4)
            check(acknowledge_startup_world_runtime_page(s,top.id)==E::none,
                  "post-information startup dialogue uses real confirmation");
        page_tick(s);
    }
    check(s.scripts.pages.back().kind==ref::WorldScriptPageKind::scene &&
              open_startup_world_information_menu(s)==E::none,"after real return next information entry is available");
    const auto next=s.scripts.pages.back().id;page_tick(s);
    check(next!=menu && !s.page_phases.count(menu) && !s.page_counters.count(menu) &&
              !s.page_phases.count(id) && !s.page_counters.count(id),"new entry retains no retired information IDs");
    check(input_startup_world_information_page(s,next,select)==E::none &&
              input_startup_world_information_page(s,next,confirm)==E::none,"next menu opens a fresh36 object");
    const auto fresh=s.scripts.pages.back().id;page_tick(s);
    check(fresh!=id && s.page_phases.at(fresh)==0,"fresh36 starts month0 rather than retaining previous year");
    check(cancel_startup_world_runtime_page(s,fresh)==E::none && s.scripts.pages.back().lifecycle==4,
          "runtime cancel bridge closes36 through the maintained Owner action");
}
std::uint64_t enter_information_catalog(StartupWorldRuntimeState &s,int row) {
    using E=StartupWorldRuntimeError;
    check(open_startup_world_information_menu(s)==E::none,"catalogue path opens actual information9 from scene");
    const auto menu=s.scripts.pages.back().id;page_tick(s);
    StartupInformationInput select;select.select_row=row;
    StartupInformationInput confirm;confirm.confirm=true;
    check(input_startup_world_information_page(s,menu,select)==E::none &&
              input_startup_world_information_page(s,menu,confirm)==E::none,
          "catalogue path selects original information menu row");
    const auto id=s.scripts.pages.back().id;
    check(s.scripts.pages.back().legacy_page==(row==0?35:row==1?34:row==3?37:38) &&
              s.scripts.pages.back().lifecycle==0 && !s.information_page_data.count(id),
          "new information directory waits for framework Init before owning frozen IDs");
    return id;
}
void town_facility_information_pages() {
    using E=StartupWorldRuntimeError;
    auto s=test_support::world_fixture();
    const auto town=enter_information_catalog(s,1);page_tick(s);
    const auto before=startup_world_state_digest(s);
    const auto view=inspect_startup_world_information_page(s,town);
    check(view && view->raw==34 && view->town && view->town->rank==s.rank &&
          view->village_name==s.scripts.village_name && !view->facilities &&
          startup_world_state_digest(s)==before,"34 reads existing statistics/village name without hidden state writes");
    StartupInformationInput confirm;confirm.confirm=true;
    StartupInformationInput cancel;cancel.cancel=true;
    for(int fault=0;fault<7;++fault) {
        auto invalid=s;StartupInformationInput input;
        if(fault==0)input.up=true;
        if(fault==1)input.down=true;
        if(fault==2)input.left=true;
        if(fault==3)input.right=true;
        if(fault==4)input.select_row=0;
        if(fault==5){input=confirm;invalid.page_phases.at(town)=1;}
        if(fault==6){input=confirm;invalid.page_counters.erase(town);}
        const auto digest=startup_world_state_digest(invalid);
        check(input_startup_world_information_page(invalid,town,input)!=E::none &&
              startup_world_state_digest(invalid)==digest,"34 unsupported directions/selection/bad payload reject atomically");
    }
    auto blocked=s;blocked.scripts.page_mutations_locked=true;
    const auto blocked_before=startup_world_state_digest(blocked);
    check(input_startup_world_information_page(blocked,town,confirm)!=E::none &&
          startup_world_state_digest(blocked)==blocked_before,"34 failed39 insertion preserves parent lifecycle and outputs");
    auto returned=s;
    check(input_startup_world_information_page(returned,town,cancel)==E::none &&
          returned.scripts.pages.back().lifecycle==4,"34 cancel retires only itself");
    const auto units=s.scene.calendar.units;
    const auto draws=s.scene.random.draws();
    page_tick(s);
    check(s.scene.calendar.units==units && s.scene.random.draws()==draws,"34 modal frame does not run world");
    StartupInformationInput both=confirm;both.cancel=true;
    check(input_startup_world_information_page(s,town,both)==E::none && s.scripts.pages.back().legacy_page==39,
          "34 confirm wins simultaneous cancel and creates39 with real34 parent");
    const auto id=s.scripts.pages.back().id;
    auto wrong_parent=s;
    for(auto &page:wrong_parent.scripts.pages)if(page.id==town)page.legacy_page=36;
    check(!prepare_startup_world_runtime(wrong_parent).candidate,"39 cannot initialize beneath a different parent kind");
    const auto pending=s;
    page_tick(s);
    const auto &data=s.information_page_data.at(id);
    check(data.lists.empty() && !data.facilities.empty(),"39 owns stable instance IDs, never definition int lists");
    std::vector<std::uint64_t> expected;
    for(const auto facility:s.scene.world.facility_order)
        if(s.scene.world.world.facilities.at(facility).kind==3)expected.push_back(facility);
    check(data.facilities==expected,"39 filters original live order to kind3 without sorting or counting historical ledgers");
    const auto selected=expected.front();
    // 原月3：利润只计0..3；本段是账目/编号显示条件，不宣称已自然收入。
    s.facility_ordinals.at(selected)=7;
    s.facility_monthly_cash.at(selected)={};
    s.facility_monthly_cash.at(selected)[0]={100,7};
    s.facility_monthly_cash.at(selected)[3]={20,40};
    s.facility_monthly_cash.at(selected)[4]={999,0};
    const auto projection=inspect_startup_world_information_page(s,id);
    const auto definition=s.scene.world.world.facilities.at(selected).placement.definition_id;
    const auto source=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
                                  [=](const auto &d){return d.id==definition;});
    check(projection && projection->facilities && projection->facilities->front().instance==selected &&
          projection->facilities->front().ordinal==7 && projection->facilities->front().name==source->name &&
          projection->facilities->front().icon==source->legacy_icon && projection->facilities->front().profit==73,
          "39 profit ends at current month; displayed ordinal is original7 plus1, not directory countQ/B");
    s.facility_monthly_cash.at(selected)[3]={0,140};
    check(inspect_startup_world_information_page(s,id)->facilities->front().profit==-47,
          "39 preserves negative per-instance profit, not world funds or all12 buckets");
    s.facility_monthly_cash.at(selected)={};
    s.facility_monthly_cash.at(selected)[0]={std::numeric_limits<int>::max(),0};
    s.facility_monthly_cash.at(selected)[3]={1,0};
    check(inspect_startup_world_information_page(s,id)->facilities->front().profit==std::numeric_limits<int>::min(),
          "39 original int32 accumulation wraps explicitly, without C++ signed overflow");
    for(int fault=0;fault<10;++fault) {
        auto invalid=s;
        if(fault==0)invalid.information_page_data.erase(id);
        if(fault==1)invalid.information_page_data.at(id).lists={{1}};
        if(fault==2)invalid.information_page_data.at(id).facilities.front()=std::uint64_t{1}<<40;
        if(fault==3)invalid.information_page_data.at(id).selection=static_cast<int>(expected.size());
        if(fault==4)invalid.facility_monthly_cash.erase(selected);
        if(fault==5)invalid.facility_ordinals.erase(selected);
        if(fault==6)invalid.scene.world.world.facilities.erase(selected);
        if(fault==7)invalid.scene.world.world.facilities.at(selected).kind=9;
        if(fault==8)invalid.scene.world.facility_order.push_back(selected);
        if(fault==9)invalid.page_phases.at(id)=1;
        const auto digest=startup_world_state_digest(invalid);
        check(input_startup_world_information_page(invalid,id,confirm)!=E::none &&
              !inspect_startup_world_information_page(invalid,id) && startup_world_state_digest(invalid)==digest,
              "39 stale/typed/bad ledger/source/selection rejects without partial scene switch");
    }
    auto closed=s;
    check(input_startup_world_information_page(closed,id,cancel)==E::none,"39 actual return accepts");page_tick(closed);
    check(closed.scripts.pages.back().id==town && !closed.information_page_data.count(id) &&
          !closed.page_counters.count(id) && !closed.page_phases.count(id),"39 return restores real34 and retires all directory references");
    auto scroll=pending;
    // 只供目录/输入的六实例条件：不声称新建或推进该副本的经营/镜头。
    for(int count=static_cast<int>(expected.size()),ordinal=20;count<6;++count,++ordinal) {
        const auto extra=scroll.next_facility_identity++;
        auto instance=scroll.scene.world.world.facilities.at(selected);instance.placement.instance_id={extra};
        scroll.scene.world.world.facilities.emplace(extra,instance);scroll.scene.world.facility_order.push_back(extra);
        scroll.facility_ordinals.emplace(extra,ordinal);scroll.facility_monthly_cash[extra]={};
    }
    page_tick(scroll);
    StartupInformationInput up;up.up=true;
    check(input_startup_world_information_page(scroll,id,up)==E::none &&
          scroll.information_page_data.at(id).selection==5 && scroll.information_page_data.at(id).first_visible==1,
          "39 six-row condition wraps up and scrolls original five-row window");
    StartupInformationInput directions;directions.up=true;directions.down=true;
    check(input_startup_world_information_page(scroll,id,directions)==E::none &&
          scroll.information_page_data.at(id).selection==5,"39 independent up/down cancel, unlike menu9 priority");
    auto denied=s;denied.scripts.page_mutations_locked=true;
    const auto denied_before=startup_world_state_digest(denied);
    check(input_startup_world_information_page(denied,id,confirm)!=E::none &&
          startup_world_state_digest(denied)==denied_before,"39 late scene activation failure rolls back stack and camera");
    StartupInformationInput focus=both;focus.down=true;
    const auto destination=expected[expected.size()>1?1:0];
    check(input_startup_world_information_page(s,id,focus)==E::none && s.scene.scene_state==7 &&
          s.scripts.selection_mode==0 && s.scripts.selected_facility==destination &&
          s.scripts.pages.back().kind==ref::WorldScriptPageKind::scene && s.scripts.pages.back().lifecycle==2,
          "39 moves selection before confirm, confirm beats cancel and activates scene7 rather than a shop page");
    for(const auto old:{town,id})check(std::any_of(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p) {
        return p.id==old&&p.lifecycle==4;
    }),"39 focus retires both actual34/39 instead of retaining hidden modal parent");
    // 空目录保留地图实体且只在私有规则变体改kind镜像；不删除真实世界关联数据。
    auto empty=test_support::world_fixture();StartupWorldRules rules=*empty.rules;empty.rules=&rules;
    for(auto &d:rules.facilities)if(d.kind==3)d.kind=9;
    for(auto &entry:empty.scene.world.world.facilities)if(entry.second.kind==3)entry.second.kind=9;
    const auto empty_town=enter_information_catalog(empty,1);page_tick(empty);
    check(input_startup_world_information_page(empty,empty_town,confirm)==E::none,"empty type3 condition enters real39");
    const auto empty_id=empty.scripts.pages.back().id;
    auto failed_empty=empty;failed_empty.scripts.page_mutations_locked=true;
    const auto failed_before=startup_world_state_digest(failed_empty);
    check(!prepare_startup_world_runtime(failed_empty).candidate && startup_world_state_digest(failed_empty)==failed_before,
          "39 empty17 insertion failure does not commit directory or closure");
    page_tick(empty);
    check(empty.scripts.event_calls.at(17)==1 && std::any_of(empty.scripts.pages.begin(),empty.scripts.pages.end(),[=](const auto &p) {
        return p.id==empty_id&&p.lifecycle==4;
    }),"39 empty directory issues actual17 and closes, never imports kind9 facilities");
}
void owned_item_information_pages() {
    using E=StartupWorldRuntimeError;
    auto s=test_support::world_fixture();
    // 六条正库存为目录/滚动条件夹具；不声称经自然购入，不修改静态原定义顺序。
    for(auto &item:s.items) {
        item.second.inventory=item.first<6?1:0;
        item.second.newly_unlocked=true;
        s.catalog.at({0,item.first}).inventory=item.second.inventory;
        s.catalog.at({0,item.first}).newly_unlocked=true;
    }
    s.items.at(0).status=0;s.catalog.at({0,0}).status=0;
    s.catalog.at({1,0}).newly_unlocked=true;
    const auto id=enter_information_catalog(s,3);
    auto missing_source=s;missing_source.catalog.erase({0,35});
    const auto missing_digest=startup_world_state_digest(missing_source);
    check(!prepare_startup_world_runtime(missing_source).candidate &&
              startup_world_state_digest(missing_source)==missing_digest &&
              !missing_source.information_page_data.count(id),
          "37 initialization source failure leaves no frozen list, page payload or output commit");
    auto paused=s;paused.scene.framework_paused=true;page_tick(paused);
    check(!paused.information_page_data.count(id) && paused.scripts.pages.back().lifecycle==0,
          "paused37 does not install a directory ahead of framework init");
    page_tick(s);
    auto view=inspect_startup_world_information_page(s,id);
    check(view && view->items && view->items->size()==6 && view->selection==0 && view->first_visible==0 &&
              s.information_page_data.at(id).lists==std::vector<std::vector<int>>{{0,1,2,3,4,5}},
          "37 init freezes positive stock in original order includingstatus0, starts five-row viewport at0");
    StartupInformationInput up;up.up=true;
    check(input_startup_world_information_page(s,id,up)==E::none,"37 up wraps to last owned item");
    view=inspect_startup_world_information_page(s,id);
    check(view && view->selection==5 && view->first_visible==1,"37 sixth row scrolls five-row viewport to1");
    StartupInformationInput both;both.up=true;both.down=true;
    check(input_startup_world_information_page(s,id,both)==E::none &&
              s.information_page_data.at(id).selection==5 && s.information_page_data.at(id).first_visible==1,
          "37 up/down are independent and both approved directions return to same selected row");
    StartupInformationInput down;down.down=true;
    check(input_startup_world_information_page(s,id,down)==E::none &&
              s.information_page_data.at(id).selection==0 && s.information_page_data.at(id).first_visible==0,
          "37 down wraps last row back to first and restores viewport0");
    const auto updates=s.scene.world.updates;
    const auto random=s.scene.random.draws();const auto date=s.scene.calendar.units;
    const auto counter=s.page_counters.at(id);page_tick(s);
    check(s.page_counters.at(id)==counter+1 && s.scene.world.updates==updates &&
              s.scene.random.draws()==random && s.scene.calendar.units==date,
          "37 modal callback advances page count without world, random or calendar");
    StartupInformationInput close;close.confirm=true;
    const auto wrong_id_state=startup_world_state_digest(s);
    check(input_startup_world_information_page(s,id+1000,close)!=E::none &&
              startup_world_state_digest(s)==wrong_id_state,"37 stale or unknown page identity rejects without effects");
    for(int fault=0;fault<9;++fault) {
        auto bad=s;
        if(fault==0)bad.information_page_data.erase(id);
        if(fault==1)bad.information_page_data.at(id).lists[0][0]=999;
        if(fault==2)bad.information_page_data.at(id).selection=6;
        if(fault==3)bad.information_page_data.at(id).first_visible=6;
        if(fault==4){bad.page_phases.erase(id);bad.page_counters.erase(id);}
        if(fault==5)bad.scripts.pages.back().lifecycle=3;
        if(fault==6)bad.scene.framework_paused=true;
        if(fault==7)bad.information_page_data.at(id).lists.push_back({});
        if(fault==8)bad.catalog.erase({0,35}); // 零库存仍属正常关闭应清NEW的整类，不能只验可见六项。
        const auto before=startup_world_state_digest(bad);
        check(input_startup_world_information_page(bad,id,close)!=E::none &&
                  startup_world_state_digest(bad)==before,
              "37 bad directory/scroll/lifecycle or late zero-stock catalog miss rejects entire Owner with no outputs");
        if(fault==4)check(!prepare_startup_world_runtime(bad).candidate,
                         "initialized37 with both common maps missing cannot reinitialize over frozen list");
    }
    const auto before_items=s.items;const auto cash=s.scene.world.world.ai.accounting.funds();
    auto late_close=s;
    // close_page只按目标ID退休；真正的晚期拒绝来自脚本回写时检查职业同步键。
    // 该目录不参与37载荷验证，故拒绝发生在候选已清NEW之后。
    late_close.scripts.professions.emplace(
        static_cast<int>(late_close.scene.world.world.ai.professions.size()), ref::WorldScriptUnlockDefinition{});
    const auto late_digest=startup_world_state_digest(late_close);
    check(input_startup_world_information_page(late_close,id,close)==E::script_failed &&
              startup_world_state_digest(late_close)==late_digest,
          "37 late page-close failure rolls back candidate whole-class NEW clearing and all outputs");
    for(bool cancel:{false,true}) {
        auto closed=s;StartupInformationInput input;input.cancel=cancel;input.confirm=!cancel;
        check(input_startup_world_information_page(closed,id,input)==E::none &&
                  closed.scripts.pages.back().lifecycle==4,"37 confirm and return close without using the selected item");
        for(const auto &item:closed.items)
            check(!item.second.newly_unlocked && !closed.catalog.at({0,item.first}).newly_unlocked &&
                      item.second.inventory==before_items.at(item.first).inventory &&
                      item.second.status==before_items.at(item.first).status,
                  "37 normal close clears whole ordinary-item NEW including zero stock, preservesinventory/status");
        check(closed.catalog.at({1,0}).newly_unlocked && closed.scene.random.draws()==random &&
                  closed.scene.world.world.ai.accounting.funds()==cash,"37 close does not clear equipment NEW or charge/draw");
        page_tick(closed);
        check(!closed.information_page_data.count(id) && !closed.page_phases.count(id) && !closed.page_counters.count(id),
              "37 retirement releases frozen list, selection, scroll and common page maps");
    }
    auto empty=test_support::world_fixture();
    for(auto &item:empty.items) {
        item.second.inventory=0;item.second.newly_unlocked=true;
        empty.catalog.at({0,item.first}).inventory=0;empty.catalog.at({0,item.first}).newly_unlocked=true;
    }
    const auto empty_id=enter_information_catalog(empty,3);
    const auto calls=empty.scripts.event_calls.count(15)?empty.scripts.event_calls.at(15):0;
    page_tick(empty);
    check(empty.scripts.event_calls.count(15) && empty.scripts.event_calls.at(15)==calls+1 &&
              !inspect_startup_world_information_page(empty,empty_id),
          "empty37 invokes real event15 and retires instead of fabricating an empty selectable list");
    for(const auto &item:empty.items)
        check(item.second.newly_unlocked && empty.catalog.at({0,item.first}).newly_unlocked,
              "empty37 initialization exit does not run normal-close NEW clearing");
    page_tick(empty);
    check(empty.scripts.event_calls.at(15)==calls+1 && !empty.information_page_data.count(empty_id) &&
              !empty.page_phases.count(empty_id) && !empty.page_counters.count(empty_id),
          "empty37 retirement releases its payload without requesting event15 twice");
}
void equipment_information_pages() {
    using E=StartupWorldRuntimeError;
    auto s=test_support::world_fixture();
    for(auto &entry:s.catalog)if(entry.first.first>0)entry.second.newly_unlocked=true;
    const auto id=enter_information_catalog(s,4);page_tick(s);
    auto view=inspect_startup_world_information_page(s,id);
    check(view && view->equipment && view->selection_or_period==0 && view->selection==0 && view->first_visible==0,
          "38 real init opens Steam weapon tab without borrowed human selection");
    const auto &lists=s.information_page_data.at(id).lists;
    check(lists.size()==4 && lists[0].size()==33 && lists[1].size()==16 && lists[2].size()==33 && lists[3].size()==27,
          "38 freezes four actual Steam UI catalogues33/16/33/27");
    StartupInformationInput select;select.select_row=5;
    check(input_startup_world_information_page(s,id,select)==E::none &&
              s.information_page_data.at(id).selection==5 && s.information_page_data.at(id).first_visible==2,
          "38 fifth zero-based row scrolls the original four-row window to2");
    StartupInformationInput both;both.left=true;both.right=true;
    check(input_startup_world_information_page(s,id,both)==E::none && s.page_phases.at(id)==0 &&
              s.information_page_data.at(id).selection==5 && s.information_page_data.at(id).first_visible==2,
          "38 both tab directions return to original tab without clearing selection or scroll");
    StartupInformationInput next;next.right=true;next.down=true;
    check(input_startup_world_information_page(s,id,next)==E::none && s.page_phases.at(id)==1 &&
              s.information_page_data.at(id).selection==1 && s.information_page_data.at(id).first_visible==0,
          "38 single tab change resets first, then same-input down selects row1");
    StartupInformationInput previous;previous.left=true;previous.up=true;
    check(input_startup_world_information_page(s,id,previous)==E::none && s.page_phases.at(id)==0 &&
              s.information_page_data.at(id).selection==32 && s.information_page_data.at(id).first_visible==29,
          "38 tab reset occurs before same-input up wraps weapon list to32");
    StartupInformationInput close;close.confirm=true;
    for(int fault=0;fault<6;++fault) {
        auto bad=s;
        if(fault==0)bad.information_page_data.erase(id);
        if(fault==1)bad.information_page_data.at(id).lists[3][0]=999;
        if(fault==2)bad.information_page_data.at(id).first_visible=33;
        if(fault==3)bad.page_phases.at(id)=4;
        if(fault==4)bad.information_page_data.at(id).lists.pop_back();
        if(fault==5){bad.page_phases.erase(id);bad.page_counters.erase(id);}
        const auto before=startup_world_state_digest(bad);
        check(input_startup_world_information_page(bad,id,close)!=E::none && startup_world_state_digest(bad)==before,
              "38 validates all four frozen lists, scroll and common maps before any close side effect");
    }
    const auto updates=s.scene.world.updates;
    const auto draws=s.scene.random.draws();
    const auto count=s.page_counters.at(id);page_tick(s);
    check(s.page_counters.at(id)==count+1 && s.scene.world.updates==updates && s.scene.random.draws()==draws,
          "38 is independently registered as modal while its own count advances");
    auto paused=s;paused.scene.framework_paused=true;
    const auto paused_count=paused.page_counters.at(id);page_tick(paused);
    const auto paused_digest=startup_world_state_digest(paused);
    check(paused.page_counters.at(id)==paused_count &&
              input_startup_world_information_page(paused,id,close)!=E::none &&
              startup_world_state_digest(paused)==paused_digest,"38 framework pause freezes counter and refuses close input");
    const auto old_catalog=s.catalog;
    const auto old_humans=s.shop_humans;
    check(acknowledge_startup_world_runtime_page(s,id)==E::none && s.scripts.pages.back().lifecycle==4,
          "38 runtime confirmation closes the catalogue");
    for(const auto &entry:old_catalog)if(entry.first.first>0)
        check(s.catalog.at(entry.first).newly_unlocked==entry.second.newly_unlocked &&
                  s.catalog.at(entry.first).inventory==entry.second.inventory &&
                  s.catalog.at(entry.first).free_purchases==entry.second.free_purchases,
              "38 closing preserves equipment NEW, stock and free grants");
    for(const auto &human:old_humans)
        check(s.shop_humans.at(human.first).equipment==human.second.equipment,"38 confirmation never equips any person");
    page_tick(s);
    check(!s.information_page_data.count(id),"38 retirement releases all four retained definition lists");
    // 私有规则空饰品类只验证空目录输入，不改变原表、不把无来源空类称自然状态。
    auto empty=test_support::world_fixture();StartupWorldRules rules=*empty.rules;
    rules.equipment.erase(std::remove_if(rules.equipment.begin(),rules.equipment.end(),[](const auto &d){
        return d.shop.kind==3;}),rules.equipment.end());empty.rules=&rules;
    const auto empty_id=enter_information_catalog(empty,4);page_tick(empty);
    StartupInformationInput right;right.right=true;
    for(int n=0;n<3;++n)check(input_startup_world_information_page(empty,empty_id,right)==E::none,"38 reaches empty accessory tab");
    both={};both.up=true;both.down=true;
    check(input_startup_world_information_page(empty,empty_id,both)==E::none &&
              empty.information_page_data.at(empty_id).selection==0 &&
              empty.information_page_data.at(empty_id).first_visible==0,
          "38 empty class skips modulo and retains legal zero selection/scroll");
    for(bool cancel:{false,true}) {
        auto closed=empty;StartupInformationInput input;input.cancel=cancel;input.confirm=!cancel;
        check(input_startup_world_information_page(closed,empty_id,input)==E::none &&
                  closed.scripts.pages.back().lifecycle==4,"38 empty class still supports confirm and return");
    }
}
void adventurer_information_pages() {
    using E=StartupWorldRuntimeError;
    auto s=test_support::world_fixture();
    // 定义状态/业绩是最小目录条件；不创建W或伪称六人已自然到访。
    for(auto &entry:s.human_presence)entry.second=entry.first>=1&&entry.first<=6?(entry.first==6?2:1):0;
    for(auto &entry:s.scripts.humans)entry.second.pending_notice=true;
    for(auto &entry:s.human_calendar)entry.second.contribution=99;
    s.human_calendar.at(0).contribution=77;
    s.scene.world.world.ai.battle.humans.at(6).killed_stat1=60;
    const int medals=s.medal_count;
    const auto awards=s.award_rankings;
    const auto id=enter_information_catalog(s,0);
    auto broken=s;broken.human_calendar.erase(6);
    const auto broken_before=startup_world_state_digest(broken);
    check(!prepare_startup_world_runtime(broken).candidate && startup_world_state_digest(broken)==broken_before &&
          !broken.information_page_data.count(id),"35 missing contribution source rejects entire Init candidate");
    auto paused=s;paused.scene.framework_paused=true;page_tick(paused);
    check(!paused.information_page_data.count(id),"35 paused entry cannot calculate contribution or freeze a list");
    page_tick(s);
    check(s.information_page_data.at(id).lists==std::vector<std::vector<int>>{{1,2,3,4,5,6}} &&
          s.page_phases.at(id)==0 && s.information_page_data.at(id).selection==0,
          "35 includes p2 and preserves source order, not live W count or contribution ranking");
    check(s.human_calendar.at(6).contribution>s.human_calendar.at(1).contribution &&
          s.human_calendar.at(1).contribution!=99 && s.human_calendar.at(0).contribution==77 &&
          s.medal_count==medals && s.award_rankings==awards,
          "35 commits shared contribution refresh without awarding medals or changing absent definitions");
    check(std::all_of(s.scripts.humans.begin(),s.scripts.humans.end(),[](const auto &v){return v.second.pending_notice;}),
          "35 Init recalculates contribution without clearing any person NEW");
    StartupInformationInput up;up.up=true;
    check(input_startup_world_information_page(s,id,up)==E::none &&
          s.information_page_data.at(id).selection==5 && s.information_page_data.at(id).first_visible==1,
          "35 up wraps original six-person list and scrolls its five-row window");
    StartupInformationInput page;page.right=true;
    check(input_startup_world_information_page(s,id,page)==E::none && s.page_phases.at(id)==1 &&
          s.information_page_data.at(id).selection==5 && s.information_page_data.at(id).first_visible==1,
          "35 changing tab preserves person selection and scroll unlike equipment38");
    StartupInformationInput both;both.up=true;both.down=true;both.left=true;both.right=true;
    check(input_startup_world_information_page(s,id,both)==E::none && s.page_phases.at(id)==1 &&
          s.information_page_data.at(id).selection==5 && s.information_page_data.at(id).first_visible==1,
          "35 independent up/down then left/right cancel their own movement without reset");
    const auto before=startup_world_state_digest(s);
    const auto view=inspect_startup_world_information_page(s,id);
    check(view && view->humans && view->humans->size()==6 &&
          view->humans->back().details.definition==6 && view->humans->back().presence==2 &&
          view->humans->back().yearly_town_points==60 && view->humans->back().yearly_spending==0 &&
          view->humans->back().newly_unlocked && !view->humans->back().details.live_actor &&
          startup_world_state_digest(s)==before,
          "35 view cannot rerun contribution, clear NEW or advance its clock");
    const auto cash=s.scene.world.world.ai.accounting.funds();
    const auto draws=s.scene.random.draws();
    const auto date=s.scene.calendar.units;
    page_tick(s);
    check(s.scene.random.draws()==draws && s.scene.calendar.units==date &&
          s.scene.world.world.ai.accounting.funds()==cash,"35 modal update does not run world operations");
    StartupInformationInput confirm;confirm.confirm=true;
    for(int fault=0;fault<7;++fault) {
        auto invalid=s;
        if(fault==0)invalid.information_page_data.erase(id);
        if(fault==1)invalid.information_page_data.at(id).lists[0][0]=6;
        if(fault==2)invalid.page_phases.at(id)=4;
        if(fault==3)invalid.information_page_data.at(id).selection=6;
        if(fault==4)invalid.information_page_data.at(id).first_visible=2;
        if(fault==5)invalid.scripts.humans.erase(6);
        if(fault==6)invalid.information_page_data.at(id).facilities.push_back(1);
        const auto digest=startup_world_state_digest(invalid);
        check(input_startup_world_information_page(invalid,id,confirm)!=E::none &&
              startup_world_state_digest(invalid)==digest,"35 bad list/tab/selection/NEW source rejects whole action");
    }
    auto blocked=s;blocked.scripts.page_mutations_locked=true;
    const auto blocked_before=startup_world_state_digest(blocked);
    check(input_startup_world_information_page(blocked,id,confirm)!=E::none &&
          startup_world_state_digest(blocked)==blocked_before,"35 late60 insertion failure rolls back complete NEW clearing");
    auto cancelled=s;StartupInformationInput cancel;cancel.cancel=true;
    check(input_startup_world_information_page(cancelled,id,cancel)==E::none &&
          cancelled.scripts.pages.back().lifecycle==4,"35 actual return closes only itself");
    for(const auto &entry:cancelled.scripts.humans)
        check(entry.second.pending_notice==(entry.first<1||entry.first>6),
              "35 return clears all directory NEW including offscreen p2, preserving excluded p0 NEW");
    page_tick(cancelled);
    check(!cancelled.information_page_data.count(id) && !cancelled.page_phases.count(id) &&
          !cancelled.page_counters.count(id),"35 next framework retirement releases every directory payload");
    check(input_startup_world_information_page(s,id,confirm)==E::none && s.scripts.pages.back().legacy_page==60 &&
          s.page_human_bindings.at(s.scripts.pages.back().id)==6,
          "35 confirm opens60 bound to selected definition6 despite absence of a live actor");
    for(const auto &entry:s.scripts.humans)
        check(entry.second.pending_notice==(entry.first<1||entry.first>6),
              "35 confirm shares directory-wide NEW clear with return");
    const auto parent=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    check(parent!=s.scripts.pages.end() && parent->lifecycle==3 &&
          s.information_page_data.at(id).selection==5 && s.page_phases.at(id)==1,
          "60 covers actual35 retaining tab, scroll and frozen selected definition");
    const auto detail=s.scripts.pages.back().id;
    check(s.human_detail_contexts.at(detail).chase_mode==1 && !s.human_detail_contexts.at(detail).actor,
          "35-to60 preserves explicit chase source1 without fabricating a W binding");
    page_tick(s);
    // 首次60真实压入111教程；按真实确认恢复父页，不伪造已读标志。
    for(int n=0;n<16;++n) {
        const auto top=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
                                   [](const auto &p){return p.lifecycle!=4;});
        check(top!=s.scripts.pages.rend(),"first60 tutorial retains an active page");
        if(top->id==detail && top->lifecycle==2)break;
        if(top->id!=detail) {
            check(top->kind==ref::WorldScriptPageKind::dialogue && top->source_record==88,
                  "only actual111 character-information tutorial covers first60");
            check(acknowledge_startup_world_runtime_page(s,top->id)==E::none,
                  "first60 tutorial uses actual confirmation");
        }
        page_tick(s);
    }
    check(s.scripts.pages.back().id==detail && s.scripts.pages.back().lifecycle==2,
          "first60 tutorial returns to active detail within bounded framework updates");
    const auto presentation=inspect_startup_world_human_presentation(s,detail);
    check(presentation && presentation->tracking_available && !presentation->live,
          "source1 view exposes chase eligibility even when no live W exists for this definition");
    auto returned=s;
    check(act_startup_world_human_page(returned,detail,StartupHumanPageAction::cancel)==E::none,
          "source1 detail can return to actual35");
    page_tick(returned);
    check(returned.scripts.pages.back().id==id && returned.information_page_data.at(id).selection==5 &&
          returned.information_page_data.at(id).first_visible==1 && returned.page_phases.at(id)==1 &&
          !returned.human_detail_contexts.count(detail),"60 retirement restores35 selection/tab and releases context");
    const auto calls=s.scripts.event_calls.count(137)?s.scripts.event_calls.at(137):0;
    check(act_startup_world_human_page(s,detail,StartupHumanPageAction::track)==E::none &&
          s.scripts.event_calls.at(137)==calls+1 && s.human_detail_contexts.count(detail) &&
          !s.scripts.selected_actor && s.scene.scene_state==0,
          "source1 without live W runs real137 and retains detail instead of inventing actor/camera");
    const auto after_prompt=startup_world_state_digest(s);
    check(act_startup_world_human_page(s,detail,StartupHumanPageAction::track)!=E::none &&
          startup_world_state_digest(s)==after_prompt,"covered60 cannot issue137 again before prompt is consumed");
    auto direct=human_fixture(60);const auto direct_id=direct.scripts.pages.back().id;
    auto paused_detail=direct;paused_detail.scene.framework_paused=true;page_tick(paused_detail);
    check(paused_detail.scripts.pages.back().lifecycle==0 &&
          !paused_detail.human_pages_initialized.count(direct_id),"paused new60 retains true uninitialized lifecycle");
    paused_detail.scene.framework_paused=false;page_tick(paused_detail);
    check(startup_world_human_page_ready(paused_detail,direct_id),"unpaused60 initializes once through actual framework");
    page_tick(direct);
    const auto direct_before=startup_world_state_digest(direct);
    const auto direct_view=inspect_startup_world_human_presentation(direct,direct_id);
    check(direct_view && !direct_view->tracking_available,"source0 detail view does not advertise chase input");
    check(act_startup_world_human_page(direct,direct_id,StartupHumanPageAction::track)!=E::none &&
          startup_world_state_digest(direct)==direct_before,"source0 management detail refuses source1-only chase atomically");
    for(int fault=0;fault<6;++fault) {
        auto invalid=direct;
        if(fault==0)invalid.human_detail_contexts.erase(direct_id);
        if(fault==1)invalid.page_phases.at(direct_id)=4;
        if(fault==2)invalid.human_page_selections.at(direct_id)=-1;
        if(fault==3)invalid.page_counters.at(direct_id)=std::numeric_limits<int>::max();
        if(fault==4)invalid.human_detail_contexts.at(direct_id).chase_mode=1; // scene父没有W，不能伪造35来源。
        if(fault==5)invalid.human_pages_initialized.erase(direct_id);
        check(!startup_world_human_page_ready(invalid,direct_id) &&
              !inspect_startup_world_human_presentation(invalid,direct_id) &&
              !prepare_startup_world_runtime(invalid).candidate,
              "initialized60 missing context or invalid phase/selection/counter cannot recover by reinitializing");
    }
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
              test_support::audio_ids(tick.candidate->sound_requests) == std::vector<int>{4} &&
              tick.candidate->sound_requests.front().operation == StartupAudioOperation::jingle,
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
    check(tick.candidate && test_support::audio_ids(tick.candidate->sound_requests) == std::vector<int>{5} &&
              tick.candidate->sound_requests.front().operation == StartupAudioOperation::jingle,
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
    check(s.page_counters.at(id) == 1 && test_support::audio_ids(s.sound_requests) == std::vector<int>{5} &&
              s.sound_requests.front().operation == StartupAudioOperation::jingle &&
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
              test_support::audio_ids(tick.candidate->sound_requests) == std::vector<int>{3} &&
              tick.candidate->sound_requests.front().operation == StartupAudioOperation::replace_bgm,
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
              celebration.sound_requests.back().id == 1 &&
              celebration.sound_requests.back().operation == StartupAudioOperation::replace_bgm &&
              celebration.scene.world.updates == 0 &&
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
              test_support::audio_ids(tick.candidate->sound_requests) == std::vector<int>{3} &&
              tick.candidate->sound_requests.front().operation == StartupAudioOperation::replace_bgm &&
              tick.candidate->award_rankings.count(id) && tick.candidate->award_announced.at(id),
          "raw87 actual page initialization increments j once and first update sounds3 only");
    s = *tick.candidate;
    const auto repeated = prepare_startup_world_runtime(s);
    check(repeated.candidate && repeated.candidate->medal_count == 2 &&
              test_support::audio_ids(repeated.candidate->sound_requests) == std::vector<int>{3} &&
              repeated.candidate->sound_requests.front().operation == StartupAudioOperation::replace_bgm,
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
              s.sound_requests.back().id == 1 &&
              s.sound_requests.back().operation == StartupAudioOperation::replace_bgm &&
              s.scene.world.world.ai.accounting.funds() == cash &&
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
                test_support::audio_ids(tick.candidate->sound_requests) ==
                    (command == 2 ? std::vector<int>{} : std::vector<int>{command == 0 ? 4 : 6}) &&
                tick.candidate->scene.world.updates == 0 &&
                tick.candidate->scene.random.draws() == 0,
            "raw11 source sound only on first page update, no world/random tick");
        check(command == 2 || tick.candidate->sound_requests.front().operation == StartupAudioOperation::jingle,
              "raw11来源D4/D6保留jingle资格，不是按ID猜普通播放");
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
        check(test_support::audio_ids(s.sound_requests) == std::vector<int>{5} &&
                  s.sound_requests.front().operation == StartupAudioOperation::jingle,
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
              s.sound_requests.back().id == 5 &&
              s.sound_requests.back().operation == StartupAudioOperation::jingle,
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
    {
        // 高星商会调用点的条件夹具，不模拟自然升到四星；不改64原表及后续金币建设费。
        auto rank3 = fixture(85);
        rank3.rank = 3;
        const auto prior_page = rank3.scripts.pages.back().id;
        check(initialize_startup_world_commerce_pages(rank3) &&
                  std::find(rank3.commerce_page_lists.at(prior_page).begin(),
                            rank3.commerce_page_lists.at(prior_page).end(), 64) ==
                      rank3.commerce_page_lists.at(prior_page).end(),
              "original museum64 is absent from actual rank3 facility commerce catalogue");
        auto museum = fixture(85);
        museum.rank = 4;
        museum.village_points = 199;
        const auto page = museum.scripts.pages.back().id;
        check(initialize_startup_world_commerce_pages(museum), "rank4 actual85 initializes");
        const auto &entries = museum.commerce_page_lists.at(page);
        const auto row = std::find(entries.begin(), entries.end(), 64);
        check(row != entries.end() && museum.rules->facility_initial.at(64).capacity == 200 &&
                  museum.facility_presence.at(64) == 0 && museum.facility_free_builds.at(64) == 0 &&
                  act_startup_world_commerce_page(museum, page, A::select,
                      static_cast<int>(row - entries.begin())) == E::none,
              "rank4 catalogue selects original museum64 with200-point entitlement price");
        const auto cash = museum.scene.world.world.ai.accounting.funds();
        const auto instances = museum.scene.world.world.facilities.size();
        const auto next_instance = museum.next_facility_identity;
        auto insufficient = museum;
        check(act_startup_world_commerce_page(insufficient, page, A::confirm) == E::none &&
                  ref::world_script_seen(insufficient.scripts, 12) && insufficient.village_points == 199 &&
                  insufficient.facility_presence.at(64) == 0 && insufficient.facility_free_builds.at(64) == 0 &&
                  insufficient.scene.world.world.ai.accounting.funds() == cash &&
                  insufficient.next_facility_identity == next_instance &&
                  std::none_of(insufficient.scripts.pages.begin(), insufficient.scripts.pages.end(),
                               [](const auto &p) { return p.lifecycle != 4 && p.legacy_page == 93; }),
              "museum199 points produces actual shortage12 without payment, entitlement or pending93");
        museum.village_points = 200; // 只调整边界夹具，不回滚或伪造原付款动作。
        check(act_startup_world_commerce_page(museum, page, A::confirm) == E::none &&
                  museum.village_points == 0 && museum.facility_presence.at(64) == 0 &&
                  museum.facility_free_builds.at(64) == 0,
              "museum200 points pays at85 while actual museum entitlement is still pending");
        const auto reward = museum.scripts.pages.back();
        check(reward.legacy_page == 93 && reward.legacy_r == 3 && reward.legacy_s == 64 &&
                  initialize_startup_world_commerce_pages(museum),
              "museum purchase binds real93/r3/s64 rather than generic95 or an existing building");
        check(act_startup_world_commerce_page(museum, reward.id, A::confirm) == E::none &&
                  museum.page_counters.at(reward.id) == 40 && museum.facility_presence.at(64) == 0 &&
                  museum.facility_free_builds.at(64) == 0 &&
                  act_startup_world_commerce_page(museum, reward.id, A::confirm) == E::none &&
                  museum.facility_presence.at(64) == 2 && museum.facility_free_builds.at(64) == 1 &&
                  museum.facility_unlock_notices.at(64) && museum.village_points == 0 &&
                  museum.scene.world.world.ai.accounting.funds() == cash &&
                  museum.scene.world.world.facilities.size() == instances &&
                  museum.next_facility_identity == next_instance,
              "museum93 at40 grants p2/H1 only; it neither spends3000G nor constructs an instance");
    }
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

// 以下是原表上的人工条件组合；不等同于自然经营取得壶、元素或配方。
StartupWorldRuntimeState magic_pot_fixture() {
    auto s = test_support::world_fixture(ref::WorldRandomStream::from_raw({1}));
    s.scripts.pages.front().lifecycle = 3;
    s.scripts.executing_page.reset();
    s.scripts.user_flags |= 3U;
    s.scripts.event_calls[101] = 1; // 隔离首次说明，不跳过被测业务页。
    s.legacy_n = {};
    s.legacy_n[11] = 1;
    s.legacy_n[12] = s.scene.calendar.year * 48 + s.scene.calendar.month * 4 +
                     s.scene.calendar.subperiod;
    for (auto &entry : s.magic_pot_recipes)
        entry.second.status = 1;
    return s;
}
ref::WorldScriptPage magic_pot_top(const StartupWorldRuntimeState &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                               [](const auto &v) { return v.lifecycle != 4; });
    check(p != s.scripts.pages.rend(), "magic pot fixture retains a live framework root");
    return *p;
}
std::uint64_t reach_magic_pot_page(StartupWorldRuntimeState &s, int raw) {
    for (int n = 0; n < 200; ++n) {
        const auto before = magic_pot_top(s);
        if (raw == -1 && before.kind == ref::WorldScriptPageKind::scene)
            return before.id;
        page_tick(s); // 栈顶才初始化；下层41保留原生命周期0。
        const auto p = magic_pot_top(s);
        if (p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == raw)
            return p.id;
        if (p.kind != ref::WorldScriptPageKind::scene &&
            !(p.legacy_page >= 41 && p.legacy_page <= 47))
            check(acknowledge_startup_world_runtime_page(s, p.id) == StartupWorldRuntimeError::none,
                  "magic pot script dialogue is consumed through its actual framework action");
    }
    const auto p = magic_pot_top(s);
    const auto counter = s.page_counters.find(p.id);
    throw std::runtime_error("magic pot page sequence exceeded its fixture bound: expected=" +
        std::to_string(raw) + ", top=" + std::to_string(p.legacy_page) + ", id=" +
        std::to_string(p.id) + ", lifecycle=" + std::to_string(p.lifecycle) + ", counter=" +
        std::to_string(counter == s.page_counters.end() ? -1 : counter->second) +
        ", recipe p2/p5/p7=" + std::to_string(s.magic_pot_recipes.at(2).status) + "/" +
        std::to_string(s.magic_pot_recipes.at(5).status) + "/" +
        std::to_string(s.magic_pot_recipes.at(7).status));
}
void magic_pot_entry_and_retirement() {
    using A = StartupMagicPotAction;
    using E = StartupWorldRuntimeError;
    for (const auto entry : {StartupMagicPotEntry::main_menu,
                             StartupMagicPotEntry::development_menu}) {
        auto s = magic_pot_fixture();
        s.legacy_n[1] = 3;
        --s.legacy_n[12]; // 非零日期差、待元素全零：原m重写aQ但不改13槽。
        s.magic_pot_output = {8, 7, 6, 5};
        const auto original = s.legacy_n;
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto date = s.scene.calendar.units;
        check(open_startup_world_magic_pot(s, entry) == E::none && s.legacy_n == original &&
                  s.magic_pot_output == std::array<std::int32_t, 4>{} &&
                  (s.scripts.user_flags & 2U) ==
                      (entry == StartupMagicPotEntry::main_menu ? 0U : 2U) &&
                  s.scene.random.draws() == 0,
              "two original entry points differ in bit2; zero production still clears aQ without draws");
        const auto id = reach_magic_pot_page(s, 41);
        const auto view = inspect_startup_world_magic_pot_page(s, id);
        check(view && view->entries.empty() && view->selection == 0 &&
                  s.magic_pot_recipes.size() == 40 &&
                  s.scene.world.world.ai.accounting.funds() == cash && s.scene.calendar.units == date,
              "41 reads one fixed recipe catalogue and leaves cash/calendar untouched");
        for (int fault = 0; fault < 5; ++fault) {
            auto broken = s;
            if (fault == 0) broken.magic_pot_page_data.erase(id);
            if (fault == 1) broken.magic_pot_page_lists.erase(id);
            if (fault == 2) broken.page_counters.erase(id);
            if (fault == 3) broken.magic_pot_page_data.at(id)[0] = 2;
            if (fault == 4) broken.magic_pot_page_parents[id] = id;
            check(!inspect_startup_world_magic_pot_page(broken, id) &&
                      act_startup_world_magic_pot_page(broken, id, A::confirm) == E::missing_source &&
                      !prepare_startup_world_runtime(broken).candidate && broken.legacy_n == original &&
                      broken.scene.random.draws() == 0,
                  "initialized41 rejects missing fields, invalid choice and forged parent explicitly");
        }
        check(act_startup_world_magic_pot_page(s, id, A::select, 2) == E::invalid_page &&
                  s.magic_pot_page_data.at(id)[0] == 0,
              "41 out-of-range selection cannot leave a partial payload");
        check(act_startup_world_magic_pot_page(s, id, A::cancel) == E::none &&
                  act_startup_world_magic_pot_page(s, id, A::confirm) == E::invalid_page,
              "closed41 refuses stale input before framework retirement");
        page_tick(s);
        check(s.magic_pot_pages_initialized.empty() && s.magic_pot_page_data.empty() &&
                  s.magic_pot_page_lists.empty() && s.magic_pot_page_parents.empty() &&
                  s.magic_pot_recipes.size() == 40 && s.legacy_n == original &&
                  s.magic_pot_output == std::array<std::int32_t, 4>{},
              "actual framework retires four transient payload families and retains global pot state");
    }
    auto same_date = magic_pot_fixture();
    same_date.magic_pot_output = {1, 2, 3, 4};
    check(open_startup_world_magic_pot(same_date, StartupMagicPotEntry::main_menu) == E::none &&
              same_date.magic_pot_output == std::array<std::int32_t, 4>{1, 2, 3, 4},
          "same-date original m early return preserves prior aQ");
    auto late = magic_pot_fixture();
    late.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
    const auto flags = late.scripts.user_flags;
    check(open_startup_world_magic_pot(late, StartupMagicPotEntry::main_menu) == E::script_failed &&
              late.scripts.user_flags == flags && late.scripts.pages.size() == 1,
          "failed41 insertion rolls back main-menu bit2 consumption");
}
void magic_pot_deposit_pages() {
    using A = StartupMagicPotAction;
    using E = StartupWorldRuntimeError;
    auto s = magic_pot_fixture();
    s.scripts.event_calls[105] = 1; // 返回说明已读，仅隔离这项既有脚本。
    check(open_startup_world_magic_pot(s, StartupMagicPotEntry::main_menu) == E::none,
          "real menu opens deposit parent41");
    const auto parent = reach_magic_pot_page(s, 41);
    check(acknowledge_startup_world_runtime_page(s, parent) == E::none,
          "common confirmation routes original41 into42");
    const auto list = reach_magic_pot_page(s, 42);
    auto view = inspect_startup_world_magic_pot_page(s, list);
    check(view && std::find(view->entries.begin(), view->entries.end(), 0) != view->entries.end(),
          "42 exposes actual initial potato inventory");
    const auto choice = static_cast<int>(std::find(view->entries.begin(), view->entries.end(), 0) -
                                        view->entries.begin());
    check(act_startup_world_magic_pot_page(s, list, A::select, choice) == E::none,
          "42 selects actual original item through its input");
    auto wrong_parent = s;
    wrong_parent.magic_pot_page_parents.at(list) = wrong_parent.scripts.pages.front().id;
    check(act_startup_world_magic_pot_page(wrong_parent, list, A::confirm) == E::missing_source &&
              wrong_parent.items.at(0).inventory == 2 && wrong_parent.scene.random.draws() == 0,
          "42 wrong parent cannot consume held inventory or common random");
    auto late = s;
    late.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
    const auto pot = late.legacy_n;
    check(act_startup_world_magic_pot_page(late, list, A::confirm) == E::script_failed &&
              late.items.at(0).inventory == 2 && late.catalog.at({0, 0}).inventory == 2 &&
              late.legacy_n == pot && late.scene.random.draws() == 0 && late.magic_pot_comment.empty(),
          "late44 insertion failure rolls back inventory, elements, comment and drawn ticket");
    const auto stock = s.shop_item_stock.at(0).quantity;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    check(act_startup_world_magic_pot_page(s, list, A::confirm) == E::none &&
              s.items.at(0).inventory == 1 && s.catalog.at({0, 0}).inventory == 1 &&
              s.shop_item_stock.at(0).quantity == stock && s.legacy_n[1] == 1 &&
              s.legacy_n[4] == 2 && s.legacy_n[8] == 1 &&
              s.scene.random.draws() == 1 && s.magic_pot_comment == "就那样吧" &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "42 immediately consumes held potato once, not shop A; ice/pending/comment share one ticket");
    const auto result = reach_magic_pot_page(s, 44);
    check(act_startup_world_magic_pot_page(s, list, A::confirm) == E::invalid_page,
          "covered42 rejects stale input while44 owns the top");
    s.page_counters.at(result) = 35;
    check(act_startup_world_magic_pot_page(s, result, A::confirm) == E::none &&
              s.page_counters.at(result) == 35, "44 before36 cannot fast-forward");
    s.page_counters.at(result) = 36;
    check(act_startup_world_magic_pot_page(s, result, A::confirm) == E::none &&
              s.page_counters.at(result) == 85 &&
              act_startup_world_magic_pot_page(s, result, A::confirm) == E::none &&
              s.page_counters.at(result) == 91,
          "44 original confirmation gates advance36 to85 then91 without another deposit");
    check(cancel_startup_world_runtime_page(s, result) == E::none && s.items.at(0).inventory == 1 &&
              s.scene.random.draws() == 1 && s.legacy_n[4] == 2,
          "44 cancel does not refund already committed42 inventory/elements/random");
    (void)reach_magic_pot_page(s, 42);
    check(act_startup_world_magic_pot_page(s, list, A::cancel) == E::none,
          "42 can return to its actual parent after result dismissal");
    (void)reach_magic_pot_page(s, 41);
    check(act_startup_world_magic_pot_page(s, parent, A::cancel) == E::none,
          "complete deposit chain returns41 and exits to the scene");
    page_tick(s);
    check(s.magic_pot_pages_initialized.empty() && s.magic_pot_page_data.empty() &&
              s.magic_pot_page_lists.empty() && s.magic_pot_page_parents.empty() &&
              s.magic_pot_recipes.size() == 40 && s.magic_pot_comment == "就那样吧" &&
              s.magic_pot_display[2][1] == 2 && s.sound_requests.empty(),
          "deposit retirement leaves no dangling page references; global aM/aN stay and no sound replays");
}
void magic_pot_processing_and_discovery() {
    using A = StartupMagicPotAction;
    using E = StartupWorldRuntimeError;
    auto s = magic_pot_fixture();
    const auto stamp = s.legacy_n[12];
    // 原表列4是经验阈值（2/5/7分别48/30/9），列5..8才是四元素成本。
    s.legacy_n = {47, 3, 0, 0, 0, 0, 0, 12, 12, 12, 12, 1, stamp - 1};
    for (int id : {2, 5, 7}) s.magic_pot_recipes.at(id).status = 0;
    check(open_startup_world_magic_pot(s, StartupMagicPotEntry::development_menu) == E::none &&
              s.legacy_n == ref::WorldMagicPotState{48, 2, 1, 3, 3, 3, 3, 9, 9, 9, 9, 1, stamp} &&
              s.magic_pot_output == std::array<std::int32_t, 4>{3, 3, 3, 3} &&
              s.scene.random.draws() == 0,
          "menu m consumes one period into literal processed state/output without random");
    const auto lower = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [](const auto &p) { return p.legacy_page == 41; });
    check(lower != s.scripts.pages.end() && lower->lifecycle == 0 &&
              !s.magic_pot_pages_initialized.count(lower->id),
          "processing stack retains legitimate uninitialized lower41 until its actual turn");
    check(std::count_if(s.scripts.pages.begin(), s.scripts.pages.end(),
              [](const auto &p) { return p.lifecycle != 4 && p.legacy_page == 46; }) == 3,
          "processing creates three original46 discoveries for prepared p0 recipes2/5/7");
    const auto result = reach_magic_pot_page(s, 45);
    check(act_startup_world_magic_pot_page(s, result, A::cancel) == E::invalid_page &&
              act_startup_world_magic_pot_page(s, result, A::confirm) == E::none &&
              s.page_counters.at(result) == 77 &&
              act_startup_world_magic_pot_page(s, result, A::confirm) == E::none &&
              magic_pot_top(s).id == result, "45 skips to77 but cannot cancel or close before83");
    s.page_counters.at(result) = 83;
    check(act_startup_world_magic_pot_page(s, result, A::confirm) == E::none,
          "45 at83 closes processed result");
    for (int expected : {2, 5, 7}) {
        const auto id = reach_magic_pot_page(s, 46);
        check(inspect_startup_world_magic_pot_page(s, id)->binding == expected &&
                  s.magic_pot_recipes.at(expected).status == 0,
              "46 original static order preserves undiscovered p before confirmation");
        s.page_counters.at(id) = 6;
        check(act_startup_world_magic_pot_page(s, id, A::confirm) == E::none &&
                  s.magic_pot_recipes.at(expected).status == 0,
              "46 counter6 cannot discover recipe");
        s.page_counters.at(id) = 7;
        check(acknowledge_startup_world_runtime_page(s, id) == E::none &&
                  s.magic_pot_recipes.at(expected).status == 1 &&
                  s.magic_pot_recipes.at(expected).pending_notice &&
                  act_startup_world_magic_pot_page(s, id, A::confirm) == E::invalid_page,
              "46 counter7 commits p/r once and rejects stale discovery");
    }
    const auto main = reach_magic_pot_page(s, 41);
    check(s.magic_pot_output == std::array<std::int32_t, 4>{3, 3, 3, 3} &&
              s.magic_pot_display[2][4] == 1 && s.magic_pot_recipes.size() == 40 &&
              ref::world_script_seen(s.scripts, 102) && s.scripts.activities.at(28).status == 1,
          "processed output and fixed recipes survive discovery page consumption");
    check(act_startup_world_magic_pot_page(s, main, A::cancel) == E::none,
          "processed stack finally reaches original41");
    page_tick(s);
    check(s.magic_pot_pages_initialized.empty() && s.magic_pot_page_data.empty() &&
              s.magic_pot_page_lists.empty() && s.magic_pot_page_parents.empty(),
          "processing/discovery chain retires all transient references");
}
void magic_pot_recipe_and_facility_reward() {
    using A = StartupMagicPotAction;
    using E = StartupWorldRuntimeError;
    for (const int recipe : {7, 35}) {
        auto s = magic_pot_fixture();
        // 元素仅准备原成本边界；奖励定义仍来自冻结原表，未改表或算法。
        const std::array<std::int32_t, 4> available = recipe == 7
            ? std::array<std::int32_t, 4>{20, 20, 20, 20}
            : std::array<std::int32_t, 4>{30, 0, 30, 5};
        for (int n = 0; n < 4; ++n) s.legacy_n[3 + n] = available[n];
        if (recipe == 7) {
            // 首次低层道具解锁条件夹具；三份原状态字段保持镜像，未改原表。
            auto &item = s.items.at(29);
            item.status = 0;
            item.unlock_counter = 7;
            item.newly_unlocked = false;
            s.catalog.at({0, 29}) = item;
            s.shop_item_stock.at(29).presence = 0;
            s.shop_item_stock.at(29).legacy_q = 7;
            s.shop_item_stock.at(29).newly_available = false;
        }
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto inventory = s.items.at(29).inventory;
        const auto rewards = s.item_rewards;
        const auto notices = s.scripts.notices.size();
        const auto free = s.facility_free_builds.at(44);
        const auto presence = s.facility_presence.at(44);
        check(open_startup_world_magic_pot(s, StartupMagicPotEntry::main_menu) == E::none,
              "recipe fixture uses real magic pot menu entry");
        const auto main = reach_magic_pot_page(s, 41);
        check(act_startup_world_magic_pot_page(s, main, A::select, 1) == E::none &&
                  acknowledge_startup_world_runtime_page(s, main) == E::none,
              "41 second row opens original recipe catalogue43");
        const auto catalogue = reach_magic_pot_page(s, 43);
        auto view = inspect_startup_world_magic_pot_page(s, catalogue);
        check(view && std::find(view->entries.begin(), view->entries.end(), 38) == view->entries.end() &&
                  act_startup_world_magic_pot_page(s, catalogue, A::next_tab) == E::none &&
                  s.page_phases.at(catalogue) == 1 &&
                  act_startup_world_magic_pot_page(s, catalogue, A::previous_tab) == E::none &&
                  s.page_phases.at(catalogue) == 0,
              "43 respects original bit2 catalogue and exactly two attribute tabs");
        const auto row = std::find(view->entries.begin(), view->entries.end(), recipe);
        check(row != view->entries.end() &&
                  act_startup_world_magic_pot_page(s, catalogue, A::select,
                      static_cast<int>(row - view->entries.begin())) == E::none,
              "43 selects actual original reward recipe");
        auto unknown = s;
        unknown.magic_pot_recipes.at(recipe).status = 0;
        const auto size = unknown.scripts.pages.size();
        check(act_startup_world_magic_pot_page(unknown, catalogue, A::confirm) == E::none &&
                  unknown.scripts.pages.size() == size && unknown.legacy_n == s.legacy_n,
              "43 undiscovered selection silently preserves elements and pages");
        check(acknowledge_startup_world_runtime_page(s, catalogue) == E::none,
              "43 known original recipe opens47 without consuming cost");
        const auto question = reach_magic_pot_page(s, 47);
        const auto before = s.legacy_n;
        s.page_counters.at(question) = 6;
        check(acknowledge_startup_world_runtime_page(s, question) == E::none && s.legacy_n == before,
              "47 counter6 cannot charge recipe or produce reward");
        s.page_counters.at(question) = 7;
        for (int fault = 0; fault < 3; ++fault) {
            auto broken = s;
            if (fault == 0) broken.magic_pot_page_parents.at(question) = main;
            if (fault == 1) broken.magic_pot_page_data.at(question)[2] = -1;
            if (fault == 2) broken.magic_pot_page_data.at(catalogue)[0] = 999;
            check(!inspect_startup_world_magic_pot_page(broken, question) &&
                      act_startup_world_magic_pot_page(broken, question, A::confirm) == E::missing_source &&
                      broken.legacy_n == before && broken.items.at(29).inventory == inventory &&
                      broken.facility_free_builds.at(44) == free && broken.scene.random.draws() == 0,
                  "47 rejects wrong parent/binding or damaged parent selection before any cost/reward");
        }
        auto late = s;
        late.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
        check(acknowledge_startup_world_runtime_page(late, question) == E::script_failed &&
                  late.legacy_n == before && late.items.at(29).inventory == inventory &&
                  late.facility_free_builds.at(44) == free &&
                  late.facility_presence.at(44) == presence && !ref::world_script_seen(late.scripts, 107),
              "47 late107 insertion failure rolls back tentative cost and all reward side effects");
        auto shortfall = s;
        shortfall.legacy_n[recipe == 7 ? 5 : 6] = recipe == 7 ? 9 : 4;
        const auto short_elements = shortfall.legacy_n;
        check(acknowledge_startup_world_runtime_page(shortfall, question) == E::none &&
                  shortfall.legacy_n == short_elements && shortfall.items.at(29).inventory == inventory &&
                  shortfall.facility_free_builds.at(44) == free &&
                  !ref::world_script_seen(shortfall.scripts, 107),
              "47 late-element shortfall closes confirmation without partial earlier-element charges");
        check(acknowledge_startup_world_runtime_page(s, question) == E::none &&
                  ref::world_script_seen(s.scripts, 107) &&
                  s.scene.world.world.ai.accounting.funds() == cash && s.scene.random.draws() == 0,
              "47 ready confirmation creates original107 and reward without charging cash/random");
        check(act_startup_world_magic_pot_page(s, question, A::confirm) == E::invalid_page,
              "committed47 stale input cannot grant another reward");
        if (recipe == 7) {
            check(s.legacy_n[3] == 10 && s.legacy_n[4] == 10 && s.legacy_n[5] == 10 &&
                      s.legacy_n[6] == 20 && s.items.at(29).inventory == inventory + 1 &&
                      s.catalog.at({0, 29}).inventory == inventory + 1 &&
                      s.items.at(29).status == 1 && s.items.at(29).newly_unlocked &&
                      s.items.at(29).unlock_counter == 0 &&
                      s.shop_item_stock.at(29).presence == 1 &&
                      s.shop_item_stock.at(29).legacy_q == 0 &&
                      s.shop_item_stock.at(29).newly_available && s.item_rewards == rewards &&
                      s.scripts.notices.size() == notices && !ref::world_script_seen(s.scripts, 151),
                  "recipe7 low-level item unlock mirrors p/r/q/inventory without high-level E/notice/151");
        } else {
            check(s.legacy_n[3] == 0 && s.legacy_n[4] == 0 && s.legacy_n[5] == 0 &&
                      s.legacy_n[6] == 0 && s.facility_free_builds.at(44) == free &&
                      s.facility_presence.at(44) == presence && magic_pot_top(s).legacy_page != 93,
                  "facility recipe cost commits now;107 precedes93 and facility remains pending");
            const auto gift = reach_magic_pot_page(s, 93);
            const auto sounds = s.sound_requests.size();
            check(sounds > 0 && s.sound_requests.back().id == 5 &&
                      s.sound_requests.back().operation == StartupAudioOperation::jingle &&
                      s.facility_free_builds.at(44) == free,
                  "93 first framework update emits sound5 while reward still waits for confirmation");
            page_tick(s);
            check(s.sound_requests.size() == sounds &&
                      acknowledge_startup_world_runtime_page(s, gift) == E::none &&
                      s.page_counters.at(gift) == 40 && s.facility_free_builds.at(44) == free,
                  "93 second update does not replay sound; early confirm only advances40");
            check(acknowledge_startup_world_runtime_page(s, gift) == E::none &&
                      s.facility_free_builds.at(44) == free + 1 && s.facility_presence.at(44) == 2 &&
                      acknowledge_startup_world_runtime_page(s, gift) == E::invalid_page,
                  "actual93 grants one free original facility and rejects duplicate receipt");
        }
        (void)reach_magic_pot_page(s, 43);
        check(act_startup_world_magic_pot_page(s, catalogue, A::cancel) == E::none,
              "recipe reward returns original43 before exit");
        (void)reach_magic_pot_page(s, 41);
        check(act_startup_world_magic_pot_page(s, main, A::cancel) == E::none,
              "reward chain exits its actual41 parent");
        page_tick(s);
        check(s.magic_pot_pages_initialized.empty() && s.magic_pot_page_data.empty() &&
                  s.magic_pot_page_lists.empty() && s.magic_pot_page_parents.empty() &&
                  s.commerce_pages_initialized.empty() && s.commerce_page_data.empty() &&
                  s.magic_pot_recipes.size() == 40 &&
                  s.scene.world.world.ai.accounting.funds() == cash,
              "recipe/reward retirement clears both page owners and retains fixed recipe/global state");
    }
    auto owned = magic_pot_fixture();
    owned.catalog.at({1, 9}).status = 1; // 原装备p1为已持有，测试43而非复制装备奖励规则。
    check(open_startup_world_magic_pot(owned, StartupMagicPotEntry::main_menu) == E::none,
          "already-owned equipment fixture enters actual pot menu");
    auto id = reach_magic_pot_page(owned, 41);
    check(act_startup_world_magic_pot_page(owned, id, A::select, 1) == E::none &&
              acknowledge_startup_world_runtime_page(owned, id) == E::none,
          "already-owned equipment fixture reaches43");
    id = reach_magic_pot_page(owned, 43);
    const auto view = inspect_startup_world_magic_pot_page(owned, id);
    const auto row = std::find(view->entries.begin(), view->entries.end(), 14);
    const auto original = owned.legacy_n;
    check(row != view->entries.end() &&
              act_startup_world_magic_pot_page(owned, id, A::select,
                  static_cast<int>(row - view->entries.begin())) == E::none &&
              acknowledge_startup_world_runtime_page(owned, id) == E::none &&
              ref::world_script_seen(owned.scripts, 106) && owned.legacy_n == original,
          "43 already-owned weapon emits106 without charging cost or opening47");
}
void village_magic_pot_pages() {
    using E = StartupWorldRuntimeError;
    for (const auto scenario : {std::array<int, 3>{28, 1, 2}, {29, 2, 3},
                                {28, 3, 3}, {30, 1, 1}}) {
        auto s = village_fixture(scenario[0]);
        s.legacy_n[11] = scenario[1];
        s.scripts.user_flags &= ~3U;
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto draws = s.scene.random.draws();
        const auto parent = magic_pot_top(s).id;
        check(acknowledge_startup_world_runtime_page(s, parent) == E::none,
              "original village kind5/6 opens52 through actual51 input");
        page_tick(s);
        check(acknowledge_startup_world_runtime_page(s, magic_pot_top(s).id) == E::none,
              "kind5/6 source52 starts original activity");
        const auto animation = magic_pot_top(s).id;
        page_tick(s);
        check(s.village_points == 100 && s.events_held == 1 &&
                  s.activity_counts.at(scenario[0]) == 1 && s.quarter_counter == 3 &&
                  s.legacy_n[11] == scenario[1] && (s.scripts.user_flags & 3U) == 0,
              "kind5/6 52 charges100 points/m/F but defers q and pot effects to53");
        s.page_counters.at(animation) = 119;
        check(acknowledge_startup_world_runtime_page(s, animation) == E::none &&
                  s.quarter_counter == 3 && s.legacy_n[11] == scenario[1],
              "kind5/6 counter119 cannot fast-forward the confirmed effect");
        page_tick(s);
        const auto continuations = s.scripts.continuations.size();
        if (scenario[0] == 30) {
            auto late = s;
            late.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
            check(acknowledge_startup_world_runtime_page(late, animation) != E::none &&
                      late.quarter_counter == 3 && (late.scripts.user_flags & 3U) == 0 &&
                      !ref::world_script_seen(late.scripts, 104) &&
                      !ref::world_script_seen(late.scripts, 219) &&
                      late.scripts.continuations.size() == continuations && late.village_points == 100 &&
                      late.scene.random.draws() == draws,
                  "kind6 late104 failure rolls back q, user flags, seen/delayed events and pages");
        }
        check(acknowledge_startup_world_runtime_page(s, animation) == E::none &&
                  s.quarter_counter == 2 && s.legacy_n[11] == scenario[2] &&
                  s.village_points == 100 && s.scene.random.draws() == draws &&
                  s.scene.world.world.ai.accounting.funds() == cash &&
                  std::none_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                               [](const auto &p) { return p.lifecycle != 4 && p.legacy_page == 54; }),
              "kind5/6 ready53 commits q then original capped pot effect without human result54/random/cash");
        check(acknowledge_startup_world_runtime_page(s, animation) == E::invalid_page,
              "completed pot activity cannot spend quarterly slot twice");
        if (scenario[0] == 30)
            check((s.scripts.user_flags & 3U) == 3U && ref::world_script_seen(s.scripts, 104) &&
                      ref::world_script_seen(s.scripts, 219) &&
                      s.scripts.continuations.size() > continuations,
                  "kind6 enables two original flags, creates104 and schedules219 rather than eager news19");
    }
}
void second_rank_magic_pot_unlock_chain() {
    using E = StartupWorldRuntimeError;
    using A = StartupVillageActivityAction;
    // 明确的已初始化48条件世界：只给二星条件缓存与200点开展预算。
    // 不声称四宅/12任务/800人气由自然经营取得；以下开放、领取、扣点和入口均走实际Owner。
    auto s = fixture(48);
    const auto rank_page = magic_pot_top(s).id;
    s.rank = 1;
    s.rank_met.fill(true);
    s.page_counters[rank_page] = 1;
    s.village_points = 200;
    s.quarter_counter = 3;
    check(s.scripts.activities.at(30).status == 0 && s.activity_counts.at(30) == 0 &&
              (s.scripts.user_flags & 3U) == 0,
          "conditional second-rank source has neither activity30 nor magic-pot permission");
    const auto initial_n = s.legacy_n;
    check(act_startup_world_runtime_rank_page(s, rank_page) == E::none && s.rank == 2 &&
              ref::world_script_seen(s.scripts, 43) && s.scripts.activities.at(30).status == 0 &&
              s.scripts.activities.at(10).status == 1 && s.scripts.activities.at(24).status == 1 &&
              s.village_points == 200 && (s.scripts.user_flags & 3U) == 0,
          "actual second-rank promotion opens j2 activities but leaves30 to delayed43 reward");
    const auto advance_to = [&](int raw) {
        for (int tick = 0; tick < 600; ++tick) {
            const auto before = magic_pot_top(s);
            if (raw == -1 && before.kind == ref::WorldScriptPageKind::scene &&
                std::none_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                             [](const auto &page) { return page.lifecycle == 4; })) return before.id;
            page_tick(s);
            const auto p = magic_pot_top(s);
            if (p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == raw &&
                p.lifecycle != 0 && s.page_counters.count(p.id)) return p.id;
            if (p.kind == ref::WorldScriptPageKind::dialogue ||
                p.kind == ref::WorldScriptPageKind::simple_message ||
                p.kind == ref::WorldScriptPageKind::newspaper ||
                (p.kind == ref::WorldScriptPageKind::raw_page &&
                 (p.legacy_page == 50 || (p.legacy_page == 11 && p.source_record == 6)))) {
                // 二星事件54的原5,6生成raw11/source6，与普通dialogue不是同一页种。
                check(acknowledge_startup_world_runtime_page(s, p.id) == E::none,
                      "bounded unlock chain consumes only actual script messages and rank celebration");
            } else {
                const bool known = p.kind == ref::WorldScriptPageKind::scene ||
                          (p.kind == ref::WorldScriptPageKind::raw_page &&
                           (p.legacy_page == raw || p.legacy_page == 16 || p.legacy_page == 56 ||
                            p.legacy_page == 57 || p.legacy_page == 97 || p.legacy_page == 98));
                if (!known) throw std::runtime_error("unlock chain unexpected modal: target=" +
                    std::to_string(raw) + ", raw=" + std::to_string(p.legacy_page) + ", kind=" +
                    std::to_string(static_cast<int>(p.kind)) + ", lifecycle=" + std::to_string(p.lifecycle) +
                    ", id=" + std::to_string(p.id));
            }
        }
        throw std::runtime_error("second-rank functional unlock chain exceeded600 page callbacks");
    };
    const auto reward = advance_to(95);
    const auto reward_page = magic_pot_top(s);
    check(reward_page.legacy_r == 11 && reward_page.legacy_s == 30 &&
              s.scripts.activities.at(30).status == 0 && (s.scripts.user_flags & 3U) == 0,
          "actual delayed43 opcode33 binds reward95/r11/s30 without pot introduction");
    const auto points = s.village_points;
    check(s.page_counters.at(reward) < 40 &&
              acknowledge_startup_world_runtime_page(s, reward) == E::none &&
              s.page_counters.at(reward) == 40 && s.scripts.activities.at(30).status == 0 &&
              acknowledge_startup_world_runtime_page(s, reward) == E::none &&
              s.scripts.activities.at(30).status == 1 && s.scripts.activities.at(30).pending_notice &&
              s.activity_counts.at(30) == 0 && s.village_points == points &&
              (s.scripts.user_flags & 3U) == 0 && s.legacy_n == initial_n,
          "specific30 reward confirms at40 without100-point payment or type6 side effects");
    (void)advance_to(-1);
    check(!s.page_counters.count(reward) &&
              open_startup_world_magic_pot(s, StartupMagicPotEntry::main_menu) == E::invalid_page,
          "claiming activity30 retires95 but does not prematurely enable the main magic-pot entry");
    check(open_startup_world_village_activities(s) == E::none,
          "claimed30 enters the actual village catalogue without fixture status injection");
    const auto menu = advance_to(51);
    const auto view = inspect_startup_world_village_activity_page(s, menu);
    check(view.has_value(), "actual51 initialized after claimed activity30");
    const auto row = std::find(view->entries.begin(), view->entries.end(), 30);
    check(row != view->entries.end() &&
              act_startup_world_village_activity_page(s, menu, A::select,
                  static_cast<int>(row - view->entries.begin())) == E::none &&
              act_startup_world_village_activity_page(s, menu, A::confirm) == E::none,
          "actual51 includes received30 and opens its bound52");
    const auto offer = advance_to(52);
    check(act_startup_world_village_activity_page(s, offer, A::confirm) == E::none,
          "actual52 pays for the same received30");
    const auto animation = advance_to(53);
    check(s.village_points == points - 100 && s.activity_counts.at(30) == 1 &&
              s.events_held == 1 && s.quarter_counter == 3 && (s.scripts.user_flags & 3U) == 0,
          "30 payment and holding precede actual magic-pot introduction");
    for (int tick = 0; tick < 120 && s.page_counters.at(animation) < 120; ++tick) page_tick(s);
    check(s.page_counters.at(animation) >= 120, "introduced30 reaches120 within bounded modal callbacks");
    check(acknowledge_startup_world_runtime_page(s, animation) == E::none &&
              (s.scripts.user_flags & 3U) == 3U && s.quarter_counter == 2 &&
              ref::world_script_seen(s.scripts, 104) && ref::world_script_seen(s.scripts, 219),
          "same30 type6 at120 enables both flags through original104 and delayed219");
    (void)advance_to(51);
    check(act_startup_world_village_activity_page(s, menu, A::cancel) == E::none,
          "introduced activity returns through real51 parent");
    (void)advance_to(-1);
    check(s.activity_pages_initialized.empty() && s.activity_page_parents.empty() &&
              s.activity_page_answers.empty() && s.activity_page_bindings.empty(),
          "completed introduction retires village transient references before main entry");
    check(open_startup_world_magic_pot(s, StartupMagicPotEntry::main_menu) == E::none &&
              (s.scripts.user_flags & 3U) == 1U && s.village_points == points - 100 &&
              s.legacy_n == initial_n,
          "main entry after real introduction clears only tip bit and neither recharges nor repeats pot effect");
    const auto pot = advance_to(41);
    check(inspect_startup_world_magic_pot_page(s, pot).has_value() &&
              act_startup_world_magic_pot_page(s, pot, StartupMagicPotAction::cancel) == E::none,
          "introduced main entry produces actual ready41 and explicit return");
    (void)advance_to(-1);
    check(s.magic_pot_pages_initialized.empty() && s.magic_pot_page_data.empty() &&
              s.magic_pot_page_lists.empty() && s.magic_pot_page_parents.empty(),
          "linked41 retires all pot page payloads without deleting shared recipe progress");
    check(!s.sound_requests.empty() &&
              std::all_of(s.sound_requests.begin(), s.sound_requests.end(), [](const auto &request) {
                  return static_cast<int>(request.operation) >= 0 && static_cast<int>(request.operation) <= 2 &&
                         request.id >= 0 && request.id < 26;
              }), "linked unlock keeps typed original audio requests available to one consumer");
    s.sound_requests.clear(); // 条件套件显式消费本链输出；不反复重放主场景或模拟年度。
}
std::uint64_t install_magic_pot_shop(StartupWorldRuntimeState &s, int definition) {
    const auto &map = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
            const auto footprint = ref::facility_footprint(
                static_cast<ref::FacilityShape>(s.rules->facilities.at(definition).shape),
                ref::FacilityOrientation::first, {x, y}, map.width, map.height);
            if (footprint.error != ref::GeometryError::none ||
                !std::all_of(footprint.cells.begin(), footprint.cells.end(), [&](const auto &c) {
                    const auto &tile = map.cells.at(c.position.y * map.width + c.position.x);
                    return c.position.x > bounds[0].x && c.position.x < bounds[1].x &&
                           c.position.y > bounds[1].y && c.position.y < bounds[0].y &&
                           !tile.facility && tile.legacy_state != 1 && tile.legacy_state != 2 &&
                           tile.legacy_state != 10;
                })) continue;
            // 原定义真实Owner安装原语，仅准备商店实例组合；不是自然建设/付款轨迹。
            const auto result = install_startup_world_facility(s, definition, {x, y},
                                                               ref::FacilityOrientation::first);
            check(result.created.has_value(), "magic pot shop fixture installs a real original shop");
            return *result.created;
        }
    throw std::runtime_error("no legal original shop footprint for magic pot fixture");
}
void magic_pot_equipment_low_level_rewards() {
    using A = StartupMagicPotAction;
    using E = StartupWorldRuntimeError;
    // 原配方14/22/30分别奖励武器9/防具13/饰品20；最后一槽只标216是否已见。
    for (const auto scenario : {std::array<int, 4>{14, 1, 9, 0}, {14, 1, 9, 1},
                                {22, 2, 13, 0}, {30, 3, 20, 0}}) {
        auto s = magic_pot_fixture();
        StartupWorldRules private_rules = *s.rules;
        s.rules = &private_rules;
        const auto key = std::make_pair(scenario[1], scenario[2]);
        auto &reward = s.catalog.at(key);
        reward.status = 0;
        reward.newly_unlocked = false;
        reward.free_purchases = 0;
        if (scenario[1] == 1) {
            // 原配方武器9旗位4不含32；216分支另用私有规则/目录旗位夹具，不能称真实配方。
            check((reward.flags & 32U) == 0, "original pot weapon9 does not carry great-weapon flag32");
            reward.flags |= 32U;
            const auto definition = std::find_if(private_rules.equipment.begin(),
                private_rules.equipment.end(), [&](const auto &d) { return d.shop.kind == 1 && d.shop.id == 9; });
            check(definition != private_rules.equipment.end(), "private flag fixture binds original weapon9");
            definition->initial.flags |= 32U;
            if (scenario[3]) s.scripts.event_calls[216] = 1;
        }
        const std::array<std::uint64_t, 3> shops{
            install_magic_pot_shop(s, 30), install_magic_pot_shop(s, 31), install_magic_pot_shop(s, 32)};
        const auto target = shops[static_cast<std::size_t>(scenario[1] - 1)];
        const auto another = install_magic_pot_shop(s, scenario[1] == 1 ? 30 : scenario[1] == 2 ? 31 : 32);
        const int category = scenario[1] == 1 ? 1 : scenario[1] == 2 ? 4 : 5;
        check(s.shops.at(target).category == category && s.shops.at(another).category == category,
              "original shop definitions bind equipment categories1/4/5");
        s.facility_details.at(target).notices.push_back({7, 17}); // 原c(7)按种类去重，保留计数。
        std::map<std::uint64_t, std::vector<std::array<int, 2>>> shop_notices;
        for (const auto &shop : s.shops) shop_notices.emplace(shop.first, s.facility_details.at(shop.first).notices);
        const auto rewards = s.item_rewards;
        const auto notices = s.scripts.notices.size();
        const auto continuations = s.scripts.continuations.size();
        for (int n = 3; n < 7; ++n) s.legacy_n[n] = 999;
        check(open_startup_world_magic_pot(s, StartupMagicPotEntry::main_menu) == E::none,
              "equipment reward combination enters real pot menu");
        const auto main = reach_magic_pot_page(s, 41);
        check(act_startup_world_magic_pot_page(s, main, A::select, 1) == E::none &&
                  acknowledge_startup_world_runtime_page(s, main) == E::none,
              "equipment reward combination opens real43");
        const auto catalogue = reach_magic_pot_page(s, 43);
        const auto view = inspect_startup_world_magic_pot_page(s, catalogue);
        check(view.has_value(), "equipment combination has initialized original43 payload");
        const auto row = std::find(view->entries.begin(), view->entries.end(), scenario[0]);
        check(row != view->entries.end() && act_startup_world_magic_pot_page(s, catalogue, A::select,
                  static_cast<int>(row - view->entries.begin())) == E::none &&
                  acknowledge_startup_world_runtime_page(s, catalogue) == E::none,
              "known equipment recipe43 opens its real47");
        const auto question = reach_magic_pot_page(s, 47);
        s.page_counters.at(question) = 7;
        if (scenario[1] == 1 && !scenario[3]) {
            auto late = s;
            late.facility_details.erase(another);
            const auto pot = late.legacy_n;
            check(acknowledge_startup_world_runtime_page(late, question) == E::script_failed &&
                      late.legacy_n == pot && late.catalog.at(key).status == 0 &&
                      !late.catalog.at(key).newly_unlocked && late.catalog.at(key).free_purchases == 0 &&
                      !ref::world_script_seen(late.scripts, 107) && !ref::world_script_seen(late.scripts, 216) &&
                      late.scripts.continuations.size() == continuations &&
                      late.facility_details.at(target).notices == shop_notices.at(target),
                  "late missing second shop payload rolls back recipe cost, NEW/free,107/216 and shop notice");
        }
        check(acknowledge_startup_world_runtime_page(s, question) == E::none &&
                  s.catalog.at(key).status == 1 && s.catalog.at(key).newly_unlocked &&
                  s.catalog.at(key).free_purchases == 1 && s.item_rewards == rewards &&
                  s.scripts.notices.size() == notices && !ref::world_script_seen(s.scripts, 110) &&
                  !ref::world_script_seen(s.scripts, 151) && s.scene.random.draws() == 0,
              "47 low-level equipment unlock grants p/r/free once without high-level110/notice/E151");
        for (const auto &shop : s.shops) {
            auto expected = shop_notices.at(shop.first);
            if (shop.second.category == category &&
                std::none_of(expected.begin(), expected.end(), [](const auto &n) { return n[0] == 7; }))
                expected.push_back({7, 0});
            check(s.facility_details.at(shop.first).notices == expected,
                  "47 adds deduplicated7 only to matching original shop category and preserves existing counter");
        }
        check(ref::world_script_seen(s.scripts, 216) == (scenario[1] == 1) &&
                  s.scripts.continuations.size() == continuations +
                      (scenario[1] == 1 && !scenario[3] ? 1U : 0U),
              "flag32 private weapon schedules216 only first time; armour/accessory create no216");
        (void)reach_magic_pot_page(s, 43);
        const auto pot = s.legacy_n;
        const auto pending = s.scripts.continuations.size();
        check(acknowledge_startup_world_runtime_page(s, catalogue) == E::none &&
                  ref::world_script_seen(s.scripts, 106) && s.legacy_n == pot &&
                  s.catalog.at(key).free_purchases == 1 && s.scripts.continuations.size() == pending &&
                  s.scripts.notices.size() == notices,
              "repeat equipment43 emits106 and cannot give another free purchase, cost,216 or global notice");
    }
}

} // namespace
int main() {
    try {
        facility_commodity_pages();
        facility_reputation_pages();
        information_menu_pages();
        income_information_pages();
        town_facility_information_pages();
        owned_item_information_pages();
        equipment_information_pages();
        adventurer_information_pages();
        ordinary_item_pages();
        commerce_after_reward_writeback();
        commerce_transactions();
        commerce_facility_and_projection();
        magic_pot_entry_and_retirement();
        magic_pot_deposit_pages();
        magic_pot_processing_and_discovery();
        magic_pot_recipe_and_facility_reward();
        magic_pot_equipment_low_level_rewards();
        village_magic_pot_pages();
        second_rank_magic_pot_unlock_chain();
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
