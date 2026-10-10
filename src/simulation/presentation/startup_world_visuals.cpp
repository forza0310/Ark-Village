#include "ark/simulation/presentation/startup_world_visuals.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace ark::simulation {
std::optional<StartupPortrait> startup_world_portrait(const StartupWorldRuntimeState &s, int id) {
    if (!s.rules || id < 0 || id >= static_cast<int>(s.rules->humans.size()))
        return {};
    const auto human = startup_world_human_profile(s, id);
    const auto growth = s.scene.world.world.ai.growth.find(id);
    if (!human || growth == s.scene.world.world.ai.growth.end())
        return {};
    const int job = growth->second.definition.current_profession;
    if (job < 0 || job >= static_cast<int>(s.rules->jobs.size()))
        return {};
    return StartupPortrait{s.rules->jobs.at(job).sprites.at(human->sex)};
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
std::optional<std::vector<StartupCoinEffectDraw>> startup_world_coin_effect_draws(
    const StartupWorldRuntimeState &s) {
    std::vector<StartupCoinEffectDraw> result;
    for (std::size_t index=0; index<s.visual_effects.size(); ++index) {
        const auto &record=s.visual_effects[index];
        if (record.size()<2) return {};
        if (record[0]!=4) continue;
        if (record.size()!=6) return {};
        if (record[1]<0) continue;
        // 原分支先计算二次项再检查11门槛，坏载荷不能因末段覆盖而绕过范围核验。
        const auto movement=lift_height(record[1],record[4],record[5],0);
        if (!movement) return {};
        const int dy=record[1]>=11 ? -16 : std::min(0,*movement);
        StartupVisualDraw image{StartupVisualResource::common,94,144,(record[1]%14)/2,0,
                                {},{0,dy},index};
        result.push_back({{record[2],record[3]},image});
    }
    return result;
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
    for (std::size_t index = 0; index < context->second.effects.display.size(); ++index) {
        const auto &r = context->second.effects.display[index];
        if (r.size() < 2) return {};
        if (r[0] != 15 && r[0] != 21 && r[0] != 22) continue;
        if (r[1] < 0) continue;
        if (actor->second.kind != ref::ActorKind::human) return {};
        const auto first_command = result.size();
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
        for (std::size_t n = first_command; n < result.size(); ++n)
            result[n].record_index = index;
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
std::optional<std::vector<StartupAttributeGainDraw>> startup_world_attribute_gain_draws(
    const StartupWorldRuntimeState &s, ref::CharacterId id,
    const std::function<int(const std::string &)> &measure) {
    const auto &ai = s.scene.world.world.ai;
    const auto actor = ai.battle.actors.find(id);
    const auto context = ai.contexts.find(id);
    if (actor == ai.battle.actors.end() || context == ai.contexts.end() || !measure) return {};
    std::vector<StartupAttributeGainDraw> result;
    if (actor->second.control.flags & 1U) return result;
    if (!ref::valid_actor_effect_state(context->second.effects)) return {};
    constexpr std::array<const char *,6> names{"体力","力量","灵活","结实","魔力","运气"};
    const auto &display = context->second.effects.display;
    for (std::size_t index = 0; index < display.size(); ++index) {
        const auto &r = display[index];
        if (r[0] != 13) continue;
        // 隐藏相位也验证属性载荷；不让坏ID等到测量/图像map.at才失败。
        if (r.size() < 4 || r[2] < 0 || r[2] >= 6 || actor->second.kind != ref::ActorKind::human)
            return {};
        if (r[1] < 0) continue;
        StartupAttributeGainDraw draw;
        draw.record_index = index; draw.age = r[1]; draw.attribute = r[2]; draw.delta = r[3];
        draw.text = names[r[2]];
        const auto raw_number = std::to_string(r[3]);
        const int label_width = measure(draw.text), number_width = measure(raw_number);
        // 原12px画布下的实测文字远小于此；4096是维护输出预算，不是原表数值/字体宽事实。
        if (label_width < 0 || label_width > 4096 || number_width < 0) return {};
        draw.width = label_width + 36;
        const int left = -(draw.width / 2), right = draw.width / 2;
        const auto add = [&](std::vector<StartupVisualDraw> &commands, int sprite, int image,
                             int frame, std::array<int,4> crop, std::array<int,2> offset) {
            commands.push_back({StartupVisualResource::common,sprite,image,frame,0,crop,offset,index});
        };
        if (draw.width <= 60) {
            add(draw.before_text,-1,7,0,{36-draw.width/2,0,draw.width,21},{left,-40});
        } else {
            // 宽文字重复27px底纹，最后叠中央5px尾尖；原图没有横向缩放。
            for (int x = left, remaining = draw.width; remaining > 0;) {
                const int width = std::min(27,remaining);
                add(draw.before_text,-1,7,0,{6,0,width,21},{x,-40});
                x += width; remaining -= width;
            }
            add(draw.before_text,-1,7,0,{33,0,5,21},{-2,-40});
        }
        add(draw.before_text,28,-1,0,{}, {left-6,-40});
        add(draw.before_text,28,-1,1,{}, {right,-40});
        add(draw.before_text,-1,37,0,{r[2]==5?96:r[2]*16,16,16,16},{left,-39});
        draw.text_offset = {left+18,-37};
        int x = right - static_cast<int>(raw_number.size()) * 8;
        const std::size_t digit_start = r[3] < 0 ? 1U : 0U;
        const std::size_t digit_count = raw_number.size() - digit_start;
        for (std::size_t n = 0; n < raw_number.size(); ++n) {
            // 原'-'-'0'请求frame−3，SEB首关键帧0之外为空绘，仍占8px；不补猜负号素材。
            if (raw_number[n] != '-') {
                add(draw.after_text,15,-1,raw_number[n]-'0',{}, {x,-36});
                if (n > digit_start && (digit_count - (n - digit_start)) % 3 == 0)
                    add(draw.after_text,15,-1,10,{}, {x-2,-36});
            }
            x += 8;
        }
        // DEX确认long数值重载；+始终补帧14，位置单独读字体对raw signed串的测量。
        const auto plus_x = std::int64_t(right) - (std::int64_t(number_width) / 6) * 8 - 8;
        if (plus_x < std::numeric_limits<int>::min() || plus_x > std::numeric_limits<int>::max())
            return {};
        add(draw.after_text,15,-1,14,{}, {static_cast<int>(plus_x),-36});
        result.push_back(std::move(draw));
    }
    return result;
}
std::optional<std::vector<StartupVisualDraw>> startup_world_item_icon_draws(
    const StartupWorldRuntimeState &s, int id) {
    if (!s.rules) return {};
    const auto item = std::find_if(s.rules->items.begin(),s.rules->items.end(),
        [=](const auto &d) { return d.identity == id; });
    if (item == s.rules->items.end()) return {};
    // 原a.g.C/D是图标分类和背景槽，独立于业务category/effect。
    constexpr std::array<int,89> categories{{
        0,0,0,3,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,3,3,0,2,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,
        1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,2,2,2,2,2,2,2,2}};
    constexpr std::array<int,5> backgrounds{{1,4,6,2,3}};
    const int icon = item->render_icon;
    if (icon < 0 || icon >= static_cast<int>(categories.size())) return {};
    const int background = backgrounds[categories[icon]];
    return std::vector<StartupVisualDraw>{
        {StartupVisualResource::common,-1,24,0,0,{background*18,0,18,18},{-1,-1},{}},
        {StartupVisualResource::common,-1,9,0,0,{(icon%15)*16,(icon/15)*16,16,16},{0,0},{}}};
}
std::optional<StartupVisualDraw> startup_world_facility_icon_draw(
    const StartupWorldRuntimeState &s, int id) {
    if (!s.rules) return {};
    const auto definition = std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [=](const auto &d) { return d.id == id; });
    if (definition == s.rules->facilities.end() || definition->legacy_icon < 0 ||
        definition->legacy_icon > 6) return {};
    // common91实际112×16；原85定义只用0..6，不能用%10把坏ID变成合法图块。
    return StartupVisualDraw{StartupVisualResource::common,-1,91,0,0,
        {definition->legacy_icon*16,0,16,16},{0,0},{}};
}
std::optional<StartupVisualDraw> startup_world_attribute_icon_draw(int id) {
    if (id < 0 || id >= 6) return {};
    return StartupVisualDraw{StartupVisualResource::common,-1,37,0,0,
        {id == 5 ? 96 : id*16,16,16,16},{0,0},{}};
}
std::optional<std::vector<StartupFacilityExitEffectDraw>> startup_world_facility_exit_effect_draws(
    const StartupWorldRuntimeState &s, int id) {
    if (!s.rules) return {};
    const auto definition = std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [=](const auto &d) { return d.id == id; });
    if (definition == s.rules->facilities.end()) return {};
    // 4096是坏私有rules的输出预算，非原表最大效果/奖励；固定输入最多两行、每行三个+。
    if (definition->exit_effects.size() > 4096) return {};
    std::size_t command_count = definition->exit_effects.size();
    for (const auto &effect : definition->exit_effects) {
        if (!startup_world_attribute_icon_draw(effect.attribute_index)) return {};
        const auto positive = static_cast<std::size_t>(std::max(0,effect.delta));
        if (positive > 4096 - command_count) return {};
        command_count += positive;
    }
    std::vector<StartupFacilityExitEffectDraw> result;
    for (std::size_t slot = 0; slot < definition->exit_effects.size(); ++slot) {
        const auto &effect = definition->exit_effects[slot];
        auto icon = *startup_world_attribute_icon_draw(effect.attribute_index);
        const int y = 127 + static_cast<int>(slot) * 17;
        icon.offset = {136,y};
        StartupFacilityExitEffectDraw row{slot,effect.attribute_index,effect.delta,icon,{}};
        for (int n = 0; n < effect.delta; ++n)
            row.pluses.push_back({StartupVisualResource::common,15,-1,14,0,{},
                                  {192-8*n,y+3},{}});
        result.push_back(std::move(row));
    }
    return result;
}
std::optional<std::vector<StartupBuildingDraw>> startup_world_building_draws(
    const StartupWorldRuntimeState &s, int id, ref::FacilityOrientation orientation) {
    if (!s.rules) return {};
    const auto definition = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
        [=](const auto &d) { return d.id == id; });
    if (definition == s.rules->facilities.end()) return {};
    const auto &displays = startup_evidence().displays;
    const auto art = std::find_if(displays.begin(), displays.end(),
        [&](const auto &d) { return d.id == definition->display_id; });
    if (art == displays.end() || art->sprite.empty()) return {};
    const auto source = std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [&](const auto &d) { return d.id == art->definition_id; });
    // 原helper经bs.g读绘制定义；固定85定义的kind/shape均相同，坏私有映射明确拒绝。
    if (source == s.rules->facilities.end() || source->shape != definition->shape ||
        source->kind != definition->kind) return {};
    // 复用已证ah/ai分片序，但放到完整容纳三种形状的局部坐标，避免真实候选越界时丢图。
    const auto shape = ref::facility_footprint(static_cast<ref::FacilityShape>(definition->shape),
                                               orientation, {1,1}, 3,3);
    if (shape.error != ref::GeometryError::none) return {};
    std::vector<StartupBuildingDraw> result;
    for (const auto &part : shape.cells) {
        const int dx = part.position.x - 1, dy = part.position.y - 1;
        // 原kind6道路不使用普通ah帧序，朝向0/1分别是11/1。
        const int frame = definition->kind == 6
            ? (orientation == ref::FacilityOrientation::second ? 1 : 11) : part.fragment_index;
        result.push_back({art->sprite, frame, {30*(dx+dy),15*(dx-dy)}});
    }
    return result;
}
std::optional<std::vector<StartupBuildingDraw>> startup_world_building_preview_draws(
    const StartupWorldRuntimeState &s, ref::Position cursor, ref::FacilityOrientation orientation) {
    const auto &map = s.scene.world.world.map;
    std::vector<StartupBuildingDraw> empty;
    if (s.scene.scene_state != 1 || (s.build_mode != 0 && s.build_mode != 7)) return empty;
    if (!s.build_definition || s.scene.scene_counter < 0 || !ref::valid_legacy_map(map)) return {};
    if (cursor.x < 0 || cursor.y < 0 || cursor.x >= map.width || cursor.y >= map.height) return empty;
    // 坏定义/朝向仍明确拒绝；隐藏相位不是掩盖坏载荷的兜底。
    auto result = startup_world_building_draws(s,*s.build_definition,orientation);
    if (!result) return {};
    return s.scene.scene_counter % 20 < 10 ? result : std::optional<std::vector<StartupBuildingDraw>>(empty);
}
} // namespace ark::simulation
