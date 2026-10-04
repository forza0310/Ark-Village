#include "dungeon_village_reference/world_equipment_display.hpp"

#include <algorithm>
#include <utility>

namespace dungeon_village_reference {
WorldEquipmentDisplayResult prepare_world_equipment_display(const RescueWorldState &s,
                                                            const WorldEquipmentDisplayInput &i) {
    const auto fail = [](WorldEquipmentDisplayError e) -> WorldEquipmentDisplayResult {
        return {e, {}};
    };
    const auto a = s.ai.battle.actors.find(i.actor);
    if (!i.actor.value || a == s.ai.battle.actors.end() || !(a->second.id == i.actor) ||
        a->second.kind != ActorKind::human || !s.ai.contexts.count(i.actor) ||
        !s.actors.count(i.actor) ||
        std::count(s.ai.human_order.begin(), s.ai.human_order.end(), i.actor) != 1)
        return fail(WorldEquipmentDisplayError::stale_actor);
    const auto &request = i.request;
    const auto &command = request.command;
    if (request.kind != ShopWorldRequestKind::equipment_display || !valid_actor_control(command) ||
        (command[0] != 27 && command[0] != 29) || request.first != command[0] ||
        request.second != 0 || command[2] < 0 || (command[0] == 27 && command[1] < 0) ||
        (command[0] == 29 && command[1] != 21 && command[1] != 22) ||
        !valid_actor_effect_state(s.ai.contexts.at(i.actor).effects))
        return fail(WorldEquipmentDisplayError::invalid_input);
    WorldEquipmentDisplayCandidate c{s, {}};
    if (command[0] == 27) {
        if (!i.cached_view)
            return fail(WorldEquipmentDisplayError::missing_cached_view);
        // cx={0,20},cy={8,32,40}: a(2000,8)->571,b(2000,8)->-71; cq.back=24.
        c.appended.push_back(
            {15, 0, i.cached_view->x, i.cached_view->y, command[1], command[2], 571, -71});
        c.appended.push_back({7, -8, 0, 0}); // -(40/2 - 24/2), cd not ce, no c(7,x,z).
    } else {
        // cI={16,28},cJ={12,22}: a(1200,12)->218,b(1200,12)->-18.
        c.appended.push_back({command[1], 0, 218, -18, command[2]});
    }
    auto &effects = c.state.ai.contexts.at(i.actor).effects;
    effects.display.insert(effects.display.end(), c.appended.begin(), c.appended.end());
    if (!valid_actor_effect_state(effects))
        return fail(WorldEquipmentDisplayError::invalid_input);
    return {WorldEquipmentDisplayError::none, std::move(c)};
}
} // namespace dungeon_village_reference
