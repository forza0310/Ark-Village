#include "ark/simulation/rules/world_calendar_tasks.hpp"

#include <filesystem>
#include <fstream>
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
std::string read(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("missing published script input");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
WorldScriptCatalog catalog() {
    const auto root = std::filesystem::path(ARK_WORLD_TEST_DATA) / "scripts/original";
    const auto result =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(result.catalog.has_value(), "real published scripts parse");
    return *result.catalog;
}
// 所有地图/人物/任务实例与factory/持久consumer都为显式夹具，不代表原新局或完整f(kind)。
WorldCalendarTasksState fixture() {
    WorldCalendarTasksState state;
    WorldScriptPage scene;
    scene.id = 1;
    state.scripts.pages = {scene};
    state.scripts.next_page_id = 2;
    state.scripts.executing_page = 1;
    state.scripts.village_name = "FIXTURE";
    state.scripts.human_catalog_complete = true;
    state.scripts.facility_catalog_complete = true;
    state.scripts.humans[0] = {1, false, "fixture", 0, 0};
    state.scripts.humans[1] = {1, false, "fixture", 1, 0};
    state.human_details[0] = {{1, 2}, 0};
    state.human_details[1] = {{60}, 0};
    state.finish.task_progress.definitions[30] = {0, 0, 0, 9};
    state.finish.task_progress.definitions[31] = {1, 0, 0, 10};
    state.task_details[30] = {"fixture exploration", "fixture exploration"};
    state.task_details[31] = {"fixture battle", "fixture battle"};
    state.rank_terms = fixed_calendar_task_rank_terms();
    state.random = WorldRandomStream::from_raw({});
    return state;
}
void task(WorldCalendarTasksState &state, std::uint64_t identity, int definition) {
    state.finish.tasks[identity] = {identity, definition, 1, 5, {}, {}};
    state.finish.task_order.push_back(identity);
}
void facility(WorldCalendarTasksState &state, std::uint64_t identity, int definition, int category,
              int status = 1) {
    RescueFacility value;
    value.placement = {
        {identity}, definition, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    value.category = category;
    value.kind = category;
    value.status = status;
    state.finish.dungeon.world.facilities[identity] = value;
    state.facility_order.push_back(identity);
}
CalendarTaskExternalConsumer factory(bool succeed = true) {
    return
        [succeed](const WorldCalendarTasksState &state, const CalendarTaskExternalRequest &request)
            -> std::optional<CalendarTaskExternalCandidate> {
            if (request.kind != CalendarTaskExternalKind::create_task)
                return {};
            auto next = state;
            if (!succeed)
                return CalendarTaskExternalCandidate{std::move(next), {}};
            std::uint64_t identity = 2;
            while (next.finish.tasks.count(identity))
                ++identity;
            task(next, identity, request.task_kind == 0 ? 30 : 31);
            if (request.task_kind == 0) {
                facility(next, 99, 43, 5);
                next.finish.dungeon.facilities[99] = {};
                next.finish.tasks.at(identity).facility = 99;
                next.finish.tasks.at(identity).site = Position{1, 1};
            }
            return CalendarTaskExternalCandidate{std::move(next), identity};
        };
}
CalendarTaskResult prepare(const WorldCalendarTasksState &state, const WorldCalendarState &date,
                           WorldCalendarStage stage, const WorldScriptCatalog &scripts,
                           const CalendarTaskExternalConsumer &external = {}) {
    return prepare_world_calendar_tasks(state, date, stage, scripts, external);
}
void specials(const WorldScriptCatalog &scripts) {
    for (int year : {0, 14, 15, 16})
        for (int month = 0; month < 12; ++month) {
            auto state = fixture();
            bool called{};
            const CalendarTaskExternalConsumer persist =
                [&](const WorldCalendarTasksState &owner,
                    const CalendarTaskExternalRequest &request)
                -> std::optional<CalendarTaskExternalCandidate> {
                called = true; // 同步私有候选接受的测试观测，不写真实文件。
                check(request.kind == CalendarTaskExternalKind::endgame_checkpoint &&
                          owner.completion_mode == 1 && owner.system_completion_mode == 1 &&
                          owner.scripts.event_calls.at(3) == 1,
                      "endgame checkpoint observes script3/page17 then both flags");
                auto next = owner;
                next.system_unlock_data = {std::vector<std::uint8_t>{0, 1},
                                           std::vector<std::uint8_t>{0, 0}};
                return CalendarTaskExternalCandidate{std::move(next), {}};
            };
            const auto result =
                prepare(state, {year, month, 0, 0, 0, 0}, WorldCalendarStage::month_special_scripts,
                        scripts, persist);
            check(result.candidate && called == (year == 15 && month == 3),
                  "only exact raw15/3 reaches real endgame checkpoint");
            if (month == 11)
                check(result.candidate->state.scripts.event_calls.at(26) == 1 &&
                          result.candidate->state.scripts.continuations[0].remaining_updates ==
                              1340,
                      "year-end26 actual source wait1340, no eager medals or placeholder page");
            if (called)
                check(result.candidate->pages.size() == 2 &&
                          result.candidate->pages[0].source_record == 0 &&
                          result.candidate->pages[1].legacy_page == 17 &&
                          !result.candidate->state.system_unlock_data[0].empty(),
                      "endgame script resolved and dynamic page17 inserted before checkpoint");
        }
    auto state = fixture();
    const auto denied =
        prepare(state, {15, 3, 0, 0, 0, 0}, WorldCalendarStage::month_special_scripts, scripts);
    check(denied.error == CalendarTaskError::missing_consumer && !denied.candidate &&
              state.scripts.pages.size() == 1 && state.completion_mode == 0,
          "missing actual checkpoint rolls back script/page/flags instead of empty success");
}
void ranks(const WorldScriptCatalog &scripts) {
    auto state = fixture();
    state.popularity = 300;
    state.highest_month_income = 5000;
    state.events_held = 2;
    facility(state, 5, 35, 3, 0);
    for (int missing = -1; missing < 4; ++missing) {
        auto input = state;
        if (missing == 0)
            input.popularity = 299;
        if (missing == 1)
            input.highest_month_income = 4999;
        if (missing == 2)
            input.events_held = 1;
        if (missing == 3)
            input.finish.dungeon.world.facilities.at(5).placement.definition_id = 36;
        const auto result =
            prepare(input, {1, 0, 0, 0, 0, 0}, WorldCalendarStage::month_rank_check, scripts);
        check(result.candidate && result.candidate->state.rank == 0 &&
                  world_script_seen(result.candidate->state.scripts, 36) == (missing == -1),
              "all four exact source thresholds must meet; prompt never really advances rank");
        if (missing == -1)
            check(result.candidate->pages.size() == 2 &&
                      result.candidate->pages[0].source_record == 39 &&
                      result.candidate->pages[1].legacy_page == 48 &&
                      result.candidate->state.rank_values[3] == 0,
                  "rank checks planned-status0 facility too, criteria3 display stays0");
    }
    state.rank = 1;
    state.popularity = 800;
    state.finish.task_progress.successes = 12;
    state.facility_order.clear();
    state.finish.dungeon.world.facilities.clear();
    for (int id = 1; id <= 14; ++id)
        facility(state, id, id, id <= 5 ? 3 : id <= 10 ? 9 : 12, 0);
    for (auto &entry : state.finish.dungeon.world.facilities)
        entry.second.category = 5; // 真种类与活动类别刻意不同，防止f82e/f83f投影混用。
    const auto second =
        prepare(state, {1, 0, 0, 0, 0, 0}, WorldCalendarStage::month_rank_check, scripts);
    check(second.candidate &&
              second.candidate->state.rank_values == std::array<int, 4>{800, 10, 4, 12} &&
              world_script_seen(second.candidate->state.scripts, 36),
          "facility criterion counts categories3/9; housing criterion counts instances12 not "
          "residents");
    state = fixture();
    state.rank_bypass = 1;
    const auto bypass =
        prepare(state, {0, 0, 0, 0, 0, 0}, WorldCalendarStage::month_rank_check, scripts);
    check(bypass.candidate && world_script_seen(bypass.candidate->state.scripts, 36),
          "only explicit sourceau1 bypasses unmet cached criteria, no inferred cheat");
    state.rank = 5;
    state.rank_met.fill(true);
    const auto max =
        prepare(state, {0, 0, 0, 0, 0, 0}, WorldCalendarStage::month_rank_check, scripts);
    check(max.candidate && max.candidate->state.rank_met == state.rank_met &&
              max.candidate->pages.empty(),
          "rank5 returns before clearing old i/j buffers evenau1");
}
void deadlines(const WorldScriptCatalog &scripts) {
    for (int old = 0; old <= 14; ++old) {
        auto state = fixture();
        task(state, 1, 30);
        state.finish.active_task = 1;
        state.finish.participants = {0, 0, 1};
        state.task_subperiods = old;
        const auto result = prepare(state, {1, 0, 0, 0, 0, 0},
                                    WorldCalendarStage::subperiod_task_deadline, scripts);
        check(result.candidate && result.candidate->state.task_subperiods == old + 1,
              "single global y increments once per changed subperiod, not per participant");
        check(result.candidate->pages.size() == (old + 1 >= 12 ? 2u : 0u) &&
                  result.candidate->state.scripts.notices.size() == (old + 1 == 8 ? 1u : 0u),
              ">=12 repeats continuation prompt; exactly8 emits notice27 only");
        if (old + 1 >= 12)
            check(result.candidate->state.human_details.at(0).continuation_cost == 250 &&
                      result.candidate->state.human_details.at(1).continuation_cost == 3000 &&
                      result.candidate->pages[1].legacy_f == 3500 &&
                      result.candidate->pages[1].task_identity == std::optional<std::uint64_t>{1} &&
                      result.candidate->pages[1].task_definition == std::optional<int>{30} &&
                      result.candidate->state.deadline_page == result.candidate->pages[1].id,
                  "all profession levels fee capped per human then duplicate participants repeat "
                  "sum");
    }
    auto state = fixture();
    state.task_subperiods = 100;
    const auto none =
        prepare(state, {0, 0, 0, 0, 0, 0}, WorldCalendarStage::subperiod_task_deadline, scripts);
    check(none.candidate && none.candidate->state.task_subperiods == 100,
          "no globalh means no y increment or participant dependency");
    task(state, 1, 30);
    state.finish.active_task = 1;
    state.finish.participants = {0, 99};
    const auto missing =
        prepare(state, {0, 0, 0, 0, 0, 0}, WorldCalendarStage::subperiod_task_deadline, scripts);
    check(!missing.candidate && state.human_details.at(0).continuation_cost == 0,
          "late missing participant rolls back earlier n() cost and y");
}
void rank_promotion_and_celebration() {
    const auto terms = fixed_calendar_task_rank_terms();
    for (int rank = 0; rank < 5; ++rank) {
        const auto result = prepare_world_rank_promotion(
            rank, terms.at(rank), {true, true, true, true}, 0, true, "FIXTURE");
        check(result && result->promoted && result->rank == rank + 1 && result->mark_user_flag &&
                  result->before_promotion.front().event == 37 &&
                  result->after_promotion.front().event == rank + 53,
              "all five promotions keep manual37 before rank and true rank script afterward");
        check(result->after_promotion.size() == (rank == 0 || rank == 2 || rank == 4 ? 4u : 3u),
              "only new ranks1/3/5 append206/208/210");
        if (rank == 4)
            check(result->after_promotion[1].event == 47 && result->after_promotion[2].event == 46,
                  "rank5 executes47 then46, no rank-below5 script");
        else
            check(result->after_promotion[1].replacement ==
                          "FIXTURE\t" + std::to_string(rank + 1) &&
                      result->after_promotion[2].event == 41,
                  "rank scripts retain village and actual new rank payload");
    }
    for (int count = 0; count < 4; ++count) {
        std::array<bool, 4> met{};
        std::fill_n(met.begin(), count, true);
        const auto result = prepare_world_rank_promotion(0, terms.at(0), met, 0, false, "FIXTURE");
        check(result && !result->promoted && result->mark_user_flag &&
                  result->before_promotion.front().event == (count == 3 ? 39 : 38),
              "three met conditions use39, all other denials38; marku8 without promotion");
    }
    const auto explain = prepare_world_rank_promotion(0, terms.at(0), {}, 1, false, "FIXTURE");
    check(explain && !explain->mark_user_flag && !explain->promoted &&
              explain->before_promotion.front().event == 159,
          "selected popularity criterion emits5+154 without userflag or rank");
    check(!prepare_world_rank_promotion(5, terms.at(0), {}, 0, false, "FIXTURE") &&
              !prepare_world_rank_promotion(0, {}, {}, 0, false, "FIXTURE"),
          "max rank and incomplete criterion catalogue reject");
    const auto rng = WorldRandomStream::from_raw({2, 0, 1});
    const auto celebration = prepare_world_rank_celebration({1, 3, 5}, rng);
    check(celebration && celebration->random.draws() == 3 && rng.draws() == 0 &&
              celebration->participants == std::vector<std::array<int, 5>>{{3, 287, 143, 3, 0},
                                                                           {1, 273, 135, 1, 1},
                                                                           {5, 287, 135, 2, 1}},
          "source all-range swap, not Fisher-Yates; definition-index layout retained");
    const auto empty = prepare_world_rank_celebration({}, WorldRandomStream::from_raw({}));
    check(empty && empty->participants.empty() && empty->random.draws() == 0,
          "empty celebration draws nothing");
    check(!prepare_world_rank_celebration({1}, WorldRandomStream::from_raw({})) &&
              !prepare_world_rank_celebration({1, 3, 5}, WorldRandomStream::from_raw({2, 0})),
          "single participant still needs one draw; late exhaustion leaves no partial candidate");
    std::vector<int> many(12);
    for (int n = 0; n < 12; ++n)
        many[n] = n;
    const auto capped = prepare_world_rank_celebration(
        many, WorldRandomStream::from_raw(std::vector<std::int32_t>(12, 0)));
    check(capped && capped->participants.size() == 10 && capped->random.draws() == 12,
          "all12 shuffled before ten-layout truncation, no shortened random stream");
}
void midpoint(const WorldScriptCatalog &scripts) {
    auto state = fixture();
    task(state, 1, 30);
    task(state, 2, 31);
    state.finish.task_progress.definitions.at(31).flags = 2;
    state.scripts.popularity_queue = {{1, 7, 0}};
    for (int period = 0; period < 4; ++period) {
        const auto result = prepare(state, {1, 0, period, 0, 0, 0},
                                    WorldCalendarStage::subperiod_task_midpoint, scripts);
        check(result.candidate &&
                  result.candidate->state.scripts.notices.size() == (period == 2 ? 1u : 0u),
              "only rawsubperiod2 consumes first task whose definition has flag2");
        if (period == 2)
            check(result.candidate->state.scripts.notices[0].message == 32 &&
                      result.candidate->state.scripts.notices[0].counter == 1 &&
                      result.candidate->state.scripts.notices[0].definitions ==
                          std::vector<int>{10} &&
                      result.candidate->state.scripts.popularity_queue ==
                          std::vector<std::array<int, 3>>{{85, -10, 1}, {1, 7, 0}},
                  "midpoint uses special note32 positive1 and slot3 delay85, no immediate "
                  "popularity");
    }
}
void generation(const WorldScriptCatalog &scripts) {
    for (int month = 0; month <= 5; ++month)
        for (int period = 0; period < 4; ++period) {
            auto state = fixture();
            state.generation_retry = true;
            const auto result = prepare(state, {0, month, period, 0, 0, 0},
                                        WorldCalendarStage::subperiod_task_generation, scripts);
            check(
                result.candidate && result.candidate->random_draws == 0 &&
                    result.candidate->state.generation_retry == (period != 1 && period != 3),
                "first6months veto even retry/emptyq, qualifying odd subperiod clears retry first");
        }
    auto state = fixture();
    state.random = WorldRandomStream::from_raw({3, 9});
    const auto created = prepare(state, {0, 6, 1, 0, 0, 0},
                                 WorldCalendarStage::subperiod_task_generation, scripts, factory());
    check(created.candidate && created.candidate->generation_kind == 0 &&
              created.candidate->created_task == std::optional<std::uint64_t>{2} &&
              created.candidate->random_draws == 2 &&
              created.candidate->state.finish.tasks.at(2).facility ==
                  std::optional<std::uint64_t>{99} &&
              created.candidate->state.scripts.continuations[0].event == 59 &&
              created.candidate->state.scripts.notices[0].counter == -80 &&
              (created.candidate->state.scripts.user_flags & 4) != 0,
          "equal balance consumes100 even shop override then month7 forces0, real factory and "
          "notice59");
    const auto no_factory =
        prepare(state, {0, 6, 1, 0, 0, 0}, WorldCalendarStage::subperiod_task_generation, scripts);
    check(no_factory.error == CalendarTaskError::missing_consumer && !no_factory.candidate &&
              state.random.draws() == 0 && state.finish.tasks.empty(),
          "missing actualtaskfactory fails after private draw without committing random or fake "
          "task");
    const auto no_space =
        prepare(state, {0, 6, 1, 0, 0, 0}, WorldCalendarStage::subperiod_task_generation, scripts,
                factory(false));
    check(no_space.candidate && no_space.candidate->state.generation_retry &&
              no_space.candidate->random_draws == 1 && no_space.candidate->pages.empty(),
          "actual factory ordinary null preserves its random and setsY retry, no human draw");
    state = fixture();
    task(state, 1, 30);
    state.random = WorldRandomStream::from_raw({});
    const auto early_existing =
        prepare(state, {0, 7, 1, 0, 0, 0}, WorldCalendarStage::subperiod_task_generation, scripts,
                factory());
    check(early_existing.candidate && early_existing.candidate->random_draws == 0 &&
              early_existing.candidate->external_calls.empty(),
          "first8months with existing exploration forces-1 after balance selection, no factory");
    state = fixture();
    state.finish.task_progress.task_pool_progress = 500;
    state.random = WorldRandomStream::from_raw({0, 1});
    const auto battle = prepare(state, {1, 0, 1, 0, 0, 0},
                                WorldCalendarStage::subperiod_task_generation, scripts, factory());
    check(battle.candidate && battle.candidate->generation_kind == 1 &&
              battle.candidate->random_draws == 2 &&
              battle.candidate->state.scripts.continuations[0].event == 60,
          "G500 forces battle after already-consumed equal draw, actual first60 continuation");
    state = fixture();
    task(state, 1, 30);
    state.finish.task_order = {1, 1, 1};
    state.generation_retry = true;
    const auto full =
        prepare(state, {1, 1, 3, 0, 0, 0}, WorldCalendarStage::subperiod_task_generation, scripts);
    check(full.candidate && !full.candidate->state.generation_retry &&
              full.candidate->random_draws == 0,
          "source capacity counts all orderedrefs including duplicate references, retry cleared "
          "before full");
    state = fixture();
    state.random = WorldRandomStream::from_raw({0, 1});
    state.scripts.human_catalog_complete = false;
    const auto late = prepare(state, {1, 0, 1, 0, 0, 0},
                              WorldCalendarStage::subperiod_task_generation, scripts, factory());
    check(
        !late.candidate && state.finish.tasks.empty() && state.scripts.notices.empty() &&
            state.random.draws() == 0,
        "late incompletehuman catalog rolls back factory/map/notice/flags/random in one candidate");
}
void capacity(const WorldScriptCatalog &scripts) {
    for (int available = 0; available <= 5; ++available)
        for (int count = 0; count <= 9; ++count) {
            auto state = fixture();
            state.rank = 3;
            for (int id = 0; id < available; ++id)
                state.facility_definition_presence[id] = 1;
            for (int id = 1; id <= count; ++id)
                facility(state, id, 35, 3, 0);
            const auto result = prepare(state, {1, 0, 3, 0, 0, 0},
                                        WorldCalendarStage::subperiod_capacity_hint, scripts);
            const bool expected = available > 0 && count * 100 / available < 150;
            check(result.candidate &&
                      world_script_seen(result.candidate->state.scripts, 163) == expected,
                  "capacity has integer percentage strict150 cutoff and needs nonzero available "
                  "defs");
            if (expected)
                check(result.candidate->pages[0].replacement ==
                              "FIXTURE\t" + std::to_string(count) &&
                          result.candidate->pages[0].source_record == 173,
                      "capacity actual163 resolves village and currentinstance count parameters");
        }
}
void special_generation(const WorldScriptCatalog &scripts) {
    for (int flags : {0, 2, 4, 6})
        for (int victories : {0, 1, 8, 9}) {
            auto state = fixture();
            state.finish.task_progress.task_pool_progress = 500;
            state.finish.task_progress.definitions.at(31).flags = flags;
            state.finish.task_progress.definitions.at(31).completed = 1;
            state.monster_details[10] = {"fixture monster", 0, victories};
            state.scripts.event_calls[60] = 1;
            state.finish.event_calls = state.scripts.event_calls;
            state.random = WorldRandomStream::from_raw({0, 1});
            const auto result =
                prepare(state, {1, 0, 1, 0, 0, 0}, WorldCalendarStage::subperiod_task_generation,
                        scripts, factory());
            check(result.candidate && result.candidate->random_draws == 2,
                  "special generation consumes exactly balance and speaking-human draw");
            const auto &next = result.candidate->state;
            const bool boss = (flags & 2) != 0;
            const bool replay = !boss && (flags & 4) != 0;
            check(world_script_seen(next.scripts, boss ? 134 : 133) &&
                      result.candidate->pages.front().speaker_kind == 1 &&
                      result.candidate->pages.front().speaker_definition == 1,
                  "normal and special task first talk uses chosen definition, flags2 precedes4");
            check(next.finish.task_progress.task_pool_progress == (boss ? 0 : 500),
                  "boss branch alone resets G after its complete source script sequence");
            if (boss) {
                check(result.candidate->pages.at(1).legacy_page == 99 &&
                          result.candidate->pages.at(1).monster_definition ==
                              std::optional<int>{10} &&
                          world_script_seen(next.scripts, 139) &&
                          world_script_seen(next.scripts, 138),
                      "boss source99/family139/name138 order creates real payloads");
                check(world_script_seen(next.scripts, 165) == (victories > 0) &&
                          world_script_seen(next.scripts, 166) == (victories >= 9),
                      "monster victory boundaries distinguish first reprise and ninth enhancement");
                for (const auto &page : result.candidate->pages)
                    if (page.source_record == 175 || page.source_record == 176)
                        check(page.speaker_kind == 2 && page.speaker_definition == 10,
                              "boss reprise talks use monster speaker2, not chosen human");
            } else if (replay) {
                check(result.candidate->pages.at(1).legacy_page == 100 &&
                          world_script_seen(next.scripts, 167) &&
                          result.candidate->pages.back().speaker_kind == 2,
                      "flag4 task source100 and completed167 have monster speaker");
            } else
                check(result.candidate->pages.size() == 1 &&
                          result.candidate->pages[0].replacement == "fixture battle",
                      "seen60 ordinary battle has only actual133 with title parameter");
            auto seen = next;
            seen.finish.task_order.clear();
            seen.finish.tasks.clear();
            seen.finish.task_progress.task_pool_progress = 500;
            seen.random = WorldRandomStream::from_raw({0, 0});
            const auto repeat =
                prepare(seen, {1, 0, 1, 0, 0, 0}, WorldCalendarStage::subperiod_task_generation,
                        scripts, factory());
            auto calls = [](const WorldScriptState &owner, int event) {
                const auto found = owner.event_calls.find(event);
                return found == owner.event_calls.end() ? 0 : found->second;
            };
            check(repeat.candidate &&
                      calls(repeat.candidate->state.scripts, 165) == calls(next.scripts, 165) &&
                      calls(repeat.candidate->state.scripts, 166) == calls(next.scripts, 166) &&
                      calls(repeat.candidate->state.scripts, 167) == calls(next.scripts, 167),
                  "first-time reprise guards do not append extra previously-seen scripted entry");
        }
    auto state = fixture();
    state.finish.task_progress.task_pool_progress = 500;
    state.scripts.event_calls[60] = 1;
    state.finish.event_calls = state.scripts.event_calls;
    state.scripts.page_mutations_locked = true;
    state.random = WorldRandomStream::from_raw({0, 0});
    const auto locked = prepare(state, {1, 0, 1, 0, 0, 0},
                                WorldCalendarStage::subperiod_task_generation, scripts, factory());
    check(locked.candidate && locked.candidate->pages.empty() &&
              locked.candidate->state.finish.tasks.size() == 1 &&
              world_script_seen(locked.candidate->state.scripts, 133),
          "page mutation lock suppresses insertion, not factory/script/speaker object preparation");
    state.scripts.page_mutations_locked = false;
    state.finish.task_progress.definitions.at(31).flags = 2;
    const auto missing = prepare(state, {1, 0, 1, 0, 0, 0},
                                 WorldCalendarStage::subperiod_task_generation, scripts, factory());
    check(!missing.candidate && state.random.draws() == 0 && state.finish.tasks.empty() &&
              state.scripts.notices.empty(),
          "missing monster after actual task creation rolls back map/task/notice/random");
}
} // namespace
int main() {
    try {
        const auto scripts = catalog();
        specials(scripts);
        ranks(scripts);
        rank_promotion_and_celebration();
        deadlines(scripts);
        midpoint(scripts);
        generation(scripts);
        special_generation(scripts);
        capacity(scripts);
        std::cout << "calendar task checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
