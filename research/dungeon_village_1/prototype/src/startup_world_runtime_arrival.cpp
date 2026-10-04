#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_reference/world_arrivals.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
ref::WorldArrivalsState arrivals(const State &s) {
    ref::WorldArrivalsState r;
    r.finish = startup_world_runtime_finish(s);
    r.scripts = startup_world_runtime_scripts(s);
    r.random = s.scene.random;
    r.spawn_cells = s.scene.world.spawn_cells;
    r.arrival_counter = s.arrival_counter;
    r.camera_delay = s.camera_delay;
    r.camera_follow = s.camera_follow;
    for (const auto &h : s.rules->humans) {
        const auto &spells = s.scene.world.world.ai.growth.at(h.identity).derived.available_spells;
        r.definitions.push_back({h.identity, s.human_presence.at(h.identity),
                                 s.human_calendar.at(h.identity).absent_months,
                                 s.human_flags.at(h.identity),
                                 std::vector<bool>(spells.begin(), spells.end())});
    }
    return r;
}
bool write_arrivals(State &s, const ref::WorldArrivalsState &r) {
    if (!write_startup_world_runtime_finish(s, r.finish) ||
        !write_startup_world_runtime_scripts(s, r.scripts))
        return false;
    s.scene.random = r.random;
    s.arrival_counter = r.arrival_counter;
    s.camera_delay = r.camera_delay;
    s.camera_follow = r.camera_follow;
    for (const auto &h : r.definitions) {
        s.human_presence.at(h.identity) = h.presence;
        s.human_calendar.at(h.identity).absent_months = h.priority;
    }
    s.event89_count = ref::world_script_seen(s.scripts, 89) ? 1 : 0;
    return true;
}
std::optional<ref::CharacterId> create(State &s, const ref::WorldArrivalCreationInput &input) {
    auto &world = s.scene.world.world;
    auto &ai = world.ai;
    const auto definition =
        std::find_if(s.rules->humans.begin(), s.rules->humans.end(),
                     [&](const auto &h) { return h.identity == input.definition; });
    if (definition == s.rules->humans.end() || !ai.growth.count(input.definition) ||
        ai.next_actor_id == 0 || ai.next_actor_id == std::numeric_limits<std::uint64_t>::max())
        return {};
    const ref::CharacterId identity{ai.next_actor_id++};
    if (ai.battle.actors.count(identity) || ai.retired_actors.count(identity) ||
        s.actor_metadata.count(identity))
        return {};
    auto &human = ai.growth.at(input.definition);
    const auto old_capacity = human.derived.combat[0]; // d(e.f())先于a(e,v0)重新派生。
    auto &shop = s.shop_humans.at(input.definition);
    const int weapon = shop.equipment[0].value_or(-1);
    if (weapon == -1)
        human.definition.equipment[0].reset();
    else {
        const auto equipment =
            std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                         [&](const auto &e) { return e.shop.kind == 1 && e.shop.id == weapon; });
        if (equipment == s.rules->equipment.end())
            return {};
        human.definition.equipment[0] = equipment->shop.combat;
    }
    const auto derived = ref::derive_human_stats(human.definition, ai.professions);
    if (!derived.candidate)
        return {};
    human.derived = *derived.candidate;
    ai.battle.humans.at(input.definition).luck = human.derived.attributes[5];
    shop.reselect[0] = 6;
    // h()读取同定义共享w0，已有/被持有的实例上限随刷新变化，但不改它们当前HP或ae。
    for (auto &[id, old] : ai.battle.actors) {
        (void)id;
        if (old.kind == ref::ActorKind::human && old.definition == input.definition)
            old.capacity = human.derived.combat[0];
    }
    for (auto &[id, old] : ai.retired_actors) {
        (void)id;
        if (old.kind == ref::ActorKind::human && old.definition == input.definition)
            old.capacity = human.derived.combat[0];
    }
    ref::BattleActorRecord actor;
    actor.id = identity;
    actor.definition = input.definition;
    actor.legacy_id = input.legacy_uid;
    actor.kind = ref::ActorKind::human;
    actor.control.flags = 2;
    actor.control.queue = {{8, 0}};
    actor.hp = {0, old_capacity, old_capacity, old_capacity, false, 0};
    actor.capacity = human.derived.combat[0];
    actor.position = {input.spawn.x * 100.0f + 50.0f, 0, input.spawn.y * 100.0f + 50.0f};
    actor.attack_armed = false;
    actor.combo_count = 0;
    actor.perceived_distance = 0;
    ai.battle.actors.emplace(identity, actor);
    ai.human_order.push_back(identity);
    ref::RewardActorContext cache;
    cache.cell = input.spawn;
    cache.half_cell = {input.spawn.x * 2 + 1, input.spawn.y * 2 + 1};
    ai.contexts.emplace(identity, cache);
    ref::RescueActorContext context;
    context.destination = ref::Position{}; // Character.O全零，不创建虚拟设施binding。
    world.actors.emplace(identity, context);
    s.shop_actors.emplace(identity, ref::ShopActorRecord{weapon, {}, {}, {}});
    const float x = actor.position.x * 0.3f + actor.position.z * 0.3f;
    const float y = actor.position.x * -0.15f + actor.position.z * 0.15f;
    s.actor_metadata.emplace(identity,
                             StartupWorldActorMetadata{definition->sex,
                                                       human.definition.current_profession,
                                                       weapon,
                                                       {static_cast<int>(x), static_cast<int>(y)}});
    return identity;
}
} // namespace
void configure_startup_world_runtime_arrival_adapter(ref::WorldRuntimeAdapter<State> &a) {
    const auto catalog = a.catalog;
    a.arrival = [catalog](const State &s) -> std::optional<State> {
        State next = s;
        const auto result = ref::prepare_world_arrivals(
            arrivals(s), catalog,
            [&](const ref::WorldArrivalsState &current, const ref::WorldArrivalCreationInput &input)
                -> std::optional<ref::WorldArrivalCreation> {
                if (!write_arrivals(next, current))
                    return {};
                const auto created = create(next, input);
                return created
                           ? std::optional<ref::WorldArrivalCreation>{{arrivals(next), *created}}
                           : std::nullopt;
            });
        if (!result.candidate || !write_arrivals(next, result.candidate->state))
            return {};
        return next;
    };
}
} // namespace dungeon_village_prototype
