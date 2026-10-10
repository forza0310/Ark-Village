#include "ark/simulation/actors/startup_world_profile.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"

#include <algorithm>
#include <cstdint>

namespace ark::simulation {
namespace {
// 4096字节为维护载荷预算，非原平台的12字符/全角6文字限制。
bool valid_name(const std::string &text) {
    if (text.size() > 4096)
        return false;
    for (std::size_t at = 0; at < text.size();) {
        const auto lead = static_cast<unsigned char>(text[at++]);
        std::uint32_t code{};
        int rest{};
        std::uint32_t minimum{};
        if (lead < 0x80) {
            code = lead;
        } else if (lead >= 0xc2 && lead <= 0xdf) {
            code = lead & 0x1fU; rest = 1; minimum = 0x80;
        } else if (lead >= 0xe0 && lead <= 0xef) {
            code = lead & 0x0fU; rest = 2; minimum = 0x800;
        } else if (lead >= 0xf0 && lead <= 0xf4) {
            code = lead & 0x07U; rest = 3; minimum = 0x10000;
        } else {
            return false;
        }
        while (rest-- > 0) {
            if (at == text.size())
                return false;
            const auto next = static_cast<unsigned char>(text[at++]);
            if ((next & 0xc0U) != 0x80U)
                return false;
            code = (code << 6U) | (next & 0x3fU);
        }
        if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff) ||
            code < 0x20 || (code >= 0x7f && code <= 0x9f))
            return false;
    }
    return true; // 原空串资格未闭合；不擅自补非空规则。
}
const StartupWorldHuman *definition(const StartupWorldRules *rules, int id) {
    if (!rules || id < 0)
        return nullptr;
    const auto found = std::find_if(rules->humans.begin(), rules->humans.end(),
                                    [id](const auto &h) { return h.identity == id; });
    return found == rules->humans.end() ? nullptr : &*found;
}
} // namespace
bool valid_startup_world_human_profile(const StartupWorldHumanProfile &p) {
    return p.sex >= 0 && p.sex <= 1 && valid_name(p.name);
}
bool valid_startup_world_human_profiles(const StartupWorldRuntimeState &s) {
    if (!s.rules || s.human_profiles.size() > 1)
        return false;
    for (const auto &[id, profile] : s.human_profiles)
        if (id != 0 || !definition(s.rules, id) || !valid_startup_world_human_profile(profile))
            return false;
    return true;
}
std::optional<StartupWorldHumanProfile>
startup_world_human_profile(const StartupWorldRuntimeState &s, int id) {
    if (!valid_startup_world_human_profiles(s))
        return {};
    const auto source = definition(s.rules, id);
    if (!source)
        return {};
    const auto override = s.human_profiles.find(id);
    if (override != s.human_profiles.end())
        return override->second;
    return startup_world_human_profile(*s.rules, id);
}
std::optional<StartupWorldHumanProfile>
startup_world_human_profile(const StartupWorldRules &rules, int id) {
    const auto source = definition(&rules, id);
    if (!source)
        return {};
    StartupWorldHumanProfile value{source->name, source->sex, false};
    return valid_startup_world_human_profile(value) ? std::optional<StartupWorldHumanProfile>{value}
                                                   : std::nullopt;
}
bool install_startup_world_main_character(StartupWorldRuntimeState &s,
                                         const StartupWorldHumanProfile &p) {
    if (s.rules != &startup_world_rules() || !definition(s.rules, 0) ||
        !valid_startup_world_human_profile(p) || !s.human_profiles.empty() ||
        s.simulation_steps != 0 || s.scene.world.updates != 0 || s.global_updates != 0)
        return false;
    const auto scripts = s.scripts.humans.find(0);
    if (scripts == s.scripts.humans.end())
        return false;
    const auto &ai = s.scene.world.world.ai;
    for (const auto *actors : {&ai.battle.actors, &ai.retired_actors})
        for (const auto &[id, actor] : *actors) {
            (void)id;
            if (actor.kind == ref::ActorKind::human && actor.definition == 0)
                return false;
        }
    s.human_profiles.emplace(0, p);
    scripts->second.name = p.name; // 持久脚本目录缓存与唯一定义覆盖同次安装。
    return true;
}
} // namespace ark::simulation
