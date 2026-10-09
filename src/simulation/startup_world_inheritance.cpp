#include "ark/simulation/startup_world_inheritance.hpp"
#include "ark/simulation/startup_world_building.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
bool short_values(const std::vector<std::uint8_t> &bytes, std::size_t count, int low, int high) {
    if (bytes.empty())
        return true;
    if (count > std::numeric_limits<std::size_t>::max() / 2 || bytes.size() != count * 2)
        return false;
    for (std::size_t n = 0; n < count; ++n) {
        const int bits = (static_cast<int>(bytes[2 * n]) << 8) | bytes[2 * n + 1];
        const int value = bits >= 0x8000 ? bits - 0x10000 : bits;
        if (value < low || value > high)
            return false;
    }
    return true;
}
int short_at(const std::vector<std::uint8_t> &bytes, std::size_t n) {
    return (static_cast<int>(bytes[2 * n]) << 8) | bytes[2 * n + 1]; // 已经完整值资格预检。
}
bool integer(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
} // namespace
bool install_startup_world_inheritance(StartupWorldRuntimeState &s,
                                       const std::array<std::vector<std::uint8_t>, 2> &sections) {
    if (s.rules != &startup_world_rules() || s.simulation_steps != 0 || s.global_updates != 0 ||
        s.scene.world.updates != 0 || !valid_startup_world_human_profiles(s) ||
        !short_values(sections[0], s.rules->facilities.size(), 1, 5) ||
        !short_values(sections[1], s.rules->jobs.size(), 0, 1))
        return false;
    const auto &world = s.scene.world.world;
    for (const auto &d : s.rules->facilities)
        if (!world.facility_uses.count(d.id) || !s.scripts.facilities.count(d.id))
            return false;
    for (std::size_t n = 0; n < s.rules->jobs.size(); ++n)
        if (!s.scripts.professions.count(static_cast<int>(n)) || n >= world.ai.professions.size())
            return false;
    for (const auto &d : s.rules->humans) {
        const auto growth = world.ai.growth.find(d.identity);
        if (growth == world.ai.growth.end() || !s.human_presence.count(d.identity))
            return false;
        const int job = growth->second.definition.current_profession;
        if (job < 0 || static_cast<std::size_t>(job) >= s.rules->jobs.size() ||
            s.rules->jobs[job].type < 0 || s.rules->jobs[job].type >= 10)
            return false;
    }
    for (const auto &entry : world.facilities) {
        const auto id = entry.first;
        const auto definition_id = entry.second.placement.definition_id;
        if (!s.neighbourhood.count(id) || !world.facility_uses.count(definition_id) ||
            std::none_of(s.rules->facilities.begin(), s.rules->facilities.end(), [&](const auto &d) {
                return d.id == definition_id;
            }))
            return false;
    }
    auto next = s; // 所有解码、重算及职业继承都在一个Owner私有候选。
    if (!sections[0].empty()) {
        for (std::size_t n = 0; n < s.rules->facilities.size(); ++n) {
            const auto &d = s.rules->facilities[n];
            const int level = short_at(sections[0], n);
            next.scene.world.world.facility_uses.find(d.id)->second.level = level;
            next.scripts.facilities.find(d.id)->second.level = level;
            const auto values = startup_world_build_quote(next, d.id);
            if (!values)
                return false;
            for (std::size_t slot = 0; slot < 4; ++slot) {
                if (!integer(values->definition_attributes[slot]))
                    return false;
                next.scripts.facilities.find(d.id)->second.attributes[slot] =
                    static_cast<int>(values->definition_attributes[slot]);
            }
            for (auto &[id, instance] : next.scene.world.world.facilities) {
                if (instance.placement.definition_id != d.id)
                    continue;
                const auto actual = startup_world_facility_values(next, id);
                if (!actual || !integer(actual->instance_attributes[0]))
                    return false;
                instance.price = static_cast<int>(actual->instance_attributes[0]);
            }
        }
        // 原每次G setter的k→e→h.d还刷新职业人数；不以职业开放p充当人数。
        if (!refresh_startup_world_profession_economy(next))
            return false;
    }
    if (!sections[1].empty())
        for (std::size_t n = 0; n < s.rules->jobs.size(); ++n) {
            const int status = short_at(sections[1], n);
            next.scripts.professions.find(static_cast<int>(n))->second.status = status;
            next.scene.world.world.ai.professions[n].unlocked = status != 0;
        }
    s = std::move(next);
    return true;
}
} // namespace ark::simulation
