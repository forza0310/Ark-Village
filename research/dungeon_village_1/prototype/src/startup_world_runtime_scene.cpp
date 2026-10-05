#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_reference/world_notices.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Stage = ref::WorldSceneStage;
using Step = ref::OwnedWorldSceneStep<State>;
constexpr std::array<int, 25> effect_limits{
    {12, 10, 20, 20, 23, 24, 16, 16, 24, 73, 10, 5, 12, 5, 0, 5, 0, 5, 0, 0, 9, 9, 11, 22, 34}};
bool increment(int &value) {
    if (value == std::numeric_limits<int>::max())
        return false;
    ++value;
    return true;
}
bool display(State &s) {
    for (std::size_t n = s.delayed_effects.size(); n > 0; --n) {
        auto &effect = s.delayed_effects[n - 1];
        if (effect[1] > 0) {
            --effect[1];
            continue;
        }
        if (effect[0] >= 4 && effect[0] <= 6)
            s.sound_requests.push_back(effect[0] + 13);
        s.visual_effects.push_back({19, 0, effect[0], effect[2], effect[3]});
        s.delayed_effects.erase(s.delayed_effects.begin() + static_cast<std::ptrdiff_t>(n - 1));
    }
    // 原X是前向遍历且删除后不回退；被移入当前位置的下一项本轮不推进。
    for (std::size_t n = 0; n < s.visual_effects.size(); ++n) {
        auto &effect = s.visual_effects[n];
        if (effect.size() < 2 || effect[0] < 0 ||
            effect[0] >= static_cast<int>(effect_limits.size()))
            return false;
        int limit = effect_limits[effect[0]];
        if (effect[0] == 19) {
            constexpr std::array<int, 7> spell_duration{{0, 0, 0, 0, 9, 12, 6}};
            if (effect.size() < 3 || effect[2] < 0 || effect[2] >= 7)
                return false;
            limit = spell_duration[effect[2]];
        }
        if (limit == 0 && effect[0] != 19)
            continue;
        if (!increment(effect[1]))
            return false;
        if (effect[1] >= limit)
            s.visual_effects.erase(s.visual_effects.begin() + static_cast<std::ptrdiff_t>(n));
    }
    return true;
}
bool camera_focus(State &s, const std::array<float, 2> &target) {
    const float dx = target[0] - s.camera[0];
    const float dy = target[1] - s.camera[1];
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (!std::isfinite(distance))
        return false;
    const float speed = std::clamp(5.0f + (distance - 10.0f) * 21.0f / 140.0f, 5.0f, 26.0f);
    if (distance < speed) {
        s.camera = target;
        s.previous_camera = target;
        s.scene.scene_state = 0;
        s.scene.scene_counter = 0;
    } else {
        s.camera_velocity = {dx * speed / distance, dy * speed / distance};
        for (std::size_t k = 0; k < 2; ++k) {
            s.camera[k] += s.camera_velocity[k];
            s.previous_camera[k] += s.camera_velocity[k];
        }
    }
    return true;
}
bool build_update(State &s) {
    if (s.build_feedback_counter < 0 || s.build_mode < 0 || s.build_mode >= 8)
        return false;
    if (s.build_feedback_counter > 0 && --s.build_feedback_counter == 0) {
        static const std::string prompts[]{"要建在哪里呢", "从哪里开始铺呢", "铺到哪里呢",
                                           "撤除哪里呢",   "从哪里开始撤除", "撤到哪里",
                                           "移动哪个",     "移动去哪里"};
        s.build_feedback_message = prompts[s.build_mode];
    }
    // MainScene state1仅调用Tenant.d提示队首，不调用推进施工/探索的Tenant.c。
    for (const auto id : s.scene.world.facility_order) {
        if (!s.scene.world.world.facilities.count(id) || !s.facility_details.count(id))
            return false;
        auto &notices = s.facility_details.at(id).notices;
        if (notices.empty())
            continue;
        auto &front = notices.front();
        if (front[0] < 0 || front[0] >= 8 || !increment(front[1]))
            return false;
        constexpr int duration[]{44, 43, 43, 43, 43, 43, 43, 60};
        if (front[1] >= duration[front[0]])
            notices.erase(notices.begin());
    }
    return true;
}
} // namespace

