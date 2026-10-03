#include "dungeon_village_reference/object_ai.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
bool has(const GroundObjectStepCandidate &c, ObjectRewardRequestKind k) {
    for (const auto &r : c.requests)
        if (r.kind == k)
            return true;
    return false;
}
void drops() {
    DropSelectionInput i;
    i.luck = 100;
    i.progress = 5;
    i.rank_ticket = 59;
    i.equipment_ticket = 14;
    i.definitions = {
        {1, 0, 1, 2, 1}, {2, 1, 8, 2, 0}, {3, 1, 1, 2, 1}, {4, 2, 1, 2, 2}, {5, 3, 1, 2, 0}};
    for (int ticket = 0; ticket < 3; ++ticket) {
        i.selection_ticket = ticket;
        auto c = prepare_drop_selection(i).candidate;
        check(c && c->maximum_rank == 8 && c->equipment_branch && c->consumed_selection &&
                  c->selected &&
                  c->selected->id == (ticket == 0   ? 2
                                      : ticket == 1 ? 4
                                                    : 5),
              "equipment combines original weapon/armor/accessory, excludes only unlocked1");
    }
    i.equipment_ticket = 15;
    i.selection_ticket = 0;
    auto c = prepare_drop_selection(i).candidate;
    check(c && !c->equipment_branch && c->selected->kind == 0,
          "equipment threshold15 excluded, item ignores unlocked1");
    i.equipment_ticket = 0;
    i.definitions = {{1, 0, 1, 2, 0}, {2, 1, 1, 2, 1}};
    c = prepare_drop_selection(i).candidate;
    check(c && c->equipment_branch && c->selected->kind == 0, "empty equipment falls back to item");
    i.definitions.clear();
    i.selection_ticket.reset();
    check(prepare_drop_selection(i).candidate &&
              !prepare_drop_selection(i).candidate->consumed_selection &&
              !prepare_drop_selection(i).candidate->selected,
          "empty branch has no extra random and no fabricated drop");
    for (int luck : {-100, 5, 50, 100, 999})
        for (int progress : {-1, 0, 3, 5, 9})
            for (int ticket = 0; ticket < 60; ++ticket) {
                i.luck = luck;
                i.progress = progress;
                i.rank_ticket = ticket;
                c = prepare_drop_selection(i).candidate;
                check(c && c->maximum_rank >= 1 && c->maximum_rank <= 8,
                      "rank inputs clamp, reachable range1..8 not assumed9");
            }
    i.rank_ticket = 60;
    check(prepare_drop_selection(i).error == ObjectError::invalid_ticket,
          "mandatory rank random checked without candidate");
    i.rank_ticket = 0;
    i.definitions = {{1, 2, 0, 2, 0}, {2, 1, 0, 2, 0}};
    check(prepare_drop_selection(i).error == ObjectError::invalid_input,
          "malformed category ordering refused, not sorted by arbitrary ID");
}
void lifecycle() {
    auto s = prepare_ground_drop({0}, {150, 0, 250}, 0, 1);
    check(s && s->state == 6 && s->delay == 20 && s->cached_cell == Position{1, 2},
          "death drop retains original ID0 and cached whole cell, waits20");
    for (int t = 1; t <= 20; ++t) {
        const auto c = advance_ground_object(*s, true, 0, false).candidate;
        check(c && !c->remove && c->state.state == (t == 20 ? 3 : 6) &&
                  c->state.counter == (t == 20 ? 0 : t) && c->state.inside_town,
              "object new tick20 enters3/reset counter and no grant");
        s = c->state;
    }
    auto thrown = prepare_ground_throw({1}, {50, 0, 50}, {150, 0, 50}, 0, 1);
    for (int t = 1; t <= 26; ++t) {
        const auto c = advance_ground_object(*thrown, false, 0, false).candidate;
        check(c && c->state.state == (t == 26 ? 3 : 2) && c->state.position.height >= 0 &&
                  c->state.cached_cell == Position{0, 0},
              "thrown26 moves but cached town cell not recomputed");
        thrown = c->state;
    }
    for (int kind = 0; kind < 4; ++kind)
        for (int old : {0, 18, 19, 20, 58, 59, 60}) {
            s->state = 5;
            s->counter = old;
            s->kind = kind;
            const auto c = advance_ground_object(*s, false, 9, false).candidate;
            check(c && c->remove == (old >= 59) &&
                      has(*c, ObjectRewardRequestKind::grant_definition) == (old == 19) &&
                      has(*c, ObjectRewardRequestKind::event151) == (old == 19 && kind == 0),
                  "grant exactly new20, remove new60, event151 only ordinary tenth item");
            if (old == 19) {
                check(c->requests[0].kind == ObjectRewardRequestKind::pickup_ground_effect &&
                          c->requests[1].kind == (kind == 0
                                                      ? ObjectRewardRequestKind::notice
                                                      : ObjectRewardRequestKind::grant_definition),
                      "notice/grant order differs item/equipment");
            }
        }
    s->kind = 0;
    s->counter = 19;
    check(!has(*advance_ground_object(*s, false, 9, true).candidate,
               ObjectRewardRequestKind::event151),
          "event151 duplicate suppression");
    auto wrap =
        advance_ground_object(*s, false, std::numeric_limits<int>::max() - 1, false).candidate;
    check(wrap && !has(*wrap, ObjectRewardRequestKind::event151) &&
              wrap->requests[3].parameter == 0,
          "global collected count modulo INT_MAX wraps0");
}
void pickup() {
    const std::vector<ObjectProbe> objects = {
        {{1}, 3, {4, 0, 0}}, {{2}, 3, {1, 0, 0}}, {{3}, 3, {1, 0, 0}}, {{4}, 6, {0, 0, 0}}};
    check(select_ground_object({}, objects).selected == ObjectId{3},
          "reverse strict exchange selects later nearest tie, excludes state6");
    auto tied = objects;
    tied[0].position.x = 1;
    check(select_ground_object({}, tied).selected == ObjectId{1}, "already minimum head keeps tie");
    PickupInput i;
    i.actor_position = {150, 10, 250};
    i.nearest = prepare_ground_drop({3}, {100, 0, 200}, 0, 1);
    i.nearest->state = 3;
    i.nearest->counter = 9;
    i.battle_ready = true;
    i.carried_slot = -2;
    auto c = prepare_ground_pickup(i).candidate;
    check(c && c->action == PickupAction::battle_prepare && c->expression == 6 &&
              c->actor_state == 18,
          "F priority precedes carried slot guard");
    i.battle_ready = false;
    check(prepare_ground_pickup(i).candidate->action == PickupAction::baseline,
          "carrying rescue prevents ground pickup");
    i.carried_slot = -1;
    c = prepare_ground_pickup(i).candidate;
    check(c && c->action == PickupAction::chase && c->object->state == 3,
          "chasing does not claim object before collision");
    i.touching = true;
    c = prepare_ground_pickup(i).candidate;
    check(c && c->action == PickupAction::pickup && c->actor_state == 12 && c->facing == 2 &&
              c->object->state == 5 && c->object->counter == 0 && c->object->position.x == 130 &&
              c->object->position.z == 230 && c->object->position.height == 0 &&
              c->object->cached_cell == Position{1, 2},
          "touch claims object5 before actor12, positions -20/-20 without cached-cell update");
    const std::vector<LegacyActorControl> expected = {
        {6, 64},   {7, 2}, {1, 10, 0}, {3, 10},    {6, 2},  {1, 12, 0}, {33},
        {1, 6, 0}, {3, 0}, {7, 2},     {1, 30, 0}, {7, 64}, {6, 2}};
    check(c->queue == expected, "pickup full queue, sound33 not item grant");
    auto object = *c->object;
    int grant_count{};
    for (int tick = 1; tick <= 60; ++tick) {
        auto step = advance_ground_object(object, false, 0, false).candidate;
        grant_count += has(*step, ObjectRewardRequestKind::grant_definition) ? 1 : 0;
        check(step->remove == (tick == 60), "picked object persists after reward20 until60");
        object = step->state;
    }
    check(grant_count == 1, "one reward by object update, not character sound/finish");
    i.nearest->state = 5;
    check(prepare_ground_pickup(i).error == ObjectError::stale_object,
          "second character cannot claim object already state5");
}
} // namespace
int main() {
    try {
        drops();
        lifecycle();
        pickup();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
