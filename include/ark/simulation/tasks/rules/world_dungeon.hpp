#pragma once

// Expedition is a facility activity. Canonical occupation/HP/control remain in the shared world.
#include "ark/simulation/ai/rules/dungeon_ai.hpp"
#include "ark/simulation/combat/rules/object_commit.hpp"
#include "ark/simulation/combat/rules/rescue_commit.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace ark::simulation::rules {
struct DungeonFacilityProgress {
    int updates{}; // f, caller supplies the post-prefix count; this module does not run Tenant.d.
    int progress{};
    int extent{};
    int percent{};
    int previous_percent{};
    std::vector<DungeonChallenge> challenges;
};
struct DungeonWorldState {
    RescueWorldState world;
    std::map<std::uint64_t, DungeonFacilityProgress> facilities;
    std::map<CharacterId, DungeonActorProgress> actors; // One V/W/X/Y/Z/aa/bw owner per object.
    std::map<std::pair<int, int>, ObjectCatalogRecord> catalog;
    std::map<std::uint64_t, ObjectShopRecord> shops;
    std::vector<std::uint64_t> shop_order;
    int item_rewards{};
};
struct DungeonLaunchTickets {
    int destination{}; // Current legal candidate count, then two offsets80.
    int x_offset{};
    int z_offset{};
};
enum class DungeonWorldError {
    none,
    invalid_input,
    stale_actor,
    stale_facility,
    missing_ticket,
    preparation_failed
};
struct DungeonWorldRequest {
    DungeonCrewRequest source;
    std::vector<ObjectCommitRequest> catalog_requests;
    std::optional<ObjectId> object;
};
struct DungeonWorldCandidate {
    DungeonWorldState state;
    std::vector<DungeonWorldRequest> requests;
    std::size_t consumed_launches{};
    bool completed{};
    bool removed_occupation{};
    bool landed{};
    std::optional<int> entry_event;
};
using DungeonWorldRequestConsumer = std::function<std::optional<DungeonWorldState>(
    const DungeonWorldState &, const DungeonWorldRequest &)>;
struct DungeonWorldResult {
    DungeonWorldError error{DungeonWorldError::none};
    std::optional<DungeonWorldCandidate> candidate;
};
// c.k.a(任务定义)的任务所有者投影，不另建长期世界/任务存储。
struct DungeonTaskDefinitionProgress {
    int kind{};
    std::uint32_t flags{};
    int completed{};
    int monster_definition{};
};
struct DungeonMonsterAvailability {
    int status{};          // 原怪物p，即使旧值非零，c()仍写1。
    bool pending_notice{}; // 原r，仅旧p0时置真。
};
struct DungeonTaskSuccessState {
    int successes{};                                       // UserData.v
    int ordinary_explorations{};                           // UserData.w，仅kind0且无flag8。
    int exploration_stage{};                               // UserData.x，至多5，不是人物等级。
    std::array<std::array<int, 2>, 7> exploration_dates{}; // UserData.J[1]，原年/月。
    int task_pool_progress{};                              // UserData.G，不是金币/人气/村子点数。
    std::map<int, DungeonTaskDefinitionProgress> definitions;
    std::map<int, DungeonMonsterAvailability> monsters;
    std::vector<int> remaining_task_definitions; // clear_active_task前的当前bq。
};
struct DungeonTaskSuccessCandidate {
    DungeonTaskSuccessState state;
    std::vector<int> threshold_notice_ids; // 消息29/30/31，不是摘要页面ID。
};
// 不隐式清任务、增长经验/现金/人气或执行UI。重复调用确实再加计数；
// 有序阶段2在移除任务之前调用一次，不另加原版不存在的幂等屏障。
std::optional<DungeonTaskSuccessCandidate>
prepare_dungeon_task_success(const DungeonTaskSuccessState &state, int definition, int raw_year,
                             int raw_month);
// Front opcode21 reads the CURRENT O tile's instance; no extra old-s/status guard.
// Re-entry resets progress, preserves bw and appends duplicates. Empty uniform >=6 records
// can issue168/169 or170/171; only the actually emitted notice consumes a draw2.
DungeonWorldResult
prepare_world_dungeon_entry(const DungeonWorldState &state, CharacterId actor,
                            std::optional<int> notice_ticket = {},
                            const std::function<std::optional<int>(int)> &draw = {});
// Actual Character.a(cell): teleport BEFORE checking eight outside-state4 cells. No target
// succeeds as a no-launch candidate and consumes no tickets; q removal uses the new s.
DungeonWorldResult
prepare_world_dungeon_retreat(const DungeonWorldState &state, CharacterId actor, Position origin,
                              TownBounds town, std::optional<DungeonLaunchTickets> tickets = {},
                              const std::function<std::optional<int>(int)> &draw = {});
struct DungeonWorldCrewInput {
    std::uint64_t facility{};
    TownBounds town;
    std::optional<int> active_task_extent;
    std::vector<DungeonLaunchTickets> launches; // Actual successful retreats/throws, source order.
    std::function<std::optional<int>(int)> draw{};
};
// Phase1 after Tenant counter/notices: live retreat -> crew rules -> direct grants/ground throws
// -> c0/wait/index*5/activity0. All domains commit only if the entire candidate succeeds.
DungeonWorldResult prepare_world_dungeon_crew(const DungeonWorldState &state,
                                              const DungeonWorldCrewInput &input,
                                              const DungeonWorldRequestConsumer &consumer = {});
using DungeonLandingDeparture =
    std::function<std::optional<DungeonWorldState>(const DungeonWorldState &, CharacterId)>;
// State20 c: move horizontally first, oldB>=36 land -> OLD cached u display -> c0 -> DIRECT
// o0 -> c2. The callback operates only on the private candidate; no-route is a successful
// candidate too. Missing/failed consumers roll back, never turn this into queued activity8.
DungeonWorldResult prepare_world_dungeon_landing(const DungeonWorldState &state, CharacterId actor,
                                                 std::optional<Position> old_view = {},
                                                 const DungeonLandingDeparture &departure = {});
} // namespace ark::simulation::rules
