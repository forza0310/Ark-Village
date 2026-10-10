#include "ark/simulation/world/startup_world_runtime.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
const ref::BattleActorRecord *actor(const State &s, ref::CharacterId id) {
    const auto &ai = s.scene.world.world.ai;
    const auto live = ai.battle.actors.find(id);
    if (live != ai.battle.actors.end())
        return &live->second;
    const auto retired = ai.retired_actors.find(id);
    return retired == ai.retired_actors.end() ? nullptr : &retired->second;
}
ref::WorldNonactorWriteback globals(const State &s) {
    return {ref::encounter_external_writeback(s.scene.world.world.ai), {},
            s.scene.world.popularity_queue, {}};
}
bool sound_visible(const State &s, ref::Position p) {
    const auto &v = s.reference_viewport;
    return p.x - 7 <= v[0] + v[2] && p.x + 7 >= v[0] &&
        p.y - 20 <= v[1] + v[3] && p.y >= v[1];
}
bool notice(State &s, const ref::WorldNonactorRequest &r) {
    if (!r.object)
        return false;
    const auto object = s.scene.world.world.ai.battle.objects.find(r.identity);
    if (object == s.scene.world.world.ai.battle.objects.end() || !s.rules)
        return false;
    const auto kind = object->second.kind;
    std::string name;
    if (kind == 0) {
        const auto item = std::find_if(s.rules->items.begin(), s.rules->items.end(),
            [&](const auto &d) { return d.identity == r.object->definition; });
        if (item == s.rules->items.end())
            return false;
        name = item->name;
    } else {
        const auto item = std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
            [&](const auto &d) { return d.shop.kind == kind && d.shop.id == r.object->definition; });
        if (item == s.rules->equipment.end())
            return false;
        name = item->name;
    }
    const int id = r.object->parameter;
    if (id != 2 && id != 10 && id != 11)
        return false;
    // c.e.a: 饰品拾取也使用原notice11，不能按物品类别修正原版文字。
    const std::string text = (id == 10 ? "武器 " : id == 11 ? "防具 " : "") +
        std::string("<co=0064FF>") + name + "</co> 入手" + (id == 2 ? "!" : "");
    s.scripts.notices.push_back({id, -10, 80, name, text});
    return true;
}
} // namespace

