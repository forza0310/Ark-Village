#include "ark/simulation/startup_world_clear_score.hpp"
#include "ark/simulation/startup_world_runtime.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
// 最小独立调用点夹具；数值不是原版新局或自然16年路线。
StartupWorldRuntimeState fixture(const StartupWorldRules &rules) {
    StartupWorldRuntimeState s;
    s.rules = &rules;
    s.popularity = 101;
    s.task_progress.successes = 3;
    for (const auto &d : rules.facilities) s.facility_presence.emplace(d.id, 1);
    for (const auto &h : rules.humans) s.human_presence.emplace(h.identity, 1);
    auto &growth = s.scene.world.world.ai.growth;
    growth[0].definition.profession_levels = {1, 2, 3};
    growth[0].definition.legacy_u = 11;
    growth[1].definition.profession_levels = {4, 5, 6};
    growth[1].definition.legacy_u = 22;
    // 未开放的人物全部字段不参与计算，也不要求有活跃人物实例。
    s.human_presence[2] = 0;
    growth[2].definition.profession_levels = {999, 999, 999};
    growth[2].definition.legacy_u = 999;
    for (std::uint64_t id = 1; id <= 4; ++id) {
        s.scene.world.facility_order.push_back(id);
        s.scene.world.world.facilities[id].placement.definition_id = id <= 2 ? 12 : 2;
    }
    return s;
}
StartupWorldRules fixture_rules() {
    StartupWorldRules r;
    r.jobs.resize(3);
    for (const auto [id, kind] : std::array<std::array<int, 2>, 4>{{{2, 2}, {3, 3}, {12, 12}, {99, 0}}}) {
        StartupDefinition d;
        d.id = id; d.kind = kind;
        r.facilities.push_back(d);
    }
    for (int id = 0; id < 3; ++id) {
        StartupWorldHuman h; h.identity = id;
        r.humans.push_back(h);
    }
    return r;
}
void projection() {
    const auto rules = fixture_rules();
    auto s = fixture(rules);
    const auto result = startup_world_clear_score(s);
    check(result.error == StartupClearScoreError::none && result.candidate.has_value(), "valid projection");
    const std::array<std::int64_t, 6> counts{{101, 3, 2, 21, 2, 33}};
    const std::array<std::int64_t, 6> scores{{505, 900, 1000, 1050, 2000, 330}};
    for (std::size_t i = 0; i < 6; ++i)
        check((*result.candidate)[i].count == counts[i] && (*result.candidate)[i].score == scores[i], "independent six-category oracle");
    s.popularity = 16777217;
    auto binary = startup_world_clear_score(s);
    check(binary.candidate && (*binary.candidate)[0].score == 83886080, "binary32 differs from integer multiply");
    s.popularity = std::numeric_limits<int>::max();
    binary = startup_world_clear_score(s);
    check(binary.candidate && (*binary.candidate)[0].score == 10737418240LL, "binary32 rounding at int32 upper bound");
    auto missing = s;
    missing.facility_presence.erase(3);
    auto bad = startup_world_clear_score(missing);
    check(bad.error == StartupClearScoreError::missing_binding && !bad.candidate, "missing shared presence explicitly rejected");
    missing = s;
    missing.scene.world.world.ai.growth.erase(1);
    bad = startup_world_clear_score(missing);
    check(bad.error == StartupClearScoreError::missing_binding && !bad.candidate, "missing opened definition explicitly rejected");
    missing = s;
    missing.scene.world.facility_order.push_back(1);
    bad = startup_world_clear_score(missing);
    check(bad.error == StartupClearScoreError::invalid_input && !bad.candidate, "duplicate in-book instance rejected");
    missing = s;
    missing.scene.world.world.ai.growth[0].definition.profession_levels = {std::numeric_limits<int>::max(), 1, 0};
    bad = startup_world_clear_score(missing);
    check(bad.error == StartupClearScoreError::numeric_overflow && !bad.candidate, "int32 original accumulation overflow rejected safely");
    missing = s;
    missing.scene.world.world.ai.growth[0].definition.legacy_u = -1;
    bad = startup_world_clear_score(missing);
    check(bad.error == StartupClearScoreError::invalid_input && !bad.candidate, "negative effort rejected");
    check(s.scene.world.world.ai.growth.at(0).definition.legacy_u == 11, "read-only failures preserve owner");
}
StartupClearScorePageState finish(const StartupClearScoreRows &rows, std::int64_t high) {
    StartupClearScorePageState page;
    page.captured_high_score = high;
    int pulses{};
    for (int steps = 0; !page.finished && steps < 2500; ++steps) {
        const auto next = prepare_startup_clear_score_page(rows, page, true);
        check(next.error == StartupClearScoreError::none && next.candidate.has_value(), "page progression accepted");
        check(next.sounds.empty(), "source update has no direct sound emission");
        pulses += next.finished_pulse;
        page = *next.candidate;
    }
    check(page.finished && page.stage == 7 && page.counter == 0 && pulses == 1, "single completion pulse and terminal stage");
    const auto repeated = prepare_startup_clear_score_page(rows, page, true);
    check(repeated.error == StartupClearScoreError::already_finished && !repeated.candidate && !repeated.finished_pulse, "repeat completion rejected without output");
    return page;
}
void animation() {
    const auto rules = fixture_rules();
    const auto rows = *startup_world_clear_score(fixture(rules)).candidate;
    StartupClearScorePageState page;
    for (int i = 0; i < 100; ++i) page = *prepare_startup_clear_score_page(rows, page, false).candidate;
    check(page.stage == 0 && page.counter == 75 && page.sum == 0, "intro stops without confirmation");
    page = *prepare_startup_clear_score_page(rows, page, true).candidate;
    check(page.stage == 1 && page.counter == 0, "confirm is one Update pulse");
    for (int i = 0; i < 165; ++i) page = *prepare_startup_clear_score_page(rows, page, false).candidate;
    check(page.stage == 3 && page.counter == 45 && page.sum == 505, "row credited exactly at threshold");
    for (int i = 0; i < 20; ++i) page = *prepare_startup_clear_score_page(rows, page, false).candidate;
    check(page.stage == 3 && page.row == 0 && page.sum == 505, "row wait does not duplicate credit");
    StartupClearScorePageState early;
    early.stage = 3;
    const auto premature = prepare_startup_clear_score_page(rows, early, true);
    check(premature.candidate && premature.candidate->stage == 3 &&
              premature.candidate->counter == 1 && premature.candidate->sum == 0,
          "early confirmation cannot credit or advance a row");
    early.counter = 44;
    const auto exact = prepare_startup_clear_score_page(rows, early, true);
    check(exact.candidate && exact.candidate->stage == 5 && exact.candidate->counter == 0 &&
              exact.candidate->row == 1 && exact.candidate->sum == 505,
          "threshold credit precedes same-Update confirmation");
    auto bad_page = page;
    bad_page.row = 6;
    auto bad = prepare_startup_clear_score_page(rows, bad_page, true);
    check(bad.error == StartupClearScoreError::invalid_input && !bad.candidate, "out-of-range row rejected");
    bad_page = page;
    bad_page.sum = 0;
    bad = prepare_startup_clear_score_page(rows, bad_page, true);
    check(bad.error == StartupClearScoreError::invalid_input && !bad.candidate, "forged accumulated score rejected");
    auto forged_rows = rows;
    forged_rows[0].score = 506;
    bad = prepare_startup_clear_score_page(forged_rows, {}, false);
    check(bad.error == StartupClearScoreError::invalid_input && !bad.candidate, "invalid row score rejected before any advancement");
    const auto equal = finish(rows, 5785);
    check(equal.sum == 5785 && !equal.new_record && equal.trophy == 0, "equal high does not replace trophy or village");
    const auto lower = finish(rows, 6000);
    check(!lower.new_record && lower.trophy == 0, "lower high does not replace existing record");
    const auto high = finish(rows, 5784);
    check(high.new_record && high.trophy == 1, "strict new high bronze");
    for (const auto [total, trophy] : std::array<std::array<int, 2>, 6>{{{49995, 1}, {50000, 2}, {74995, 2}, {75000, 3}, {99995, 3}, {100000, 4}}}) {
        StartupClearScoreRows single{};
        single[0] = {total / 5, total};
        const auto ended = finish(single, 0);
        check(ended.sum == total && ended.new_record && ended.trophy == trophy, "independent trophy boundary table");
    }
}
} // namespace
int run_startup_world_clear_score_tests() {
    try {
        projection(); animation();
        std::cout << "clear score: " << checks << " checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
