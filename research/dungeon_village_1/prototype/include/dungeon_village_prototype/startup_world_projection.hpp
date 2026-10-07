#pragma once

#include "dungeon_village_prototype/startup_ai.hpp"
#include "dungeon_village_reference/world_actor_routes.hpp"
#include "dungeon_village_reference/world_calendar_maintenance.hpp"
#include "dungeon_village_reference/world_scripts.hpp"
#include "dungeon_village_reference/world_magic_pot.hpp"
#include "dungeon_village_reference/world_task_creation.hpp"

namespace dungeon_village_prototype {
struct StartupWorldJob {
    int role{}; // 原a.h.g，战斗策略角色，不是job ID。
    int type{}; // 原a.h.i，职业人数/维护分组。
    std::array<int, 2> sprites;
    std::array<int, 2> satisfaction;
    std::array<int, 2> fee;
    std::string name;
    std::uint32_t flags{};
    int initial_status{};
    int script_extra{};             // 原h.f41f，脚本profession提示载荷。
    int sort_order{};               // 原h.d，目录交换排序，不是职业ID。
    int gift_profile{};             // 原h.h，装备评价矩阵的行。
    int change_points{};            // 原h.s，消耗村子点数，不是金币。
    int required_medals{};          // 原h.t，人物E，不是当前全局勋章库存。
    int mastery_attribute{};        // 原h.v，>=10表示魔法，<6为满级属性奖励。
    int mastery_value{};            // 原h.w。
    std::vector<int> item_affinity; // 原h.u，job索引17，普通道具分类评价。
};
struct StartupWorldHuman {
    int identity{};
    std::string name;
    int sex{};
    std::uint32_t flags{};
    int status{};
    ref::HumanDefinitionStatsInput definition; // 无继承r()/h()后的真实初值。
    std::array<int, 4> equipment;
    int residence_threshold{};                         // 原e.g，character索引7。
    ref::WorldScriptProgram residence_request_program; // 原e.j，character索引10。
    int residence_fee{}; // 原e.h，character索引8；与设施造价、任务征集费分开。
    ref::WorldScriptProgram residence_completion_program; // 原e.i，character索引9。
};
struct StartupWorldEquipment {
    ref::ShopEquipmentDefinition shop;
    ref::ObjectCatalogRecord initial;
    ref::CombatWeaponRule battle; // 只kind1使用，其他命名空间不冒充武器。
    int reward_difficulty{};      // 武器g、防具/饰品f，用于任务奖励池。
    std::string name;
    int gift_rating{}; // 原weapon.t / armor.i / accessory.i，仅评价，不是装备准入。
    int gift_order{};  // 原weapon.u / armor.j / accessory.j。
    int render_image{}; // 武器p.e是weapon图片索引；防具/饰品e是18格图标ID，不能互换。
    int render_style{}; // 仅武器p.h的挥动/弓/枪/大剑0..3；不取战斗类别代替。
};
struct StartupWorldMonster {
    int identity{};
    std::string name;
    std::uint32_t flags{};
    int challenge_strength{};
    int points_per_defeat{}; // a.k.l，区别于即时金币S。
    ref::RewardMonsterDefinition initial;
    ref::WorldScriptProgram introduction_program; // 原k.t，解锁介绍实际同步执行。
};
struct StartupWorldItem {
    int identity{};
    ref::ObjectCatalogRecord initial;
    ref::CalendarMaintenanceShopItem maintenance;
    int difficulty{}; // 原g.j，任务奖励难度与普通道具评价共用。
    std::string name;
    int category{};                             // 原g.e，item索引3。
    int effect{};                               // 原g.f，属性／回复／学习分支。
    int spell{};                                // 原g.h，学习魔法槽。
    int attribute_amount{};                     // 原g.m，人物永久extra增量。
    int recovery{};                             // 原g.w，实际恢复量。
    std::array<int, 3> facility_improvements{}; // 原g.k，设施三项增量。
    int commerce_price{};                       // 原g.u，商会金币价格，出售向零除2。
    std::array<int, 4> magic_pot_elements{}; // 原g.l，item索引12..15火／冰／雷／暗。
};
struct StartupWorldTask {
    ref::TaskCreationDefinition factory;
    std::string name;
    std::string title;
    int initial_status{};
    int encounter_quota{};
    std::vector<int> encounter_monsters; // 原a.m.j，第10列，保留原序。
    int recruitment_fee{};               // 原a.m.g，第6列；页23确认扣现金类别4。
    int crew_rating_penalty{};           // 原a.m.s，第15列；只用于页28评价，不拒绝出发。
};
struct StartupWorldFacilityInitial {
    std::uint32_t flags{};
    int status{};
    int shared_n{};           // 原新对象N=0；存档读取不能与原表字段混合。
    int construction_limit{}; // 原o.o bit64分支：定义24为1，其余280，否则0。
    int capacity{};           // 原o.s，第12列，同时为85的村子点报价，另于施工m.x。
    bool pending_notice{};    // 原a()后r：bit2。
    bool initial_available{}; // 原a()后O：bit1。
    ref::WorldScriptProgram completion_program; // 原o.E，注册2000+原定义下标。
    std::vector<int> item_affinities;           // 原o.F，第34列，道具适配，不是月经营模式。
};
struct StartupWorldScriptSources {
    std::string events;
    std::string talks;
    std::string news;
    std::string event_messages;
    std::string popularity_rewards;
};
struct StartupWorldActivity {
    int identity{};
    std::string name;
    std::array<int, 7> parameters; // 原c.d/e/f/g/h/i/j，第2..8列完整保存。
    std::string detail;
    std::string description;
    std::uint32_t flags{};
    int initial_status{};
};
// 编译期核验固定原表哈希；运行只访问不可变C++，不读APK/work/JSON。
struct StartupWorldRules {
    std::vector<StartupWorldJob> jobs;
    std::vector<StartupWorldHuman> humans;
    std::vector<StartupWorldEquipment> equipment;
    std::vector<StartupWorldMonster> monsters;
    std::vector<StartupWorldItem> items;
    std::vector<StartupWorldTask> tasks;
    std::vector<StartupDefinition> facilities;        // 全85定义，包括未开放/无初始实例。
    std::vector<std::array<ref::Position, 2>> fences; // 真实h.l，读取后Y翻转。
    std::vector<std::array<ref::Position, 2>> generation_bounds; // 真实h.m。
    std::map<int, std::vector<int>> unconsumed_exit_deltas; // 原A可长于z；完整尾部保留，不执行。
    std::vector<StartupWorldFacilityInitial> facility_initial;
    StartupWorldScriptSources script_sources; // 严格哈希原文，由共同Owner显式解析目录。
    std::vector<StartupWorldActivity> activities;
    std::string localized_gold_template; // h.f512d未初始化，h.b("G")真实回退"G"。
    std::vector<int> base_variants;      // 固定首局h.i在c/d前保存；实例单格q0，原地表i保留。
    std::vector<ref::WorldMagicPotRecipeDefinition> magic_pot_recipes; // 固定40定义，p/r在Owner。
};
const StartupWorldRules &startup_world_rules();
struct StartupWorldActorMetadata {
    int sex{};
    int profession{};                       // 首访创建写入的ad；后续职业更新由真正Owner维护。
    int weapon{};                           // 首访ae。
    ref::Position cached_view;              // n.a时写u；与逻辑cell分开。
    ref::CombatPoint render_position{};     // 原o：new b零初值，仅真实frame渲染裁剪后更新，不是au。
    ref::Position cached_screen_position{}; // 原bm：new b零，render/部分控制显式写，不是raw u。
};
// 一次性初始化/接管候选；提交后销毁StartupState，不保存StartupSession与routes双权威。
struct StartupWorldProjection {
    ref::WorldActorRoutesState routes;
    const StartupWorldRules *rules{};
    std::vector<std::uint64_t> facility_order;
    std::map<std::uint64_t, int> facility_original_ids;
    std::map<std::uint64_t, int> facility_ordinals;
    std::map<std::uint64_t, int> facility_residents; // 初始所有Tenant.t=-1。
    std::map<std::uint64_t, std::array<int, 3>> neighbourhood;
    std::vector<ref::DungeonFinishSurface> surface;
    std::vector<std::array<bool, 2>> road_patches;
    std::map<int, std::array<int, 4>> human_homes; // bv.D，全定义零初值有来源，不由位置猜。
    std::map<int, int> human_presence;
    std::map<ref::CharacterId, StartupWorldActorMetadata> actor_metadata;
    std::array<int, 4> calendar;
    int popularity{};
    int arrival_counter{};
    int event89_count{};
    int ground_definition{};
    int special_ground_definition{};
    std::uint64_t simulation_steps{};
};
enum class StartupWorldProjectionError {
    none,
    invalid_snapshot,
    missing_definition,
    source_mismatch,
    projection_failed
};
struct StartupWorldProjectionResult {
    StartupWorldProjectionError error{StartupWorldProjectionError::none};
    std::optional<StartupWorldProjection> candidate;
};
// 支持真实未改图新局及已安装尚未推进AI的首访快照；拒绝旧预览/改图/第二事实来源。
// 随机必须来自明确外层Owner。投影本身不抽号，不重新选择出生点或运行首访脚本。
StartupWorldProjectionResult prepare_startup_world_projection(const StartupState &startup,
                                                              const ref::WorldRandomStream &random);
} // namespace dungeon_village_prototype
