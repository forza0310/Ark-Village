#include "dungeon_village_prototype/startup_world_visuals.hpp"

#include <algorithm>

namespace dungeon_village_prototype {
std::optional<StartupPortrait> startup_world_portrait(const StartupWorldRuntimeState &s, int id) {
    if (!s.rules || id < 0 || id >= static_cast<int>(s.rules->humans.size()))
        return {};
    const auto &human = s.rules->humans.at(id);
    const auto growth = s.scene.world.world.ai.growth.find(id);
    if (human.identity != id || growth == s.scene.world.world.ai.growth.end())
        return {};
    const int job = growth->second.definition.current_profession;
    if (job < 0 || job >= static_cast<int>(s.rules->jobs.size()) || human.sex < 0 || human.sex > 1)
        return {};
    return StartupPortrait{s.rules->jobs.at(job).sprites.at(human.sex)};
}
std::optional<std::vector<StartupInnRow>> startup_world_inn_rows(const StartupWorldRuntimeState &s,
                                                                 std::uint64_t id) {
    const auto &world = s.scene.world.world;
    const auto facility = world.facilities.find(id);
    if (facility == world.facilities.end())
        return {};
    std::vector<StartupInnRow> rows;
    if (facility->second.category != 2)
        return rows;
    const auto &occupants = facility->second.occupants;
    for (std::size_t n = 0; n < std::min<std::size_t>(4, occupants.size()); ++n) {
        const auto actor = world.ai.battle.actors.find(occupants[n]);
        if (actor == world.ai.battle.actors.end())
            return {};
        const auto &a = actor->second;
        if (!(a.control.flags & 32U))
            continue;
        const auto portrait = startup_world_portrait(s, a.definition);
        const auto growth = world.ai.growth.find(a.definition);
        if (!portrait || growth == world.ai.growth.end() || a.state_counter < 0)
            return {};
        const int capacity = growth->second.derived.combat[0]; // 原h()读取当前共享人物派生值。
        if (capacity <= 0)
            return {};
        const bool healing = a.state_counter >= 170;
        const auto value =
            healing ? std::clamp(a.hp.displayed, 0, capacity) : std::clamp(a.state_counter, 0, 170);
        const auto bar = static_cast<int>(static_cast<std::int64_t>(value) * (healing ? 26 : 30) /
                                          (healing ? capacity : 170));
        rows.push_back({a.id, *portrait, static_cast<int>(rows.size()), healing, bar, capacity});
    }
    return rows;
}
} // namespace dungeon_village_prototype
