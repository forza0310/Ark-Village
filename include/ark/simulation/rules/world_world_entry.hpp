#pragma once

#include "ark/simulation/rules/encounter_creation.hpp"
#include "ark/simulation/rules/world_calendar.hpp"
#include "ark/simulation/rules/world_dungeon_finish.hpp"
#include "ark/simulation/rules/world_random.hpp"
#include "ark/simulation/rules/world_scripts.hpp"

namespace ark::simulation::rules {
struct WorldWorldEntryState {
    DungeonFinishState finish; // 唯一地图/任务/人物/怪物/事件临时聚合。
    WorldScriptState scripts;
    WorldRandomStream random;
    std::vector<int> map_surface;              // i.g，供实际f.a末段事件图重建，和legacy_state独立。
    std::vector<std::uint32_t> map_flags;      // i.o，f.a末段须把实际bit2刷新写回同一Owner。
    std::array<Position, 2> generation_bounds; // 原h.m[0]，左下/右上，不是城镇边界。
    TownBounds town;                           // 原h.l[n.o]，k.c用严格内部，f.a上边三格用含边界。
    int updates{};                             // UserData.V，每次(V+1)%Integer.MAX_VALUE。
    int global_updates{};                      // static n.aK，源单独++，不是V的别名。
};
struct WorldWorldEntryCreation {
    WorldWorldEntryState state;
    std::optional<std::uint64_t> created;
    EncounterCreationDenial denial{EncounterCreationDenial::none};
};
// 实际c/f.a(0,cell)完整消费者，输入draw须只写私有Owner的random；回调不能写外部世界。
// 必须同步怪物首次定义脚本/页89/地图bit2刷新；缺回调失败，不能只排requests后空成功。
// null表示消费者失败；普通f.a拒绝要返回实际denial，不能用空created/none冒充执行。
using WorldWorldEntryCreationConsumer = std::function<std::optional<WorldWorldEntryCreation>(
    const WorldWorldEntryState &, const EncounterCreationInput &)>;
enum class WorldWorldEntryError {
    none,
    invalid_owner,
    overflow,
    random_failed,
    missing_consumer,
    consumer_failed
};
struct WorldWorldEntryCandidate {
    WorldWorldEntryState state;
    std::vector<int> random_bounds; // 本入口概率和k.c落点抽取；f.a内流继续由同Owner保存。
    std::optional<Position> selected_site;
    std::optional<std::uint64_t> created;
    EncounterCreationDenial denial{EncounterCreationDenial::none};
    bool attempted_creation{};
};
struct WorldWorldEntryResult {
    WorldWorldEntryError error{WorldWorldEntryError::none};
    std::optional<WorldWorldEntryCandidate> candidate;
};
// UserData.e最前段，仅V/aK与自发遭遇；其后仍是月报→bj.e影响场→镜头/到访→共同AI。
// 内建真实c/k.c最多20次半开抽取，不收外部票号或“找到点”默认成功回调。
// 不推进日期、不重建影响场、不把l.h镜头延迟误当全世界暂停。
WorldWorldEntryResult
prepare_world_world_entry(const WorldWorldEntryState &state, const WorldCalendarState &date,
                          const WorldScriptCatalog &catalog,
                          const WorldWorldEntryCreationConsumer &consumer = {});
} // namespace ark::simulation::rules