std::optional<std::array<float, 2>> startup_world_runtime_facility_target(const State &s,
                                                                          std::uint64_t id) {
    if (!ref::valid_legacy_map(s.scene.world.world.map))
        return {};
    const auto instance = s.scene.world.world.facilities.find(id);
    if (instance == s.scene.world.world.facilities.end())
        return {};
    const auto &placement = instance->second.placement;
    const auto footprint =
        ref::facility_footprint(placement.shape, placement.orientation, placement.anchor,
                                s.scene.world.world.map.width, s.scene.world.world.map.height);
    if (footprint.cells.empty())
        return {};
    // Tenant.f读取z[0].n，保留原占地序；pair/square的首格不等于anchor。
    const auto first = footprint.cells.front().position;
    for (const auto &cell : footprint.cells) {
        const auto index = static_cast<std::size_t>(
            cell.position.y * s.scene.world.world.map.width + cell.position.x);
        const auto &binding = s.scene.world.world.map.cells.at(index).facility;
        if (!binding || binding->instance_id.value != id ||
            binding->definition_id != placement.definition_id)
            return {};
    }
    int x = (first.x + first.y) * 30;
    int y = (first.y - first.x) * 15 + 15;
    if (placement.shape == ref::FacilityShape::single) {
        x += 30;
        y -= 15;
    } else if (placement.shape == ref::FacilityShape::pair) {
        x += placement.orientation == ref::FacilityOrientation::first ? 15 : 45;
        y -= 22;
    } else if (placement.shape == ref::FacilityShape::square) {
        x += 30;
        y -= 30;
    } else
        return {};
    return std::array<float, 2>{static_cast<float>(x), static_cast<float>(y)};
}

