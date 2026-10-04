#include "dungeon_village_reference/object_commit.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
ObjectCommitState fixture(int kind) {
    ObjectCommitState s;
    s.catalog.emplace(std::make_pair(kind, 4), ObjectCatalogRecord{32U, 0, 8, false, 0, 0});
    s.shops = {{1, {1, {{7, 3}}}}, {2, {1, {{2, 8}}}}, {3, {4, {}}}, {4, {5, {}}}};
    s.shop_order = {4, 2, 1, 3};
    return s;
}
void grants() {
    for (int stock = 0; stock <= 999; ++stock)
        for (int status : {0, 1, 2}) {
            auto s = fixture(0);
            auto &d = s.catalog.at({0, 4});
            d.inventory = stock;
            d.status = status;
            s.item_rewards = 9;
            const auto c = prepare_object_grant(s, 0, 4, ObjectGrantOrigin::ground_pickup);
            check(c.candidate &&
                      c.candidate->state.catalog.at({0, 4}).inventory == std::min(stock + 1, 999) &&
                      c.candidate->state.catalog.at({0, 4}).status == (status == 0 ? 1 : status),
                  "item cap999 and p0-only unlock, don't coerce existing item p2");
            check(c.candidate->state.catalog.at({0, 4}).unlock_counter == (status == 0 ? 0 : 8) &&
                      c.candidate->state.catalog.at({0, 4}).newly_unlocked == (status == 0) &&
                      c.candidate->state.item_rewards == 10 && c.candidate->state.events.count(151),
                  "first item unlock clears q, E increments even at stock cap");
            check(c.candidate->requests[0].kind == ObjectCommitRequestKind::notice &&
                      c.candidate->requests[1].kind == ObjectCommitRequestKind::grant &&
                      s.catalog.at({0, 4}).inventory == stock && s.events.empty(),
                  "ground item notice before grant and private owner unchanged");
        }
    for (int kind = 1; kind <= 3; ++kind)
        for (int status : {0, 1, 2}) {
            auto s = fixture(kind);
            auto &d = s.catalog.at({kind, 4});
            d.status = status;
            d.free_purchases = 7;
            auto c = prepare_object_grant(s, kind, 4, ObjectGrantOrigin::ground_pickup);
            check(c.candidate && c.candidate->state.catalog.at({kind, 4}).status == 1 &&
                      c.candidate->state.catalog.at({kind, 4}).free_purchases ==
                          (status == 0 ? 1 : 7) &&
                      c.candidate->state.catalog.at({kind, 4}).unlock_counter == 8 &&
                      c.candidate->state.item_rewards == 0 && !c.candidate->state.events.count(110),
                  "equipment only first unlock refills free count, pickup never event110");
            check(c.candidate->state.events.count(216) == (kind == 1 && status == 0) &&
                      c.candidate->state.shops.at(1).notices[0][1] == 3 &&
                      c.candidate->requests.front().kind == ObjectCommitRequestKind::grant &&
                      c.candidate->requests.back().kind == ObjectCommitRequestKind::notice,
                  "weapon32 nested event216, c7 dedup preserves timer, grant before notice");
            const auto repeated =
                prepare_object_grant(c.candidate->state, kind, 4, ObjectGrantOrigin::ground_pickup);
            check(repeated.candidate && repeated.candidate->requests.size() == 2 &&
                      repeated.candidate->state.catalog.at({kind, 4}).free_purchases ==
                          (status == 0 ? 1 : 7),
                  "repeat equip grant doesn't refill or re-notify shops");
        }
    auto s = fixture(0);
    s.item_rewards = std::numeric_limits<int>::max() - 1;
    auto c = prepare_object_grant(s, 0, 4, ObjectGrantOrigin::direct);
    check(c.candidate && c.candidate->state.item_rewards == 0 &&
              c.candidate->state.events.empty() &&
              c.candidate->requests.front().kind == ObjectCommitRequestKind::grant,
          "E modulo and direct item grants before notice");
    c = prepare_object_grant(fixture(1), 1, 4, ObjectGrantOrigin::direct);
    check(c.candidate && c.candidate->state.events.count(110) &&
              c.candidate->requests.back().parameter == 110,
          "direct weapon110 after notice, distinct from ground pickup");
}
void ticks() {
    for (int kind = 0; kind <= 3; ++kind) {
        auto s = fixture(kind);
        auto o = prepare_ground_drop({0}, {100, 0, 100}, kind, 4);
        o->state = 5;
        s.objects.emplace(0, *o);
        int rewards{};
        for (int tick = 1; tick <= 60; ++tick) {
            const auto c = prepare_object_update(s, {0}, true);
            check(c.candidate && c.candidate->remove == (tick == 60),
                  "ground update erases at60 only");
            if (!c.candidate->requests.empty()) {
                ++rewards;
                check(tick == 20 &&
                          c.candidate->requests[0].kind == ObjectCommitRequestKind::ground_effect,
                      "exact j20 and ground effect precedes grant");
            }
            s = c.candidate->state;
        }
        check(rewards == 1 && s.objects.empty() && s.item_rewards == (kind == 0 ? 1 : 0),
              "complete lifecycle grants once by exact counter, not death idempotence");
    }
    auto s = fixture(0);
    auto o = *prepare_ground_drop({5}, {0, 0, 0}, 0, 99);
    o.state = 5;
    o.counter = 19;
    s.objects.emplace(5, o);
    check(prepare_object_update(s, {5}, false).error == ObjectCommitError::missing_definition &&
              s.objects.at(5).counter == 19 && s.item_rewards == 0,
          "late catalog failure keeps counter and all owner fields unchanged");
    s.objects.at(5).id = {6};
    check(prepare_object_update(s, {5}, false).error == ObjectCommitError::stale_object,
          "map key and stable object identity must agree");
}
void picking() {
    BattleCommitState world;
    BattleActorRecord actor;
    actor.id = {1};
    actor.control.state = 11;
    actor.control.flags = 2U;
    actor.baseline = 5;
    actor.state_counter = 10;
    actor.state_parameter = 20;
    actor.position = {200, 0, 200};
    actor.encounter = 0;
    world.actors.emplace(actor.id, actor);
    auto object = *prepare_ground_drop({0}, {200, 0, 200}, 0, 4);
    object.state = 3;
    world.objects.emplace(0, object);
    const auto c = prepare_ground_pickup_commit(world, {{1}, {0}, false, true, 0, {}});
    check(c.candidate && c.candidate->action == PickupAction::pickup &&
              c.candidate->state.actors.at({1}).control.state == 12 &&
              c.candidate->state.actors.at({1}).state_counter == 0 &&
              c.candidate->state.objects.at(0).state == 5 &&
              c.candidate->state.objects.at(0).counter == 0 &&
              c.candidate->state.objects.at(0).position.x == 180 &&
              c.candidate->state.objects.at(0).cached_cell == Position{2, 2} &&
              c.candidate->state.actors.at({1}).object_slot == -1 &&
              !c.candidate->state.actors.at({1}).encounter && world.objects.at(0).state == 3,
          "contact atomically claims object, resets actor12, retains cached cell and N, no grant");
    auto second = actor;
    second.id = {2};
    auto next = c.candidate->state;
    next.actors.emplace(second.id, second);
    const auto later = prepare_ground_pickup_commit(next, {{2}, {0}, false, true, 0, {}});
    check(later.candidate && later.candidate->action == PickupAction::baseline &&
              later.candidate->state.actors.at({2}).state_counter == 10 &&
              later.candidate->state.actors.at({2}).state_parameter == 20 &&
              later.candidate->state.actors.at({2}).control.queue ==
                  std::vector<LegacyActorControl>{{10, 0}},
          "later actor re-queries H, cannot claim state5; b keeps B/C and enqueues wander");
    const auto chase = prepare_ground_pickup_commit(world, {{1}, {0}, false, false, 0, {}});
    check(chase.candidate && chase.candidate->chase_target &&
              chase.candidate->state.objects.at(0).state == 3,
          "chasing doesn't reserve object, grant or teleport");
    world.actors.at({1}).object_slot = -2;
    const auto battle = prepare_ground_pickup_commit(world, {{1}, {999}, true, false, 100, 0});
    check(battle.candidate && battle.candidate->action == PickupAction::battle_prepare &&
              battle.candidate->state.actors.at({1}).control.state == 18 &&
              battle.candidate->state.events.count(116) &&
              battle.candidate->state.actors.at({1}).object_slot == -2 &&
              battle.candidate->expression == 6 && world.events.empty(),
          "F precedes H/carry guard, state18 boost transaction and expression6 request separate");
    check(!prepare_ground_pickup_commit(world, {{1}, {999}, true, false, 100, {}}).candidate,
          "missing state18 sub-consumer ticket rejects all local changes");
    // Sole world object owner is projected for the inventory consumer only during this transaction.
    auto inventory = fixture(0);
    inventory.objects = c.candidate->state.objects;
    auto control = c.candidate->state.actors.at({1}).control;
    int sounds{};
    int sound_tick{};
    for (int tick = 1; tick <= 60; ++tick) {
        for (;;) {
            const auto prefix = prepare_local_control_prefix(control);
            check(prefix.candidate.has_value(), "pickup control prefix valid");
            control = prefix.candidate->state;
            if (prefix.candidate->flow != ActorControlFlow::delegated)
                break;
            check(control.queue.front()[0] == 33,
                  "only pickup sound is delegated in this interval");
            control.queue.erase(control.queue.begin());
            ++sounds;
            sound_tick = tick;
            check(inventory.catalog.at({0, 4}).inventory == 1,
                  "object j20 grant already occurred before later opcode33 sound");
        }
        const auto grant = prepare_object_update(inventory, {0}, false);
        check(grant.candidate && grant.candidate->remove == (tick == 60),
              "claimed object updates same round after actor d and deletes60");
        inventory = grant.candidate->state;
        check(inventory.catalog.at({0, 4}).inventory == (tick >= 20 ? 1 : 0),
              "contact/control/sound never shift exact j20 inventory grant");
    }
    check(sounds == 1 && sound_tick == 21 && inventory.objects.empty(),
          "wait10 then12 consumes shared boundary d, sound21 vs reward20 and deletion60");
}
} // namespace
int main() {
    try {
        grants();
        ticks();
        picking();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
