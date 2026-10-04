#pragma once

#include "ark/simulation/rules/world_dungeon_finish.hpp"
#include "ark/simulation/rules/world_random.hpp"
#include "ark/simulation/rules/world_scripts.hpp"

namespace ark::simulation::rules {
struct WorldFacilityUpdateDefinition {
    std::uint32_t flags{};   // o.o，施工完成的首次脚本位。
    int popularity_reward{}; // o.N，共享定义值；普通完成后减半至至少1。
};
struct WorldFacilityUpdateDetails {
    int construction_limit{};                // m.x，建设开始时已计算，不能按当前职业重新计算。
    int condition{};                         // m.l，b1重算h所读值。
    int completion_popularity{};             // m.m，完成时当时N的实例副本。
    int residence_mode{};                    // m.u，恰1才执行居住创建链。
    int resident_definition{-1};             // m.t，-1不执行居住创建链。
    std::vector<std::array<int, 2>> notices; // m.p，只有队首更新，后项本次不计时。
};
// 唯一Owner在一轮设施c入口的临时值投影；status/progress/f分别由finish内唯一字段保存。
struct WorldFacilityUpdateState {
    DungeonFinishState finish;
    WorldScriptState scripts;
    WorldRandomStream random;
    std::map<int, WorldFacilityUpdateDefinition> definitions;
    std::map<std::uint64_t, WorldFacilityUpdateDetails> details;
    int cycle_length{}; // d.a.Q，必须来自当前原版配置，不补80默认值。
};
enum class WorldFacilityUpdateConsumerKind { residence_join, dungeon_crew, dungeon_finish };
struct WorldFacilityUpdateRequest {
    WorldFacilityUpdateConsumerKind kind{};
    std::uint64_t facility{};
};
// 在私有Owner上同步提交真实residence/既有world_dungeon/world_exploration。
// 返回null失败、缺消费者失败；不能只排脚本ID或占位成功。
using WorldFacilityUpdateConsumer = std::function<std::optional<WorldFacilityUpdateState>(
    const WorldFacilityUpdateState &, const WorldFacilityUpdateRequest &)>;
enum class WorldFacilityUpdateError {
    none,
    invalid_owner,
    overflow,
    script_failed,
    missing_consumer,
    consumer_failed
};
struct WorldFacilityUpdateCandidate {
    WorldFacilityUpdateState state;
    std::vector<WorldScriptTrace> scripts;
    std::vector<WorldScriptPage> pages;
    std::vector<WorldFacilityUpdateRequest> consumer_calls;
    bool construction_completed{};
};
struct WorldFacilityUpdateResult {
    WorldFacilityUpdateError error{WorldFacilityUpdateError::none};
    std::optional<WorldFacilityUpdateCandidate> candidate;
};
// c/m.c全入口：f++→d只队首→施工/使用/阶段2；已证普通建设不调用地图/邻接重建。
WorldFacilityUpdateResult
prepare_world_facility_update(const WorldFacilityUpdateState &state, std::uint64_t facility,
                              const WorldScriptCatalog &catalog,
                              const WorldFacilityUpdateConsumer &consumer = {});
} // namespace ark::simulation::rules
