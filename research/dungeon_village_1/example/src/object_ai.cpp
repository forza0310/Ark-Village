#include "dungeon_village_reference/object_ai.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dungeon_village_reference {
namespace {
bool point_valid(CombatPoint p) {
    return std::isfinite(p.x) && std::isfinite(p.height) && std::isfinite(p.z) &&
           std::abs(p.x) <= 1000000 && std::abs(p.height) <= 1000000 && std::abs(p.z) <= 1000000;
}
bool valid(const GroundObjectState &s) {
    return s.state >= 0 && s.state <= 6 && s.counter >= 0 &&
           s.counter < std::numeric_limits<int>::max() && s.duration >= 0 && s.delay >= 0 &&
           s.kind >= 0 && s.kind <= 3 && s.definition >= 0 && point_valid(s.position) &&
           point_valid(s.velocity) && point_valid(s.acceleration);
}
} // namespace
std::optional<GroundObjectState> prepare_ground_drop(ObjectId id, CombatPoint position, int kind,
                                                     int definition) {
    GroundObjectState s;
    s.id = id;
    s.state = 6;
    s.delay = 20;
    s.kind = kind;
    s.definition = definition;
    s.position = position;
    const auto cell = character_world_cell({position.x, position.z});
    if (!valid(s) || !cell)
        return std::nullopt;
    s.cached_cell = *cell;
    return s;
}
std::optional<GroundObjectState> prepare_ground_throw(ObjectId id, CombatPoint origin,
                                                      CombatPoint destination, int kind,
                                                      int definition) {
    auto s = prepare_ground_drop(id, origin, kind, definition);
    if (!s || !point_valid(destination))
        return std::nullopt;
    s->state = 2;
    s->delay = 0;
    s->duration = 26;
    s->velocity = {(destination.x - origin.x) / 26.0F, 80.0F / 12.0F,
                   (destination.z - origin.z) / 26.0F};
    s->acceleration.height = -80.0F / (12 * 13);
    return s;
}
DropSelectionResult prepare_drop_selection(const DropSelectionInput &i) {
    const auto equipment_ticket = i.draw ? i.draw(100) : std::optional<int>{i.equipment_ticket};
    if (!equipment_ticket)
        return {ObjectError::missing_ticket, std::nullopt};
    if (*equipment_ticket < 0 || *equipment_ticket >= 100)
        return {ObjectError::invalid_ticket, std::nullopt};
    const auto rank_ticket = i.draw ? i.draw(60) : std::optional<int>{i.rank_ticket};
    if (!rank_ticket)
        return {ObjectError::missing_ticket, std::nullopt};
    if (*rank_ticket < 0 || *rank_ticket >= 60)
        return {ObjectError::invalid_ticket, std::nullopt};
    int last_kind{};
    for (const auto &d : i.definitions) {
        if (d.id < 0 || d.kind < last_kind || d.kind > 3 || d.rank < 0 || d.unlocked < 0)
            return {ObjectError::invalid_input, std::nullopt};
        last_kind = d.kind;
    }
    const int luck = (std::clamp(i.luck, 5, 100) - 5) * 35 / 95;
    const int progress = std::clamp(i.progress, 0, 5) * 35 / 5;
    const int maximum = 1 + std::clamp(luck + progress + *rank_ticket - 30, 0, 100) * 8 / 100;
    const bool equipment = *equipment_ticket < 15;
    std::vector<DropDefinition> items, equipment_candidates;
    for (const auto &d : i.definitions)
        if ((d.flags & 2U) && d.rank <= maximum) {
            if (d.kind == 0)
                items.push_back(d);
            else if (equipment && d.unlocked != 1)
                equipment_candidates.push_back(d);
        }
    const auto &choices = equipment && !equipment_candidates.empty() ? equipment_candidates : items;
    DropSelectionCandidate c{maximum, equipment, false, std::nullopt};
    if (choices.empty())
        return {ObjectError::none, c};
    if (choices.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {ObjectError::invalid_input, std::nullopt};
    const auto selection_ticket = i.selection_ticket ? i.selection_ticket
                                  : i.draw           ? i.draw(static_cast<int>(choices.size()))
                                                     : std::optional<int>{};
    if (!selection_ticket)
        return {ObjectError::missing_ticket, std::nullopt};
    if (*selection_ticket < 0 || static_cast<std::size_t>(*selection_ticket) >= choices.size())
        return {ObjectError::invalid_ticket, std::nullopt};
    c.selected = choices[static_cast<std::size_t>(*selection_ticket)];
    c.consumed_selection = true;
    return {ObjectError::none, c};
}
GroundObjectStepResult advance_ground_object(const GroundObjectState &s, bool town, int count,
                                             bool event_present) {
    if (!valid(s) || count < 0 || count >= std::numeric_limits<int>::max())
        return {ObjectError::invalid_input, std::nullopt};
    GroundObjectStepCandidate c{s, false, {}};
    c.state.inside_town = town;
    ++c.state.counter;
    auto state = [&c](int next) {
        c.state.state = next;
        c.state.counter = 0;
    };
    if (s.state == 2) {
        c.state.velocity.x += s.acceleration.x;
        c.state.velocity.height += s.acceleration.height;
        c.state.velocity.z += s.acceleration.z;
        c.state.position.x += c.state.velocity.x;
        c.state.position.height += c.state.velocity.height;
        c.state.position.z += c.state.velocity.z;
        if (c.state.position.height < 0) {
            c.state.position.height = 0;
            c.state.velocity.height = 0;
            c.state.acceleration.height = 0;
        }
        if (c.state.counter >= s.duration)
            state(3);
    } else if (s.state == 6) {
        if (c.state.counter >= s.delay)
            state(3);
    } else if (s.state == 5) {
        if (c.state.counter == 20) {
            c.requests.push_back({ObjectRewardRequestKind::pickup_ground_effect, 0});
            // Source preserves different notice/grant ordering for ordinary items vs equipment.
            if (s.kind == 0) {
                c.requests.push_back({ObjectRewardRequestKind::notice, 2});
                c.requests.push_back({ObjectRewardRequestKind::grant_definition, s.definition});
            } else {
                c.requests.push_back({ObjectRewardRequestKind::grant_definition, s.definition});
                c.requests.push_back({ObjectRewardRequestKind::notice, s.kind == 1 ? 10 : 11});
            }
            if (s.kind == 0) {
                const int new_count = (count + 1) % std::numeric_limits<int>::max();
                c.requests.push_back({ObjectRewardRequestKind::item_count, new_count});
                if (new_count >= 10 && !event_present)
                    c.requests.push_back({ObjectRewardRequestKind::event151, 151});
            }
        }
        c.remove = c.state.counter >= 60;
    }
    if (!point_valid(c.state.position) || !point_valid(c.state.velocity))
        return {ObjectError::invalid_input, std::nullopt};
    return {ObjectError::none, c};
}
ObjectSelectionResult select_ground_object(CombatPoint actor,
                                           const std::vector<ObjectProbe> &objects) {
    if (!point_valid(actor))
        return {ObjectError::invalid_input, std::nullopt};
    std::optional<ObjectId> selected;
    float minimum = std::numeric_limits<float>::max();
    // Only the first outer sorting pass determines the selected head; preserve reverse tie order.
    std::vector<const ObjectProbe *> ready;
    for (const auto &o : objects) {
        if (!point_valid(o.position) || o.state < 0 || o.state > 6)
            return {ObjectError::invalid_input, std::nullopt};
        if (o.state == 3)
            ready.push_back(&o);
    }
    auto distance = [actor](const ObjectProbe &o) {
        const float dx = o.position.x - actor.x, dz = o.position.z - actor.z;
        return static_cast<float>(std::sqrt(static_cast<double>(dx * dx + dz * dz)));
    };
    if (!ready.empty()) {
        selected = ready.front()->id;
        minimum = distance(*ready.front());
        for (std::size_t index = ready.size(); index-- > 1;)
            if (const float d = distance(*ready[index]); d < minimum) {
                minimum = d;
                selected = ready[index]->id;
            }
    }
    return {ObjectError::none, selected};
}
PickupResult prepare_ground_pickup(const PickupInput &i) {
    if (!point_valid(i.actor_position) || i.carried_slot < -2)
        return {ObjectError::invalid_input, std::nullopt};
    PickupCandidate c;
    if (i.battle_ready) {
        c.action = PickupAction::battle_prepare;
        c.actor_state = 18;
        c.expression = 6;
        return {ObjectError::none, c};
    }
    if (!i.nearest || i.carried_slot != -1)
        return {ObjectError::none, c};
    if (!valid(*i.nearest) || i.nearest->state != 3)
        return {ObjectError::stale_object, std::nullopt};
    c.object = i.nearest;
    if (!i.touching) {
        c.action = PickupAction::chase;
        return {ObjectError::none, c};
    }
    c.action = PickupAction::pickup;
    c.actor_state = 12;
    c.facing = 2;
    c.object->state = 5;
    c.object->counter = 0;
    c.object->position.x = i.actor_position.x - 20.0F;
    c.object->position.z = i.actor_position.z - 20.0F;
    c.queue = {{6, 64},   {7, 2}, {1, 10, 0}, {3, 10},    {6, 2},  {1, 12, 0}, {33},
               {1, 6, 0}, {3, 0}, {7, 2},     {1, 30, 0}, {7, 64}, {6, 2}};
    return {ObjectError::none, c};
}
} // namespace dungeon_village_reference