void configure_startup_world_runtime_scene_adapter(ref::WorldRuntimeAdapter<State> &adapter) {
    const auto catalog = adapter.catalog;
    adapter.normal_conditions = [catalog](const State &current) -> std::optional<Step> {
        auto next = current;
        if (next.scene.world.world.ai.accounting.funds() >= 0)
            return Step{std::move(next)};
        int event{};
        if (!ref::world_script_seen(next.scripts, 161))
            event = 161;
        else if (!ref::world_script_seen(next.scripts, 164) && next.scene.calendar.year >= 3) {
            std::int64_t items{};
            for (const auto &item : next.catalog) {
                if (item.first.first == 0 && item.second.status != 0)
                    items += item.second.inventory;
                if (items >= 20) {
                    event = 164;
                    break;
                }
            }
        }
        if (event) {
            const auto result = ref::prepare_world_script(
                catalog, startup_world_runtime_scripts(next), {event, {}, {}});
            if (!result.candidate ||
                !write_startup_world_runtime_scripts(next, result.candidate->state))
                return {};
        }
        // 负余额前置脚本后仍进入L10b；L159只由实际aL/续体扫描返回。
        return Step{std::move(next)};
    };
    adapter.entry.read = [](const State &s) {
        ref::WorldWorldEntryState value;
        value.finish = startup_world_runtime_finish(s);
        value.scripts = startup_world_runtime_scripts(s);
        value.random = s.scene.random;
        value.map_surface = s.scene.world.surface;
        value.map_flags = s.scene.world.map_flags;
        value.generation_bounds = s.rules->generation_bounds.at(s.fence_level);
        value.town = s.scene.world.town;
        value.updates = s.entry_updates;
        value.global_updates = s.global_updates;
        return value;
    };
    adapter.entry.write = [](State &s, const ref::WorldWorldEntryState &value) {
        if (!write_startup_world_runtime_finish(s, value.finish) ||
            !write_startup_world_runtime_scripts(s, value.scripts))
            return false;
        s.scene.random = value.random;
        s.scene.world.surface = value.map_surface;
        s.scene.world.map_flags = value.map_flags;
        s.entry_updates = value.updates;
        s.global_updates = value.global_updates;
        return true;
    };
    // W仅需要这四个执行入口；捕获整份adapter会在每次场景回调复制所有脚本目录。
    auto focus_dependencies = std::make_shared<ref::WorldRuntimeAdapter<State>>();
    focus_dependencies->prefix_effects = adapter.prefix_effects;
    focus_dependencies->actors.owned_command = adapter.actors.owned_command;
    focus_dependencies->actors.event = adapter.actors.event;
    focus_dependencies->actors.projected_facing = adapter.actors.projected_facing;
    const std::shared_ptr<const ref::WorldRuntimeAdapter<State>> focus_adapter = focus_dependencies;
    adapter.scene_other = [focus_adapter](const State &current,
                                          const ref::WorldSceneCall &call) -> std::optional<Step> {
        auto s = current;
        switch (call.stage) {
        case Stage::entry_task_result:
            if (s.deadline_page) {
                auto result = prepare_startup_world_runtime_deadline_result(s);
                if (!result)
                    return {};
                s = std::move(*result);
            }
            break;
        case Stage::frame_view_sync:
            if (s.scene.scene_state == 2) {
                const auto &p = s.focus_actor.actor.position;
                if (!std::isfinite(p.x) || !std::isfinite(p.z) || !std::isfinite(p.height))
                    return {};
                const auto view = startup_world_raw_projection(p);
                s.camera = {static_cast<float>(view.x), static_cast<float>(view.y)};
            }
            break; // 图集21刷新属于表现输出。
        case Stage::global_display:
            if (!display(s))
                return {};
            break;
        case Stage::build_update:
            if (!build_update(s))
                return {};
            break;
        case Stage::normal_input:
        case Stage::task_list_input:
        case Stage::task_camera_input:
        case Stage::build_input:
            if (s.confirm_input || s.cancel_input)
                return {}; // 非空页面/建设输入必须走已证命令消费者。
            break;
        case Stage::focus_actor: {
            auto next = advance_startup_world_focus(s, *focus_adapter);
            if (!next)
                return {};
            s = std::move(*next);
            break;
        }
        case Stage::focus_input:
            if (s.confirm_input)
                return {}; // 原W.e/c.j.a0攻击输入未接UI消费者，不能空成功。
            if (s.cancel_input) {
                s.scene.scene_state = 0;
                s.scene.scene_counter = 0;
                s.cancel_input = false;
            }
            break;
        case Stage::wait_input:
            if (s.confirm_input && s.scene.scene_counter >= 10) {
                s.scene.scene_state = 0;
                s.scene.scene_counter = 0;
                s.confirm_input = false;
            }
            break;
        case Stage::actor_camera_input: {
            if (!s.scripts.selected_actor)
                return {};
            const auto id = ref::CharacterId{*s.scripts.selected_actor};
            const auto actor = s.actor_metadata.find(id);
            if (actor == s.actor_metadata.end() || s.confirm_input)
                return {};
            if (!camera_focus(s, {static_cast<float>(actor->second.cached_view.x),
                                  static_cast<float>(actor->second.cached_view.y)}) ||
                !increment(s.global_updates))
                return {};
            break;
        }
        case Stage::facility_camera_input: {
            if (!s.scripts.selected_facility) {
                s.scene.scene_state = 0;
                s.scene.scene_counter = 0;
                return Step{std::move(s), ref::WorldSceneDisposition::skip_round};
            }
            const auto target =
                startup_world_runtime_facility_target(s, *s.scripts.selected_facility);
            if (!target || !camera_focus(s, *target))
                return {};
            break;
        }
        case Stage::common_display_tail: {
            const auto notices = ref::prepare_world_notices(s.scripts.notices);
            if (!notices)
                return {};
            s.scripts.notices = notices->notices;
            s.sound_requests.insert(s.sound_requests.end(), notices->sounds.begin(),
                                    notices->sounds.end());
            break;
        }
        case Stage::common_global_flag:
            for (std::size_t n = s.global_effects.size(); n > 0; --n) {
                auto &effect = s.global_effects[n - 1];
                if (effect[0] == 0) {
                    if (!increment(effect[1]))
                        return {};
                    if (effect[1] >= 50)
                        s.global_effects.erase(s.global_effects.begin() +
                                               static_cast<std::ptrdiff_t>(n - 1));
                }
            }
            break;
        case Stage::common_menu_gate:
            if (s.menu_input)
                return {};
            break;
        default:
            return {};
        }
        return Step{std::move(s)};
    };
}
} // namespace dungeon_village_prototype
