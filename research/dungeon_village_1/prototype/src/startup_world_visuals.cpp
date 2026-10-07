#include "dungeon_village_prototype/startup_world_visuals.hpp"

#include <algorithm>
#include <limits>

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
namespace {
const StartupWorldEquipment *equipment(const StartupWorldRuntimeState &s, int kind, int id) {
    if (!s.rules) return nullptr;
    const auto d = std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
        [=](const auto &e) { return e.shop.kind == kind && e.shop.id == id; });
    return d == s.rules->equipment.end() ? nullptr : &*d;
}
bool weapon_image(int id) {
    // weapon/img.inf是稀疏显式ID；不能只检查最小/最大值后让map.at抛异常。
    return (id >= 0 && id <= 5) || (id >= 10 && id <= 15) ||
           (id >= 20 && id <= 25) || id == 30 || (id >= 40 && id <= 46) ||
           (id >= 50 && id <= 56);
}
std::optional<int> lift_height(int age, int velocity, int acceleration, int initial) {
    // 源先age*acceleration，再*(age+1)/2，Java两次整数除法；坏载荷不允许32位绕回。
    const auto product = std::int64_t(age) * acceleration;
    const auto linear = std::int64_t(velocity) * age;
    const auto fits = [](std::int64_t n) { return n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max(); };
    if (!fits(product) || !fits(linear)) return {};
    const auto parabola = product * (std::int64_t(age) + 1);
    if (!fits(parabola) || !fits(parabola / 2 + linear)) return {};
    const auto height = (parabola / 2 + linear) / 100 + initial;
    return fits(height) ? std::optional<int>(static_cast<int>(height)) : std::nullopt;
}
}
std::optional<std::vector<StartupVisualDraw>> startup_world_equipment_lift_draws(
    const StartupWorldRuntimeState &s, ref::CharacterId id) {
    const auto &ai = s.scene.world.world.ai;
    const auto actor = ai.battle.actors.find(id);
    const auto context = ai.contexts.find(id);
    if (!s.rules || actor == ai.battle.actors.end() || context == ai.contexts.end() ||
        actor->second.control.facing < 0 || actor->second.control.facing > 3) return {};
    std::vector<StartupVisualDraw> result;
    if (actor->second.control.flags & 1U) return result; // 原绘制总守卫。
    if (!ref::valid_actor_effect_state(context->second.effects)) return {};
    // c/b.cd正序，先身体再各效果；保留重复记录，不替换为一个“当前举物”。
    for (const auto &r : context->second.effects.display) {
        if (r.size() < 2) return {};
        if (r[0] != 15 && r[0] != 21 && r[0] != 22) continue;
        if (r[1] < 0) continue;
        if (actor->second.kind != ref::ActorKind::human) return {};
        if (r[0] == 15) {
            if (r.size() < 8) return {};
            const int age = r[1], phaseAge = age < 8 ? age : age < 32 ? 0 : age - 32 + 8;
            const auto height = age >= 8 && age < 32 ? std::optional<int>(20) : lift_height(phaseAge, r[6], r[7], 0);
            const auto d = equipment(s, 1, r[age < 20 ? 4 : 5]);
            if (!height || !d || d->render_style < 0 || d->render_style > 3 ||
                !weapon_image(d->render_image)) return {};
            // p.A[h][dir][0]；不是武器PNG自身SEB偏移，也不是攻击姿态A[][1]。
            constexpr int offsets[4][4][2] = {
                {{-5,-36},{-4,-36},{-19,-37},{-17,-36}},
                {{-4,-35},{-4,-35},{-17,-35},{-17,-34}},
                {{-5,-36},{-4,-36},{-22,-37},{-20,-36}},
                {{-9,-45},{-8,-46},{-24,-46},{-23,-46}}};
            const int direction = actor->second.control.facing;
            const auto y = std::int64_t(offsets[d->render_style][direction][1]) - *height;
            if (y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max()) return {};
            result.push_back({StartupVisualResource::weapon, d->render_style * 4 + direction,
                d->render_image, 0, 0, {}, {offsets[d->render_style][direction][0],
                                         static_cast<int>(y)}});
        } else {
            if (r.size() < 5) return {};
            const auto height = r[1] >= 12 ? std::optional<int>(28) : lift_height(r[1], r[2], r[3], 16);
            const auto d = equipment(s, r[0] == 21 ? 2 : 3, r[4]);
            if (!height || !d || d->render_image < 0 || d->render_image >= (r[0] == 21 ? 50 : 30)) return {};
            const auto y = -std::int64_t(*height) - 16;
            if (y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max()) return {};
            const std::array<int,2> offset{-8, static_cast<int>(y)};
            result.push_back({StartupVisualResource::common, -1, 24, 0, 0, {54,0,18,18}, offset});
            result.push_back({StartupVisualResource::common, -1, r[0] == 21 ? 20 : 21, 0, 0,
                {(d->render_image % 10) * 18, (d->render_image / 10) * 18, 18, 18}, offset});
        }
    }
    return result;
}
std::optional<std::vector<StartupVisualDraw>> startup_world_facility_growth_draws(
    const StartupWorldRuntimeState &s, std::uint64_t id) {
    const auto facility = s.scene.world.world.facilities.find(id);
    const auto details = s.facility_details.find(id);
    if (facility == s.scene.world.world.facilities.end() || details == s.facility_details.end()) return {};
    std::vector<StartupVisualDraw> result;
    if (details->second.notices.empty()) return result;
    const auto &head = details->second.notices.front(); // 后项本轮不绘、不计时。
    const int kind = head[0], age = head[1];
    if (kind < 0 || kind > 7 || age < 0) return {};
    if (kind == 0 || kind == 7) return result; // 施工/NEW另有资源合同，本接口只维护增减头标。
    constexpr std::array<int,7> thresholds{2,4,6,9,12,15,43};
    const int phase = std::min(6, static_cast<int>(std::upper_bound(thresholds.begin(), thresholds.end(), age) - thresholds.begin()));
    const int clamped = std::min(age, 6);
    if (kind <= 3) {
        const int y = -24 + clamped * -7 / 6; // 向零截断；不能对负差用floor。
        result.push_back({StartupVisualResource::common, 68 + kind, -1, phase, 0, {}, {0,y}});
        if (phase >= 3 && phase <= 5)
            result.push_back({StartupVisualResource::common, 68 + kind, -1, phase, 1, {}, {0,y}});
    } else {
        const int frame = (kind - 4) * 3 + (phase <= 1 ? 0 : phase <= 3 ? 1 : 2);
        result.push_back({StartupVisualResource::common, 85, -1, frame, 0, {}, {0,-32 + clamped * 4 / 6}});
    }
    return result;
}
} // namespace dungeon_village_prototype
