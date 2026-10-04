// Real-source initial interval, ticket-controlled private owner; not an APK PRNG replay.
#include "dungeon_village_prototype/startup_ai.hpp"

#include <iostream>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
StartupSession installed() {
    StartupSession s;
    for (int i = 0; i < 420; ++i)
        check(s.update() == StartupError::none, "actual eligible startup step");
    check(s.state().character && s.state().event89_count == 1 &&
              s.state().mode == StartupMode::tutorial,
          "visitor installed before introduction, no completion of tutorial assumed");
    return s;
}
void sources() {
    const auto &r = startup_ai_rules();
    check(r.professions.size() == 23 && r.weapons.size() == 33,
          "all source professions/weapons compiled");
    check(r.weapons[0].price == 400 && r.weapons[0].selection.unlocked &&
              r.weapons[1].selection.unlocked && !r.weapons[2].selection.unlocked,
          "short sword400 and source initial weapon unlocks");
    check(r.first_definition.spell_professions == std::vector<int>{5, 12, 18, 6},
          "spell slots follow first profession for each v modulo10, not job ID sorting");
    const auto stats = ref::derive_human_stats(r.first_definition, r.professions);
    check(stats.candidate && stats.candidate->combat == startup_evidence().first_character.combat &&
              stats.candidate->attributes == startup_evidence().first_character.attributes,
          "growth module reproduces actual first visitor initial attributes and equipment");
}
void interval(ref::Position birth, int category, int facility, int expected_first) {
    const auto startup = installed();
    StartupAiSession ai(startup, birth);
    StartupAiTickets tickets{category, facility, 0, 0, std::nullopt};
    const auto first_error = ai.round(tickets);
    if (first_error != StartupAiError::none || !ai.state().journey ||
        ai.state().journey->binding.definition_id != expected_first)
        std::cerr << "birth " << birth.x << ',' << birth.y << " error "
                  << static_cast<int>(first_error) << " selected "
                  << (ai.state().journey ? ai.state().journey->binding.definition_id : -1)
                  << " expected " << expected_first << '\n';
    check(first_error == StartupAiError::none && ai.state().departures == 1 &&
              ai.state().journey->binding.definition_id == expected_first &&
              ai.state().accounting.funds() == 5000,
          "same admitted first-round d consumes activity0; ordinary priority and two-level "
          "selection");
    // Next selection uses fresh weights and excludes the just-used instance when still inside it.
    tickets.category = 0;
    tickets.facility = 0;
    int arrival_round{};
    int exit_round{};
    for (int round = 2; round <= 1000; ++round) {
        const auto before = ai.state();
        const auto error = ai.round(tickets);
        check(error == StartupAiError::none, "actual source interval tick candidate accepted");
        const auto &s = ai.state();
        if (s.arrivals == 1 && arrival_round == 0) {
            arrival_round = round;
            check(s.active_facility && s.control.state == 14 && s.occupations == 1 &&
                      s.counters.state == 1 &&
                      s.accounting.funds() == (expected_first == 30 ? 5400 : 5300),
                  "c logical arrival precedes same-round d counter1 occupation and wait decrement");
            check(s.visits.legacy_actor_total == (expected_first == 30 ? 400 : 300),
                  "actor total distinct from wallet, equipment price substitutes store price");
        }
        if (s.completions == 0 && arrival_round > 0) {
            check(s.accounting.entries().size() == 1, "waiting never charges again");
            if (expected_first == 28)
                check(s.recoveries == (round - arrival_round >= 170 ? 1 : 0),
                      "old B170 recovery at pair171");
        }
        if (s.completions == 1 && exit_round == 0) {
            exit_round = round;
            const int duration = expected_first == 28 ? 200 : 60;
            check(exit_round - arrival_round + 1 == duration && s.departures == 2 && s.journey &&
                      !s.active_facility && s.control.state == 0 &&
                      s.uses.at(expected_first).completed_uses == 1,
                  "exit24 then successful8 selects next activity in same d, preserves tail");
            for (const auto &entry : s.facilities)
                check(entry.second.occupants.empty(), "released previous occupation");
            check(s.attribute_commits == 0 && s.equipment_commits == 0,
                  "next departure happens before any pending attribute/equipment commit");
            if (expected_first == 33)
                check(s.control.queue.back()[0] == 19 && s.satisfaction == 1,
                      "actual neighbour quality and farming threshold, attribute delayed behind "
                      "activity");
        }
        if (exit_round && round > exit_round) {
            if (expected_first == 33 && s.attribute_commits == 1) {
                check(s.definition.extra[0] == 1 &&
                          s.stats.combat == startup_evidence().first_character.combat,
                      "opcode19 accumulates baseHP attribute but does not eagerly recompute combat "
                      "or heal");
                check(s.hp.target == 22, "attribute gain does not directly alter current HP");
                return;
            }
            if (expected_first == 28 && !s.presentation_requests.empty()) {
                check(s.presentation_requests.back() == ref::LegacyActorControl{18, 9, 0},
                      "inn expression remains separate presentation request after departure");
                return;
            }
            if (expected_first == 30 && s.equipment_commits == 1) {
                // First wait reaching0 continues into the second wait in the SAME d().
                check(round - exit_round == 46 && s.current_weapon == 0 &&
                          s.weapon_reselect_counter == 6 &&
                          s.stats.combat == startup_evidence().first_character.combat,
                      "weapon display then waits5+42, commit28 re-equips real short sword and "
                      "recalculates");
                return;
            }
        }
        check(s.rounds == before.rounds + 1, "whole admitted round submitted once");
    }
    throw std::runtime_error("initial AI interval did not close within bounded rounds");
}
void rollback() {
    const auto startup = installed();
    StartupAiSession ai(startup, startup_evidence().spawn_points[0]);
    const auto before = ai.state();
    check(ai.round({10000, 0, 0, 0, {}}) == StartupAiError::preparation_failed &&
              ai.state().rounds == before.rounds &&
              ai.state().control.queue == before.control.queue &&
              ai.state().position.x == before.position.x && ai.state().accounting.funds() == 5000,
          "late invalid choice discards counters,effects,motion,queue and all owners");
    StartupAiTickets tickets{0, 5, 0, 1000, {}};
    check(ai.round(tickets) == StartupAiError::none, "rollback case selects actual food shop");
    tickets.facility = 0;
    for (int n = 0; n < 1000; ++n) {
        const auto saved = ai.state();
        const auto error = ai.round(tickets);
        if (error != StartupAiError::none) {
            check(saved.arrivals == 1 && saved.completions == 0 &&
                      ai.state().rounds == saved.rounds &&
                      ai.state().control.queue == saved.control.queue &&
                      ai.state().uses.at(33).completed_uses == 0 &&
                      ai.state()
                              .facilities.at(saved.active_facility->instance_id.value)
                              .occupants.size() == 1 &&
                      ai.state().accounting.funds() == saved.accounting.funds(),
                  "invalid exit tail ticket rolls back wait-to-zero,releasing,use and next "
                  "departure");
            return;
        }
    }
    throw std::runtime_error("rollback case never reached full exit");
}
} // namespace
int main() {
    try {
        sources();
        for (const auto birth : startup_evidence().spawn_points) {
            interval(birth, 0, 5, 33);
            interval(birth, 0, 0, 30);
            interval(birth, 40, 0, 28);
        }
        rollback();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
