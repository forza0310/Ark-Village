#include "ark/simulation/rules/world_task_commands.hpp"

#include <algorithm>
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
WorldTaskCommandState fixture(int count = 4, std::int64_t cash = 5000) {
    WorldTaskCommandState s;
    s.clock_parameter = 80;
    s.cash_period = 1;
    s.random = WorldRandomStream::from_java_seed(7);
    s.definitions.emplace(0, TaskCommandDefinition{0, 1300, 1500, "任务名称夹具"});
    s.finish.tasks.emplace(1, DungeonFinishTask{1, 0, 5, 20, 10, Position{2, 2}});
    s.finish.task_order = {1};
    s.finish.task_progress.definitions.emplace(0, DungeonTaskDefinitionProgress{});
    s.finish.dungeon.world.ai.accounting = PeriodAccounting{cash};
    s.finish.dungeon.world.facilities.emplace(10, RescueFacility{});
    s.finish.dungeon.world.facilities.at(10).status = 8;
    s.finish.dungeon.facilities.emplace(10, DungeonFacilityProgress{99, 123, 456, 78, 79, {}});
    s.facility_difficulties.emplace(10, 5);
    for (int n = 0; n < count; ++n) {
        s.humans.push_back({n, 1, 0, 0, std::vector<int>(3, 1), 999, 4});
        s.finish.human_definition_flags.emplace(n, 8U | 2U);
        HumanBattleRecord battle;
        battle.task_kills = 7;
        battle.participant_downs = 8;
        battle.recent_reward = 9;
        battle.recent_kills = 10;
        s.finish.dungeon.world.ai.battle.humans.emplace(n, battle);
    }
    if (count > 0) {
        s.finish.participants = {0};
        s.finish.dungeon.world.ai.battle.participants = {0};
    }
    return s;
}
TaskCommandConsumer identity() {
    return [](const auto &s, const auto &) -> std::optional<WorldTaskCommandState> { return s; };
}
std::vector<TaskCommandEffectKind> kinds(const TaskCommandCandidate &c) {
    std::vector<TaskCommandEffectKind> result;
    for (const auto &effect : c.effects)
        result.push_back(effect.kind);
    return result;
}
void offers_and_random() {
    const auto source = fixture();
    const auto accepted = prepare_world_task_offer(source, 1, {true, false, 1}, identity());
    check(accepted.candidate && accepted.candidate->accepted &&
              accepted.candidate->state.phase == TaskCommandPhase::recruiting &&
              accepted.candidate->state.finish.dungeon.world.ai.accounting.funds() == 3700 &&
              accepted.candidate->state.finish.participants == source.finish.participants &&
              !accepted.candidate->state.finish.active_task,
          "page23 confirmation ignores selection and posts real category4 fee without task/team "
          "activation");
    check(kinds(*accepted.candidate) ==
                  std::vector<TaskCommandEffectKind>{TaskCommandEffectKind::reset_main_scene,
                                                     TaskCommandEffectKind::open_page,
                                                     TaskCommandEffectKind::close_page} &&
              accepted.candidate->effects[1].first == 24 &&
              accepted.candidate->effects[2].first == 23,
          "offer actual reset-main -> initialized recruitment24 -> close23 order");
    const auto &ledger = accepted.candidate->state.finish.dungeon.world.ai.accounting.entries();
    check(ledger.size() == 1 && ledger.begin()->second.category == CashCategory::other &&
              ledger.begin()->second.amount == 1300 &&
              ledger.begin()->second.direction == CashDirection::expense,
          "source charge category4 is other, not adventurer income or facility service");
    auto insufficient = fixture(4, 1299);
    const auto denied = prepare_world_task_offer(insufficient, 1, {true, false, 0}, identity());
    check(denied.candidate && denied.candidate->denial == TaskCommandDenial::insufficient_funds &&
              denied.candidate->effects.size() == 1 && denied.candidate->effects[0].first == 11 &&
              denied.candidate->state.random.draws() == 0 &&
              denied.candidate->state.finish.participants == insufficient.finish.participants &&
              denied.candidate->state.finish.dungeon.world.ai.accounting.entries().empty(),
          "insufficient fee only message11, no RNG/team/H/I/ledger mutation");
    const auto cancel = prepare_world_task_offer(source, 1, {false, true, 0}, identity());
    check(cancel.candidate && cancel.candidate->state.phase == TaskCommandPhase::closed &&
              cancel.candidate->state.random.draws() == 0 &&
              cancel.candidate->state.finish.dungeon.world.ai.accounting.funds() == 5000 &&
              cancel.candidate->state.finish.participants == source.finish.participants,
          "offer cancellation closes23 only, does not refund/pay or clear participants");
    check(!prepare_world_task_offer(accepted.candidate->state, 1, {true, false, 0}, identity())
               .candidate,
          "already transitioned offer cannot charge a second time");
    check(!prepare_world_task_offer(source, 999, {true, false, 0}, identity()).candidate,
          "stale stable task identity rejected before payment");
    check(prepare_world_task_offer(source, 1, {true, false, 0}).error ==
              TaskCommandError::missing_consumer,
          "required scene/page effects do not default to successful consumers");
    auto tape = fixture(1);
    tape.random = WorldRandomStream::from_raw({0, 0, 34, 5});
    const auto single = prepare_world_task_offer(tape, 1, {true, false, 0}, identity());
    check(single.candidate && single.candidate->random_bounds == std::vector<int>{1, 49, 100, 6} &&
              single.candidate->state.recruitment.entries.size() == 1 &&
              single.candidate->state.recruitment.entries[0].counter == -28 &&
              single.candidate->state.recruitment.entries[0].phrase == 5 &&
              single.candidate->state.recruitment.arrival_extent == 105 &&
              single.candidate->state.recruitment.completion_tick == 235,
          "single human still shuffle draw1; exact interval jitter and phrase6 bounds/order");
    tape = fixture(3);
    tape.random = WorldRandomStream::from_raw({0, 0, 0, 0, 0, 5, 0, 0, 99});
    const auto three = prepare_world_task_offer(tape, 1, {true, false, 0}, identity());
    check(three.candidate &&
              three.candidate->random_bounds ==
                  std::vector<int>{3, 3, 3, 25, 100, 6, 25, 25, 100} &&
              three.candidate->state.recruitment.entries[0].human == 1 &&
              three.candidate->state.recruitment.entries[1].human == 0 &&
              three.candidate->state.recruitment.entries[2].human == 2 &&
              three.candidate->state.recruitment.entries[1].phrase == -1,
          "source full-list swaps then reverse-three; a phrase suppresses next probability draw");
    for (int available = 0; available < 11; ++available) {
        auto s = fixture(available);
        for (auto &h : s.humans) {
            h.legacy_u = 100;
            h.event_participations = 3;
        }
        s.random = WorldRandomStream::from_raw(std::vector<std::int32_t>(500, 0));
        const auto result = prepare_world_task_offer(s, 1, {true, false, 0}, identity());
        check(result.candidate && result.candidate->state.recruitment.entries.size() <= 9 &&
                  result.candidate->state.recruitment.entries.size() >=
                      static_cast<std::size_t>(std::min(available, 3)),
              "mandatory min3 and upper9 retained across zero/small/large catalogue");
    }
    auto late = fixture();
    late.random = WorldRandomStream::from_raw({0});
    const auto rejected = prepare_world_task_offer(late, 1, {true, false, 0}, identity());
    check(rejected.error == TaskCommandError::random_failed && !rejected.candidate &&
              late.random.draws() == 0 && late.finish.dungeon.world.ai.accounting.funds() == 5000,
          "late recruitment RNG exhaustion rolls back already prepared fee and page reset");
}
void recruitment_and_extra() {
    auto s = fixture(5);
    const auto offer = prepare_world_task_offer(s, 1, {true, false, 0}, identity());
    check(offer.candidate.has_value(), "recruitment setup accepted");
    s = offer.candidate->state;
    s.recruitment.entries = {{3, -1, -1}, {1, -2, -1}};
    s.recruitment.completion_tick = 3;
    auto first = prepare_world_task_recruitment(s, true, identity());
    check(first.candidate && first.candidate->state.page_counter == 2 &&
              first.candidate->state.secondary_counter == 2 &&
              first.candidate->state.recruitment.entries[0].counter == 1 &&
              first.candidate->state.recruitment.entries[1].counter == 0 &&
              first.candidate->state.recruitment.portraits == std::vector<int>{3, 1} &&
              first.candidate->state.recruitment.displayed_count == 1 &&
              first.candidate->state.recruitment.portrait_timer == 8,
          "page24 held advances counters twice; each negative crossing appends Y; ar ages after "
          "arrivals");
    bool observed_old_team{};
    const auto observer = [&](const auto &current,
                              const auto &effect) -> std::optional<WorldTaskCommandState> {
        if (effect.kind == TaskCommandEffectKind::event && effect.first == 9)
            observed_old_team = current.finish.participants == std::vector<int>{0} &&
                                current.finish.dungeon.world.ai.battle.humans.at(3).task_kills == 7;
        return current;
    };
    auto finish = prepare_world_task_recruitment(first.candidate->state, false, observer);
    check(finish.candidate && observed_old_team &&
              finish.candidate->state.finish.participants == std::vector<int>{3, 1} &&
              finish.candidate->state.finish.dungeon.world.ai.battle.participants ==
                  std::vector<int>{3, 1} &&
              finish.candidate->state.phase == TaskCommandPhase::team &&
              finish.candidate->effects[0].text == "1",
          "recruit result event(as) sees old m/H/I before new definition order replaces team");
    const auto &humans = finish.candidate->state.finish.dungeon.world.ai.battle.humans;
    check(humans.at(3).task_kills == 0 && humans.at(3).participant_downs == 0 &&
              humans.at(3).recent_reward == 9 && humans.at(3).recent_kills == 10 &&
              humans.at(0).task_kills == 7 &&
              finish.candidate->state.finish.human_definition_flags.at(3) == (8U | 2U),
          "recruitment e.o resets selected H/I only, not J/K, nonselected records or flags2");
    s = finish.candidate->state;
    s.humans[0].profession_levels = {2, 3, 4};
    s.humans[4].profession_levels = std::vector<int>(30, 10);
    const auto extra = prepare_world_task_extra_candidates(s, identity());
    check(extra.candidate && extra.candidate->state.extra_candidates == std::vector<int>{0, 2, 4} &&
              extra.candidate->state.humans[0].extra_fee == 500 &&
              extra.candidate->state.humans[4].extra_fee == 3000 &&
              extra.candidate->state.phase == TaskCommandPhase::extra,
          "page27 current bv order excludes team and recomputes200+50 per profession extra level "
          "capped3000");
    const auto cancelled =
        prepare_world_task_hire(extra.candidate->state, -99, {false, true, 0}, identity());
    check(cancelled.candidate && cancelled.candidate->state.phase == TaskCommandPhase::team &&
              cancelled.candidate->state.finish.participants == s.finish.participants &&
              cancelled.candidate->state.finish.dungeon.world.ai.accounting.funds() == 3700,
          "extra cancel does not require a selected human and never charge/mutate team");
    bool observed_hired{};
    const auto hired_observer = [&](const auto &current,
                                    const auto &effect) -> std::optional<WorldTaskCommandState> {
        if (effect.kind == TaskCommandEffectKind::event && effect.first == 64)
            observed_hired = current.finish.participants == std::vector<int>{3, 1, 0} &&
                             current.finish.dungeon.world.ai.battle.humans.at(0).task_kills == 0 &&
                             (current.finish.human_definition_flags.at(0) & 2U) &&
                             effect.second == 1 && effect.human == 0;
        return current;
    };
    const auto hired =
        prepare_world_task_hire(extra.candidate->state, 0, {true, false, 1}, hired_observer);
    check(hired.candidate && hired.candidate->accepted && observed_hired &&
              hired.candidate->state.finish.dungeon.world.ai.accounting.funds() == 3200 &&
              hired.candidate->state.phase == TaskCommandPhase::team,
          "hire source confirm ignores selection, charges first then H/I/m/bit2 then64 j1/k and "
          "close27");
    check(
        !prepare_world_task_hire(hired.candidate->state, 0, {true, false, 0}, identity()).candidate,
        "closed extra page cannot repeat hiring/payment");
    auto poor = extra.candidate->state;
    poor.finish.dungeon.world.ai.accounting = PeriodAccounting{499};
    const auto denied = prepare_world_task_hire(poor, 0, {true, false, 0}, identity());
    check(denied.candidate && denied.candidate->denial == TaskCommandDenial::insufficient_funds &&
              denied.candidate->state.finish.participants == poor.finish.participants &&
              denied.candidate->state.finish.dungeon.world.ai.battle.humans.at(0).task_kills == 7,
          "insufficient extra fee only notice11, keeps current m/H/I and phase27");
    auto full = fixture(10);
    full.phase = TaskCommandPhase::team;
    full.finish.participants = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    const auto limited = prepare_world_task_extra_candidates(full, identity());
    check(limited.candidate && limited.candidate->denial == TaskCommandDenial::team_full &&
              limited.candidate->effects.size() == 1 && limited.candidate->effects[0].first == 63,
          "nine-person guard before opening27 emits63 without candidate prices/RNG");
}
void departure() {
    auto s = fixture(3);
    s.phase = TaskCommandPhase::team;
    s.selected_task = 1;
    s.finish.participants = {2, 0};
    s.finish.dungeon.world.ai.battle.participants = s.finish.participants;
    s.active_task_updates = 88;
    s.task_subperiods = 9;
    const auto opened = prepare_world_task_departure_page(s, identity());
    check(opened.candidate && opened.candidate->state.predicted_result == 6 &&
              opened.candidate->state.phase == TaskCommandPhase::departure_prompt &&
              !opened.candidate->state.finish.active_task,
          "kind0 poor prediction maps to6; opening28 never starts task or resets global p");
    const auto nope =
        prepare_world_task_departure(opened.candidate->state, {true, false, 1}, identity());
    const auto cancel =
        prepare_world_task_departure(opened.candidate->state, {false, true, 0}, identity());
    check(nope.candidate && cancel.candidate &&
              nope.candidate->state.phase == TaskCommandPhase::team &&
              cancel.candidate->state.phase == TaskCommandPhase::team &&
              !nope.candidate->state.finish.active_task &&
              cancel.candidate->state.active_task_updates == 88,
          "page28 selected no and prompt cancel close28 without task/p/facility/flags changes");
    auto begun =
        prepare_world_task_departure(opened.candidate->state, {true, false, 0}, identity());
    check(begun.candidate && begun.candidate->state.phase == TaskCommandPhase::departing &&
              begun.candidate->state.page_counter == 0 &&
              !begun.candidate->state.finish.active_task,
          "confirm yes enters animation i1, resets f124d; no early task installation");
    auto waiting = begun.candidate->state;
    waiting.page_counter = 38;
    const auto too_early = prepare_world_task_departure(waiting, {true, false, 0}, identity());
    const auto accelerated =
        prepare_world_task_departure(too_early.candidate->state, {true, false, 0}, identity());
    check(too_early.candidate && !too_early.candidate->state.accelerate_departure &&
              accelerated.candidate && accelerated.candidate->state.accelerate_departure,
          "page28 extra confirm accelerates only after unified prefix reaches bi1=40");
    waiting = begun.candidate->state;
    waiting.page_counter = 94;
    const auto before = prepare_world_task_departure(waiting, {false, true, 0}, identity());
    check(before.candidate && before.candidate->state.page_counter == 95 &&
              before.candidate->state.phase == TaskCommandPhase::departing &&
              !before.candidate->state.finish.active_task,
          "departing cancel ignored and pre96 never installs task");
    bool switched_before_install{}, notice_before_y_reset{};
    const auto observer = [&](const auto &current,
                              const auto &effect) -> std::optional<WorldTaskCommandState> {
        if (effect.kind == TaskCommandEffectKind::activate_main_scene)
            switched_before_install =
                !current.finish.active_task && current.active_task_updates == 88;
        if (effect.kind == TaskCommandEffectKind::notice15)
            notice_before_y_reset = current.finish.active_task == 1 &&
                                    current.active_task_updates == 0 &&
                                    current.task_subperiods == 9 &&
                                    current.finish.dungeon.world.facilities.at(10).status == 1 &&
                                    current.humans[2].absence == 0 &&
                                    current.humans[1].absence == 4 && effect.text == "任务名称夹具";
        return current;
    };
    const auto completed = prepare_world_task_departure(before.candidate->state, {}, observer);
    check(completed.candidate && completed.candidate->departed && switched_before_install &&
              notice_before_y_reset && completed.candidate->state.task_subperiods == 0 &&
              completed.candidate->state.finish.dungeon.world.ai.task_active &&
              completed.candidate->state.phase == TaskCommandPhase::closed,
          "96 actual order activate-main -> h/p0 -> facility1 -> flags/absence -> notice15 -> y0 "
          "-> close28");
    const auto &progress = completed.candidate->state.finish.dungeon.facilities.at(10);
    check(progress.updates == 0 && progress.progress == 0 && progress.extent == 14400 &&
              progress.percent == 78 && progress.previous_percent == 79 &&
              completed.candidate->state.finish.human_definition_flags.at(0) == (8U | 2U) &&
              completed.candidate->state.finish.human_definition_flags.at(1) == 8U &&
              completed.candidate->state.finish.human_definition_flags.at(2) == (8U | 2U),
          "Tenant.b1 uses actualQ80/l5 extent; preserves Y/Z; all definitions clear2 then selected "
          "set2");
    check(completed.candidate->state.finish.participants == std::vector<int>{2, 0} &&
              completed.candidate->state.finish.dungeon.world.ai.accounting.funds() == 5000 &&
              completed.candidate->state.random.draws() == 0,
          "formal start preserves party order, does not repay recruitment or draw random");
    auto missing = before.candidate->state;
    missing.facility_difficulties.clear();
    check(
        !prepare_world_task_departure(missing, {}, observer).candidate &&
            !missing.finish.active_task && missing.active_task_updates == 88 &&
            missing.finish.dungeon.world.facilities.at(10).status == 8,
        "late missing facility input gives no partially active task after candidate scene switch");
    const auto reject_notice = [](const auto &current,
                                  const auto &effect) -> std::optional<WorldTaskCommandState> {
        if (effect.kind == TaskCommandEffectKind::notice15)
            return {};
        return current;
    };
    check(
        prepare_world_task_departure(before.candidate->state, {}, reject_notice).error ==
                TaskCommandError::consumer_failed &&
            !before.candidate->state.finish.active_task,
        "notice15 failure rolls back task/facility/flags/absence and main-scene request together");
}
void further_boundaries() {
    for (const int probability : {14, 15}) {
        auto boundary = fixture(4);
        auto raw = std::vector<std::int32_t>(100, 99);
        std::fill(raw.begin(), raw.begin() + 4, 0);
        raw[4] = probability;
        boundary.random = WorldRandomStream::from_raw(raw);
        const auto selected = prepare_world_task_offer(boundary, 1, {true, false, 0}, identity());
        check(selected.candidate && selected.candidate->state.recruitment.entries.size() ==
                                        static_cast<std::size_t>(probability == 14 ? 4 : 3),
              "fourth entrant probability15 is strict-less after all full-list shuffle draws");
    }
    auto decreasing = fixture(5);
    for (auto &h : decreasing.humans) {
        h.legacy_u = 100;
        h.event_participations = 3;
    }
    auto raw = std::vector<std::int32_t>(100, 99);
    std::fill(raw.begin(), raw.begin() + 5, 0);
    raw[5] = 59;
    raw[6] = 49;
    decreasing.random = WorldRandomStream::from_raw(raw);
    const auto five = prepare_world_task_offer(decreasing, 1, {true, false, 0}, identity());
    check(five.candidate && five.candidate->state.recruitment.entries.size() == 5 &&
              five.candidate->state.recruitment.entries[3].human == 0 &&
              five.candidate->state.recruitment.entries[4].human == 4,
          "successful fourth entrant changes next threshold60 to50 with reverse remainder order");
    const auto reject_open = [](const auto &current,
                                const auto &effect) -> std::optional<WorldTaskCommandState> {
        return effect.kind == TaskCommandEffectKind::open_page
                   ? std::nullopt
                   : std::optional<WorldTaskCommandState>{current};
    };
    const auto source = fixture();
    check(prepare_world_task_offer(source, 1, {true, false, 0}, reject_open).error ==
                  TaskCommandError::consumer_failed &&
              source.random.draws() == 0 &&
              source.finish.dungeon.world.ai.accounting.funds() == 5000,
          "late page24 consumer failure rolls back fee and successful random selection");
    auto none = fixture(3);
    none.phase = TaskCommandPhase::team;
    none.finish.participants = {0, 1, 2};
    const auto no_candidates = prepare_world_task_extra_candidates(none, identity());
    check(no_candidates.candidate &&
              no_candidates.candidate->denial == TaskCommandDenial::no_extra_candidates &&
              kinds(*no_candidates.candidate) ==
                  std::vector<TaskCommandEffectKind>{TaskCommandEffectKind::open_page,
                                                     TaskCommandEffectKind::event,
                                                     TaskCommandEffectKind::close_page} &&
              no_candidates.candidate->effects[1].first == 63,
          "empty extra27 opens then message63 then closes, not a paid or silently empty page");
    auto start = fixture(3);
    start.phase = TaskCommandPhase::departing;
    start.selected_task = 1;
    start.page_counter = 95;
    for (const int difficulty : {0, 1, 5, 9, 10}) {
        auto extent = start;
        extent.facility_difficulties.at(10) = difficulty;
        const auto finished = prepare_world_task_departure(extent, {}, identity());
        const int expected = difficulty <= 1 ? 9600 : difficulty >= 9 ? 19200 : 14400;
        check(finished.candidate &&
                  finished.candidate->state.finish.dungeon.facilities.at(10).extent == expected,
              "Tenant.b1 exact clamped l1..9 mapping Q*120..Q*240 includes out-of-range l");
    }
    auto jump = start;
    jump.accelerate_departure = true;
    jump.page_counter = 88;
    const auto jumped = prepare_world_task_departure(jump, {}, identity());
    jump.page_counter = 89;
    const auto stable = prepare_world_task_departure(jump, {}, identity());
    check(jumped.candidate && jumped.candidate->state.page_counter == 94 && stable.candidate &&
              stable.candidate->state.page_counter == 90 &&
              !jumped.candidate->state.finish.active_task,
          "prefixed89 acceleration can overshoot to94; prefixed90 does not add5");
    auto ordinary = start;
    ordinary.page_counter = 0;
    ordinary.accelerate_departure = false;
    for (int frame = 0; frame < 96; ++frame) {
        const auto result = prepare_world_task_departure(ordinary, {}, identity());
        check(result.candidate && result.candidate->departed == (frame == 95),
              "all96 actual departure frames retain delayed installation until final source guard");
        ordinary = result.candidate->state;
    }
    auto combat = fixture(3);
    combat.phase = TaskCommandPhase::team;
    combat.selected_task = 1;
    combat.finish.task_progress.definitions.at(0).kind = 1;
    combat.finish.tasks.at(1).facility.reset();
    const auto combat_page = prepare_world_task_departure_page(combat, identity());
    check(combat_page.candidate && combat_page.candidate->state.predicted_result != 6,
          "battle kind preserves low prediction0/1 instead of exploration remap6");
    auto combat_start = combat_page.candidate->state;
    combat_start.phase = TaskCommandPhase::departing;
    combat_start.page_counter = 95;
    const auto no_facility = prepare_world_task_departure(combat_start, {}, identity());
    check(no_facility.candidate && no_facility.candidate->departed &&
              no_facility.candidate->state.finish.dungeon.world.facilities.at(10).status == 8,
          "task without Tenant starts h/flags and preserves unrelated facility state");
}
} // namespace
int main() {
    try {
        offers_and_random();
        recruitment_and_extra();
        departure();
        further_boundaries();
        std::cout << "world task command checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