ref::Position startup_world_raw_projection(ref::CombatPoint p) {
    // c.a.a -> c.d.a的float乘后除；高度先截断，再与投影相加，最终截断。
    return {static_cast<int>((p.x * 30.0F) / 100.0F + (p.z * 30.0F) / 100.0F),
            static_cast<int>((p.x * -15.0F) / 100.0F + (p.z * 15.0F) / 100.0F +
                             static_cast<int>(p.height))};
}
ref::Position startup_world_view_projection(const State &s, ref::CombatPoint p) {
    const auto raw = startup_world_raw_projection(p);
    const auto &v = s.reference_viewport;
    const int middle_x = (v[0] + v[2]) / 2;
    const int middle_y = (v[1] + v[3]) / 2;
    return {middle_x + raw.x - static_cast<int>(s.camera[0]),
            v[1] + v[3] - (middle_y + raw.y - static_cast<int>(s.camera[1]))};
}
std::optional<bool> startup_world_actor_visible(const State &s, ref::CharacterId id) {
    const auto *a = actor(s, id);
    if (!a)
        return {};
    const auto p = startup_world_view_projection(s, a->position);
    const auto &v = s.reference_viewport;
    // b.c.o/p为宽高；a.a的Q/R却由调用者传右/下边界，b.c原m=0使X相同。
    return !(a->control.flags & 1U) && p.x - 7 <= v[0] + v[2] && p.x + 7 >= v[0] &&
        p.y - 20 <= v[1] + v[3] && p.y >= v[1];
}
std::optional<bool> startup_world_hit_sound_visible(const State &s, ref::CharacterId id) {
    const auto metadata = s.actor_metadata.find(id);
    if (!actor(s, id) || metadata == s.actor_metadata.end())
        return {};
    const auto p = startup_world_view_projection(s, metadata->second.render_position);
    const auto &v = s.reference_viewport;
    return p.x - 7 <= v[0] + v[2] && p.x + 7 >= v[0] &&
        p.y - 20 <= v[1] + v[3] && p.y >= v[1];
}
bool update_startup_world_render_cache(State &s) {
    for (const auto &entry : s.scene.world.world.ai.battle.actors) {
        const auto visible = startup_world_actor_visible(s, entry.first);
        if (!visible)
            return false;
        const auto &a = entry.second;
        const auto metadata = s.actor_metadata.find(entry.first);
        if (metadata == s.actor_metadata.end())
            return false;
        if (a.control.flags & 1U)
            continue;
        metadata->second.cached_screen_position = startup_world_view_projection(s, a.position);
        if (!*visible)
            continue;
        auto &o = metadata->second.render_position;
        const bool E = a.kind == ref::ActorKind::monster &&
            (a.control.action == 3 || a.control.action == 6 || a.control.action == 9 ||
             a.control.state == 8);
        const auto &source = E ? a.attack_position : a.position;
        o.x = source.x;
        o.z = source.z;
        if (a.physics_pause > 0)
            o = a.position;
        if (E)
            metadata->second.cached_screen_position =
                startup_world_view_projection(s, a.attack_position);
    }
    return true;
}
bool emit_startup_world_actor_sound(State &s, ref::CharacterId id, int sound) {
    const auto metadata = s.actor_metadata.find(id);
    if (metadata == s.actor_metadata.end() || sound < 0)
        return false;
    if (sound_visible(s, metadata->second.cached_screen_position))
        s.sound_requests.push_back({StartupAudioOperation::ordinary_play, sound});
    return true;
}
std::optional<int> startup_world_actor_direction(const State &s, ref::CharacterId actor_id,
                                                ref::CharacterId target_id) {
    const auto self = s.actor_metadata.find(actor_id);
    const auto target = s.actor_metadata.find(target_id);
    if (self == s.actor_metadata.end() || target == s.actor_metadata.end())
        return {};
    const auto a = self->second.cached_view;
    const auto t = target->second.cached_view;
    return t.y > a.y ? (t.x > a.x ? 0 : 3) : (t.x > a.x ? 1 : 2);
}
std::optional<ref::DropSelectionInput> startup_world_drop_selection(const State &s,
                                                                  ref::CharacterId id) {
    const auto *a = actor(s, id);
    if (!a || !s.rules || a->kind != ref::ActorKind::human)
        return {};
    const auto growth = s.scene.world.world.ai.growth.find(a->definition);
    if (growth == s.scene.world.world.ai.growth.end())
        return {};
    ref::DropSelectionInput input;
    input.luck = growth->second.derived.attributes[5];
    input.progress = s.rank;
    for (const auto &d : s.rules->items)
        input.definitions.push_back({d.identity, 0, d.difficulty, d.initial.flags,
                                     s.catalog.at({0, d.identity}).status});
    for (const auto &d : s.rules->equipment)
        input.definitions.push_back({d.shop.id, d.shop.kind, d.reward_difficulty, d.initial.flags,
                                     s.catalog.at({d.shop.kind, d.shop.id}).status});
    return input;
}

