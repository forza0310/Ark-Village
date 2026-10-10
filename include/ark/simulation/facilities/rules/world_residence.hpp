#pragma once

#include "ark/simulation/actors/rules/human_growth.hpp"
#include "ark/simulation/facilities/rules/world_facility_update.hpp"

namespace ark::simulation::rules {
struct WorldResidenceHuman {
    HumanDefinitionStatsInput definition; // u/t/l/z/M/v/s投影，不复制人物实例/出生位置。
    HumanDerivedStats derived;            // x/y/w/Q真实共享缓存；不跨努力十位则保持旧缓存。
    WorldScriptProgram arrival_program;   // e.i，character.txt第9列，没有aM事件身份。
    std::string localized_name;           // e.q()真实本地化结果，202参数不能自定名字。
    int celebrations{};                   // e.E，住宅n.a(...,false)不增加此字段。
};
struct WorldResidencePageBinding {
    int human{}; // 原gVar.m，和说话者j/k独立，即使talk已经指定另一说话者也绑定住户。
    std::optional<int> facility_definition; // 原页96.o；不是设施实例另存模型。
};
struct WorldResidenceState {
    WorldFacilityUpdateState facility;
    std::map<int, WorldResidenceHuman> humans;
    std::vector<HumanProfessionRule> professions;
    std::array<std::array<int, 2>, 3> reward_display{}; // n.aH：[旧C/u、新C/u、请求5/15]。
    std::array<std::array<int, 4>, 3> effort_display{}; // e.as[0]，跨十位才覆盖旧/新w与差额。
    std::map<std::uint64_t, WorldResidencePageBinding> page_bindings; // 同一实际页栈的m/o引用。
};
enum class WorldResidenceError {
    none,
    invalid_owner,
    invalid_construction_route,
    overflow,
    stats_failed,
    script_failed,
    page_failed,
    unsupported_program
};
struct WorldResidenceCandidate {
    WorldResidenceState state;
    std::vector<WorldScriptTrace> scripts;
    std::vector<WorldScriptPage> pages;
    bool effort_threshold_crossed{};
};
struct WorldResidenceResult {
    WorldResidenceError error{WorldResidenceError::none};
    std::optional<WorldResidenceCandidate> candidate;
};
// c/m施工完成u1/kind12/t!=-1后的同步消费者：奖励→58→96→可选67→e.i→首次202。
// 不创建人物、不补出生/HP/装备、不改变bl/D/现金；全部页插入沿用scripts锚与l锁。
// e.i严格支持固定25人物实际使用的2/24/25/26/27/28，不借虚构ID登记aM或伪造续体。
WorldResidenceResult prepare_world_residence(const WorldResidenceState &state,
                                             std::uint64_t facility,
                                             const WorldScriptCatalog &catalog);
} // namespace ark::simulation::rules
