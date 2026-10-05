#pragma once

#include "ark/simulation/rules/combat_commit.hpp"
#include "ark/simulation/rules/world_daily.hpp"
#include "ark/simulation/rules/world_dungeon.hpp"
#include "ark/simulation/rules/world_equipment_display.hpp"
#include "ark/simulation/rules/world_misc_control.hpp"
#include "ark/simulation/rules/world_random_consumers.hpp"
#include "ark/simulation/rules/world_wander.hpp"

namespace ark::simulation::rules {
// 一个共同人物/设施所有者。商店、探索等结构只在调用时投影，不长期存第二个world。
struct WorldActorRoutesState {
    RescueWorldState world;
    WorldMapFacts facts;
    WorldEventTask task;
    std::map<int, ShopHumanRecord> shop_humans;
    std::map<CharacterId, ShopActorRecord> shop_actors;
    std::map<int, ObjectCatalogRecord> items;
    std::vector<std::array<int, 3>> popularity_queue;
    std::map<std::uint64_t, DungeonFacilityProgress> dungeon_facilities;
    std::map<CharacterId, DungeonActorProgress> dungeon_actors;
    std::map<std::pair<int, int>, ObjectCatalogRecord> catalog;
    std::map<std::uint64_t, ObjectShopRecord> shops;
    std::vector<std::uint64_t> shop_order;
    int item_rewards{};
    std::map<int, int> human_definition_state;
    WorldRandomStream random;
};
enum class WorldActorRouteError {
    none,
    stale_actor,
    invalid_input,
    missing_fact,
    missing_consumer,
    consumer_failed,
    preparation_failed,
    control_failed
};
using WorldActorEventConsumer =
    std::function<std::optional<WorldActorRoutesState>(const WorldActorRoutesState &, int)>;
using WorldActorEncounterConsumer = std::function<std::optional<WorldActorRoutesState>(
    const WorldActorRoutesState &, const EncounterCreationRequest &)>;
struct WorldActorPresentationRequest {
    CharacterId actor;
    std::optional<CharacterId> target;
    std::optional<WorldAttackRequest> attack;
    std::optional<HitRequest> hit;
    std::optional<WorldMiscSoundRequest> sound;
    std::optional<ShopWorldRequest> shop;
    std::optional<LifecycleRequest> lifecycle{};
    std::optional<int> definition{};   // 删除后的怪物仍需原定义载荷，不依赖已回收实例。
    std::optional<int> cached_sound{}; // n.a(sound,旧bm)，不回写或重投影bm。
};
using WorldActorPresentationConsumer = std::function<std::optional<WorldActorRoutesState>(
    const WorldActorRoutesState &, const WorldActorPresentationRequest &)>;
struct WorldActorDecisionInput {
    CharacterId actor;
    bool use_shared_random{};
    std::optional<bool> primary_expression_table;
    WorldDailyInput daily;
    std::optional<ShopArrivalInput> shop_arrival; // 仅P实际到达普通/装备商店时消费。
    WorldLifecycleInput lifecycle;
    std::optional<WorldCombatPolicyInput> combat;
    std::optional<WorldPathInput> monster_path;
    std::optional<CollisionBox> actor_box;
    std::optional<CollisionBox> rescue_box;
    std::optional<WorldExpressionTicket> rescue_expression;
    std::optional<WorldExpressionTicket> special_expression;
    std::optional<Position> cached_view;
    std::optional<WorldDepartureInput> landing_departure;
    WorldActorEventConsumer event;
    WorldActorEncounterConsumer encounter;
    WorldActorPresentationConsumer presentation{}; // 在实际c调用点同步消费，不移到全人物之后。
    // 类别8递归交付：被救者使用旧s投影，之后才复制救援者的O/n/s。
    std::function<std::optional<Position>(CharacterId, int)> rescue_direction_target{};
};
struct WorldActorDecisionCandidate {
    WorldActorRoutesState state;
    bool removed{};          // 状态3提交已移除，调度使用already_removed，不再次r/释放占用。
    bool delete_requested{}; // P状态0请求删除，仍由c名单原时点实际移除。
    std::vector<LifecycleRequest> lifecycle_requests;
    std::vector<WorldAttackRequest> attack_requests;
    std::vector<ShopWorldRequest> shop_requests;
    std::vector<int> consumed_events;
    std::optional<WorldDailyCandidate> daily;
    std::optional<WorldLifecycleCandidate> lifecycle;
    std::optional<WorldMonsterActCandidate> monster;
};
struct WorldActorDecisionResult {
    WorldActorRouteError error{WorldActorRouteError::none};
    std::optional<WorldActorDecisionCandidate> candidate;
};
// 全A0..20路由，只运行共同c前段之后的分支；不重复计数、感知、d或随机。
WorldActorDecisionResult prepare_world_actor_decision(const WorldActorRoutesState &state,
                                                      const WorldActorDecisionInput &input);
struct WorldActorCommandInput {
    bool use_shared_random{};
    std::optional<bool> primary_expression_table;
    std::optional<int> boost_ticket;
    std::optional<WorldDepartureControlInput> departure;
    std::vector<int> wander_tickets;
    std::optional<WorldAttackInput> attack;
    WorldFacilityControlInput facility;
    std::optional<int> dungeon_notice_ticket;
    std::optional<ShopExitInput> shop_exit;
    std::vector<ShopEquipmentDefinition> equipment;
    std::optional<Position> cached_view;
    MiscSoundProjection sound_projection;
    WorldActorEventConsumer event;
    WorldActorPresentationConsumer presentation{}; // 在本命令续行之前消费cd/声音/朝向。
};
// 输入在实际队首和当前所有者上构建，不允许给整个FIFO预选一套过时的装备/目标。
using WorldActorCommandProvider = std::function<std::optional<WorldActorCommandInput>(
    const WorldActorRoutesState &, CharacterId, const LegacyActorControl &)>;
struct WorldActorControlCandidate {
    WorldActorRoutesState state;
    WorldControlFlow flow{WorldControlFlow::finished};
    std::size_t local_commands{};
    std::size_t domain_segments{};
    std::vector<WorldAttackRequest> attack_requests;
    std::vector<ShopWorldRequest> shop_requests;
    std::vector<WorldMiscSoundRequest> sounds;
    std::vector<int> consumed_events;
};
struct WorldActorControlResult {
    WorldActorRouteError error{WorldActorRouteError::none};
    std::optional<WorldActorControlCandidate> candidate;
    WorldControlError control_error{WorldControlError::none};
};
// 全34码解码复用world_control，实际领域按当前0/2/8/10/12..19/21..30/32/33路由。
// 成功8必须hold；退出/r同次续行；26/失败8删除跳过d尾部。缺实际输入拒绝整段。
WorldActorControlResult prepare_world_actor_control(const WorldActorRoutesState &state,
                                                    CharacterId actor,
                                                    const WorldActorCommandProvider &provider,
                                                    std::size_t budget = 4096);
} // namespace ark::simulation::rules