void configure_startup_world_runtime_nonactor_adapter(ref::WorldRuntimeAdapter<State> &adapter) {
    adapter.nonactors.projectile = [](const State &s, std::uint64_t id)
        -> std::optional<ref::WorldProjectileInput> {
        const auto &ai = s.scene.world.world.ai;
        const auto found = ai.projectiles.find(id);
        if (found == ai.projectiles.end() || !s.rules)
            return {};
        ref::WorldProjectileInput input;
        input.projectile = id;
        input.box = found->second.kind == ref::ProjectileKind::arrow
            ? ref::CollisionBox{-30, 20, 30, 20} : ref::CollisionBox{-240, 240, 240, 240};
        input.monster_boxes = {ref::CollisionBox{-30, 30, 30, 30},
                              ref::CollisionBox{-30, 30, 30, 30},
                              ref::CollisionBox{-30, 30, 30, 30},
                              ref::CollisionBox{-40, 40, 40, 40}};
        const auto *caster = actor(s, found->second.caster);
        if (!caster)
            return input; // 原b引用失效分支不读取N()/可见性/drop目录。
        const auto visible = startup_world_hit_sound_visible(s, caster->id);
        const auto shop = s.shop_actors.find(caster->id);
        if (!visible || shop == s.shop_actors.end())
            return {};
        input.caster_visible = *visible;
        const auto weapon = std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
            [&](const auto &d) { return d.shop.kind == 1 && d.shop.id == shop->second.weapon; });
        if (weapon == s.rules->equipment.end())
            return {};
        input.current_weapon_kind = weapon->battle.kind;
        input.drop_selection = startup_world_drop_selection(s, caster->id);
        if (!input.drop_selection)
            return {};
        return input;
    };
    const auto previous = adapter.nonactors.request;
    adapter.nonactors.request = [previous](State &s, const ref::WorldNonactorRequest &r)
        -> std::optional<ref::WorldNonactorWriteback> {
        using Kind = ref::WorldNonactorRequestKind;
        auto out = globals(s);
        if (r.kind == Kind::projectile_contact ||
            (r.kind == Kind::projectile_visual && r.visual >= 4 && r.visual <= 9)) {
            if (!r.target || !r.source_position)
                return {};
            const auto *target = actor(s, *r.target);
            auto context = s.scene.world.world.ai.contexts.find(*r.target);
            if (!target || context == s.scene.world.world.ai.contexts.end())
                return {};
            auto effects = context->second.effects;
            auto position = *r.source_position;
            if (r.kind == Kind::projectile_contact) {
                const float x = position.x - target->position.x;
                const float z = position.z - target->position.z;
                const float distance = std::sqrt(x * x + z * z);
                // Java float NaN转int为0；同点接触不能让C++执行未定义的NaN整数转换。
                ref::Position p{};
                if (distance > 0.0F) {
                    position = {x * 20.0F / distance + target->position.x, 0.0F,
                                z * 20.0F / distance + target->position.z};
                    p = startup_world_raw_projection(position);
                }
                effects.display.push_back({10, 0, p.x, p.y + 10,
                                           target->kind == ref::ActorKind::human ? 1 : 0});
            } else {
                position = {(position.x + target->position.x) / 2.0F, 0.0F,
                            (position.z + target->position.z) / 2.0F};
                const auto p = startup_world_raw_projection(position);
                if (r.visual <= 6) {
                    if (!emit_startup_world_actor_sound(s, *r.target, r.visual + 13))
                        return {};
                    effects.display.insert(effects.display.begin(), {16, 0, r.visual, p.x, p.y});
                } else {
                    effects.delayed.push_back({r.visual - 3, 0, p.x, p.y});
                    effects.delayed.push_back({r.visual - 3, 3, p.x - 5, p.y + 5});
                    effects.delayed.push_back({r.visual - 3, 6, p.x + 5, p.y + 8});
                }
            }
            out.target_effects = std::move(effects);
            return out;
        }
        if (r.kind == Kind::projectile_visual && r.visual == 22) {
            if (!r.source_position)
                return {};
            const auto p = startup_world_raw_projection(*r.source_position);
            s.visual_effects.push_back({22, 0, p.x, p.y, 0});
            return out;
        }
        if (r.kind == Kind::projectile_hit && r.hit) {
            if (r.hit->kind == ref::HitRequestKind::attack_sound) {
                if (!r.target || !emit_startup_world_actor_sound(s, *r.target, r.hit->parameter))
                    return {};
                return out;
            }
            if (r.hit->kind == ref::HitRequestKind::face_attacker) {
                if (!r.caster || !r.target || !s.actor_metadata.count(*r.caster) ||
                    !s.actor_metadata.count(*r.target))
                    return {};
                out.target_facing = startup_world_actor_direction(s, *r.target, *r.caster);
                return out;
            }
        }
        if (r.kind == Kind::object && r.object) {
            if (r.object->kind == ref::ObjectCommitRequestKind::ground_effect) {
                if (!r.source_position)
                    return {};
                const auto p = startup_world_raw_projection(*r.source_position);
                s.visual_effects.push_back({12, 0, p.x, p.y, 0});
                return out;
            }
            if (r.object->kind == ref::ObjectCommitRequestKind::notice)
                return notice(s, r) ? std::optional<ref::WorldNonactorWriteback>(out) : std::nullopt;
        }
        if (r.kind == Kind::encounter && r.encounter &&
            r.encounter->kind != ref::EncounterRequestKind::event) {
            const auto before = s.scene.world.world.ai.monster_growth;
            if (!consume_startup_world_runtime_task_encounter_request(s, *r.encounter))
                return {};
            auto out = globals(s);
            if (r.encounter->kind == ref::EncounterRequestKind::mark_task_complete) {
                out.globals.monster_availability.emplace();
                for (const auto &entry : s.scene.world.world.ai.monster_growth) {
                    const auto old = before.find(entry.first);
                    if (old == before.end())
                        return {};
                    if (old->second.status != entry.second.status ||
                        old->second.newly_unlocked != entry.second.newly_unlocked)
                        out.globals.monster_availability->emplace(entry.first,
                            std::array<int, 2>{entry.second.status, entry.second.newly_unlocked ? 1 : 0});
                }
            }
            return out;
        }
        return previous ? previous(s, r) : std::nullopt;
    };
    const auto nonactor = adapter.nonactors.request;
    adapter.actors.presentation = [nonactor](const State &current,
        const ref::WorldActorPresentationRequest &r) -> std::optional<State> {
        auto s = current;
        if (r.task_encounter_start) {
            if (r.cached_sound || r.lifecycle || r.sound || r.hit || r.attack || r.shop ||
                r.definition || r.target ||
                !consume_startup_world_runtime_task_start(s, r.actor, *r.task_encounter_start))
                return {};
            return s;
        }
        if (r.cached_sound)
            return emit_startup_world_actor_sound(s, r.actor, *r.cached_sound)
                ? std::optional<State>(std::move(s)) : std::nullopt;
        if (r.lifecycle) {
            if (!s.actor_metadata.count(r.actor))
                return {};
            const auto raw = s.actor_metadata.at(r.actor).cached_view;
            using Lifecycle = ref::LifecycleRequestKind;
            if (r.lifecycle->kind == Lifecycle::normal_death_rewards) {
                if (!r.definition || !s.scene.world.world.ai.battle.monsters.count(*r.definition))
                    return {};
                const int cash = s.scene.world.world.ai.battle.monsters.at(*r.definition).statF;
                s.visual_effects.push_back({20, 0, raw.x, raw.y});
                s.visual_effects.push_back({3, -2, raw.x, raw.y, cash, 0, 0});
                s.visual_effects.push_back({4, -2, raw.x, raw.y, -320, 29});
                const auto &v = s.reference_viewport;
                s.actor_metadata.at(r.actor).cached_screen_position = {
                    (v[0] + v[2]) / 2 + raw.x - static_cast<int>(s.camera[0]),
                    v[1] + v[3] - ((v[1] + v[3]) / 2 + raw.y - static_cast<int>(s.camera[1]))};
                if (!emit_startup_world_actor_sound(s, r.actor, 23))
                    return {};
                return s; // 增长/入账已在prepare_monster_death_commit提交，不重复结算。
            }
            if (r.lifecycle->kind == Lifecycle::cancelled_death_effect) {
                s.visual_effects.push_back({21, 0, raw.x, raw.y});
                return s;
            }
            if (r.lifecycle->kind == Lifecycle::ground_effect) {
                if (r.lifecycle->parameter != 20 && r.lifecycle->parameter != 21)
                    return {};
                s.visual_effects.push_back({r.lifecycle->parameter, 0, raw.x, raw.y});
                return s;
            }
            if (r.lifecycle->kind == Lifecycle::landing_effect) {
                const auto context = s.scene.world.world.ai.contexts.find(r.actor);
                if (context == s.scene.world.world.ai.contexts.end() || r.lifecycle->parameter != 18)
                    return {};
                context->second.effects.display.push_back({18, 0, raw.x, raw.y});
                return s;
            }
            return {};
        }
        if (r.sound) {
            s.actor_metadata.at(r.actor).cached_screen_position = r.sound->position;
            if (!emit_startup_world_actor_sound(s, r.actor, r.sound->sound))
                return {};
            return s;
        }
        if (r.hit || (r.attack && r.attack->kind == ref::WorldAttackVisual::contact)) {
            const auto *source = actor(s, r.actor);
            if (!source)
                return {};
            ref::WorldNonactorRequest request;
            request.kind = r.hit ? ref::WorldNonactorRequestKind::projectile_hit
                                 : ref::WorldNonactorRequestKind::projectile_contact;
            request.caster = r.actor;
            request.target = r.target;
            request.hit = r.hit;
            request.source_position = source->position;
            const auto out = nonactor(s, request);
            if (!out)
                return {};
            if (out->target_effects) {
                if (!r.target || !s.scene.world.world.ai.contexts.count(*r.target) ||
                    !ref::valid_actor_effect_state(*out->target_effects))
                    return {};
                s.scene.world.world.ai.contexts.at(*r.target).effects = *out->target_effects;
            }
            if (out->target_facing) {
                if (!r.target)
                    return {};
                auto &ai = s.scene.world.world.ai;
                const auto live = ai.battle.actors.find(*r.target);
                const auto retired = ai.retired_actors.find(*r.target);
                if (live == ai.battle.actors.end() && retired == ai.retired_actors.end())
                    return {};
                (live != ai.battle.actors.end() ? live->second : retired->second).control.facing =
                    *out->target_facing;
            }
            return s;
        }
        if (r.attack) {
            if (!s.actor_metadata.count(r.actor) || !s.scene.world.world.ai.contexts.count(r.actor))
                return {};
            const auto raw = s.actor_metadata.at(r.actor).cached_view;
            auto &effects = s.scene.world.world.ai.contexts.at(r.actor).effects;
            using Visual = ref::WorldAttackVisual;
            switch (r.attack->kind) {
            case Visual::spell_source:
                effects.display.push_back({4, 0, raw.x, raw.y - 1});
                break;
            case Visual::healing_source:
                effects.display.push_back({4, 0, raw.x, raw.y});
                break;
            case Visual::healing_target: {
                if (!r.target || !s.actor_metadata.count(*r.target))
                    return {};
                const auto target_raw = s.actor_metadata.at(*r.target).cached_view;
                // 原h(target.u)追加到施法者cd，不是被治疗者cd。
                effects.display.push_back({6, 0, target_raw.x, target_raw.y});
                break;
            }
            case Visual::telegraph:
                effects.display.push_back({23, 0, raw.x, raw.y});
                break;
            case Visual::cast_sound:
                if (!emit_startup_world_actor_sound(s, r.actor, r.attack->parameter))
                    return {};
                break;
            case Visual::expression:
                return {}; // shared random已即时消费表情，不允许缺hook再丢弃或抽第二次。
            case Visual::contact:
                return {};
            }
            return ref::valid_actor_effect_state(effects) ? std::optional<State>(std::move(s))
                                                         : std::nullopt;
        }
        if (r.shop) {
            if (r.shop->kind == ref::ShopWorldRequestKind::delivered_item_notice) {
                const auto item = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                    [&](const auto &d) { return d.identity == r.shop->first; });
                if (item == s.rules->items.end())
                    return {};
                s.scripts.notices.push_back({2, -1, 80, item->name,
                    "<co=0064FF>" + item->name + "</co> 入手!"});
                return s;
            }
            if (r.shop->kind == ref::ShopWorldRequestKind::cash_display) {
                const auto context = s.scene.world.world.ai.contexts.find(r.actor);
                if (context == s.scene.world.world.ai.contexts.end())
                    return {};
                const auto cell = context->second.cell;
                const auto raw = startup_world_raw_projection(
                    {cell.x * 100.0F, 0, cell.y * 100.0F});
                s.visual_effects.push_back({2, -10, raw.x, raw.y, r.shop->first, -4000, 666});
                return s;
            }
            return {}; // 装备display已由原领域consumer即时处理，不重放。
        }
        return {};
    };
}
} // namespace ark::simulation
