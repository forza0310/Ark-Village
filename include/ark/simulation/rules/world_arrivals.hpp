#pragma once

#include "ark/simulation/rules/world_dungeon_finish.hpp"
#include "ark/simulation/rules/world_random.hpp"
#include "ark/simulation/rules/world_scripts.hpp"

namespace ark::simulation::rules {
struct WorldArrivalDefinition {
    int identity{};                // bv原数组顺序由definitions保存，不能用map迭代代替。
    int presence{};                // e.p，非零才进入常规候选；不是解锁status。
    int priority{};                // e.aq。
    std::uint32_t flags{};         // e.o，first覆盖只查bit8。
    std::vector<bool> job_history; // e.Q，消费者须回写e.a()刷新后的值，再检测218。
};
struct WorldArrivalsState {
    DungeonFinishState finish;
    WorldScriptState scripts;
    WorldRandomStream random;
    std::vector<WorldArrivalDefinition> definitions;
    std::vector<Position> spawn_cells; // bj.f180f原数组顺序。
    int arrival_counter{};             // n.B，新局420由外层提供。
    int camera_delay{};                // l.h，即使旧值为正仍可继续共同AI。
    int camera_follow{};               // l.i，仅恰1选择快速倒计时。
    int soft_limit{20};                // static n.Y，新局20，与aa两个独立门槛。
    int hard_limit{20};                // static n.aa。
    int debug_mode{};                  // e.aj，0/其他=不用override，1选ah，2选ai。
    std::array<std::vector<int>, 2> debug_definitions;
};
struct WorldArrivalCreationInput {
    int definition{};
    int legacy_uid{}; // c.b.i首个未占用bl UID，非stableID。
    Position spawn;
};
struct WorldArrivalCreation {
    WorldArrivalsState state;
    CharacterId actor;
};
// 在私有候选上实际new Character/a(0,UID,def)、性别/当前职业、旧derived HP三值、
// e.a(actor,e,v0)共享属性/Q刷新/A0=6、定位n/s/t/u/v、b(0)前插[8,0]、bl尾追加。
// 不执行89/218，本模块在真实追加后执行它们。不能修改外部唯一Owner。
using WorldArrivalCreationConsumer = std::function<std::optional<WorldArrivalCreation>(
    const WorldArrivalsState &, const WorldArrivalCreationInput &)>;
enum class WorldArrivalsError {
    none,
    invalid_owner,
    random_failed,
    numeric_overflow,
    missing_consumer,
    consumer_failed,
    script_failed
};
struct WorldArrivalsCandidate {
    WorldArrivalsState state;
    std::vector<int> random_bounds;
    std::vector<int> sorted_candidates;
    std::vector<int> top_candidates;
    std::optional<int> selected_definition;
    std::optional<CharacterId> created;
    bool due{};
    bool first{};
    std::vector<int> executed_events;
};
struct WorldArrivalsResult {
    WorldArrivalsError error{WorldArrivalsError::none};
    std::optional<WorldArrivalsCandidate> candidate;
};
// UserData.e影响场之后、共同人物c之前；到期即先抽/重置B，即使最后无候选。
// 原右向逆序交换不是stable_sort；首访覆盖之前仍真实消费常规/debug选择。
// 89=2,69&12,0，真实human_order[0]/scene6；218=6,300&4,18。
// 晚期任何创建/脚本错误均不给candidate，输入随机/计数/名单/页栈不变。
WorldArrivalsResult prepare_world_arrivals(const WorldArrivalsState &state,
                                           const WorldScriptCatalog &catalog,
                                           const WorldArrivalCreationConsumer &consumer = {});
} // namespace ark::simulation::rules
