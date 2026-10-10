#pragma once

// o(activity)目标/路线与P消费者；共同c前段、L、状态17资格及d仍由调度所有者执行。
#include "ark/simulation/combat/rules/rescue_commit.hpp"
#include "ark/simulation/combat/rules/world_encounters.hpp"
#include "ark/simulation/facilities/rules/world_facilities.hpp"

#include <functional>

namespace ark::simulation::rules {
struct WorldDepartureHome {
    Position cell; // 当前共享人物定义的 D0/1，不由村界推测住宅位置。
    int state{};   // D2，仅恰好为 1 时启用住宅偏好抽取。
};
struct WorldDepartureInput {
    CharacterId actor;
    int activity{};
    ActivityCandidateInput catalogue;       // 当前地图定义/魅力及原 bq 任务记录投影。
    std::map<int, int> definition_details;  // 原 o.g，活动 8 的类别 6 选择需要该详情。
    std::optional<WorldDepartureHome> home; // 仅实际选择类别 3 时要求住宅投影。
    std::vector<Position> exits;            // 原序 Map.f 真实出口；住宅命中时不读取此字段。
    Position task_center;                   // 仅当前任务有效时读取的任务中心。
    std::vector<std::int64_t> tickets;      // 按真实顺序消费，未使用队尾不会消费。
    std::function<std::optional<std::int64_t>(int)> draw;
    // tickets耗尽时按实际上限请求一个号；只能操作外层私有随机候选，失败整体回滚。
};
enum class WorldDepartureError {
    none,
    invalid_input,
    stale_actor,
    invalid_catalogue,
    missing_ticket,
    invalid_ticket,
    preparation_failed
};
enum class WorldDepartureDenial { none, no_candidates, no_selection, no_route };
struct WorldDepartureCandidate {
    RescueWorldState state;
    ActivityCandidateSnapshot snapshot;
    DepartureOverrideKind priority{DepartureOverrideKind::ordinary};
    WorldDepartureDenial denial{WorldDepartureDenial::none};
    std::optional<Position> goal;
    std::optional<LegacyPathResult> route;
    std::size_t consumed_tickets{};
    bool succeeded{};
    bool unbound_motion{}; // 地面/出口/任务的明确运动交接，不能冒充普通设施 q/P。
    bool event116{};
};
struct WorldDepartureResult {
    WorldDepartureError error{WorldDepartureError::none};
    std::optional<WorldDepartureCandidate> candidate;
};
// 原上限仅阻止展开过界节点，已发现前沿仍有有限距离；同成本按原库存反向顺序选择。
LegacySearchResult prepare_world_departure_field(const LegacyMap &map, Position start,
                                                 std::int64_t expanded_cost_limit);
// 不删除控制 8、不重置 A、不推进共同计数、不收费或占用设施。
// 普通 false 仍返回已清 G/H 的候选；非法输入或票号整批回滚。
WorldDepartureResult prepare_world_departure(const RescueWorldState &state,
                                             const WorldDepartureInput &input);
struct WorldDepartureControlInput {
    WorldDepartureInput departure; // 人物/目录/票号；活动参数以实际队首 8 为准。
    std::optional<WorldExpressionTicket> failure_expression; // 仅旧 1024，真实 c(18,0) 抽取。
    std::function<std::optional<WorldExpressionTicket>(const ActorEffectState &, int)>
        expression_draw{};
};
struct WorldDepartureControlCandidate {
    RescueWorldState state;
    WorldDepartureDenial denial{WorldDepartureDenial::none};
    std::optional<Position> goal;
    std::size_t consumed_tickets{};
    bool departure_succeeded{};
    bool cleaned_up{};
    bool delete_instance{};      // 原 v() 的 true 删除请求；实际名单删除归共同调度。
    bool continue_interpreter{}; // 失败 8/r 后，必须在同次 d 从新队首继续解释。
    bool consumed_expression{};
    bool consumed_variant{};
};
struct WorldDepartureControlResult {
    WorldDepartureError error{WorldDepartureError::none};
    std::optional<WorldDepartureControlCandidate> candidate;
};
// 先移除队首 8 再调用 o；成功直写 A0 并保留队尾至后续 d。
// 失败依次读取旧 1024/32768、执行真实表情 18，再改 512/r 并返回同次续行请求。
// 不推进共同计数、不执行 P、不提前删除人物或替外层循环执行第二条 8。
WorldDepartureControlResult
prepare_world_departure_control(const RescueWorldState &state,
                                const WorldDepartureControlInput &input);
// 仅MainScene.W；不改变普通名单入口或P寻路执行入口的live约束。
WorldDepartureControlResult
prepare_world_detached_departure_control(const RescueWorldState &state,
                                         const WorldDepartureControlInput &input);

// 复杂a(m,o)分支必须由真实领域消费者提交，不用普通到访默认值替代救援/装备/物体交付。
struct WorldPathFacilityRequest {
    CharacterId actor;
    ArrivalBinding binding;
    bool human_arrival_and_use{}; // true:完整a(m,o)→清256/选择use；false:只执行怪物a(m,o)。
};
struct WorldPathFacilityCandidate {
    RescueWorldState state;
    bool ground_effect20{};
};
using WorldPathFacilityConsumer = std::function<std::optional<WorldPathFacilityCandidate>(
    const RescueWorldState &, const WorldPathFacilityRequest &)>;
// 原F的任务创建尝试发生于状态切换之前；允许尝试失败后仍F=true，不能假定创建成功。
using WorldPathTaskAttempt = std::function<std::optional<WorldEventEntryCandidate>(
    const AiRewardState &, const WorldMapFacts &, CharacterId, const WorldEventTask &)>;
struct WorldPathInput {
    CharacterId actor;
    WorldMapFacts facts; // 当前map/flags/town，真实F适配使用；与状态中的地图须完全一致。
    WorldEventTask task;
    std::optional<std::vector<Position>> exits;             // 原Map.f；未提供与真实空数组不同。
    std::map<int, int> definition_directions;               // 路点6/7的当前o.m；不是实例q朝向。
    std::optional<WorldExpressionTicket> nearby_expression; // F人物相邻异legacyID时c(7,0)。
    std::optional<int> boost_ticket;                        // F人物c18，只在尚无2048时消费。
    std::optional<Position> use_world_target;               // 特殊6/3、8/2所需真实h.e投影。
    std::optional<int> use_direction_ticket;
    WorldPathTaskAttempt task_attempt;
    WorldPathFacilityConsumer facility_consumer;
    WorldExpressionDraw expression_draw{};
    std::function<std::optional<int>(int)> draw{};
    std::function<std::optional<Position>(int)> use_direction_target{};
};
enum class WorldPathError {
    none,
    invalid_input,
    stale_actor,
    missing_fact,
    missing_domain,
    missing_ticket,
    invalid_ticket,
    invalid_callback,
    preparation_failed
};
struct WorldPathCandidate {
    RescueWorldState state;
    WorldMapFacts facts; // 任务创建刷新bit2必须与当前事件候选一同交给外层唯一所有者。
    WorldEventTask task; // 原k.g的安装不能仅保存在AiRewardState中。
    bool event_preempted{};
    bool attempted_task_creation{};
    std::optional<std::uint64_t> created_task_encounter;
    EncounterCreationDenial task_creation_denial{EncounterCreationDenial::none};
    bool music2{};
    bool notice24{};
    bool consumed_expression{};
    bool consumed_variant{};
    bool consumed_boost_ticket{};
    bool event116{};
    bool advanced_waypoint{};
    bool moved{};
    bool arrived{};
    bool cleaned_up{};
    bool scheduled_exit{};     // 只排0→26，不在c阶段改共享定义m或删除实例。
    bool path_returned_true{}; // 原P返回值；怪物17/T1据此阻止L500清理，T4忽略。
    bool delete_instance{};    // 状态0才把怪物category3的P=true传播为c名单删除。
    bool ground_effect20{};
};
struct WorldPathResult {
    WorldPathError error{WorldPathError::none};
    std::optional<WorldPathCandidate> candidate;
};
// 在共同c前段及状态0的L之后，或已判定monster17/T1/T4实际调用P时执行一次。
// 内部重新调用真实F；状态17调用方仍处理L500与P返回值，不能把P=true都当c删除。
// G非空且旧s到O才执行j/入场；n运动不刷新s/t/u，最后路点H保留至到达分支消费。
// 不把G铺成m0，不跑d计数/物理。纯领域回调失败、缺事实或随机票号整批回滚。
WorldPathResult prepare_world_path_c(const RescueWorldState &state, const WorldPathInput &input);
// Transfer a disposable full world after the original entry validation. Narrow
// old actor/path/map/nearby snapshots preserve P's pre-callback observations.
WorldPathResult prepare_world_path_c_consuming(RescueWorldState &&state,
                                               const WorldPathInput &input);
} // namespace ark::simulation::rules
