#include "ark/simulation/combat/rules/encounter_creation.hpp"

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
AiRewardState fixture() {
    AiRewardState s;
    s.next_actor_id = 10;
    for (int id : {0, 1, 2}) {
        RewardMonsterDefinition d;
        d.growth = id == 0 ? 8 : 0;
        d.base_hp = 20;
        d.base_death_reward = 10;
        d.base_cash_reward = 10;
        d.status = id == 0 ? 1 : 0;
        d.has_introduction_script = true;
        s.monster_growth.emplace(id, d);
        s.battle.monsters.emplace(id, MonsterBattleRecord{0, 0, 10, 10, 1, 0U});
        s.monster_definition_order.push_back(id);
    }
    BattleActorRecord actor;
    actor.id = {1};
    actor.control.state = 5;
    s.battle.actors.emplace(actor.id, actor);
    s.contexts.emplace(actor.id, RewardActorContext{{10, 10}, false, {}, {}});
    s.human_order = {actor.id};
    return s;
}
EncounterCreationInput input() {
    EncounterCreationInput i;
    i.center = {10, 10};
    i.year_index = 0;
    i.month_index = 3;
    i.count_ticket = 99;
    i.nearby_ticket = 0;
    i.monsters = {{0, {0, 99}}};
    return i;
}
void guards() {
    for (int x = -6; x <= 6; ++x)
        for (int y = -6; y <= 6; ++y)
            for (int kind = 0; kind <= 2; ++kind) {
                auto s = fixture();
                RewardEncounter e;
                e.runtime = {0, {10 + x, 10 + y}, 2, 0, 0, 0, 0, 0};
                s.encounters.emplace(0, e);
                auto i = input();
                i.kind = kind;
                const auto c = prepare_encounter_creation(s, i);
                const int radius = kind == 0 ? 4 : 2;
                const bool denial = kind != 1 && std::abs(x) <= radius && std::abs(y) <= radius;
                check(c.candidate && c.candidate->created.has_value() == !denial &&
                          c.candidate->consumed_count == (kind == 0 && !denial),
                      "ordinary closed radius4, quest radius2, retained bypasses overlap");
                check(c.candidate->state.encounters.size() == (denial ? 1U : 2U) &&
                          s.encounters.size() == 1 &&
                          (denial ||
                           c.candidate->state.encounters.at(1).runtime.state == (kind == 0   ? 0
                                                                                 : kind == 1 ? 2
                                                                                             : 3)),
                      "guard denials no allocation, distinct runtime0/2/3 initial states");
            }
    auto s = fixture();
    auto i = input();
    i.upper_band_town = {false, true, false};
    auto c = prepare_encounter_creation(s, i);
    check(c.candidate && c.candidate->denial == EncounterCreationDenial::town &&
              !c.candidate->consumed_count && c.candidate->state.next_encounter_id == 1,
          "common upper band town gate before count/ID draws");
    i = input();
    i.probe = EncounterCreationProbe{{1}, false, 0, 0, 4, {{12, 12}}};
    c = prepare_encounter_creation(s, i);
    check(c.candidate && c.candidate->denial == EncounterCreationDenial::task_overlap &&
              c.candidate->consumed_probe && !c.candidate->consumed_count,
          "L draw successful, k.a still rejects every bq center at closed radius2");
    i.probe->task_centers.clear();
    i.probe->logical_state = 3;
    c = prepare_encounter_creation(s, i);
    check(c.candidate && c.candidate->denial == EncounterCreationDenial::cell,
          "L success not sufficient, source current logical state4 required");
    i.probe->logical_state = 4;
    s.battle.actors.at({1}).control.state = 0;
    c = prepare_encounter_creation(s, i);
    check(c.candidate && c.candidate->denial == EncounterCreationDenial::probe &&
              c.candidate->consumed_probe,
          "read resulting live state0: probability0 still consumes1000");
    s.battle.actors.at({1}).control.state = 5;
    i.probe->destination_inside_town = true;
    i.probe->ticket.reset();
    c = prepare_encounter_creation(s, i);
    check(c.candidate && !c.candidate->consumed_probe,
          "early destination-town guard neither needs nor consumes probe ticket");
}
void creation() {
    for (int count_ticket = 0; count_ticket < 100; ++count_ticket) {
        auto s = fixture();
        auto i = input();
        i.count_ticket = count_ticket;
        const auto c = prepare_encounter_creation(s, i);
        check(c.candidate && c.candidate->created == 1 && c.candidate->consumed_count &&
                  c.candidate->consumed_nearby && c.candidate->consumed_definitions == 1 &&
                  c.candidate->consumed_offsets == 2 &&
                  c.candidate->state.monster_order.size() == 1,
              "early first-year override happens after both count draws, one actual spawn");
        check(c.candidate->state.monster_growth.at(1).status == 1 &&
                  c.candidate->state.monster_growth.at(1).newly_unlocked &&
                  c.candidate->state.monster_growth.at(1).introduced &&
                  c.candidate->state.battle.actors.at({10}).definition == 1 &&
                  c.candidate->state.encounters.at(1).runtime.spawned == 1 &&
                  c.candidate->requests[0].kind ==
                      EncounterCreationRequestKind::definition_script &&
                  c.candidate->requests[1].kind == EncounterCreationRequestKind::page89 &&
                  c.candidate->requests.back().kind == EncounterCreationRequestKind::refresh_map,
              "unlock participates immediately, intro before spawn, original requests ordered");
    }
    auto s = fixture();
    s.monster_limit = 1;
    auto i = input();
    i.year_index = 4;
    i.monsters = {{0, {0, 99}}, {0, {20, 30}}, {0, {40, 50}}, {0, {60, 70}}};
    auto c = prepare_encounter_creation(s, i);
    check(c.candidate && c.candidate->state.monster_order.size() == 4 &&
              c.candidate->state.encounters.at(1).runtime.spawned == 4 &&
              c.candidate->consumed_offsets == 8 && c.candidate->requests.size() == 3,
          "total-limit precheck not a batch cap, repeated chosen definition intro once");
    i.monsters[3].offset_tickets[1] = 100;
    c = prepare_encounter_creation(s, i);
    check(!c.candidate && s.encounters.empty() && s.monster_order.empty() &&
              s.monster_growth.at(1).status == 0 && s.next_actor_id == 10 &&
              s.next_encounter_id == 1,
          "late fourth-spawn failure rolls back event, earlier spawns/unlock/intro and both IDs");
    i = input();
    s.task_active = true;
    c = prepare_encounter_creation(s, i);
    check(c.candidate && c.candidate->requests.size() == 1 &&
              !c.candidate->state.monster_growth.at(1).introduced,
          "active task suppresses normal definition introduction without suppressing spawn");
    s.legacy_encounter_counter = std::numeric_limits<int>::max() - 1;
    i.kind = 1;
    c = prepare_encounter_creation(s, i);
    check(c.candidate && c.candidate->state.encounters.at(1).legacy_id == 0 &&
              c.candidate->state.encounters.at(1).runtime.quota == 0,
          "event source ID wraps to0, retained creation doesn't invent quota or monsters");
}
void synchronous_intro() {
    const auto source = fixture();
    std::vector<EncounterCreationRequestKind> order;
    const auto result = prepare_encounter_creation(
        source, input(),
        [&](const AiRewardState &current,
            const EncounterCreationRequest &request) -> std::optional<AiRewardState> {
            order.push_back(request.kind);
            check(current.monster_order.size() ==
                      (request.kind == EncounterCreationRequestKind::refresh_map ? 1U : 0U),
                  "source introduction and page89 occur before current monster spawn");
            auto next = current;
            if (request.kind == EncounterCreationRequestKind::definition_script)
                next.pending_completion = 37;
            if (request.kind == EncounterCreationRequestKind::page89)
                check(next.pending_completion == 37,
                      "next typed request sees actual script mutation");
            return next;
        });
    check(result.candidate && result.candidate->state.pending_completion == 37 &&
              order ==
                  std::vector<EncounterCreationRequestKind>{
                      EncounterCreationRequestKind::definition_script,
                      EncounterCreationRequestKind::page89,
                      EncounterCreationRequestKind::refresh_map},
          "one synchronous sequence consumes actual candidate requests in source order");
    const auto rejected = prepare_encounter_creation(
        source, input(),
        [](const AiRewardState &current,
           const EncounterCreationRequest &request) -> std::optional<AiRewardState> {
            if (request.kind == EncounterCreationRequestKind::page89)
                return {};
            return current;
        });
    check(!rejected.candidate && source.monster_order.empty() &&
              source.monster_growth.at(1).status == 0 && source.pending_completion == 0,
          "late synchronous page failure exposes no spawn/unlock/script candidate");
}
} // namespace
int main() {
    try {
        guards();
        creation();
        synchronous_intro();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
